/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved		by Bram Moolenaar
 *				GUI/Motif support by Robert Webb
 *				Athena port by Bill Foster
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/MenuButton.h>
#include <X11/Xaw/SmeBSB.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Dialog.h>

#include "vim.h"
#include "gui_at_sb.h"

#define puller_width	19
#define puller_height	19

extern Widget vimShell;

static Widget vimForm = (Widget)0;
static Widget textArea = (Widget)0;
#ifdef WANT_MENU
static Widget menuBar = (Widget)0;
static void gui_athena_pullright_action __ARGS((Widget, XEvent *, String *,
						Cardinal *));
static void gui_athena_pullleft_action __ARGS((Widget, XEvent *, String *,
						Cardinal *));
static XtActionsRec	pullAction[2] = {{ "menu-pullright",
				(XtActionProc)gui_athena_pullright_action},
					 { "menu-pullleft",
				(XtActionProc)gui_athena_pullleft_action}};
#endif

static void gui_athena_scroll_cb_jump	__ARGS((Widget, XtPointer, XtPointer));
static void gui_athena_scroll_cb_scroll __ARGS((Widget, XtPointer, XtPointer));
#ifdef WANT_MENU
static XtTranslations	popupTrans, parentTrans, menuTrans, supermenuTrans;
static Pixmap		pullerBitmap;
static char_u puller_bits[] =
{
    0x00,0x00,0xf8,0x00,0x00,0xf8,0xf8,0x7f,0xf8,0x04,0x80,0xf8,0x04,0x80,0xf9,
    0x84,0x81,0xf9,0x84,0x83,0xf9,0x84,0x87,0xf9,0x84,0x8f,0xf9,0x84,0x8f,0xf9,
    0x84,0x87,0xf9,0x84,0x83,0xf9,0x84,0x81,0xf9,0x04,0x80,0xf9,0x04,0x80,0xf9,
    0xf8,0xff,0xf9,0xf0,0x7f,0xf8,0x00,0x00,0xf8,0x00,0x00,0xf8
};
#endif

/*
 * Scrollbar callback (XtNjumpProc) for when the scrollbar is dragged with the
 * left or middle mouse button.
 */
/* ARGSUSED */
    static void
gui_athena_scroll_cb_jump(w, client_data, call_data)
    Widget	w;
    XtPointer	client_data, call_data;
{
    GuiScrollbar *sb, *sb_info;
    long	value;

    sb = gui_find_scrollbar((long)client_data);

    if (sb == NULL)
	return;
    else if (sb->wp != NULL)	    /* Left or right scrollbar */
    {
	/*
	 * Careful: need to get scrollbar info out of first (left) scrollbar
	 * for window, but keep real scrollbar too because we must pass it to
	 * gui_drag_scrollbar().
	 */
	sb_info = &sb->wp->w_scrollbars[0];
    }
    else	    /* Bottom scrollbar */
	sb_info = sb;

    value = (long)(*((float *)call_data) * (float)(sb_info->max + 1) + 0.001);
    if (value > sb_info->max)
	value = sb_info->max;

    gui_drag_scrollbar(sb, value, TRUE);
}

/*
 * Scrollbar callback (XtNscrollProc) for paging up or down with the left or
 * right mouse buttons.
 */
/* ARGSUSED */
    static void
gui_athena_scroll_cb_scroll(w, client_data, call_data)
    Widget	w;
    XtPointer	client_data, call_data;
{
    GuiScrollbar *sb, *sb_info;
    long	value;
    int		data = (int)(long)call_data;
    int		page;

    sb = gui_find_scrollbar((long)client_data);

    if (sb == NULL)
	return;
    if (sb->wp != NULL)		/* Left or right scrollbar */
    {
	/*
	 * Careful: need to get scrollbar info out of first (left) scrollbar
	 * for window, but keep real scrollbar too because we must pass it to
	 * gui_drag_scrollbar().
	 */
	sb_info = &sb->wp->w_scrollbars[0];

	if (sb_info->size > 5)
	    page = sb_info->size - 2;	    /* use two lines of context */
	else
	    page = sb_info->size;
	switch (data)
	{
	    case  ONE_LINE_DATA: data = 1; break;
	    case -ONE_LINE_DATA: data = -1; break;
	    case  ONE_PAGE_DATA: data = page; break;
	    case -ONE_PAGE_DATA: data = -page; break;
	    case  END_PAGE_DATA: data = sb_info->max; break;
	    case -END_PAGE_DATA: data = -sb_info->max; break;
			default: data = 0; break;
	}
    }
    else			/* Bottom scrollbar */
    {
	sb_info = sb;
	if (data < -1)
	    data = -(Columns - 5);
	else if (data > 1)
	    data = (Columns - 5);
    }

    value = sb_info->value + data;
    if (value > sb_info->max)
	value = sb_info->max;
    else if (value < 0)
	value = 0;

    /* Update the bottom scrollbar an extra time (why is this needed?? */
    if (sb->wp == NULL)		/* Bottom scrollbar */
	gui_mch_set_scrollbar_thumb(sb, (int)value, sb->size, sb->max);

    gui_drag_scrollbar(sb, value, FALSE);
}

/*
 * Create all the Athena widgets necessary.
 */
    void
gui_x11_create_widgets()
{
    /*
     * We don't have any borders handled internally by the textArea to worry
     * about so only skip over the configured border width.
     */
    gui.border_offset = gui.border_width;

    XtInitializeWidgetClass(formWidgetClass);
    XtInitializeWidgetClass(boxWidgetClass);
    XtInitializeWidgetClass(coreWidgetClass);
#ifdef WANT_MENU
    XtInitializeWidgetClass(menuButtonWidgetClass);
#endif
    XtInitializeWidgetClass(simpleMenuWidgetClass);
    XtInitializeWidgetClass(vim_scrollbarWidgetClass);

    /* The form containing all the other widgets */
    vimForm = XtVaCreateManagedWidget("vimForm",
	formWidgetClass,	vimShell,
	XtNborderWidth,		0,
	XtNforeground,		gui.menu_fg_pixel,
	XtNbackground,		gui.menu_bg_pixel,
	NULL);

#ifdef WANT_MENU
    /* The top menu bar */
    menuBar = XtVaCreateManagedWidget("menuBar",
	boxWidgetClass,		vimForm,
	XtNresizable,		True,
	XtNtop,			XtChainTop,
	XtNbottom,		XtChainTop,
	XtNleft,		XtChainLeft,
	XtNright,		XtChainRight,
	XtNforeground,		gui.menu_fg_pixel,
	XtNbackground,		gui.menu_bg_pixel,
	XtNborderColor,		gui.menu_fg_pixel,
	NULL);
#endif

    /* The text area. */
    textArea = XtVaCreateManagedWidget("textArea",
	coreWidgetClass,	vimForm,
	XtNresizable,		True,
	XtNtop,			XtChainTop,
	XtNbottom,		XtChainTop,
	XtNleft,		XtChainLeft,
	XtNright,		XtChainLeft,
	XtNbackground,		gui.back_pixel,
	XtNborderWidth,		0,
	NULL);

    /*
     * Install the callbacks.
     */
    gui_x11_callbacks(textArea, vimForm);

#ifdef WANT_MENU
    popupTrans = XtParseTranslationTable("<EnterWindow>: highlight()\n<LeaveWindow>: unhighlight()\n<BtnDown>: notify() unhighlight() MenuPopdown()\n<Motion>: highlight() menu-pullright()");
    parentTrans = XtParseTranslationTable("<EnterWindow>: highlight()\n<LeaveWindow>: unhighlight()\n<BtnUp>: notify() unhighlight() MenuPopdown()\n<BtnMotion>: highlight() menu-pullright()");
    menuTrans = XtParseTranslationTable("<EnterWindow>: highlight()\n<LeaveWindow>: unhighlight() MenuPopdown()\n<BtnUp>: notify() unhighlight() MenuPopdown()\n<BtnMotion>: highlight() menu-pullright()");
    supermenuTrans = XtParseTranslationTable("<EnterWindow>: highlight() menu-pullleft()\n<LeaveWindow>:\n<BtnUp>: notify() unhighlight() MenuPopdown()\n<BtnMotion>:");

    XtAppAddActions(XtWidgetToApplicationContext(vimForm), pullAction, 2);

    pullerBitmap = XCreateBitmapFromData(gui.dpy, DefaultRootWindow(gui.dpy),
			    (char *)puller_bits, puller_width, puller_height);
#endif

    /* Pretend we don't have input focus, we will get an event if we do. */
    gui.in_focus = FALSE;
}

/*
 * Called when the GUI is not going to start after all.
 */
    void
gui_x11_destroy_widgets()
{
    textArea = NULL;
#ifdef WANT_MENU
    menuBar = NULL;
#endif
}

    void
gui_mch_set_text_area_pos(x, y, w, h)
    int	    x;
    int	    y;
    int	    w;
    int	    h;
{
    XtUnmanageChild(textArea);
    XtVaSetValues(textArea,
		  XtNhorizDistance, x,
		  XtNvertDistance, y,
		  XtNwidth, w,
		  XtNheight, h,
		  NULL);
    XtManageChild(textArea);
}

    void
gui_x11_set_back_color()
{
    if (textArea != NULL)
	XtVaSetValues(textArea,
		  XtNbackground, gui.back_pixel,
		  NULL);
}

#if defined(WANT_MENU) || defined(PROTO)
/*
 * Menu stuff.
 */

static char_u *make_pull_name __ARGS((char_u * name));
static void gui_mch_submenu_colors __ARGS((VimMenu *mp));
static void gui_athena_reorder_menus	__ARGS((void));
static Widget get_popup_entry __ARGS((Widget w));

    void
gui_mch_enable_menu(flag)
    int	    flag;
{
    if (flag)
	XtManageChild(menuBar);
    else
	XtUnmanageChild(menuBar);
}

    void
gui_mch_set_menu_pos(x, y, w, h)
    int	    x;
    int	    y;
    int	    w;
    int	    h;
{
    Dimension	border;
    int		height;

    XtUnmanageChild(menuBar);
    XtVaGetValues(menuBar,
		XtNborderWidth, &border,
		NULL);
    /* avoid trouble when there are no menu items, and h is 1 */
    height = h - 2 * border;
    if (height < 0)
	height = 1;
    XtVaSetValues(menuBar,
		XtNhorizDistance, x,
		XtNvertDistance, y,
		XtNwidth, w - 2 * border,
		XtNheight, height,
		NULL);
    XtManageChild(menuBar);
}

/* ARGSUSED */
    void
gui_mch_add_menu(menu, parent, idx)
    VimMenu	*menu;
    VimMenu	*parent;
    int		idx;
{
    char_u	*pullright_name;
    Dimension	height, space, border;

    if (parent == NULL)
    {
	if (popup_menu(menu->dname))
	{
	    menu->submenu_id = XtVaCreatePopupShell((char *)menu->dname,
		simpleMenuWidgetClass, vimShell,
		XtNforeground, gui.menu_fg_pixel,
		XtNbackground, gui.menu_bg_pixel,
		NULL);
	}
	else if (menubar_menu(menu->dname))
	{
	    menu->id = XtVaCreateManagedWidget((char *)menu->dname,
		menuButtonWidgetClass, menuBar,
		XtNmenuName, menu->dname,
		XtNforeground, gui.menu_fg_pixel,
		XtNbackground, gui.menu_bg_pixel,
		NULL);
	    if (menu->id == (Widget)0)
		return;

	    menu->submenu_id = XtVaCreatePopupShell((char *)menu->dname,
		simpleMenuWidgetClass, menu->id,
		XtNforeground, gui.menu_fg_pixel,
		XtNbackground, gui.menu_bg_pixel,
		NULL);

	    /* Don't update the menu height when it was set at a fixed value */
	    if (!gui.menu_height_fixed)
	    {
		/*
		 * When we add a top-level item to the menu bar, we can figure
		 * out how high the menu bar should be.
		 */
		XtVaGetValues(menuBar,
			XtNvSpace,	&space,
			XtNborderWidth, &border,
			NULL);
		XtVaGetValues(menu->id,
			XtNheight,	&height,
			NULL);
		gui.menu_height = height + 2 * (space + border);
	    }

	    gui_athena_reorder_menus();
	}
    }
    else if (parent->submenu_id != (Widget)0)
    {
	menu->id = XtVaCreateManagedWidget((char *)menu->dname,
	    smeBSBObjectClass, parent->submenu_id,
	    XtNforeground, gui.menu_fg_pixel,
	    XtNbackground, gui.menu_bg_pixel,
	    XtNrightMargin, puller_width,
	    XtNrightBitmap, pullerBitmap,
	    NULL);
	if (menu->id == (Widget)0)
	    return;
	XtAddCallback(menu->id, XtNcallback, gui_x11_menu_cb,
	    (XtPointer)menu);

	pullright_name = make_pull_name(menu->dname);
	menu->submenu_id = XtVaCreatePopupShell((char *)pullright_name,
	    simpleMenuWidgetClass, parent->submenu_id,
	    XtNforeground, gui.menu_fg_pixel,
	    XtNbackground, gui.menu_bg_pixel,
	    XtNtranslations, menuTrans,
	    NULL);
	vim_free(pullright_name);

	XtOverrideTranslations(parent->submenu_id, parentTrans);
    }
}

/*
 * Make a submenu name into a pullright name.
 * Replace '.' by '_', can't include '.' in the submenu name.
 */
    static char_u *
make_pull_name(name)
    char_u * name;
{
    char_u  *pname;
    char_u  *p;

    pname = vim_strnsave(name, STRLEN(name) + strlen("-pullright"));
    if (pname != NULL)
    {
	strcat((char *)pname, "-pullright");
	while ((p = vim_strchr(pname, '.')) != NULL)
	    *p = '_';
    }
    return pname;
}

/* ARGSUSED */
    void
gui_mch_add_menu_item(menu, parent, idx)
    VimMenu	*menu;
    VimMenu	*parent;
    int		idx;
{
    /* Don't add menu separator */
    if (is_menu_separator(menu->name))
	return;

    if (parent != NULL && parent->submenu_id != (Widget)0)
    {
	menu->submenu_id = (Widget)0;
	menu->id = XtVaCreateManagedWidget((char *)menu->dname,
		smeBSBObjectClass, parent->submenu_id,
		XtNforeground, gui.menu_fg_pixel,
		XtNbackground, gui.menu_bg_pixel,
		NULL);
	if (menu->id == (Widget)0)
	    return;
	XtAddCallback(menu->id, XtNcallback, gui_x11_menu_cb,
		(XtPointer)menu);
    }
}

/* ARGSUSED */
    void
gui_mch_toggle_tearoffs(enable)
    int		enable;
{
    /* no tearoff menus */
}

    void
gui_mch_new_menu_colors()
{
    if (menuBar == NULL)
	return;
    XtVaSetValues(menuBar,
	XtNforeground, gui.menu_fg_pixel,
	XtNbackground, gui.menu_bg_pixel,
	XtNborderColor,	gui.menu_fg_pixel,
	NULL);

    gui_mch_submenu_colors(root_menu);
}

    static void
gui_mch_submenu_colors(mp)
    VimMenu	*mp;
{
    while (mp != NULL)
    {
	if (mp->id != (Widget)0)
	    XtVaSetValues(mp->id,
		    XtNforeground, gui.menu_fg_pixel,
		    XtNbackground, gui.menu_bg_pixel,
		    NULL);
	if (mp->submenu_id != (Widget)0)
	    XtVaSetValues(mp->submenu_id,
		    XtNforeground, gui.menu_fg_pixel,
		    XtNbackground, gui.menu_bg_pixel,
		    NULL);

	/* Set the colors for the children */
	if (mp->children != NULL)
	    gui_mch_submenu_colors(mp->children);
	mp = mp->next;
    }
}

/*
 * We can't always delete widgets, it would cause a crash.
 * Keep a list of dead widgets, so that we can avoid re-managing them.  This
 * means that they are still there but never shown.
 */
struct deadwid
{
    struct deadwid	*next;
    Widget		id;
};

static struct deadwid *first_deadwid = NULL;

/*
 * Destroy the machine specific menu widget.
 */
    void
gui_mch_destroy_menu(menu)
    VimMenu *menu;
{
    if (menu->submenu_id != (Widget)0)
    {
	XtDestroyWidget(menu->submenu_id);
	menu->submenu_id = (Widget)0;
    }
    if (menu->id != (Widget)0)
    {
#if 0
	Widget	parent;

	/*
	 * This is a hack to stop the Athena simpleMenuWidget from getting a
	 * BadValue error when a menu's last child is destroyed. We check to
	 * see if this is the last child and if so, don't delete it. The parent
	 * will be deleted soon anyway, and it will delete it's children like
	 * all good widgets do.
	 */
	parent = XtParent(menu->id);
	if (parent != menuBar)
	{
	    int num_children;

	    XtVaGetValues(parent, XtNnumChildren, &num_children, NULL);
	    if (num_children > 1)
		XtDestroyWidget(menu->id);
	}
	else
	    XtDestroyWidget(menu->id);
#else
	/*
	 * The code above causes a crash.  Apparently because the highlighting
	 * is still there, and removing it later causes the crash.
	 * This fix just unmanages the menu item, without destroying it.  The
	 * problem now is that the highlighting will be wrong, and we need to
	 * remember the ID to avoid that the item will be re-managed later...
	 */
	struct deadwid    *p;

	p = (struct deadwid *)alloc((unsigned)sizeof(struct deadwid));
	if (p != NULL)
	{
	    p->id = menu->id;
	    p->next = first_deadwid;
	    first_deadwid = p;
	}
	XtUnmanageChild(menu->id);
#endif
	menu->id = (Widget)0;
    }
}

/*
 * Reorder the menus to make them appear in the right order.
 * (This doesn't work for me...).
 */
    static void
gui_athena_reorder_menus()
{
    Widget	*children;
    Widget	swap_widget;
    int		num_children;
    int		to, from;
    VimMenu	*menu;
    struct deadwid *p;

    XtVaGetValues(menuBar,
	    XtNchildren,    &children,
	    XtNnumChildren, &num_children,
	    NULL);

    XtUnmanageChildren(children, num_children);

    menu = root_menu;
    for (to = 0; to < num_children - 1; ++to)
    {
	for (from = to; from < num_children; ++from)
	{
	    if (strcmp((char *)XtName(children[from]),
						    (char *)menu->dname) == 0)
	    {
		if (to != from)		/* need to move this one */
		{
		    swap_widget = children[to];
		    children[to] = children[from];
		    children[from] = swap_widget;
		}
		break;
	    }
	}
	menu = menu->next;
	if (menu == NULL)	/* cannot happen */
	    break;
    }
#if 1
    /* Only manage children that have not been destroyed */
    for (to = 0; to < num_children; ++to)
    {
	for (p = first_deadwid; p != NULL; p = p->next)
	    if (p->id == children[to])
		break;
	if (p == NULL)
	    XtManageChild(children[to]);
    }
#else
    XtManageChildren(children, num_children);
#endif
}

/* ARGSUSED */
    static void
gui_athena_pullright_action(w, event, args, nargs)
    Widget	w;
    XEvent	*event;
    String	*args;
    Cardinal	*nargs;
{
    Dimension	width, height;
    Widget	popup;

    if (event->type != MotionNotify)
	return;

    XtVaGetValues(w,
	XtNwidth,   &width,
	XtNheight,  &height,
	NULL);

    if (event->xmotion.x >= (int)width || event->xmotion.y >= (int)height)
	return;

    /* We do the pull-off when the pointer is in the rightmost 1/4th */
    if (event->xmotion.x < (int)(width * 3) / 4)
	return;

    popup = get_popup_entry(w);
    if (popup == (Widget)0)
	return;

    /* Don't Popdown the previous submenu now */
    XtOverrideTranslations(w, supermenuTrans);

    XtVaSetValues(popup,
	XtNx, event->xmotion.x_root,
	XtNy, event->xmotion.y_root - 7,
	NULL);

    XtOverrideTranslations(popup, menuTrans);

    XtPopup(popup, XtGrabNonexclusive);
}

/*
 * Called when a submenu with another submenu gets focus again.
 */
/* ARGSUSED */
    static void
gui_athena_pullleft_action(w, event, args, nargs)
    Widget	w;
    XEvent	*event;
    String	*args;
    Cardinal	*nargs;
{
    Widget	popup;
    Widget	parent;

    if (event->type != EnterNotify)
	return;

    /* Do Popdown the submenu now */
    popup = get_popup_entry(w);
    if (popup != (Widget)0)
	XtPopdown(popup);

    /* If this is the toplevel menu item, set parentTrans */
    if ((parent = XtParent(w)) != (Widget)0 && XtParent(parent) == menuBar)
	XtOverrideTranslations(w, parentTrans);
    else
	XtOverrideTranslations(w, menuTrans);
}

    static Widget
get_popup_entry(w)
    Widget  w;
{
    Widget	menuw;
    char_u	*pullright_name;
    Widget	popup;

    /* Get the active entry for the current menu */
    if ((menuw = XawSimpleMenuGetActiveEntry(w)) == (Widget)0)
	return NULL;

    pullright_name = make_pull_name((char_u *)XtName(menuw));
    popup = XtNameToWidget(w, (char *)pullright_name);
    vim_free(pullright_name);

    return popup;
}

/* ARGSUSED */
    void
gui_mch_show_popupmenu(menu)
    VimMenu *menu;
{
    int		rootx, rooty, winx, winy;
    Window	root, child;
    unsigned int mask;

    if (menu->submenu_id == (Widget)0)
	return;

    /* Position the popup menu at the pointer */
    if (XQueryPointer(gui.dpy, XtWindow(vimShell), &root, &child,
		&rootx, &rooty, &winx, &winy, &mask))
    {
	rootx -= 30;
	if (rootx < 0)
	    rootx = 0;
	rooty -= 5;
	if (rooty < 0)
	    rooty = 0;
	XtVaSetValues(menu->submenu_id,
		XtNx, rootx,
		XtNy, rooty,
		NULL);
    }

    XtOverrideTranslations(menu->submenu_id, popupTrans);
    XtPopupSpringLoaded(menu->submenu_id);
}

#endif /* WANT_MENU */


/*
 * Scrollbar stuff.
 */

    void
gui_mch_set_scrollbar_thumb(sb, val, size, max)
    GuiScrollbar    *sb;
    int		    val;
    int		    size;
    int		    max;
{
    double	    v, s;

    if (sb->id == (Widget)0)
	return;

    /*
     * Athena scrollbar must go from 0.0 to 1.0.
     */
    if (max == 0)
    {
	/* So you can't scroll it at all (normally it scrolls past end) */
	vim_XawScrollbarSetThumb(sb->id, 0.0, 1.0, 0.0);
    }
    else
    {
	v = (double)val / (double)(max + 1);
	s = (double)size / (double)(max + 1);
	vim_XawScrollbarSetThumb(sb->id, v, s, 1.0);
    }
}

    void
gui_mch_set_scrollbar_pos(sb, x, y, w, h)
    GuiScrollbar    *sb;
    int		    x;
    int		    y;
    int		    w;
    int		    h;
{
    if (sb->id == (Widget)0)
	return;

    XtUnmanageChild(sb->id);
    XtVaSetValues(sb->id,
		  XtNhorizDistance, x,
		  XtNvertDistance, y,
		  XtNwidth, w,
		  XtNheight, h,
		  NULL);
    XtManageChild(sb->id);
}

    void
gui_mch_create_scrollbar(sb, orient)
    GuiScrollbar    *sb;
    int		    orient;	/* SBAR_VERT or SBAR_HORIZ */
{
    sb->id = XtVaCreateWidget("scrollBar",
	    vim_scrollbarWidgetClass, vimForm,
	    XtNresizable,   True,
	    XtNtop,	    XtChainTop,
	    XtNbottom,	    XtChainTop,
	    XtNleft,	    XtChainLeft,
	    XtNright,	    XtChainLeft,
	    XtNborderWidth, 0,
	    XtNorientation, (orient == SBAR_VERT) ? XtorientVertical
						  : XtorientHorizontal,
	    XtNforeground, gui.scroll_fg_pixel,
	    XtNbackground, gui.scroll_bg_pixel,
	    NULL);
    if (sb->id == (Widget)0)
	return;

    XtAddCallback(sb->id, XtNjumpProc,
		  gui_athena_scroll_cb_jump, (XtPointer)sb->ident);
    XtAddCallback(sb->id, XtNscrollProc,
		  gui_athena_scroll_cb_scroll, (XtPointer)sb->ident);

    vim_XawScrollbarSetThumb(sb->id, 0.0, 1.0, 0.0);
}

    void
gui_mch_destroy_scrollbar(sb)
    GuiScrollbar    *sb;
{
    if (sb->id != (Widget)0)
	XtDestroyWidget(sb->id);
}

    void
gui_mch_set_scrollbar_colors(sb)
    GuiScrollbar    *sb;
{
    if (sb->id != (Widget)0)
	XtVaSetValues(sb->id,
	    XtNforeground, gui.scroll_fg_pixel,
	    XtNbackground, gui.scroll_bg_pixel,
	    NULL);
}

/*
 * Miscellaneous stuff:
 */
    Window
gui_x11_get_wid()
{
    return( XtWindow(textArea) );
}

#if defined(USE_BROWSE) || defined(PROTO)
/*
 * Put up a file requester.
 * Returns the selected name in allocated memory, or NULL for Cancel.
 */
/* ARGSUSED */
    char_u *
gui_mch_browse(saving, title, dflt, ext, initdir, filter)
    int		saving;		/* select file to write */
    char_u	*title;		/* not used (title for the window) */
    char_u	*dflt;		/* not used (default name) */
    char_u	*ext;		/* not used (extension added) */
    char_u	*initdir;	/* initial directory, NULL for current dir */
    char_u	*filter;	/* not used (file name filter) */
{
    Position x, y;

    /* Position the file selector just below the menubar */
    XtTranslateCoords(vimShell, (Position)0, (Position)
#ifdef WANT_MENU
	    gui.menu_height
#else
	    0
#endif
	    , &x, &y);
    return (char_u *)vim_SelFile(vimShell, (char *)title, (char *)initdir,
		  NULL, (int)x, (int)y, gui.menu_fg_pixel, gui.menu_bg_pixel);
}
#endif

#if defined(GUI_DIALOG) || defined(PROTO)

int	dialogStatus;

static void butproc __ARGS((Widget w, XtPointer client_data, XtPointer call_data));

/* ARGSUSED */
    static void
butproc(w, client_data, call_data)
    Widget	w;
    XtPointer	client_data;
    XtPointer	call_data;
{
    dialogStatus = (int)(long)client_data + 1;
}

/* ARGSUSED */
    int
gui_mch_dialog(type, title, message, buttons, dfltbutton)
    int		type;
    char_u	*title;
    char_u	*message;
    char_u	*buttons;
    int		dfltbutton;
{
    char_u		*buts;
    char_u		*p, *next;
    XtAppContext	app;
    XEvent		event;
    Position		wd, hd;
    Position		wv, hv;
    Position		x, y;
    Widget		dialog;
    Widget		dialogshell;
    Widget		dialogmessage;
    Widget		dialogButton;
    Widget		prev_dialogButton = NULL;
    int			butcount;
    int			vertical;

    if (title == NULL)
	title = (char_u *)"Vim dialog";
    dialogStatus = -1;

    /* if our pointer is currently hidden, then we should show it. */
    gui_mch_mousehide(FALSE);

    /* Check 'v' flag in 'guioptions': vertical button placement. */
    vertical = (vim_strchr(p_go, GO_VERTICAL) != NULL);

    /* The shell is created each time, to make sure it is resized properly */
    dialogshell = XtVaCreatePopupShell("dialogShell",
	    transientShellWidgetClass, vimShell,
	    XtNlabel, title,
	    NULL);
    if (dialogshell == (Widget)0)
	goto error;
    dialog = XtVaCreateManagedWidget("dialog",
	    formWidgetClass, dialogshell,
	    XtNdefaultDistance, 20,
	    XtNforeground, gui.menu_fg_pixel,
	    XtNbackground, gui.menu_bg_pixel,
	    NULL);
    if (dialog == (Widget)0)
	goto error;
    dialogmessage = XtVaCreateManagedWidget("dialogMessage",
	    labelWidgetClass, dialog,
	    XtNlabel, message,
	    XtNtop, XtChainTop,
	    XtNbottom, XtChainTop,
	    XtNleft, XtChainLeft,
	    XtNright, XtChainLeft,
	    XtNresizable, True,
	    XtNborderWidth, 0,
	    XtNforeground, gui.menu_fg_pixel,
	    XtNbackground, gui.menu_bg_pixel,
	    NULL);

    /* make a copy, so that we can insert NULs */
    buts = vim_strsave(buttons);
    if (buts == NULL)
	return -1;

    p = buts;
    for (butcount = 0; *p; ++butcount)
    {
	for (next = p; *next; ++next)
	{
	    if (*next == DLG_HOTKEY_CHAR)
		mch_memmove(next, next + 1, STRLEN(next));
	    if (*next == DLG_BUTTON_SEP)
	    {
		*next++ = NUL;
		break;
	    }
	}
	dialogButton = XtVaCreateManagedWidget("button",
		commandWidgetClass, dialog,
		XtNlabel, p,
		XtNtop, XtChainBottom,
		XtNbottom, XtChainBottom,
		XtNleft, XtChainLeft,
		XtNright, XtChainLeft,
		XtNfromVert, dialogmessage,
		XtNvertDistance, vertical ? 4 : 20,
		XtNforeground, gui.menu_fg_pixel,
		XtNbackground, gui.menu_bg_pixel,
		XtNresizable, False,
		NULL);
	if (butcount > 0)
	    XtVaSetValues(dialogButton,
		    vertical ? XtNfromVert : XtNfromHoriz, prev_dialogButton,
		    NULL);

	XtAddCallback(dialogButton, XtNcallback, butproc, (XtPointer)butcount);
	p = next;
	prev_dialogButton = dialogButton;
    }
    vim_free(buts);

    XtRealizeWidget(dialogshell);

    XtVaGetValues(dialogshell,
	    XtNwidth, &wd,
	    XtNheight, &hd,
	    NULL);
    XtVaGetValues(vimShell,
	    XtNwidth, &wv,
	    XtNheight, &hv,
	    NULL);
    XtTranslateCoords(vimShell,
	    (Position)((wv - wd) / 2),
	    (Position)((hv - hd) / 2),
	    &x, &y);
    if (x < 0)
	x = 0;
    if (y < 0)
	y = 0;
    XtVaSetValues(dialogshell, XtNx, x, XtNy, y, NULL);

    app = XtWidgetToApplicationContext(dialogshell);

    XtPopup(dialogshell, XtGrabNonexclusive);

    while (1)
    {
	XtAppNextEvent(app, &event);
	XtDispatchEvent(&event);
	if (dialogStatus >= 0)
	    break;
    }

    XtPopdown(dialogshell);

error:
    XtDestroyWidget(dialogshell);

    return dialogStatus;
}
#endif
