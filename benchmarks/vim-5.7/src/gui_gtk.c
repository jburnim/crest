/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved		by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

/*
 * Porting to GTK+ was done by:
 *
 * (C) 1998,1999 by Marcin Dalecki <dalecki@cs.net.pl>
 * with GREAT support and continuous encouragements by Andy Kahn and of
 * course Bram Moolenaar!
 *
 *	"I'm a one-man software company. If you have anything UNIX, net or
 *	embedded systems related, which seems to cause some serious trouble for
 *	your's in-house developers, maybe we need to talk badly with each other
 *	:-) <dalecki@cs.net.pl> (My native language is polish and I speak
 *	native grade german too. I'm living in Göttingen.de.)
 *	--mdcki"
 *
 *
 * Although some #ifdefs suggest that GTK 1.0 is supported, it isn't.  The
 * code requires GTK version 1.1.16 or later.  Stuff for older versions will
 * be removed some time.
 */

#include "gui_gtk_f.h"

#ifdef MIN
# undef MIN
#endif
#ifdef MAX
# undef MAX
#endif

#include "vim.h"

#ifdef GUI_DIALOG
# include "../pixmaps/alert.xpm"
# include "../pixmaps/error.xpm"
# include "../pixmaps/generic.xpm"
# include "../pixmaps/info.xpm"
# include "../pixmaps/quest.xpm"
#endif

#ifdef USE_TOOLBAR
/*
 * Icons used by the toolbar code.
 */
#include "../pixmaps/tb_new.xpm"
#include "../pixmaps/tb_open.xpm"
#include "../pixmaps/tb_close.xpm"
#include "../pixmaps/tb_save.xpm"
#include "../pixmaps/tb_print.xpm"
#include "../pixmaps/tb_cut.xpm"
#include "../pixmaps/tb_copy.xpm"
#include "../pixmaps/tb_paste.xpm"
#include "../pixmaps/tb_find.xpm"
#include "../pixmaps/tb_find_next.xpm"
#include "../pixmaps/tb_find_prev.xpm"
#include "../pixmaps/tb_find_help.xpm"
#include "../pixmaps/tb_exit.xpm"
#include "../pixmaps/tb_undo.xpm"
#include "../pixmaps/tb_redo.xpm"
#include "../pixmaps/tb_help.xpm"
#include "../pixmaps/tb_macro.xpm"
#include "../pixmaps/tb_make.xpm"
#include "../pixmaps/tb_save_all.xpm"
#include "../pixmaps/tb_jump.xpm"
#include "../pixmaps/tb_ctags.xpm"
#include "../pixmaps/tb_load_session.xpm"
#include "../pixmaps/tb_save_session.xpm"
#include "../pixmaps/tb_new_session.xpm"
#include "../pixmaps/tb_blank.xpm"
#include "../pixmaps/tb_maximize.xpm"
#include "../pixmaps/tb_split.xpm"
#include "../pixmaps/tb_minimize.xpm"
#include "../pixmaps/tb_shell.xpm"
#include "../pixmaps/tb_replace.xpm"
#endif

#include <gdk/gdkkeysyms.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>

#include <gtk/gtk.h>

/*
 * Flags used to distinguish the different contexts in which the
 * find/replace callback may be called.
 */
#define FR_DIALOGTERM	1
#define FR_FINDNEXT	2
#define FR_R_FINDNEXT	3
#define FR_REPLACE	4
#define FR_REPLACEALL	5

static void entry_activate_cb(GtkWidget *widget, GtkWidget *with);
static void entry_changed_cb(GtkWidget *entry, GtkWidget *dialog);
static void find_direction_cb(GtkWidget *widget, gpointer data);
static void find_replace_cb(GtkWidget *widget, unsigned int flags);
static void exact_match_cb(GtkWidget *widget, gpointer data);
static void repl_dir_cb(GtkWidget * widget, gpointer data);

#ifdef USE_TOOLBAR
static void get_pixmap(char *menuname, GdkPixmap **pixmap, GdkBitmap **mask);
static void pixmap_create_blank(GdkPixmap **pixmap, GdkBitmap **mask);
static void pixmap_create_by_name(char *name, GdkPixmap **, GdkBitmap **mask);
static void pixmap_create_by_num(int pixmap_num, GdkPixmap **, GdkBitmap **);
static void pixmap_create_by_dir(char *name, GdkPixmap **, GdkBitmap **mask);
#endif

#if defined(WANT_MENU) || defined(PROTO)

/*
 * Only use accelerators when gtk_menu_ensure_uline_accel_group() is
 * available, which is in version 1.2.1.  That was the first version where
 * accelerators properly worked (according to the change log).
 */
#ifdef GTK_CHECK_VERSION
# if GTK_CHECK_VERSION(1, 2, 1)
#  define GTK_USE_ACCEL
# endif
#endif

/*
 * Create a highly customized menu item by hand instead of by using:
 *
 * gtk_menu_item_new_with_label(menu->dname);
 *
 * This is neccessary, since there is no other way in GTK+ to get the
 * not automatically parsed accellerator stuff right.
 */
/*ARGSUSED*/
static void
menu_item_new(VimMenu *menu, GtkWidget *parent_widget, int sub_menu)
{
    char *name, *tmp;
    GtkWidget *widget;
#ifdef GTK_HAVE_FEATURES_1_1_0
    GtkWidget *bin, *label;
    int num;
    guint accel_key;

    widget = gtk_widget_new(GTK_TYPE_MENU_ITEM,
			    "GtkWidget::visible", TRUE,
			    "GtkWidget::sensitive", TRUE,
			    /* "GtkWidget::parent", parent->submenu_id, */
			    NULL);
    bin = gtk_widget_new(GTK_TYPE_HBOX,
			 "GtkWidget::visible", TRUE,
			 "GtkWidget::parent", widget,
			 "GtkBox::spacing", 16,
			 NULL);
    label = gtk_widget_new(GTK_TYPE_ACCEL_LABEL,
			   "GtkWidget::visible", TRUE,
			   "GtkWidget::parent", bin,
			   "GtkAccelLabel::accel_widget", widget,
			   "GtkMisc::xalign", 0.0,
			   NULL);
    if (menu->actext)
	gtk_widget_new(GTK_TYPE_LABEL,
		       "GtkWidget::visible", TRUE,
		       "GtkWidget::parent", bin,
		       "GtkLabel::label", menu->actext,
		       "GtkMisc::xalign", 1.0,
			NULL);

    /*
     * Translate VIM accelerator tagging into GTK+'s.  Note that since GTK uses
     * underscores as the accelerator key, we need to add an additional under-
     * score for each understore that appears in the menu name.
     *
     * First count how many underscore's are in the menu name.
     */
    for (num = 0, tmp = (char *)menu->name; *tmp; tmp++)
	if (*tmp == '_')
	    num++;

    /*
     * now allocate a new buffer to hold all the menu name along with the
     * additional underscores.
     */
    name = g_new(char, strlen((char *)menu->name) + num + 1);
    for (num = 0, tmp = (char *)menu->name; *tmp; ++tmp) {
	/* actext has been added above, stop at the TAB */
	if (*tmp == TAB)
	    break;
	if (*tmp == '&') {
# ifdef GTK_USE_ACCEL
	    if (*p_wak != 'n') {
		name[num] = '_';
		num++;
	    }
# endif
	} else {
	    name[num] = *tmp;
	    num++;
	    if (*tmp == '_') {
		name[num] = '_';
		num++;
	    }
	}
    }
    name[num] = '\0';

    /* let GTK do its thing */
    accel_key = gtk_label_parse_uline(GTK_LABEL(label), name);
    g_free(name);

# ifdef GTK_USE_ACCEL
    /* Don't add accelator if 'winaltkeys' is "no" */
    if (accel_key != GDK_VoidSymbol && *p_wak != 'n') {
	if (GTK_IS_MENU_BAR(parent_widget)) {
	    gtk_widget_add_accelerator(widget,
				   "activate_item",
				   gui.accel_group,
				   accel_key, GDK_MOD1_MASK,
				   GTK_ACCEL_LOCKED);
	} else {
	    gtk_widget_add_accelerator(widget,
				   "activate_item",
				   gtk_menu_ensure_uline_accel_group(GTK_MENU(parent_widget)),
				   accel_key, 0,
				   GTK_ACCEL_LOCKED);
	}
    }
# endif

    menu->id = widget;
#else
    char *tmp2;

    /*
     * gtk+-1.0.x does not have menu accelerators, so if we see a '&' in the
     * vim menu, skip it.
     */
    name = g_strdup((const gchar *)(menu->name));
    for (tmp = (char *)(menu->name), tmp2 = name; *tmp; tmp++) {
	if (*tmp != '&') {
	    *tmp2 = *tmp;
	    tmp2++;
	}
    }
    *tmp2 = '\0';
    widget = gtk_menu_item_new_with_label(name);
    g_free(name);
    menu->id = widget;
#endif
}


/*ARGSUSED*/
void
gui_mch_add_menu(VimMenu * menu, VimMenu * parent, int idx)
{
    if (popup_menu(menu->name)) {
	menu->submenu_id = gtk_menu_new();
	return;
    }

    if (menubar_menu(menu->name) == 0 ||
	(parent != NULL && parent->submenu_id == 0)) {
	return;
    }

    if (parent == NULL)
	menu_item_new(menu, gui.menubar, TRUE);
    else
	menu_item_new(menu, parent->submenu_id, TRUE);

    if (menu->id == NULL)
	return;			/* failed */

    if (parent == NULL)
	gtk_menu_bar_insert(GTK_MENU_BAR(gui.menubar), menu->id, idx);
    else {
	/* since the tearoff should always appear first, increment idx */
	++idx;
	gtk_menu_insert(GTK_MENU(parent->submenu_id), menu->id, idx);
    }

    /*
     * The "Help" menu is a special case, and should be placed at the far right
     * hand side of the menu-bar.
     */
    if (parent == NULL && STRCMP((char *) menu->dname, "Help") == 0)
	gtk_menu_item_right_justify(GTK_MENU_ITEM(menu->id));

    if ((menu->submenu_id = gtk_menu_new()) == NULL)	/* failed */
	return;

#ifdef GTK_HAVE_FEATURES_1_1_0
    gtk_menu_set_accel_group(GTK_MENU(menu->submenu_id), gui.accel_group);
#endif
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu->id), menu->submenu_id);

#ifdef GTK_HAVE_FEATURES_1_1_0
    menu->tearoff_handle = gtk_tearoff_menu_item_new();
    gtk_widget_show(menu->tearoff_handle);
    gtk_menu_prepend(GTK_MENU(menu->submenu_id), menu->tearoff_handle);
#endif
}

/*ARGSUSED*/
static void
menu_item_activate(GtkWidget * widget, gpointer data)
{
    gui_menu_cb((VimMenu *) data);

    /* make sure the menu action is taken immediately */
    if (gtk_main_level() > 0)
	gtk_main_quit();
}

/*ARGSUSED*/
void
gui_mch_add_menu_item(VimMenu * menu, VimMenu * parent, int idx)
{
# ifdef USE_TOOLBAR
    if (toolbar_menu(parent->name)) {
	if (is_menu_separator(menu->name)) {
	    gtk_toolbar_insert_space(GTK_TOOLBAR(gui.toolbar), idx);
	} else {
	    GdkPixmap *pixmap;
	    GdkBitmap *mask;

	    get_pixmap((char *)(menu->name), &pixmap, &mask);
	    if (pixmap == NULL)
		return; /* should at least have blank pixmap, but if not... */

	    menu->id = gtk_toolbar_insert_item(
				    GTK_TOOLBAR(gui.toolbar),
				    (char *)(menu->dname),
				    (char *)(menu->strings[MENU_INDEX_TIP]),
				    (char *)(menu->dname),
				    gtk_pixmap_new(pixmap, mask),
				    GTK_SIGNAL_FUNC(menu_item_activate),
				    (gpointer)menu,
				    idx);
	}
	menu->parent = parent;
	menu->submenu_id = NULL;
	return;
    } /* toolbar menu item */
# endif

    /* No parent, must be a non-menubar menu */
    if (parent->submenu_id == 0)
	return;

    /* make place for the possible tearoff handle item */
    ++idx;
    if (is_menu_separator(menu->name)) {
	/* Separator: Just add it */
	menu->id = gtk_menu_item_new();
	gtk_widget_show(menu->id);
	gtk_menu_insert(GTK_MENU(parent->submenu_id), menu->id, idx);

	return;
    }

    /* Add textual menu item. */
    menu_item_new(menu, parent->submenu_id, FALSE);
    gtk_widget_show(menu->id);
    gtk_menu_insert(GTK_MENU(parent->submenu_id), menu->id, idx);

    if (menu->id != 0)
	gtk_signal_connect(GTK_OBJECT(menu->id), "activate",
		GTK_SIGNAL_FUNC(menu_item_activate), (gpointer) menu);
}
#endif


#ifdef USE_TOOLBAR
/*
 * Those are the pixmaps used for the default buttons.
 */
struct NameToPixmap {
    char *name;
    char **xpm;
};

static const struct NameToPixmap built_in_pixmaps[] =
{
    {"New", tb_new_xpm},
    {"Open", tb_open_xpm},
    {"Save", tb_save_xpm},
    {"Undo", tb_undo_xpm},
    {"Redo", tb_redo_xpm},
    {"Cut", tb_cut_xpm},
    {"Copy", tb_copy_xpm},
    {"Paste", tb_paste_xpm},
    {"Print", tb_print_xpm},
    {"Help", tb_help_xpm},
    {"Find", tb_find_xpm},
    {"SaveAll",	tb_save_all_xpm},
    {"SaveSesn", tb_save_session_xpm},
    {"NewSesn", tb_new_session_xpm},
    {"LoadSesn", tb_load_session_xpm},
    {"RunScript", tb_macro_xpm},
    {"Replace",	tb_replace_xpm},
    {"WinClose", tb_close_xpm},
    {"WinMax",	tb_maximize_xpm},
    {"WinMin", tb_minimize_xpm},
    {"WinSplit", tb_split_xpm},
    {"Shell", tb_shell_xpm},
    {"FindPrev", tb_find_prev_xpm},
    {"FindNext", tb_find_next_xpm},
    {"FindHelp", tb_find_help_xpm},
    {"Make", tb_make_xpm},
    {"TagJump", tb_jump_xpm},
    {"RunCtags", tb_ctags_xpm},
    {"Exit", tb_exit_xpm},
    { NULL, NULL} /* end tag */
};


/*
 * do ":h toolbar" for details on the order of things searched to
 * find the toolbar pixmap.
 */
static void
get_pixmap(char *menuname, GdkPixmap **pixmap, GdkBitmap **mask)
{
    int builtin_num;

    *pixmap = NULL;
    *mask = NULL;
    if (strncmp(menuname, "BuiltIn", (size_t)7) == 0) {
	if (isdigit((int)menuname[7]) && isdigit((int)menuname[8])) {
	    builtin_num = atoi(menuname + 7);
	    pixmap_create_by_num(builtin_num, pixmap, mask);
	} else {
	    pixmap_create_blank(pixmap, mask);
	}
    } else {
	pixmap_create_by_dir(menuname, pixmap, mask);
	if (*pixmap == NULL) {
	    pixmap_create_by_name(menuname, pixmap, mask);
	    if (*pixmap == NULL)
		pixmap_create_blank(pixmap, mask);
	}
    }
}


/*
 * creates a blank pixmap using tb_blank
 */
static void
pixmap_create_blank(GdkPixmap **pixmap, GdkBitmap **mask)
{
    *pixmap = gdk_pixmap_colormap_create_from_xpm_d(
				    NULL,
				    gtk_widget_get_colormap(gui.mainwin),
				    mask,
				    NULL,
				    tb_blank_xpm);
}


/*
 * creates a pixmap using one of the built-in pixmap names
 */
static void
pixmap_create_by_name(char *name, GdkPixmap **pixmap, GdkBitmap **mask)
{
    const struct NameToPixmap *tmp = built_in_pixmaps;

    /* lookup if we have a corresponding build-in pixmap */
    for (tmp = built_in_pixmaps; tmp->name; tmp++) {
	if (!STRCMP(tmp->name, name))
	    break;
    }

    *pixmap = gdk_pixmap_colormap_create_from_xpm_d(
				NULL,
			        gtk_widget_get_colormap(gui.mainwin),
				mask,
				NULL,
				(tmp->xpm) ? tmp->xpm : tb_blank_xpm);
}


/*
 * creates a pixmap by using a built-in number
 */
static void
pixmap_create_by_num(int pixmap_num, GdkPixmap **pixmap, GdkBitmap **mask)
{
    int num_pixmaps;

    num_pixmaps = (sizeof(built_in_pixmaps) / sizeof(built_in_pixmaps[0])) - 1;

    if (pixmap_num < 0 || pixmap_num >= num_pixmaps)
	*pixmap = gdk_pixmap_colormap_create_from_xpm_d(
				    NULL,
				    gtk_widget_get_colormap(gui.mainwin),
				    mask,
				    NULL,
				    tb_blank_xpm);
    else
	*pixmap = gdk_pixmap_colormap_create_from_xpm_d(
				    NULL,
				    gtk_widget_get_colormap(gui.mainwin),
				    mask,
				    NULL,
				    built_in_pixmaps[pixmap_num].xpm);
}

/*
 * creates a pixmap by using the pixmap name found in $VIM/bitmaps/
 *
 * MAYBE: if the custom pixmap found in the directory is "big", should we scale
 * the bitmap like the help documentation says so or leave it alone?  i think
 * it's user error if there is a bitmap of the wrong size.
 */
static void
pixmap_create_by_dir(char *name, GdkPixmap **pixmap, GdkBitmap **mask)
{
    char_u *full_pathname;

    if ((full_pathname = (char_u *)alloc(MAXPATHL+1)) == NULL) {
	pixmap_create_blank(pixmap, mask);
    } else {
	expand_env((char_u *)"$VIM/bitmaps/",
		   full_pathname,
		   (MAXPATHL + 1) - strlen(name) - 5);
	STRCAT(full_pathname, name);
	STRCAT(full_pathname, ".xpm");
	if (mch_access((const char *)full_pathname, F_OK) == 0)
	    *pixmap = gdk_pixmap_colormap_create_from_xpm(
				    NULL,
				    gtk_widget_get_colormap(gui.mainwin),
				    mask,
				    &gui.mainwin->style->bg[GTK_STATE_NORMAL],
				    (const char *)full_pathname);
	else
	    *pixmap = NULL;

	vim_free(full_pathname);
    }
}
#endif


void
gui_mch_set_text_area_pos(int x, int y, int w, int h)
{
    gtk_form_move_resize(GTK_FORM(gui.formwin), gui.drawarea, x, y, w, h);
}


#if defined(WANT_MENU) || defined(PROTO)
/*
 * Enable or disable mnemonics for the toplevel menus.
 */
/*ARGSUSED*/
void
gui_gtk_set_mnemonics(int enable)
{
#if 0
    VimMenu *menu;
    /*  FIXME: implement this later */
    for (menu = root_menu; menu != NULL; menu = menu->next)
	if (menu->id != 0)
	    XtVaSetValues(menu->id,
		    XmNmnemonic, enable ? menu->mnemonic : NUL,
		    NULL);
#endif
}


static void
recurse_tearoffs(VimMenu * menu, int val)
{
#ifdef GTK_HAVE_FEATURES_1_1_0
    while (menu != NULL) {
	if (!popup_menu(menu->name)) {
	    if (menu->submenu_id != 0) {
		if (val)
		    gtk_widget_show(menu->tearoff_handle);
		else
		    gtk_widget_hide(menu->tearoff_handle);
	    }
	    recurse_tearoffs(menu->children, val);
	}
	menu = menu->next;
    }
#endif
}


void
gui_mch_toggle_tearoffs(int enable)
{
    recurse_tearoffs(root_menu, enable);
}
#endif


#ifdef USE_TOOLBAR

/*
 * Seems like there's a hole in the GTK Toolbar API: there's no provision for
 * removing an item from the toolbar.  Therefore I need to resort to going
 * really deeply into the internal widget structures.
 */
static void
toolbar_remove_item_by_text(GtkToolbar *tb, const char *text)
{
	GtkContainer *container;
	GList *childl;
	GtkToolbarChild *gtbc;

	g_return_if_fail(tb != NULL);
	g_return_if_fail(GTK_IS_TOOLBAR(tb));
	container = GTK_CONTAINER(&tb->container);

	for (childl = tb->children; childl; childl = childl->next) {
		gtbc = (GtkToolbarChild *)childl->data;

		if (gtbc->type != GTK_TOOLBAR_CHILD_SPACE &&
		    strcmp(GTK_LABEL(gtbc->label)->label, text) == 0) {
			gboolean was_visible;

			was_visible = GTK_WIDGET_VISIBLE(gtbc->widget);
			gtk_widget_unparent(gtbc->widget);

			tb->children = g_list_remove_link(tb->children, childl);
			g_free(gtbc);
			g_list_free(childl);
			tb->num_children--;

			if (was_visible && GTK_WIDGET_VISIBLE(container))
				gtk_widget_queue_resize(GTK_WIDGET(container));

			break;
		}
	}
}
#endif

#if defined(WANT_MENU) || defined(PROTO)
/*
 * Destroy the machine specific menu widget.
 */
void
gui_mch_destroy_menu(VimMenu * menu)
{
#ifdef USE_TOOLBAR
    if (menu->parent && toolbar_menu(menu->parent->name)) {
	toolbar_remove_item_by_text(GTK_TOOLBAR(gui.toolbar),
					    (const char *)menu->dname);
	return;
    }
#endif

    if (menu->submenu_id != 0) {
	gtk_widget_destroy(menu->submenu_id);
	menu->submenu_id = 0;
    }
    if (menu->id != 0) {
	/* parent = gtk_widget_get_parent(menu->id); */
	gtk_widget_destroy(menu->id);
	menu->id = 0;
    }
}
#endif /* WANT_MENU */


/*
 * Scrollbar stuff.
 */

#ifdef GTK_HAVE_FEATURES_1_1_0
/* This variable is set when we asked for a scrollbar change ourselves.  Don't
 * pass scrollbar changes on to the GUI code then. */
static int did_ask_for_change = FALSE;
#endif

void
gui_mch_set_scrollbar_thumb(GuiScrollbar * sb, int val, int size, int max)
{
    if (sb->id != 0) {
	GtkAdjustment *adjustment = GTK_RANGE(sb->id)->adjustment;
	adjustment->lower = 0;
	adjustment->value = val;
	adjustment->upper = max + 1;
	adjustment->page_size = size;
	adjustment->page_increment = (size > 2 ? size - 2 : 1);
	adjustment->step_increment = 1;
#ifdef GTK_HAVE_FEATURES_1_1_0
	did_ask_for_change = TRUE;
	gtk_adjustment_changed(adjustment);
	did_ask_for_change = FALSE;
#endif
    }
}

void
gui_mch_set_scrollbar_pos(GuiScrollbar * sb, int x, int y, int w, int h)
{
    if (!sb->id)
	return;
    gtk_form_move_resize(GTK_FORM(gui.formwin), sb->id, x, y, w, h);
}

/*
 * Take action upon scrollbar dragging.
 */
static void
adjustment_value_changed(GtkAdjustment * adjustment, gpointer data)
{
    GuiScrollbar *sb;
    long value;

#ifdef GTK_HAVE_FEATURES_1_1_0
    if (did_ask_for_change)
	return;
#endif

    sb = gui_find_scrollbar((long) data);
    value = adjustment->value;

    /*
     * We just ignore the dragging argument, since otherwise the scrollbar
     * size will not be adjusted properly in synthetic scrolls.
     */
    gui_drag_scrollbar(sb, value, FALSE);
    if (gtk_main_level() > 0)
	gtk_main_quit();
}

/* SBAR_VERT or SBAR_HORIZ */
void
gui_mch_create_scrollbar(GuiScrollbar * sb, int orient)
{
    if (orient == SBAR_HORIZ) {
	sb->id = gtk_hscrollbar_new(NULL);
	GTK_WIDGET_UNSET_FLAGS(sb->id, GTK_CAN_FOCUS);
	gtk_widget_show(sb->id);
	gtk_form_put(GTK_FORM(gui.formwin), sb->id, 0, 0);
    }
    if (orient == SBAR_VERT) {
	sb->id = gtk_vscrollbar_new(NULL);
	GTK_WIDGET_UNSET_FLAGS(sb->id, GTK_CAN_FOCUS);
	gtk_widget_show(sb->id);
	gtk_form_put(GTK_FORM(gui.formwin), sb->id, 0, 0);
    }
    if (sb->id != NULL) {
	GtkAdjustment *adjustment;

	adjustment = gtk_range_get_adjustment(
		GTK_RANGE((GtkScrollbar *)sb->id));
	gtk_signal_connect(GTK_OBJECT(adjustment), "value_changed",
		(GtkSignalFunc) adjustment_value_changed,
			   (gpointer) sb->ident);
    }
    gui_mch_update();
}

void
gui_mch_destroy_scrollbar(GuiScrollbar * sb)
{
    if (sb->id != 0) {
	gtk_widget_destroy(sb->id);
	sb->id = 0;
    }
    gui_mch_update();
}

#if defined(USE_BROWSE) || defined(PROTO)
/*
 * Implementation of the file selector related stuff
 */

/*ARGSUSED*/
static void
browse_ok_cb(GtkWidget *widget, gpointer cbdata)
{
    Gui *vw = (Gui *)cbdata;

    if (vw->browse_fname != NULL)
	g_free(vw->browse_fname);

    vw->browse_fname = (char_u *)g_strdup(gtk_file_selection_get_filename(
					GTK_FILE_SELECTION(vw->filedlg)));
    gtk_widget_hide(vw->filedlg);
    if (gtk_main_level() > 0)
	gtk_main_quit();
}

/*ARGSUSED*/
static void
browse_cancel_cb(GtkWidget *widget, gpointer cbdata)
{
    Gui *vw = (Gui *)cbdata;

    if (vw->browse_fname != NULL)
    {
	g_free(vw->browse_fname);
	vw->browse_fname = NULL;
    }
    gtk_widget_hide(vw->filedlg);
    if (gtk_main_level() > 0)
	gtk_main_quit();
}

/*ARGSUSED*/
static gboolean
browse_destroy_cb(GtkWidget * widget)
{
    if (gui.browse_fname != NULL)
    {
	g_free(gui.browse_fname);
	gui.browse_fname = NULL;
    }
    gui.filedlg = NULL;

    if (gtk_main_level() > 0)
	gtk_main_quit();

    return FALSE;
}

/*
 * Put up a file requester.
 * Returns the selected name in allocated memory, or NULL for Cancel.
 * saving,			select file to write
 * title			title for the window
 * dflt				default name
 * ext				not used (extension added)
 * initdir			initial directory, NULL for current dir
 * filter			not used (file name filter)
 */
/*ARGSUSED*/
char_u *
gui_mch_browse(int saving,
	       char_u * title,
	       char_u * dflt,
	       char_u * ext,
	       char_u * initdir,
	       char_u * filter)
{
    GtkFileSelection *fs;	/* shortcut */
    char_u dirbuf[MAXPATHL];

    if (!gui.filedlg)
    {
	gui.filedlg = gtk_file_selection_new((const gchar *)title);
#ifdef GTK_HAVE_FEATURES_1_1_4
	gtk_window_set_modal(GTK_WINDOW(gui.filedlg), TRUE);
	gtk_window_set_transient_for(GTK_WINDOW(gui.filedlg),
		GTK_WINDOW(gui.mainwin));
#endif
	fs = GTK_FILE_SELECTION(gui.filedlg);

	gtk_container_border_width(GTK_CONTAINER(fs), 4);
	gtk_file_selection_hide_fileop_buttons(fs);

	gtk_signal_connect(GTK_OBJECT(fs->ok_button),
		"clicked", GTK_SIGNAL_FUNC(browse_ok_cb), &gui);
	gtk_signal_connect(GTK_OBJECT(fs->cancel_button),
		"clicked", GTK_SIGNAL_FUNC(browse_cancel_cb), &gui);
	/* gtk_signal_connect() doesn't work for destroy, it causes a hang */
	gtk_signal_connect_object(GTK_OBJECT(gui.filedlg),
		"destroy", GTK_SIGNAL_FUNC(browse_destroy_cb),
		GTK_OBJECT(gui.filedlg));
    }
    else
	gtk_window_set_title(GTK_WINDOW(gui.filedlg), (const gchar *)title);

    if (dflt == NULL)
	dflt = (char_u *)"";
    if (initdir == NULL || *initdir == NUL)
    {
	mch_dirname(dirbuf, MAXPATHL);
	strcat((char *)dirbuf, "/");	/* make sure this is a directory */
	initdir = dirbuf;
    }
    gtk_file_selection_set_filename(GTK_FILE_SELECTION(gui.filedlg),
						      (const gchar *)initdir);

    gtk_window_position(GTK_WINDOW(gui.filedlg), GTK_WIN_POS_MOUSE);

    gtk_widget_show(gui.filedlg);
    while (gui.filedlg && GTK_WIDGET_VISIBLE(gui.filedlg))
	gtk_main_iteration_do(TRUE);

    if (gui.browse_fname == NULL)
	return NULL;
    return vim_strsave(gui.browse_fname);
}

#endif	/* USE_BROWSE */

#ifdef GUI_DIALOG

typedef struct _ButtonData {
    int	       *status;
    int	       index;
    GtkWidget  *dialog;
} ButtonData;

typedef struct _CancelData {
    int	      *status;
    GtkWidget *dialog;
} CancelData;

/* ARGSUSED */
static void
dlg_button_clicked(GtkWidget * widget, ButtonData *data)
{
    *(data->status) = data->index + 1;
    gtk_widget_destroy(data->dialog);
}

/*
 * This makes the Escape key equivalent to the cancel button.
 */

/*ARGSUSED*/
static int
dlg_key_press_event(GtkWidget * widget, GdkEventKey * event, CancelData *data)
{
    if (event->keyval != GDK_Escape)
	return FALSE;

    /* The result value of 0 from a dialog is signaling cancelation. */
    *(data->status) = 0;
    gtk_widget_destroy(data->dialog);

    return TRUE;
}

/* ARGSUSED */
int
gui_mch_dialog(int type,		/* type of dialog */
	       char_u * title,		/* title of dialog */
	       char_u * message,	/* message text */
	       char_u * buttons,	/* names of buttons */
	       int def_but)		/* default button */
{
    char_u		*names;
    char_u		*p;
    int			i;
    int			butcount;
    int			dialog_status = -1;
    int			vertical;

    GtkWidget		*dialog;
    GtkWidget		*frame;
    GtkWidget		*vbox;
    GtkWidget		*table;
    GtkWidget		*pixmap;
    GtkWidget		*dialogmessage;
    GtkWidget		*action_area;
    GtkWidget		*sub_area;
    GtkWidget		*separator;
    GtkAccelGroup	*accel_group;

    GdkPixmap		*icon = NULL;
    GdkBitmap		*mask = NULL;
    char		**icon_data = NULL;

    GtkWidget		**button;
    ButtonData		*data;
    CancelData		cancel_data;

    if (title == NULL)
	title = (char_u *) "Vim dialog...";

    if ((type < 0) || (type > VIM_LAST_TYPE))
	type = VIM_GENERIC;

    /* Check 'v' flag in 'guioptions': vertical button placement. */
    vertical = (vim_strchr(p_go, GO_VERTICAL) != NULL);

    /* if our pointer is currently hidden, then we should show it. */
    gui_mch_mousehide(FALSE);

    dialog = gtk_window_new(GTK_WINDOW_DIALOG);
    gtk_window_set_title(GTK_WINDOW(dialog), (const gchar *)title);
    gtk_window_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);
#ifdef GTK_HAVE_FEATURES_1_1_4
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(gui.mainwin));
#endif
    gtk_widget_realize(dialog);
    gdk_window_set_decorations(dialog->window, GDK_DECOR_BORDER);
    gdk_window_set_functions(dialog->window, GDK_FUNC_MOVE);

    cancel_data.status = &dialog_status;
    cancel_data.dialog = dialog;
    gtk_signal_connect_after(GTK_OBJECT(dialog), "key_press_event",
		    GTK_SIGNAL_FUNC(dlg_key_press_event),
		    (gpointer) &cancel_data);

    gtk_grab_add(dialog);

    /* this makes it look beter on Motif style window managers */
    frame = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(dialog), frame);
    gtk_widget_show(frame);

    vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(frame), vbox);
    gtk_widget_show(vbox);

    table = gtk_table_new(1, 3, FALSE);
    gtk_table_set_row_spacings(GTK_TABLE(table), 4);
    gtk_table_set_col_spacings(GTK_TABLE(table), 8);
    gtk_container_border_width(GTK_CONTAINER(table), 4);
    gtk_box_pack_start(GTK_BOX(vbox), table, 4, 4, 0);
    gtk_widget_show(table);

    /* Add pixmap */
    switch (type) {
    case VIM_GENERIC:
	icon_data = generic_xpm;
	break;
    case VIM_ERROR:
	icon_data = error_xpm;
	break;
    case VIM_WARNING:
	icon_data = alert_xpm;
	break;
    case VIM_INFO:
	icon_data = info_xpm;
	break;
    case VIM_QUESTION:
	icon_data = quest_xpm;
	break;
    default:
	icon_data = generic_xpm;
    };
    icon = gdk_pixmap_colormap_create_from_xpm_d(NULL,
				     gtk_widget_get_colormap(dialog),
				     &mask, NULL, icon_data);
    if (icon) {
	pixmap = gtk_pixmap_new(icon, mask);
	/* gtk_misc_set_alignment(GTK_MISC(pixmap), 0.5, 0.5); */
	gtk_table_attach_defaults(GTK_TABLE(table), pixmap, 0, 1, 0, 1);
	gtk_widget_show(pixmap);
    }

    /* Add label */
    dialogmessage = gtk_label_new((const gchar *)message);
    gtk_table_attach_defaults(GTK_TABLE(table), dialogmessage, 1, 2, 0, 1);
    gtk_widget_show(dialogmessage);

    action_area = gtk_hbox_new(FALSE, 0);
    gtk_container_border_width(GTK_CONTAINER(action_area), 4);
    gtk_box_pack_end(GTK_BOX(vbox), action_area, FALSE, TRUE, 0);
    gtk_widget_show(action_area);

    /* Add a [vh]box in the hbox to center the buttons in the dialog. */
    if (vertical)
	sub_area = gtk_vbox_new(FALSE, 0);
    else
	sub_area = gtk_hbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(sub_area), 0);
    gtk_box_pack_start(GTK_BOX(action_area), sub_area, TRUE, FALSE, 0);
    gtk_widget_show(sub_area);

    /*
     * Create the buttons.
     */

    /*
     * Translate the Vim accelerator character into an underscore for GTK+.
     * Double underscores to keep them in the label.
     */
    /* count the number of underscores */
    i = 1;
    for (p = buttons; *p; ++p)
	if (*p == '_')
	    ++i;

    /* make a copy of "buttons" with the translated characters */
    names = alloc(STRLEN(buttons) + i);
    if (names == NULL)
	return -1;

    p = names;
    for (i = 0; buttons[i]; ++i)
    {
	if (buttons[i] == DLG_HOTKEY_CHAR)
	    *p++ = '_';
	else
	{
	    if (buttons[i] == '_')
		*p++ = '_';
	    *p++ = buttons[i];
	}
    }
    *p = NUL;

    /* Count the number of buttons and allocate button[] and data[]. */
    butcount = 1;
    for (p = names; *p; ++p)
	if (*p == DLG_BUTTON_SEP)
	    ++butcount;
    button = (GtkWidget **)alloc((unsigned)(butcount * sizeof(GtkWidget *)));
    data = (ButtonData *)alloc((unsigned)(butcount * sizeof(ButtonData)));
    if (button == NULL || data == NULL)
    {
	vim_free(names);
	vim_free(button);
	vim_free(data);
	return -1;
    }

    /* Attach the new accelerator group to the window. */
    accel_group = gtk_accel_group_new();
    gtk_accel_group_attach(accel_group, GTK_OBJECT(dialog));

    p = names;
    for (butcount = 0; *p; ++butcount) {
	char_u		*next;
	GtkWidget	*label;
	guint		accel_key;

	/* Chunk out this single button. */
	for (next = p; *next; ++next) {
	    if (*next == DLG_BUTTON_SEP) {
		*next++ = NUL;
		break;
	    }
	}

	button[butcount] = gtk_button_new();
	GTK_WIDGET_SET_FLAGS(button[butcount], GTK_CAN_DEFAULT);

	label = gtk_accel_label_new("");
        gtk_accel_label_set_accel_widget(GTK_ACCEL_LABEL(label), dialog);

	accel_key = gtk_label_parse_uline(GTK_LABEL(label), (const gchar *)p);
# ifdef GTK_USE_ACCEL
	/* Don't add accelator if 'winaltkeys' is "no". */
	if (accel_key != GDK_VoidSymbol) {
	    gtk_widget_add_accelerator(button[butcount],
		    "clicked",
		    accel_group,
		    accel_key, 0,
		    0);
	}
# endif

	gtk_container_add(GTK_CONTAINER(button[butcount]), label);
	gtk_widget_show_all(button[butcount]);

	data[butcount].status = &dialog_status;
	data[butcount].index = butcount;
	data[butcount].dialog = dialog;
	gtk_signal_connect(GTK_OBJECT(button[butcount]),
			   (const char *)"clicked",
			   GTK_SIGNAL_FUNC(dlg_button_clicked),
			   (gpointer) &data[butcount]);

	gtk_box_pack_start(GTK_BOX(sub_area), button[butcount],
			   TRUE, FALSE, 0);
	p = next;
    }

    vim_free(names);

    --butcount;
    --def_but;	    /* 1 is first button */
    if (def_but < 0)
	def_but = 0;
    if (def_but > butcount)
	def_but = butcount;

    gtk_widget_grab_focus(button[def_but]);
    gtk_widget_grab_default(button[def_but]);

    separator = gtk_hseparator_new();
    gtk_box_pack_end(GTK_BOX(vbox), separator, FALSE, TRUE, 0);
    gtk_widget_show(separator);

    dialog_status = -1;
    gtk_widget_show(dialog);

    /* loop here until the dialog goes away */
    while (dialog_status == -1 && GTK_WIDGET_VISIBLE(dialog))
	gtk_main_iteration_do(TRUE);

    if (dialog_status < 0)
	dialog_status = 0;

    /* let the garbage collector know that we don't need it anylonger */
    gtk_accel_group_unref(accel_group);

    vim_free(button);
    vim_free(data);

    return dialog_status;
}


#endif	/* GUI_DIALOG */

#if defined(WANT_MENU) || defined(PROTO)
void
gui_mch_show_popupmenu(VimMenu * menu)
{
    gtk_menu_popup(GTK_MENU(menu->submenu_id),
		   NULL, NULL, NULL, NULL, 3, (guint32)GDK_CURRENT_TIME);
}
#endif


/*
 * We don't create it twice.
 */

typedef struct _SharedFindReplace {
    GtkWidget *dialog;	/* the main dialog widget */
    GtkWidget *exact;	/* 'Exact match' check button */
    GtkWidget *up;	/* search direction 'Up' radio button */
    GtkWidget *down;    /* search direction 'Down' radio button */
    GtkWidget *what;	/* 'Find what' entry text widget */
    GtkWidget *with;	/* 'Replace with' entry text widget */
    GtkWidget *find;	/* 'Find Next' action button */
    GtkWidget *replace;	/* 'Replace With' action button */
    GtkWidget *all;	/* 'Replace All' action button */
} SharedFindReplace;

static SharedFindReplace find_widgets = { NULL };
static SharedFindReplace repl_widgets = { NULL };

/* ARGSUSED */
static int
find_key_press_event(GtkWidget * widget,
	GdkEventKey * event,
	SharedFindReplace * frdp)
{
    /* If the user is holding one of the key modifiers we will just bail out,
     * thus preserving the possibility of normal focus traversal.
     */
    if (event->state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK))
	return FALSE;

    /* the scape key synthesizes a cancellation action */
    if (event->keyval == GDK_Escape) {
	find_replace_cb(widget, FR_DIALOGTERM);
	gtk_widget_hide(frdp->dialog);

	return TRUE;
    }

    /* block traversal resulting from those keys */
    if (event->keyval == GDK_Left || event->keyval == GDK_Right)
	return TRUE;

    /* It would be delightfull if it where possible to do search history
     * operations on the K_UP and K_DOWN keys here.
     */

    return FALSE;
}

static void
find_replace_dialog_create(char_u *arg, int do_replace)
{
    GtkWidget	*frame;
    GtkWidget	*hbox;		/* main top down box */
    GtkWidget	*actionarea;
    GtkWidget	*table;
    GtkWidget	*tmp;
    GtkWidget	*vbox;
    gboolean	sensitive;
    SharedFindReplace *frdp;
    char_u	*entry_text = arg;
    gboolean	exact_word = FALSE;

    frdp = (do_replace) ? (&repl_widgets) : (&find_widgets);

    /*
     * If the argument is emtpy, get the last used search pattern.  If it is
     * surrounded by "\<..\>" remove that and set the "exact_word" toggle
     * button.
     */
    if (*entry_text == NUL)
	entry_text = last_search_pat();
    if (entry_text != NULL)
    {
	entry_text = vim_strsave(entry_text);
	if (entry_text != NULL)
	{
	    int len = STRLEN(entry_text);

	    if (len >= 4
		    && STRNCMP(entry_text, "\\<", 2) == 0
		    && STRNCMP(entry_text + len - 2, "\\>", 2) == 0)
	    {
		exact_word = TRUE;
		mch_memmove(entry_text, entry_text + 2, (size_t)(len - 4));
		entry_text[len - 4] = NUL;
	    }
	}
    }

    /*
     * If the dialog already exists, just raise it.
     */
    if (frdp->dialog) {
	gui_gtk_synch_fonts();
	if (!GTK_WIDGET_VISIBLE(frdp->dialog)) {
	    gtk_widget_grab_focus(frdp->what);
	    gtk_widget_show(frdp->dialog);
	}

	if (entry_text != NULL)
	{
	    gtk_entry_set_text(GTK_ENTRY(frdp->what), (char *)entry_text);
	    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(frdp->exact),
								  exact_word);
	}
	gdk_window_raise(frdp->dialog->window);

	vim_free(entry_text);
	return;
    }

    frdp->dialog = gtk_window_new(GTK_WINDOW_DIALOG);
    if (do_replace) {
	gtk_window_set_wmclass(GTK_WINDOW(frdp->dialog), "searchrepl", "gvim");
	gtk_window_set_title(GTK_WINDOW(frdp->dialog), "VIM - Search and Replace...");
    } else {
	gtk_window_set_wmclass(GTK_WINDOW(frdp->dialog), "search", "gvim");
	gtk_window_set_title(GTK_WINDOW(frdp->dialog), "VIM - Search...");
    }

    gtk_window_position(GTK_WINDOW(frdp->dialog), GTK_WIN_POS_MOUSE);
    gtk_widget_realize(frdp->dialog);
    gdk_window_set_decorations(frdp->dialog->window,
	    GDK_DECOR_TITLE | GDK_DECOR_BORDER | GDK_DECOR_RESIZEH);
    gdk_window_set_functions(frdp->dialog->window,
	    GDK_FUNC_RESIZE | GDK_FUNC_MOVE);

    /* this makes it look beter on Motif style window managers */
    frame = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(frdp->dialog), frame);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(frame), hbox);

    if (do_replace)
	table = gtk_table_new(1024, 4, FALSE);
    else
	table = gtk_table_new(1024, 3, FALSE);
    gtk_box_pack_start(GTK_BOX(hbox), table, TRUE, TRUE, 0);
    gtk_container_border_width(GTK_CONTAINER(table), 4);

    tmp = gtk_label_new("Find what:");
    gtk_misc_set_alignment(GTK_MISC(tmp), 1.0, 0.5);
    gtk_table_attach(GTK_TABLE(table), tmp, 0, 1, 0, 1,
		     GTK_FILL, GTK_EXPAND, 2, 2);
    frdp->what = gtk_entry_new();
    sensitive = (entry_text != NULL && STRLEN(entry_text) != 0);
    if (entry_text != NULL)
	gtk_entry_set_text(GTK_ENTRY(frdp->what), (char *)entry_text);
    gtk_signal_connect(GTK_OBJECT(frdp->what), "changed",
		       GTK_SIGNAL_FUNC(entry_changed_cb), frdp->dialog);
    gtk_signal_connect_after(GTK_OBJECT(frdp->what), "key_press_event",
				 GTK_SIGNAL_FUNC(find_key_press_event),
				 (gpointer) frdp);
    gtk_table_attach(GTK_TABLE(table), frdp->what, 1, 1024, 0, 1,
		     GTK_EXPAND | GTK_FILL, GTK_EXPAND, 2, 2);

    if (do_replace) {
	tmp = gtk_label_new("Replace with:");
	gtk_misc_set_alignment(GTK_MISC(tmp), 1.0, 0.5);
	gtk_table_attach(GTK_TABLE(table), tmp, 0, 1, 1, 2,
			 GTK_FILL, GTK_EXPAND, 2, 2);
	frdp->with = gtk_entry_new();
	gtk_signal_connect(GTK_OBJECT(frdp->with), "activate",
			   GTK_SIGNAL_FUNC(find_replace_cb),
			   (gpointer) FR_R_FINDNEXT);
	gtk_signal_connect_after(GTK_OBJECT(frdp->with), "key_press_event",
				 GTK_SIGNAL_FUNC(find_key_press_event),
				 (gpointer) frdp);
	gtk_table_attach(GTK_TABLE(table), frdp->with, 1, 1024, 1, 2,
			 GTK_EXPAND | GTK_FILL, GTK_EXPAND, 2, 2);

	/*
	 * Make the entry activation only change the input focus onto the
	 * with item.
	 */
	gtk_signal_connect(GTK_OBJECT(frdp->what), "activate",
			   GTK_SIGNAL_FUNC(entry_activate_cb), frdp->with);
    } else {
	/*
	 * Make the entry activation do the search.
	 */
	gtk_signal_connect(GTK_OBJECT(frdp->what), "activate",
			   GTK_SIGNAL_FUNC(find_replace_cb),
			   (gpointer) FR_FINDNEXT);
    }

    /* exact match only button */
    frdp->exact = gtk_check_button_new_with_label("Match exact word only");
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(frdp->exact), exact_word);
    gtk_signal_connect(GTK_OBJECT(frdp->exact), "clicked",
		       GTK_SIGNAL_FUNC(exact_match_cb), NULL);
    if (do_replace)
	gtk_table_attach(GTK_TABLE(table), frdp->exact, 0, 1023, 3, 4,
			 GTK_FILL, GTK_EXPAND, 2, 2);
    else
	gtk_table_attach(GTK_TABLE(table), frdp->exact, 0, 1023, 2, 3,
			 GTK_FILL, GTK_EXPAND, 2, 2);

    tmp = gtk_frame_new("Direction");
    if (do_replace)
	gtk_table_attach(GTK_TABLE(table), tmp, 1023, 1024, 2, 4,
			 GTK_FILL, GTK_FILL, 2, 2);
    else
	gtk_table_attach(GTK_TABLE(table), tmp, 1023, 1024, 1, 3,
			 GTK_FILL, GTK_FILL, 2, 2);
    vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_border_width(GTK_CONTAINER(vbox), 0);
    gtk_container_add(GTK_CONTAINER(tmp), vbox);

    /* 'Up' and 'Down' buttons */
    frdp->up = gtk_radio_button_new_with_label(NULL, "Up");
    gtk_box_pack_start(GTK_BOX(vbox), frdp->up, TRUE, TRUE, 0);
    frdp->down = gtk_radio_button_new_with_label(
			gtk_radio_button_group(GTK_RADIO_BUTTON(frdp->up)),
			"Down");
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(frdp->down), TRUE);
    if (do_replace)
	gtk_signal_connect(GTK_OBJECT(frdp->down), "clicked",
			   GTK_SIGNAL_FUNC(repl_dir_cb), NULL);
    else
	gtk_signal_connect(GTK_OBJECT(frdp->down), "clicked",
			   GTK_SIGNAL_FUNC(find_direction_cb), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), frdp->down, TRUE, TRUE, 0);

    /* vbox to hold the action buttons */
    actionarea = gtk_vbutton_box_new();
    gtk_container_border_width(GTK_CONTAINER(actionarea), 2);
    if (do_replace) {
	gtk_button_box_set_layout(GTK_BUTTON_BOX(actionarea),
				  GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(actionarea), 0);
    }
    gtk_box_pack_end(GTK_BOX(hbox), actionarea, FALSE, FALSE, 0);

    /* 'Find Next' button */
    frdp->find = gtk_button_new_with_label("Find Next");
    gtk_widget_set_sensitive(frdp->find, sensitive);
    if (do_replace)
	gtk_signal_connect(GTK_OBJECT(frdp->find), "clicked",
			   GTK_SIGNAL_FUNC(find_replace_cb),
			   (gpointer) FR_R_FINDNEXT);
    else
	gtk_signal_connect(GTK_OBJECT(frdp->find), "clicked",
			   GTK_SIGNAL_FUNC(find_replace_cb),
			   (gpointer) FR_FINDNEXT);
    GTK_WIDGET_SET_FLAGS(frdp->find, GTK_CAN_DEFAULT);
    gtk_box_pack_start(GTK_BOX(actionarea), frdp->find, FALSE, FALSE, 0);
    gtk_widget_grab_default(frdp->find);

    if (do_replace) {
	/* 'Replace' button */
	frdp->replace = gtk_button_new_with_label("Replace");
	gtk_widget_set_sensitive(frdp->replace, sensitive);
	GTK_WIDGET_SET_FLAGS(frdp->replace, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(actionarea), frdp->replace, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(frdp->replace), "clicked",
			   GTK_SIGNAL_FUNC(find_replace_cb),
			   (gpointer) FR_REPLACE);

	/* 'Replace All' button */
	frdp->all = gtk_button_new_with_label("Replace All");
	gtk_widget_set_sensitive(frdp->all, sensitive);
	GTK_WIDGET_SET_FLAGS(frdp->all, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(actionarea), frdp->all, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(frdp->all), "clicked",
			   GTK_SIGNAL_FUNC(find_replace_cb),
			   (gpointer) FR_REPLACEALL);
    }

    /* 'Cancel' button */
    tmp = gtk_button_new_with_label("Cancel");
    GTK_WIDGET_SET_FLAGS(tmp, GTK_CAN_DEFAULT);
    gtk_box_pack_end(GTK_BOX(actionarea), tmp, FALSE, FALSE, 0);
    gtk_signal_connect(GTK_OBJECT(tmp), "clicked",
		       GTK_SIGNAL_FUNC(find_replace_cb),
		       (gpointer) FR_DIALOGTERM);
    gtk_signal_connect(GTK_OBJECT(tmp), "delete_event",
		       GTK_SIGNAL_FUNC(find_replace_cb),
		       (gpointer) FR_DIALOGTERM);
    gtk_signal_connect_object(GTK_OBJECT(tmp),
			      "clicked", GTK_SIGNAL_FUNC(gtk_widget_hide),
			      GTK_OBJECT(frdp->dialog));
    gtk_signal_connect_object(GTK_OBJECT(frdp->dialog),
			      "delete_event", GTK_SIGNAL_FUNC(gtk_widget_hide),
			      GTK_OBJECT(frdp->dialog));

    tmp = gtk_vseparator_new();
    gtk_box_pack_end(GTK_BOX(hbox), tmp, FALSE, TRUE, 0);
    gtk_widget_grab_focus(frdp->what);

    gui_gtk_synch_fonts();

    gtk_widget_show_all(frdp->dialog);

    vim_free(entry_text);
}

void
gui_mch_find_dialog(char_u * arg)
{
    find_replace_dialog_create(arg, FALSE);
}


void
gui_mch_replace_dialog(char_u * arg)
{
    find_replace_dialog_create(arg, TRUE);
}


/*
 * Synchronize all gui elements, which are dependant upon the
 * main text font used. Those are in esp. the find/replace dialogs.
 * If You don't understand why this should be needed, please try to
 * search for "piê¶æ" in iso8859-2.
 */
void
gui_gtk_synch_fonts(void)
{
    SharedFindReplace *frdp;
    int do_replace;

    /* OK this loop is a bit tricky... */
    for (do_replace = 0; do_replace <= 1; ++do_replace) {
	frdp = (do_replace) ? (&repl_widgets) : (&find_widgets);
	if (frdp->dialog) {
	    GtkStyle *style;

	    /* synch the font with whats used by the text itself */
	    style = gtk_style_copy(gtk_widget_get_style(frdp->what));
	    gdk_font_unref(style->font);
	    style->font = gui.norm_font;
	    gdk_font_ref(style->font);
	    gtk_widget_set_style(frdp->what, style);
	    gtk_style_unref(style);
	    if (do_replace) {
		style = gtk_style_copy(gtk_widget_get_style(frdp->with));
		gdk_font_unref(style->font);
		style->font = gui.norm_font;
		gdk_font_ref(style->font);
		gtk_widget_set_style(frdp->with, style);
		gtk_style_unref(style);
	    }
	}
    }
}


/*
 * Convenience function.
 * Creates a button with a label, and packs it into the box specified by the
 * parameter 'parent'.
 */
GtkWidget *
gui_gtk_button_new_with_label(
    char *labelname,
    GtkSignalFunc cbfunc,
    gpointer cbdata,
    GtkWidget *parent,
    int connect_object,
    gboolean expand,
    gboolean fill)
{
    GtkWidget *tmp;

    tmp = gtk_button_new_with_label(labelname);
    if (connect_object)
	gtk_signal_connect_object(GTK_OBJECT(tmp), "clicked",
				  GTK_SIGNAL_FUNC(cbfunc), GTK_OBJECT(cbdata));
    else
	gtk_signal_connect(GTK_OBJECT(tmp), "clicked",
			   GTK_SIGNAL_FUNC(cbfunc), cbdata);
    gtk_box_pack_end(GTK_BOX(parent), tmp, expand, fill, 0);
    return tmp;
}


/*** private function definitions ***/

/*
 * Callback for actions of the find and replace dialogs
 */
/*ARGSUSED*/
static void
find_replace_cb(GtkWidget * widget, unsigned int flags)
{
    char *find_text, *repl_text, *cmd;
    gboolean direction_down = TRUE;
    gboolean exact_match = FALSE;
    int len;

    if (flags == FR_DIALOGTERM) {
	if (State == CONFIRM) {
	    add_to_input_buf((char_u *)"q", 1);
	    return;
	}
    }

    /* Get the search/replace strings from the dialog */
    if (flags == FR_FINDNEXT) {
	find_text = gtk_entry_get_text(GTK_ENTRY(find_widgets.what));
	repl_text = NULL;
	direction_down = GTK_TOGGLE_BUTTON(find_widgets.down)->active;
	exact_match = GTK_TOGGLE_BUTTON(find_widgets.exact)->active;
    } else if (flags == FR_R_FINDNEXT || flags == FR_REPLACE ||
	       flags == FR_REPLACEALL) {
	find_text = gtk_entry_get_text(GTK_ENTRY(repl_widgets.what));
	repl_text = gtk_entry_get_text(GTK_ENTRY(repl_widgets.with));
	direction_down = GTK_TOGGLE_BUTTON(repl_widgets.down)->active;
	exact_match = GTK_TOGGLE_BUTTON(repl_widgets.exact)->active;
    } else {
	find_text = NULL;
	repl_text = NULL;
    }

    /* calculate exact length of cmd buffer.  see below to count characters */
    len = 2;
    if (flags == FR_FINDNEXT || flags == FR_R_FINDNEXT) {
	len += (strlen(find_text) + 1);
	if (State != CONFIRM && exact_match)
	    len += 4;   /* \< and \>*/
    } else if (flags == FR_REPLACE || flags == FR_REPLACEALL) {
	if (State == CONFIRM)
	    len++;
	else
	    len += (strlen(find_text) + strlen(repl_text) + 11);
    }

    cmd = g_new(char, len);

    /* start stuffing in the command text */
    if (State & INSERT)
	cmd[0] = Ctrl('O');
    else if ((State | NORMAL) == 0 && State != CONFIRM)
	cmd[0] = ESC;
    else
	cmd[0] = NUL;
    cmd[1] = NUL;

    /* Synthesize the input corresponding to the different actions. */
    if (flags == FR_FINDNEXT || flags == FR_R_FINDNEXT) {
	if (State == CONFIRM) {
	    STRCAT(cmd, "n");
	} else {
	    if (direction_down)
		STRCAT(cmd, "/");
	    else
		STRCAT(cmd, "?");

	    if (exact_match)
		STRCAT(cmd, "\\<");
	    STRCAT(cmd, find_text);
	    if (exact_match)
		STRCAT(cmd, "\\>");

	    STRCAT(cmd, "\r");
	}
    } else if (flags == FR_REPLACE) {
	if (State == CONFIRM) {
	    STRCAT(cmd, "y");
	} else {
	    STRCAT(cmd, ":%sno/");
	    STRCAT(cmd, find_text);
	    STRCAT(cmd, "/");
	    STRCAT(cmd, repl_text);
	    STRCAT(cmd, "/gc\r");
	}
	/*
	 * Give main window the focus back: this is to allow
	 * handling of the confirmation y/n/a/q stuff.
	 */
	/*(void)SetFocus(s_hwnd); */
    } else if (flags == FR_REPLACEALL) {
	if (State == CONFIRM) {
	    STRCAT(cmd, "a");
	} else {
	    STRCAT(cmd, ":%sno/");
	    STRCAT(cmd, find_text);
	    STRCAT(cmd, "/");
	    STRCAT(cmd, repl_text);
	    STRCAT(cmd, "/g\r");
	}
    }

    if (*cmd) {
	add_to_input_buf((char_u *)cmd, STRLEN(cmd));
	if (gtk_main_level() > 0)
	    gtk_main_quit();	/* make sure if will be handled immediately */
    }

    g_free(cmd);
}

/* our usual callback function */
/*ARGSUSED*/
static void
entry_activate_cb(GtkWidget * widget, GtkWidget * with)
{
    gtk_widget_grab_focus(with);
}


/*
 * The following are used to synchronize the direction setting
 * between the search and the replace dialog.
 */
/*ARGSUSED*/
static void
find_direction_cb(GtkWidget * widget, gpointer data)
{
    gboolean direction_down = GTK_TOGGLE_BUTTON(widget)->active;

    if (repl_widgets.dialog) {
	GtkWidget *w;
	w = direction_down ? repl_widgets.down : repl_widgets.up;

	if (!GTK_TOGGLE_BUTTON(w)->active)
	    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(w), TRUE);
    }
}

/*ARGSUSED*/
static void
repl_dir_cb(GtkWidget *widget, gpointer data)
{
    gboolean direction_down = GTK_TOGGLE_BUTTON(widget)->active;

    if (find_widgets.dialog) {
	GtkWidget *w;
	w = direction_down ? find_widgets.down : find_widgets.up;

	if (!GTK_TOGGLE_BUTTON(w)->active)
	    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(w), TRUE);
    }
}

/*ARGSUSED*/
static void
exact_match_cb(GtkWidget * widget, gpointer data)
{
    gboolean exact_match = GTK_TOGGLE_BUTTON(widget)->active;

    if (find_widgets.dialog)
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(find_widgets.exact),
				    exact_match);
    if (repl_widgets.dialog)
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(repl_widgets.exact),
				    exact_match);
}

static void
entry_changed_cb(GtkWidget * entry, GtkWidget * dialog)
{
    gchar *entry_text;
    gboolean nonempty;

    entry_text = gtk_entry_get_text(GTK_ENTRY(entry));

    if (!entry_text)
	return;			/* shouldn't happen */

    nonempty = (strlen(entry_text) != 0);

    if (dialog == find_widgets.dialog) {
	gtk_widget_set_sensitive(find_widgets.find, nonempty);
	if (repl_widgets.dialog) {
	    gtk_widget_set_sensitive(repl_widgets.find, nonempty);
	    gtk_widget_set_sensitive(repl_widgets.replace, nonempty);
	    gtk_widget_set_sensitive(repl_widgets.all, nonempty);
	    if (strcmp(entry_text,
		       gtk_entry_get_text(GTK_ENTRY(repl_widgets.what))))
		gtk_entry_set_text(GTK_ENTRY(repl_widgets.what), entry_text);
	}
    }
    if (dialog == repl_widgets.dialog) {
	gtk_widget_set_sensitive(repl_widgets.find, nonempty);
	gtk_widget_set_sensitive(repl_widgets.replace, nonempty);
	gtk_widget_set_sensitive(repl_widgets.all, nonempty);
	if (find_widgets.dialog) {
	    gtk_widget_set_sensitive(find_widgets.find, nonempty);
	    if (strcmp(entry_text,
		       gtk_entry_get_text(GTK_ENTRY(find_widgets.what))))
		gtk_entry_set_text(GTK_ENTRY(find_widgets.what), entry_text);
	}
    }
}

/*
 * Handling of the nice additional asynchronous little help topic requester.
 */

static GtkWidget *helpfind = NULL;
static GtkWidget *help_entry = NULL;
static int	 help_ok_sensitive = FALSE;

    static void
helpfind_entry_changed(GtkWidget *entry, gpointer cbdata)
{
    GtkWidget *ok = (GtkWidget *)cbdata;
    char *txt = gtk_entry_get_text(GTK_ENTRY(entry));

    if (txt == NULL || *txt == NUL)
	return;

    gtk_widget_set_sensitive(ok, strlen(txt));
    gtk_widget_grab_default(ok);
    help_ok_sensitive = TRUE;
}

/*ARGSUSED*/
    static void
helpfind_ok(GtkWidget *wgt, gpointer cbdata)
{
    char *txt, *cmd;
    GtkEntry *entry = GTK_ENTRY(cbdata);

    if ((txt = gtk_entry_get_text(entry)) == NULL)
	return;

    /* When the OK button isn't sensitive, hitting CR means cancel. */
    if (help_ok_sensitive)
    {
	if ((cmd = (char *)alloc(strlen(txt) + 8)) == NULL)
	    return;

	/* use CTRL-\ CTRL-N to get Vim into Normal mode first */
	g_snprintf(cmd, (gulong)(strlen(txt) + 7), "\034\016:h %s\r", txt);
	add_to_input_buf((char_u *)cmd, STRLEN(cmd));
	vim_free(cmd);
    }

    /* Don't destroy this dialogue just hide it from the users view.
     * Reuse it later */
    gtk_widget_hide(helpfind);

    if (gtk_main_level() > 0)
	gtk_main_quit();
}

/*ARGSUSED*/
    static void
helpfind_cancel(GtkWidget *wgt, gpointer cbdata)
{
    /* Don't destroy this dialogue just hide it from the users view.
     * Reuse it later */
    gtk_widget_hide(helpfind);

    if (gtk_main_level() > 0)
	gtk_main_quit();
}

void
do_helpfind(void)
{
    GtkWidget *frame;
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *tmp;
    GtkWidget *action_area;
    GtkWidget *sub_area;

    if (!gui.in_use)
    {
	EMSG(e_needgui);
	return;
    }

    if (helpfind)
    {
	if (!GTK_WIDGET_VISIBLE(helpfind))
	{
	    gtk_widget_grab_focus(help_entry);
	    gtk_widget_show(helpfind);
	}

	gdk_window_raise(helpfind->window);
	return;
    }

    helpfind = gtk_window_new(GTK_WINDOW_DIALOG);
#ifdef GTK_HAVE_FEATURES_1_1_4
    gtk_window_set_transient_for(GTK_WINDOW(helpfind), GTK_WINDOW(gui.mainwin));
#endif
    gtk_window_position(GTK_WINDOW(helpfind), GTK_WIN_POS_MOUSE);
    gtk_window_set_title(GTK_WINDOW(helpfind), "VIM - Help on...");

    gtk_widget_realize(helpfind);
    gdk_window_set_decorations(helpfind->window,
		    GDK_DECOR_BORDER | GDK_DECOR_TITLE);
    gdk_window_set_functions(helpfind->window, GDK_FUNC_MOVE);

    gtk_signal_connect(GTK_OBJECT(helpfind), "destroy",
		       GTK_SIGNAL_FUNC(gtk_widget_destroyed), &helpfind);

    /* this makes it look beter on Motif style window managers */
    frame = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(helpfind), frame);
    gtk_widget_show(frame);

    vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(frame), vbox);

    hbox = gtk_hbox_new(FALSE, 6);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
    gtk_container_border_width(GTK_CONTAINER(hbox), 6);

    tmp = gtk_label_new("Topic:");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, TRUE, TRUE, 0);

    help_entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(hbox), help_entry, TRUE, TRUE, 0);

    action_area = gtk_hbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(action_area), 4);
    gtk_box_pack_end(GTK_BOX(vbox), action_area, FALSE, TRUE, 0);

    sub_area = gtk_hbox_new(TRUE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(sub_area), 0);
    gtk_box_pack_end(GTK_BOX(action_area), sub_area, FALSE, TRUE, 0);

    tmp = gui_gtk_button_new_with_label("Cancel",
	    GTK_SIGNAL_FUNC(helpfind_cancel),
	    help_entry, sub_area, TRUE, TRUE, TRUE);
    GTK_WIDGET_SET_FLAGS(tmp, GTK_CAN_DEFAULT);
    gtk_widget_grab_default(tmp);

    tmp = gui_gtk_button_new_with_label("OK",
	    GTK_SIGNAL_FUNC(helpfind_ok),
	    help_entry, sub_area, FALSE, TRUE, TRUE);
    gtk_signal_connect(GTK_OBJECT(help_entry), "changed",
		       GTK_SIGNAL_FUNC(helpfind_entry_changed), tmp);
    gtk_signal_connect(GTK_OBJECT(help_entry), "activate",
		       GTK_SIGNAL_FUNC(helpfind_ok), help_entry);
    gtk_widget_set_sensitive(tmp, FALSE);
    help_ok_sensitive = FALSE;
    GTK_WIDGET_SET_FLAGS(tmp, GTK_CAN_DEFAULT);

    tmp = gtk_hseparator_new();
    gtk_box_pack_end(GTK_BOX(vbox), tmp, FALSE, TRUE, 0);

    gtk_widget_grab_focus(help_entry);
    gtk_widget_show_all(helpfind);
}
