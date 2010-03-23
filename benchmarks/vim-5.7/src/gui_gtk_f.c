/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved		by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

/*
 * (C) 1998,1999 by Marcin Dalecki <dalecki@cs.net.pl>
 *
 *	"I'm a one-man software company. If you have anything UNIX, net or
 *	embedded systems related, which seems to cause some serious trouble for
 *	your's in-house developers, maybe we need to talk badly with each other
 *	:-) <dalecki@cs.net.pl> (My native language is polish and I speak
 *	native grade german too. I'm living in Göttingen.de.)
 *	--mdcki"
 *
 * This is a special purspose container widget, which manages arbitrary childs
 * at arbitrary positions width arbitrary sizes.  This finally puts an end on
 * our resizement problems with which we where struggling for such a long time.
 */

#include "vim.h"
#include <gtk/gtk.h>	/* without this it compiles, but gives errors at
			   runtime! */
#include "gui_gtk_f.h"
#include <gtk/gtksignal.h>
#include <gdk/gdkx.h>


static void gtk_form_class_init(GtkFormClass * class);
static void gtk_form_init(GtkForm * form);

static void gtk_form_realize(GtkWidget * widget);
static void gtk_form_unrealize(GtkWidget * widget);
static void gtk_form_map(GtkWidget * widget);
static void gtk_form_size_request(GtkWidget * widget,
				  GtkRequisition * requisition);
static void gtk_form_size_allocate(GtkWidget * widget,
				   GtkAllocation * allocation);
static void gtk_form_draw(GtkWidget * widget,
			  GdkRectangle * area);
static gint gtk_form_expose(GtkWidget * widget,
			    GdkEventExpose * event);

static void gtk_form_remove(GtkContainer * container,
			    GtkWidget * widget);
#ifdef GTK_HAVE_FEATURES_1_1_0
static void gtk_form_forall(GtkContainer * container,
			    gboolean include_internals,
			    GtkCallback callback,
			    gpointer callback_data);
#endif

static void gtk_form_realize_child(GtkForm * form,
				   GtkFormChild * child);
static void gtk_form_position_child(GtkForm * form,
				    GtkFormChild * child,
				    gboolean force_allocate);
static void gtk_form_position_children(GtkForm * form);

static GdkFilterReturn gtk_form_filter(GdkXEvent * gdk_xevent,
				       GdkEvent * event,
				       gpointer data);
static GdkFilterReturn gtk_form_main_filter(GdkXEvent * gdk_xevent,
					    GdkEvent * event,
					    gpointer data);

static void gtk_form_set_static_gravity(GdkWindow * win,
					gboolean op);

static void gtk_form_send_configure(GtkForm *form);

static GtkWidgetClass *parent_class = NULL;

/* Public interface
 */

GtkWidget *
gtk_form_new(void)
{
    GtkForm *form;

    form = gtk_type_new(gtk_form_get_type());

    return GTK_WIDGET(form);
}

void
gtk_form_put(GtkForm * form,
	     GtkWidget * child_widget,
	     gint x, gint y)
{
    GtkFormChild *child;

    g_return_if_fail(form != NULL);
    g_return_if_fail(GTK_IS_FORM(form));

    child = g_new(GtkFormChild, 1);

    child->widget = child_widget;
    child->window = NULL;
    child->x = x;
    child->y = y;
    child->widget->requisition.width = 0;
    child->widget->requisition.height = 0;
    child->mapped = FALSE;

    form->children = g_list_append(form->children, child);

    gtk_widget_set_parent(child_widget, GTK_WIDGET(form));
#ifdef GTK_HAVE_FEATURES_1_1_0	/* must use gtk+-1.1.16 or higher */
    gtk_widget_size_request(child->widget, NULL);
#else
    gtk_widget_size_request(child->widget, &child->widget->requisition);
#endif

    if (GTK_WIDGET_REALIZED(form) &&
	!GTK_WIDGET_REALIZED(child_widget))
	gtk_form_realize_child(form, child);

    gtk_form_position_child(form, child, TRUE);
}

void
gtk_form_move(GtkForm * form,
	      GtkWidget * child_widget,
	      gint x, gint y)
{
    GList *tmp_list;
    GtkFormChild *child;

    g_return_if_fail(form != NULL);
    g_return_if_fail(GTK_IS_FORM(form));

    tmp_list = form->children;
    while (tmp_list) {
	child = tmp_list->data;
	if (child->widget == child_widget) {
	    child->x = x;
	    child->y = y;

	    gtk_form_position_child(form, child, TRUE);
	    return;
	}
	tmp_list = tmp_list->next;
    }
}

void
gtk_form_set_size(GtkForm * form, guint width, guint height)
{
    g_return_if_fail(form != NULL);
    g_return_if_fail(GTK_IS_FORM(form));

    /* prevent unneccessary calls */
    if (form->width == width && form->height == height)
	return;
    form->width = width;
    form->height = height;

#ifdef GTK_HAVE_FEATURES_1_1_0
    /* signal the change */
    gtk_container_queue_resize(GTK_CONTAINER(GTK_WIDGET(form)->parent));
#endif
}

void
gtk_form_freeze(GtkForm * form)
{
    g_return_if_fail(form != NULL);
    g_return_if_fail(GTK_IS_FORM(form));

    ++form->freeze_count;
}

void
gtk_form_thaw(GtkForm * form)
{
    g_return_if_fail(form != NULL);
    g_return_if_fail(GTK_IS_FORM(form));

    if (form->freeze_count) {
	if (!(--form->freeze_count)) {
	    gtk_form_position_children(form);
	    gtk_widget_draw(GTK_WIDGET(form), NULL);
	}
    }
}

/* Basic Object handling procedures
 */
guint
gtk_form_get_type(void)
{
    static guint form_type = 0;

    if (!form_type) {
	GtkTypeInfo form_info =
	{
	    "GtkForm",
	    sizeof(GtkForm),
	    sizeof(GtkFormClass),
	    (GtkClassInitFunc) gtk_form_class_init,
	    (GtkObjectInitFunc) gtk_form_init
	};

	form_type = gtk_type_unique(GTK_TYPE_CONTAINER, &form_info);
    }
    return form_type;
}

static void
gtk_form_class_init(GtkFormClass * class)
{
    GtkWidgetClass *widget_class;
    GtkContainerClass *container_class;

    widget_class = (GtkWidgetClass *) class;
    container_class = (GtkContainerClass *) class;

    parent_class = gtk_type_class(gtk_container_get_type());

    widget_class->realize = gtk_form_realize;
    widget_class->unrealize = gtk_form_unrealize;
    widget_class->map = gtk_form_map;
    widget_class->size_request = gtk_form_size_request;
    widget_class->size_allocate = gtk_form_size_allocate;
    widget_class->draw = gtk_form_draw;
    widget_class->expose_event = gtk_form_expose;

    container_class->remove = gtk_form_remove;
#ifdef GTK_HAVE_FEATURES_1_1_0
    container_class->forall = gtk_form_forall;
#endif
}

static void
gtk_form_init(GtkForm * form)
{
    form->children = NULL;

    form->width = 1;
    form->height = 1;

    form->bin_window = NULL;

    form->configure_serial = 0;
    form->visibility = GDK_VISIBILITY_PARTIAL;

    form->freeze_count = 0;
}

/* Widget methods
 */

static void
gtk_form_realize(GtkWidget * widget)
{
    GList *tmp_list;
    GtkForm *form;
    GdkWindowAttr attributes;
    gint attributes_mask;

    g_return_if_fail(widget != NULL);
    g_return_if_fail(GTK_IS_FORM(widget));

    form = GTK_FORM(widget);
    GTK_WIDGET_SET_FLAGS(form, GTK_REALIZED);

    attributes.window_type = GDK_WINDOW_CHILD;
    attributes.x = widget->allocation.x;
    attributes.y = widget->allocation.y;
    attributes.width = widget->allocation.width;
    attributes.height = widget->allocation.height;
    attributes.wclass = GDK_INPUT_OUTPUT;
    attributes.visual = gtk_widget_get_visual(widget);
    attributes.colormap = gtk_widget_get_colormap(widget);
    attributes.event_mask = GDK_VISIBILITY_NOTIFY_MASK;

    attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

    widget->window = gdk_window_new(gtk_widget_get_parent_window(widget),
				    &attributes, attributes_mask);
    gdk_window_set_user_data(widget->window, widget);

    attributes.x = 0;
    attributes.y = 0;
    attributes.event_mask = gtk_widget_get_events(widget);

    form->bin_window = gdk_window_new(widget->window,
				      &attributes, attributes_mask);
    gdk_window_set_user_data(form->bin_window, widget);

    gtk_form_set_static_gravity(form->bin_window, TRUE);

    widget->style = gtk_style_attach(widget->style, widget->window);
    gtk_style_set_background(widget->style, widget->window, GTK_STATE_NORMAL);
    gtk_style_set_background(widget->style, form->bin_window, GTK_STATE_NORMAL);

    gdk_window_add_filter(widget->window, gtk_form_main_filter, form);
    gdk_window_add_filter(form->bin_window, gtk_form_filter, form);

    tmp_list = form->children;
    while (tmp_list) {
	GtkFormChild *child = tmp_list->data;

	if (GTK_WIDGET_VISIBLE(child->widget))
	    gtk_form_realize_child(form, child);

	tmp_list = tmp_list->next;
    }
}

static void
gtk_form_map(GtkWidget * widget)
{
    GList *tmp_list;
    GtkForm *form;

    g_return_if_fail(widget != NULL);
    g_return_if_fail(GTK_IS_FORM(widget));

    form = GTK_FORM(widget);

    GTK_WIDGET_SET_FLAGS(widget, GTK_MAPPED);

    gdk_window_show(widget->window);
    gdk_window_show(form->bin_window);

    tmp_list = form->children;
    while (tmp_list) {
	GtkFormChild *child = tmp_list->data;

	if (GTK_WIDGET_VISIBLE(child->widget) &&
	    !GTK_WIDGET_MAPPED(child->widget))
	    gtk_widget_map(child->widget);

	if (child->window)
	    gdk_window_show(child->window);

	tmp_list = tmp_list->next;
    }

}

static void
gtk_form_unrealize(GtkWidget * widget)
{
    GList *tmp_list;
    GtkForm *form;

    g_return_if_fail(widget != NULL);
    g_return_if_fail(GTK_IS_FORM(widget));

    form = GTK_FORM(widget);

    tmp_list = form->children;

    gdk_window_set_user_data(form->bin_window, NULL);
    gdk_window_destroy(form->bin_window);
    form->bin_window = NULL;

    while (tmp_list) {
	GtkFormChild *child = tmp_list->data;

	if (child->window) {
	    gdk_window_set_user_data(child->window, NULL);
	    gdk_window_destroy(child->window);
	}
	tmp_list = tmp_list->next;
    }

    if (GTK_WIDGET_CLASS (parent_class)->unrealize)
	 (* GTK_WIDGET_CLASS (parent_class)->unrealize) (widget);
}

/*ARGSUSED*/
static void
gtk_form_draw(GtkWidget * widget, GdkRectangle * area)
{
    GtkForm		*form;
    GList		*children;
    GtkFormChild	*child;
    GdkRectangle	child_area;

    g_return_if_fail(widget != NULL);
    g_return_if_fail(GTK_IS_FORM(widget));

    if (GTK_WIDGET_DRAWABLE (widget))
    {
	form = GTK_FORM(widget);

	children = form->children;

	while (children)
	{
	    child = children->data;

	    if (GTK_WIDGET_DRAWABLE(child->widget)
		    && gtk_widget_intersect (child->widget, area, &child_area))
		gtk_widget_draw(child->widget, &child_area);

	    children = children->next;
	}
    }
}

static void
gtk_form_size_request(GtkWidget * widget,
		      GtkRequisition * requisition)
{
    GList *tmp_list;
    GtkForm *form;

    g_return_if_fail(widget != NULL);
    g_return_if_fail(GTK_IS_FORM(widget));

    form = GTK_FORM(widget);

    requisition->width = form->width;
    requisition->height = form->height;

    tmp_list = form->children;

    while (tmp_list)
    {
	GtkFormChild *child = tmp_list->data;
#ifdef GTK_HAVE_FEATURES_1_1_0	/* must use gtk+-1.1.16 or higher */
	gtk_widget_size_request(child->widget, NULL);
#else
	gtk_widget_size_request(child->widget, &child->widget->requisition);
#endif
	tmp_list = tmp_list->next;
    }
}

static void
gtk_form_size_allocate(GtkWidget * widget,
		       GtkAllocation * allocation)
{
    GList *tmp_list;
    GtkForm *form;
    gboolean need_reposition;

    g_return_if_fail(widget != NULL);
    g_return_if_fail(GTK_IS_FORM(widget));

    if (widget->allocation.x == allocation->x
	    && widget->allocation.y == allocation->y
	    && widget->allocation.width == allocation->width
	    && widget->allocation.height == allocation->height)
	return;

    need_reposition = widget->allocation.width != allocation->width
		   || widget->allocation.height != allocation->height;
    form = GTK_FORM(widget);

    if (need_reposition) {
	tmp_list = form->children;

	while (tmp_list) {
	    GtkFormChild *child = tmp_list->data;
	    gtk_form_position_child(form, child, TRUE);

	    tmp_list = tmp_list->next;
	}
    }

    if (GTK_WIDGET_REALIZED(widget)) {
	gdk_window_move_resize(widget->window,
			       allocation->x, allocation->y,
			       allocation->width, allocation->height);
	gdk_window_move_resize(GTK_FORM(widget)->bin_window,
			       0, 0,
			       allocation->width, allocation->height);
    }
    widget->allocation = *allocation;
    if (need_reposition)
	gtk_form_send_configure(form);
}

static gint
gtk_form_expose(GtkWidget * widget, GdkEventExpose * event)
{
    GList *tmp_list;
    GtkForm *form;

    g_return_val_if_fail(widget != NULL, FALSE);
    g_return_val_if_fail(GTK_IS_FORM(widget), FALSE);

    form = GTK_FORM(widget);

    if (event->window == form->bin_window)
	return FALSE;

    tmp_list = form->children;
    while (tmp_list) {
	GtkFormChild *child = tmp_list->data;

	if (event->window == child->window)
	    return gtk_widget_event(child->widget, (GdkEvent *) event);

	tmp_list = tmp_list->next;
    }

    return FALSE;
}

/* Container method
 */
static void
gtk_form_remove(GtkContainer * container, GtkWidget * widget)
{
    GList *tmp_list;
    GtkForm *form;
    GtkFormChild *child;

    g_return_if_fail(container != NULL);
    g_return_if_fail(GTK_IS_FORM(container));

    form = GTK_FORM(container);

    tmp_list = form->children;
    while (tmp_list) {
	child = tmp_list->data;
	if (child->widget == widget)
	    break;
	tmp_list = tmp_list->next;
    }

    if (tmp_list) {
	if (child->window) {
	    /* FIXME: This will cause problems for reparenting NO_WINDOW
	     * widgets out of a GtkForm
	     */
	    gdk_window_set_user_data(child->window, NULL);
	    gdk_window_destroy(child->window);
	}
	gtk_widget_unparent(widget);

	form->children = g_list_remove_link(form->children, tmp_list);
	g_list_free_1(tmp_list);
	g_free(child);
    }
}

/*ARGSUSED*/
#ifdef GTK_HAVE_FEATURES_1_1_0
static void
gtk_form_forall(GtkContainer * container,
		gboolean include_internals,
		GtkCallback callback,
		gpointer callback_data)
{
    GtkForm *form;
    GtkFormChild *child;
    GList *tmp_list;

    g_return_if_fail(container != NULL);
    g_return_if_fail(GTK_IS_FORM(container));
    g_return_if_fail(callback != NULL);

    form = GTK_FORM(container);

    tmp_list = form->children;
    while (tmp_list) {
	child = tmp_list->data;
	tmp_list = tmp_list->next;

	(*callback) (child->widget, callback_data);
    }
}
#endif

/* Operations on children
 */

static void
gtk_form_realize_child(GtkForm * form,
		       GtkFormChild * child)
{
    GtkWidget *widget;
    gint attributes_mask;

    widget = GTK_WIDGET(form);

    if (GTK_WIDGET_NO_WINDOW(child->widget)) {
	GdkWindowAttr attributes;

	gint x = child->x;
	gint y = child->y;

	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.x = x;
	attributes.y = y;
	attributes.width = child->widget->requisition.width;
	attributes.height = child->widget->requisition.height;
	attributes.wclass = GDK_INPUT_OUTPUT;
	attributes.visual = gtk_widget_get_visual(widget);
	attributes.colormap = gtk_widget_get_colormap(widget);
	attributes.event_mask = GDK_EXPOSURE_MASK;

	attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
	child->window = gdk_window_new(form->bin_window,
				       &attributes, attributes_mask);
	gdk_window_set_user_data(child->window, widget);

	if (child->window)
	    gtk_style_set_background(widget->style,
				     child->window,
				     GTK_STATE_NORMAL);
    }
    gtk_widget_set_parent_window(child->widget,
		       child->window ? child->window : form->bin_window);

    gtk_widget_realize(child->widget);

    gtk_form_set_static_gravity(child->window ? child->window : child->widget->window, TRUE);
}

static void
gtk_form_position_child(GtkForm * form,
			GtkFormChild * child,
			gboolean force_allocate)
{
    gint x;
    gint y;

    x = child->x;
    y = child->y;
    if ((x >= G_MINSHORT) && (x <= G_MAXSHORT) &&
	    (y >= G_MINSHORT) && (y <= G_MAXSHORT)) {
	if (!child->mapped) {
	    child->mapped = TRUE;

	    if (GTK_WIDGET_MAPPED (form) &&
		    GTK_WIDGET_VISIBLE (child->widget)) {
		if (child->window)
		    gdk_window_show (child->window);
		if (!GTK_WIDGET_MAPPED (child->widget))
		    gtk_widget_map (child->widget);

		child->mapped = TRUE;
		force_allocate = TRUE;
	    }
	}

	if (force_allocate) {
	    GtkAllocation allocation;

	    if (GTK_WIDGET_NO_WINDOW (child->widget)) {
		if (child->window) {
		    gdk_window_move_resize (child->window,
			    x, y,
			    child->widget->requisition.width,
			    child->widget->requisition.height);
		}

		allocation.x = 0;
		allocation.y = 0;
	    } else {
		allocation.x = x;
		allocation.y = y;
	    }

	    allocation.width = child->widget->requisition.width;
	    allocation.height = child->widget->requisition.height;

	    gtk_widget_size_allocate (child->widget, &allocation);
	}
    } else {
	if (child->mapped) {
	    child->mapped = FALSE;
	    if (child->window)
		gdk_window_hide (child->window);
	    else if (GTK_WIDGET_MAPPED (child->widget))
		gtk_widget_unmap (child->widget);
	}
    }
}

static void
gtk_form_position_children(GtkForm * form)
{
    GList *tmp_list;

    tmp_list = form->children;
    while (tmp_list) {
	gtk_form_position_child(form, tmp_list->data, FALSE);

	tmp_list = tmp_list->next;
    }
}

/* Callbacks */

/* The main event filter. Actually, we probably don't really need
 * to install this as a filter at all, since we are calling it
 * directly above in the expose-handling hack.
 *
 * This routine identifies expose events that are generated when
 * we've temporarily moved the bin_window_origin, and translates
 * them or discards them, depending on whether we are obscured
 * or not.
 */
/*ARGSUSED*/
static GdkFilterReturn
gtk_form_filter(GdkXEvent * gdk_xevent,
		GdkEvent * event,
		gpointer data)
{
    XEvent *xevent;
    GtkForm *form;

    xevent = (XEvent *) gdk_xevent;
    form = GTK_FORM(data);

    switch (xevent->type) {
    case Expose:
	if (xevent->xexpose.serial == form->configure_serial) {
	    if (form->visibility == GDK_VISIBILITY_UNOBSCURED)
		return GDK_FILTER_REMOVE;
	    else
		break;
	}
	break;

    case ConfigureNotify:
	if ((xevent->xconfigure.x != 0) || (xevent->xconfigure.y != 0))
	    form->configure_serial = xevent->xconfigure.serial;
	break;
    }

    return GDK_FILTER_CONTINUE;
}

/* Although GDK does have a GDK_VISIBILITY_NOTIFY event,
 * there is no corresponding event in GTK, so we have
 * to get the events from a filter
 */
/*ARGSUSED*/
static GdkFilterReturn
gtk_form_main_filter(GdkXEvent * gdk_xevent,
		     GdkEvent * event,
		     gpointer data)
{
    XEvent *xevent;
    GtkForm *form;

    xevent = (XEvent *) gdk_xevent;
    form = GTK_FORM(data);

    if (xevent->type == VisibilityNotify) {
	switch (xevent->xvisibility.state) {
	case VisibilityFullyObscured:
	    form->visibility = GDK_VISIBILITY_FULLY_OBSCURED;
	    break;

	case VisibilityPartiallyObscured:
	    form->visibility = GDK_VISIBILITY_PARTIAL;
	    break;

	case VisibilityUnobscured:
	    form->visibility = GDK_VISIBILITY_UNOBSCURED;
	    break;
	}

	return GDK_FILTER_REMOVE;
    }
    return GDK_FILTER_CONTINUE;
}

/* Routines to set the window gravity, and check whether it is
 * functional. Extra capabilities need to be added to GDK, so
 * we don't have to use Xlib here.
 */
static void
gtk_form_set_static_gravity(GdkWindow * win, gboolean on)
{
    XSetWindowAttributes xattributes;

    xattributes.win_gravity = on ? StaticGravity : NorthWestGravity;
    xattributes.bit_gravity = on ? StaticGravity : NorthWestGravity;

    XChangeWindowAttributes(GDK_WINDOW_XDISPLAY(win),
			    GDK_WINDOW_XWINDOW(win),
			    CWBitGravity | CWWinGravity,
			    &xattributes);
}

void
gtk_form_move_resize(GtkForm * form, GtkWidget * widget,
		     gint x, gint y,
		     gint w, gint h)
{
    widget->requisition.width = w;
    widget->requisition.height = h;
    gtk_form_move(form, widget, x, y);
}

static void
gtk_form_send_configure(GtkForm *form)
{
    GtkWidget *widget;
    GdkEventConfigure event;

    widget = GTK_WIDGET (form);

    event.type = GDK_CONFIGURE;
    event.window = widget->window;
    event.x = widget->allocation.x;
    event.y = widget->allocation.y;
    event.width = widget->allocation.width;
    event.height = widget->allocation.height;

    gtk_widget_event (widget, (GdkEvent*) &event);
}
