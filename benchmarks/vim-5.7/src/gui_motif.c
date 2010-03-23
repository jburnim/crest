/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved		by Bram Moolenaar
 *				GUI/Motif support by Robert Webb
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

#include <Xm/Form.h>
#include <Xm/RowColumn.h>
#include <Xm/PushB.h>
#include <Xm/Separator.h>
#include <Xm/Label.h>
#include <Xm/CascadeB.h>
#include <Xm/ScrollBar.h>
#include <Xm/MenuShell.h>
#include <Xm/DrawingA.h>
#if (XmVersion >= 1002)
# include <Xm/RepType.h>
#endif

#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/StringDefs.h>

#include "vim.h"

#define MOTIF_POPUP

extern Widget vimShell;

static Widget vimForm;
static Widget textArea;
#ifdef WANT_MENU
# if (XmVersion >= 1002)
/* remember the last set value for the tearoff item */
static int tearoff_val = (int)XmTEAR_OFF_ENABLED;
# endif
static Widget menuBar;
#endif

static void scroll_cb __ARGS((Widget w, XtPointer client_data, XtPointer call_data));

#if (XmVersion >= 1002)
# define STRING_TAG  XmFONTLIST_DEFAULT_TAG
#else
# define STRING_TAG  XmSTRING_DEFAULT_CHARSET
#endif

/*
 * Call-back routines.
 */

/* ARGSUSED */
    static void
scroll_cb(w, client_data, call_data)
    Widget	w;
    XtPointer	client_data, call_data;
{
    GuiScrollbar *sb;
    long	value;
    int		dragging;

    sb = gui_find_scrollbar((long)client_data);

    value = ((XmScrollBarCallbackStruct *)call_data)->value;
    dragging = (((XmScrollBarCallbackStruct *)call_data)->reason ==
							      (int)XmCR_DRAG);
    /*
     * :NOTE: 1998-11-18 09:26:01 EST eralston
     * pretend we're not dragging whenever scrollbind is active in
     * the current window, so that the other scrollbind windows
     * get their scrollbars updated also.
     */
    gui_drag_scrollbar(sb, value, dragging
#ifdef SCROLLBIND
		    && (!sb->wp || !sb->wp->w_p_scb || (sb->wp != curwin))
#endif
		    );
}


/*
 * End of call-back routines
 */

/*
 * Create all the motif widgets necessary.
 */
    void
gui_x11_create_widgets()
{
    /*
     * Start out by adding the configured border width into the border offset
     */
    gui.border_offset = gui.border_width;

    /*
     * Install the tearOffModel resource converter.
     */
#if (XmVersion >= 1002)
    XmRepTypeInstallTearOffModelConverter();
#endif

    XtInitializeWidgetClass(xmFormWidgetClass);
    XtInitializeWidgetClass(xmRowColumnWidgetClass);
    XtInitializeWidgetClass(xmDrawingAreaWidgetClass);
    XtInitializeWidgetClass(xmCascadeButtonWidgetClass);
    XtInitializeWidgetClass(xmMenuShellWidgetClass);
    XtInitializeWidgetClass(xmPushButtonWidgetClass);
    XtInitializeWidgetClass(xmScrollBarWidgetClass);

    /* Make sure the "Quit" menu entry of the window manager is ignored */
    XtVaSetValues(vimShell, XmNdeleteResponse, XmDO_NOTHING, NULL);

    vimForm = XtVaCreateManagedWidget("vimForm",
	xmFormWidgetClass, vimShell,
	XmNborderWidth, 0,
	XmNhighlightThickness, 0,
	XmNshadowThickness, 0,
	XmNmarginWidth, 0,
	XmNmarginHeight, 0,
	XmNresizePolicy, XmRESIZE_ANY,
	XmNforeground, gui.menu_fg_pixel,
	XmNbackground, gui.menu_bg_pixel,
	NULL);

    textArea = XtVaCreateManagedWidget("textArea",
	xmDrawingAreaWidgetClass, vimForm,
	XmNbackground, gui.back_pixel,
	XmNleftAttachment, XmATTACH_FORM,
	XmNtopAttachment, XmATTACH_FORM,
	XmNrightAttachment, XmATTACH_OPPOSITE_FORM,
	XmNbottomAttachment, XmATTACH_OPPOSITE_FORM,

	/*
	 * These take some control away from the user, but avoids making them
	 * add resources to get a decent looking setup.
	 */
	XmNborderWidth, 0,
	XmNhighlightThickness, 0,
	XmNshadowThickness, 0,
	NULL);

#ifdef WANT_MENU
    menuBar = XtVaCreateManagedWidget("menuBar",
	xmRowColumnWidgetClass, vimForm,
	XmNrowColumnType, XmMENU_BAR,
	XmNforeground, gui.menu_fg_pixel,
	XmNbackground, gui.menu_bg_pixel,
#if (XmVersion >= 1002)
	XmNtearOffModel, tearoff_val,
#endif
	XmNleftAttachment, XmATTACH_FORM,
	XmNtopAttachment, XmATTACH_FORM,
	XmNrightAttachment, XmATTACH_FORM,
	XmNbottomAttachment, XmATTACH_OPPOSITE_FORM,
	XmNrightOffset, 0,	/* Always stick to rigth hand side */
	NULL);
#endif

    /*
     * Install the callbacks.
     */
    gui_x11_callbacks(textArea, vimForm);

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
    XtVaSetValues(textArea,
		  XmNleftOffset, x,
		  XmNtopOffset, y,
		  XmNrightOffset, -x - w,
		  XmNbottomOffset, -y - h,
		  NULL);
}

    void
gui_x11_set_back_color()
{
    if (textArea != NULL)
#if (XmVersion >= 1002)
	XmChangeColor(textArea, gui.back_pixel);
#else
	XtVaSetValues(textArea,
		  XmNbackground, gui.back_pixel,
		  NULL);
#endif
}

#if defined(WANT_MENU) || defined(PROTO)
/*
 * Menu stuff.
 */

static void gui_motif_add_actext __ARGS((VimMenu *menu));
#if (XmVersion >= 1002)
static void toggle_tearoff __ARGS((Widget wid));
static void gui_mch_recurse_tearoffs __ARGS((VimMenu *menu));
#endif
static void gui_mch_compute_menu_height __ARGS((Widget));
static void gui_mch_submenu_colors __ARGS((VimMenu *mp));

static void do_set_mnemonics __ARGS((int enable));
static int mnemonics_enabled = TRUE;
static int menu_enabled = TRUE;

    void
gui_mch_enable_menu(flag)
    int	    flag;
{
    menu_enabled = flag;

    /*
     * Must disable menu mnemonics when the menu bar is disabled, Lesstif
     * crashes when using a mnemonic then.
     */
    if (flag)
    {
	XtManageChild(menuBar);
	do_set_mnemonics(mnemonics_enabled);
    }
    else
    {
	XtUnmanageChild(menuBar);
	do_set_mnemonics(FALSE);
    }
}

/* ARGSUSED */
    void
gui_mch_set_menu_pos(x, y, w, h)
    int	    x;
    int	    y;
    int	    w;
    int	    h;
{
    XtVaSetValues(menuBar,
		  XmNleftOffset, x,
		  XmNtopOffset, y,
		  XmNbottomOffset, -y - h,
		  NULL);
}

/*
 * Enable or disable mnemonics for the toplevel menus.
 */
    void
gui_motif_set_mnemonics(enable)
    int		enable;
{
    mnemonics_enabled = enable;
    /*
     * Don't enable menu mnemonics when the menu bar is disabled, Lesstif
     * crashes when using a mnemonic then.
     */
    if (!menu_enabled)
	enable = FALSE;
    do_set_mnemonics(enable);
}

    static void
do_set_mnemonics(enable)
    int		enable;
{
    VimMenu	*menu;

    for (menu = root_menu; menu != NULL; menu = menu->next)
	if (menu->id != (Widget)0)
	    XtVaSetValues(menu->id,
		    XmNmnemonic, enable ? menu->mnemonic : NUL,
		    NULL);
}

    void
gui_mch_add_menu(menu, parent, idx)
    VimMenu	*menu;
    VimMenu	*parent;
    int		idx;
{
    XmString	label;
    Widget	shell;

#ifdef MOTIF_POPUP
    if (popup_menu(menu->name))
    {
	Arg arg[2];

	XtSetArg(arg[0], XmNbackground, gui.menu_bg_pixel);
	XtSetArg(arg[1], XmNforeground, gui.menu_fg_pixel);
	menu->submenu_id = XmCreatePopupMenu(textArea, "contextMenu", arg, 2);
	menu->id = (Widget)0;
	return;
    }
#endif

    if (!menubar_menu(menu->name)
	    || (parent != NULL && parent->submenu_id == (Widget)0))
	return;

    label = XmStringCreate((char *)menu->dname, STRING_TAG);
    if (label == NULL)
	return;
    menu->id = XtVaCreateWidget("subMenu",
	    xmCascadeButtonWidgetClass,
	    (parent == NULL) ? menuBar : parent->submenu_id,
	    XmNlabelString, label,
	    XmNforeground, gui.menu_fg_pixel,
	    XmNbackground, gui.menu_bg_pixel,
	    XmNmnemonic, p_wak[0] == 'n' ? NUL : menu->mnemonic,
#if (XmVersion >= 1002)
	    /* submenu: count the tearoff item (needed for LessTif) */
	    XmNpositionIndex, idx + (parent != NULL
			   && tearoff_val == (int)XmTEAR_OFF_ENABLED ? 1 : 0),
#endif
	    NULL);
    XmStringFree(label);

    if (menu->id == (Widget)0)		/* failed */
	return;

    /* add accelerator text */
    gui_motif_add_actext(menu);

    shell = XtVaCreateWidget("subMenuShell",
	xmMenuShellWidgetClass, menu->id,
	XmNwidth, 1,
	XmNheight, 1,
	XmNforeground, gui.menu_fg_pixel,
	XmNbackground, gui.menu_bg_pixel,
	NULL);
    menu->submenu_id = XtVaCreateWidget("rowColumnMenu",
	xmRowColumnWidgetClass, shell,
	XmNrowColumnType, XmMENU_PULLDOWN,
	XmNforeground, gui.menu_fg_pixel,
	XmNbackground, gui.menu_bg_pixel,
	NULL);

    if (menu->submenu_id == (Widget)0)		/* failed */
	return;

#if (XmVersion >= 1002)
    /* Set the colors for the tear off widget */
    toggle_tearoff(menu->submenu_id);
#endif

    XtVaSetValues(menu->id,
	XmNsubMenuId, menu->submenu_id,
	NULL);

    /*
     * The "Help" menu is a special case, and should be placed at the far right
     * hand side of the menu-bar.
     */
    if (parent == NULL && STRCMP((char *)menu->dname, "Help") == 0)
	XtVaSetValues(menuBar,
	    XmNmenuHelpWidget, menu->id,
	    NULL);

    /*
     * When we add a top-level item to the menu bar, we can figure out how
     * high the menu bar should be.
     */
    if (parent == NULL)
	gui_mch_compute_menu_height(menu->id);
}

/*
 * Add mnemonic and accelerator text to a menu button.
 */
    static void
gui_motif_add_actext(menu)
    VimMenu	*menu;
{
    XmString	label;

    /* Add accelrator text, if there is one */
    if (menu->actext != NULL && menu->id != (Widget)0)
    {
	label = XmStringCreate((char *)menu->actext, STRING_TAG);
	if (label == NULL)
	    return;
	XtVaSetValues(menu->id, XmNacceleratorText, label, NULL);
	XmStringFree(label);
    }
}

    void
gui_mch_toggle_tearoffs(enable)
    int		enable;
{
#if (XmVersion >= 1002)
    if (enable)
	tearoff_val = (int)XmTEAR_OFF_ENABLED;
    else
	tearoff_val = (int)XmTEAR_OFF_DISABLED;
    toggle_tearoff(menuBar);
    gui_mch_recurse_tearoffs(root_menu);
#endif
}

#if (XmVersion >= 1002)
/*
 * Set the tearoff for one menu widget on or off, and set the color of the
 * tearoff widget.
 */
    static void
toggle_tearoff(wid)
    Widget	wid;
{
    Widget	w;

    XtVaSetValues(wid, XmNtearOffModel, tearoff_val, NULL);
    if (tearoff_val == (int)XmTEAR_OFF_ENABLED
	    && (w = XmGetTearOffControl(wid)) != (Widget)0)
    {
#if (XmVersion>=1002)
	XmChangeColor(w, gui.menu_bg_pixel);
#endif
	XtVaSetValues(w,
	    XmNforeground, gui.menu_fg_pixel,
#if (XmVersion<1002)
	    XmNbackground, gui.menu_bg_pixel,
#endif
	    NULL);
    }
}

    static void
gui_mch_recurse_tearoffs(menu)
    VimMenu	*menu;
{
    while (menu != NULL)
    {
	if (!popup_menu(menu->name))
	{
	    if (menu->submenu_id != (Widget)0)
		toggle_tearoff(menu->submenu_id);
	    gui_mch_recurse_tearoffs(menu->children);
	}
	menu = menu->next;
    }
}
#endif

/*
 * Compute the height of the menu bar.
 * We need to check all the items for their position an height, for the case
 * there are several rows, and/or some characters extend higher or lower.
 */
    static void
gui_mch_compute_menu_height(id)
    Widget	id;		    /* can be NULL when deleting menu */
{
    Dimension	y, maxy;
    Dimension	margin, shadow;
    VimMenu	*mp;
    static Dimension	height = 21;	/* normal height of a menu item */

    /* Don't update the menu height when it was set at a fixed value */
    if (gui.menu_height_fixed)
	return;

    /*
     * Get the height of the new item, before managing it, because it will
     * still reflect the font size.  After managing it depends on the menu
     * height, which is what we just wanted to get!.
     */
    if (id != (Widget)0)
	XtVaGetValues(id, XmNheight, &height, NULL);

    /* Find any menu Widget, to be able to call XtManageChild() */
    else
	for (mp = root_menu; mp != NULL; mp = mp->next)
	    if (mp->id != (Widget)0 && menubar_menu(mp->name))
	    {
		id = mp->id;
		break;
	    }

    /*
     * Now manage the menu item, to make them all be positioned (makes an
     * extra row when needed, removes it when not needed).
     */
    if (id != (Widget)0)
	XtManageChild(id);

    /*
     * Now find the menu item that is the furthest down, and get it's position.
     */
    maxy = 0;
    for (mp = root_menu; mp != NULL; mp = mp->next)
    {
	if (mp->id != (Widget)0 && menubar_menu(mp->name))
	{
	    XtVaGetValues(mp->id, XmNy, &y, NULL);
	    if (y > maxy)
		maxy = y;
	}
    }

    XtVaGetValues(menuBar,
	XmNmarginHeight, &margin,
	XmNshadowThickness, &shadow,
	NULL);

    /*
     * This computation is the result of trial-and-error:
     * maxy =	The maximum position of an item; required for when there are
     *		two or more rows
     * height = height of an item, before managing it;	Hopefully this will
     *		change with the font height.  Includes shadow-border.
     * shadow =	shadow-border; must be subtracted from the height.
     * margin = margin around the menu buttons;  Must be added.
     * Add 4 for the underlining of shortcut keys.
     */
    gui.menu_height = maxy + height - 2 * shadow + 2 * margin + 4;
}


    void
gui_mch_add_menu_item(menu, parent, idx)
    VimMenu	*menu;
    VimMenu	*parent;
    int		idx;
{
    XmString	label;

    /* No parent, must be a non-menubar menu */
    if (parent->submenu_id == (Widget)0)
	return;

    menu->submenu_id = (Widget)0;

    /* Add menu separator */
    if (is_menu_separator(menu->name))
    {
	menu->id = XtVaCreateWidget("subMenu",
		xmSeparatorWidgetClass, parent->submenu_id,
		XmNforeground, gui.menu_fg_pixel,
		XmNbackground, gui.menu_bg_pixel,
#if (XmVersion >= 1002)
		/* count the tearoff item (neede for LessTif) */
		XmNpositionIndex, idx + (tearoff_val == (int)XmTEAR_OFF_ENABLED
								     ? 1 : 0),
#endif
		NULL);
	return;
    }

    label = XmStringCreate((char *)menu->dname, STRING_TAG);
    if (label == NULL)
	return;
    menu->id = XtVaCreateWidget("subMenu",
	xmPushButtonWidgetClass, parent->submenu_id,
	XmNlabelString, label,
	XmNforeground, gui.menu_fg_pixel,
	XmNbackground, gui.menu_bg_pixel,
	XmNmnemonic, menu->mnemonic,
#if (XmVersion >= 1002)
	/* count the tearoff item (neede for LessTif) */
	XmNpositionIndex, idx + (tearoff_val == (int)XmTEAR_OFF_ENABLED
								     ? 1 : 0),
#endif
	NULL);
    XmStringFree(label);

    if (menu->id != (Widget)0)
    {
	XtAddCallback(menu->id, XmNactivateCallback, gui_x11_menu_cb,
		(XtPointer)menu);
	/* add accelerator text */
	gui_motif_add_actext(menu);
    }
}

    void
gui_mch_new_menu_colors()
{
    if (menuBar == NULL)
	return;
#if (XmVersion >= 1002)
    XmChangeColor(menuBar, gui.menu_bg_pixel);
#endif
    XtVaSetValues(menuBar,
	XmNforeground, gui.menu_fg_pixel,
#if (XmVersion <1002)
	XmNbackground, gui.menu_bg_pixel,
#endif
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
	{
#if (XmVersion >= 1002)
	    XmChangeColor(mp->id, gui.menu_bg_pixel);
#endif
	    XtVaSetValues(mp->id,
		    XmNforeground, gui.menu_fg_pixel,
#if (XmVersion <1002)
		    XmNbackground, gui.menu_bg_pixel,
#endif
		    NULL);
	}

	if (mp->children != NULL)
	{
#if (XmVersion >= 1002)
	    /* Set the colors for the tear off widget */
	    if (mp->submenu_id != (Widget)0)
	    {
		XmChangeColor(mp->submenu_id, gui.menu_bg_pixel);
		XtVaSetValues(mp->submenu_id, XmNforeground, gui.menu_fg_pixel, NULL);
		toggle_tearoff(mp->submenu_id);
	    }
#endif
	    /* Set the colors for the children */
	    gui_mch_submenu_colors(mp->children);
	}
	mp = mp->next;
    }
}

/*
 * Destroy the machine specific menu widget.
 */
    void
gui_mch_destroy_menu(menu)
    VimMenu *menu;
{
    Widget  parent;

    if (menu->submenu_id != (Widget)0)
    {
	XtDestroyWidget(menu->submenu_id);
	menu->submenu_id = (Widget)0;
    }
    if (menu->id != (Widget)0)
    {
	/*
	 * This is a hack to stop LessTif from crashing when a menu's last
	 * child is destroyed. We check to see if this is the last child and if
	 * so, don't delete it. The parent will be deleted soon anyway, and it
	 * will delete it's children like all good widgets do.
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
	menu->id = (Widget)0;

	if (parent == menuBar)
	    gui_mch_compute_menu_height((Widget)0);
    }
}

/* ARGSUSED */
    void
gui_mch_show_popupmenu(menu)
    VimMenu *menu;
{
#ifdef MOTIF_POPUP
    XmMenuPosition(menu->submenu_id, gui_x11_get_last_mouse_event());
    XtManageChild(menu->submenu_id);
#endif
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
    if (sb->id != (Widget)0)
	XtVaSetValues(sb->id,
		  XmNvalue, val,
		  XmNsliderSize, size,
		  XmNpageIncrement, (size > 2 ? size - 2 : 1),
		  XmNmaximum, max + 1,	    /* Motif has max one past the end */
		  NULL);
}

    void
gui_mch_set_scrollbar_pos(sb, x, y, w, h)
    GuiScrollbar    *sb;
    int		    x;
    int		    y;
    int		    w;
    int		    h;
{
    if (sb->id != (Widget)0)
    {
	XtVaSetValues(sb->id,
		      XmNleftOffset, x,
		      XmNtopOffset, y,
		      XmNrightOffset, -x - w,
		      XmNbottomOffset, -y - h,
		      NULL);
	XtManageChild(sb->id);
    }
}

    void
gui_mch_create_scrollbar(sb, orient)
    GuiScrollbar    *sb;
    int		    orient;	/* SBAR_VERT or SBAR_HORIZ */
{
    sb->id = XtVaCreateManagedWidget("scrollBar",
	    xmScrollBarWidgetClass, vimForm,
	    XmNshadowThickness, 1,
	    XmNminimum, 0,
	    XmNorientation, (orient == SBAR_VERT) ? XmVERTICAL
						  : XmHORIZONTAL,
	    XmNforeground, gui.scroll_fg_pixel,
	    XmNbackground, gui.scroll_fg_pixel,
	    XmNtroughColor, gui.scroll_bg_pixel,
	    XmNleftAttachment, XmATTACH_FORM,
	    XmNtopAttachment, XmATTACH_FORM,
	    XmNrightAttachment, XmATTACH_OPPOSITE_FORM,
	    XmNbottomAttachment, XmATTACH_OPPOSITE_FORM,
	    /* Use dummy offsets to avoid a warning for Lesstif */
	    XmNleftOffset, 2,
	    XmNtopOffset, 2,
	    XmNrightOffset, -4,
	    XmNbottomOffset, -4,
	    NULL);
    if (sb->id != (Widget)0)
    {
	XtAddCallback(sb->id, XmNvalueChangedCallback,
		      scroll_cb, (XtPointer)sb->ident);
	XtAddCallback(sb->id, XmNdragCallback,
		      scroll_cb, (XtPointer)sb->ident);
	XtAddEventHandler(sb->id, KeyPressMask, FALSE, gui_x11_key_hit_cb,
	    (XtPointer)0);
    }
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
    if (sb->id != NULL)
    {
#if (XmVersion>=1002)
	XmChangeColor(sb->id, gui.scroll_bg_pixel);
#endif
	XtVaSetValues(sb->id,
	    XmNforeground, gui.scroll_fg_pixel,
#if (XmVersion<1002)
	    XmNbackground, gui.scroll_fg_pixel,
	    XmNtroughColor, gui.scroll_bg_pixel,
#endif
	    NULL);
    }
}

/*
 * Miscellaneous stuff:
 */

    Window
gui_x11_get_wid()
{
    return(XtWindow(textArea));
}

#if defined(USE_BROWSE) || defined(PROTO)

/*
 * file selector related stuff
 */

#include <Xm/FileSB.h>
#include <Xm/XmStrDefs.h>

typedef struct dialog_callback_arg
{
    char *  args;   /* not used right now */
    int	    id;
} dcbarg_t;

static Widget dialog_wgt;
static char *browse_fname = NULL;
static XmStringCharSet charset = (XmStringCharSet) XmSTRING_DEFAULT_CHARSET;
				/* used to set up XmStrings */

static void DialogCancelCB __ARGS((Widget, XtPointer, XtPointer));
static void DialogAcceptCB __ARGS((Widget, XtPointer, XtPointer));

/*
 * Put up a file requester.
 * Returns the selected name in allocated memory, or NULL for Cancel.
 */
/* ARGSUSED */
    char_u *
gui_mch_browse(saving, title, dflt, ext, initdir, filter)
    int		saving;		/* select file to write */
    char_u	*title;		/* title for the window */
    char_u	*dflt;		/* default name */
    char_u	*ext;		/* not used (extension added) */
    char_u	*initdir;	/* initial directory, NULL for current dir */
    char_u	*filter;	/* not used (file name filter) */
{
    XmString	dirstring;
    char_u	dirbuf[MAXPATHL];

    dialog_wgt = XmCreateFileSelectionDialog(vimShell, (char *)title, NULL, 0);

    if (dflt == NULL)
	dflt = (char_u *)"";
    if (initdir == NULL || *initdir == NUL)
    {
	mch_dirname(dirbuf, MAXPATHL);
	initdir = dirbuf;
    }

    dirstring = XmStringCreate((char *)initdir, STRING_TAG);

    /* How do we set the default file name to "dflt"? */
    XtVaSetValues(dialog_wgt,
	XmNdirectory,		dirstring,
/*
 These can cause a crash after ":hi Menu guifg=red".  Why?
	XmNforeground,		gui.menu_fg_pixel,
	XmNbackground,		gui.menu_bg_pixel,
 */
	XmNtroughColor,		gui.scroll_bg_pixel,
/*
    currently, the background color of the input and selection
    fields are "motif blue".  i'm sure there must be a resource
    that corresponds to this, but i don't know what it is.

	XmNhighlightColor,	gui.norm_pixel,
	XmNborderColor,		gui.back_pixel,
	XmNtopShadowColor,	gui.menu_bg_pixel,
	XmNbottomShadowColor,	gui.norm_pixel,
*/
	NULL);

    XtAddCallback(dialog_wgt, XmNokCallback, DialogAcceptCB, (XtPointer)0);
    XtAddCallback(dialog_wgt, XmNcancelCallback, DialogCancelCB, (XtPointer)0);
    /* We have no help in this window, so hide help button */
    XtUnmanageChild(XmFileSelectionBoxGetChild(dialog_wgt,
					(unsigned char)XmDIALOG_HELP_BUTTON));

    XtManageChild(dialog_wgt);

    /* sit in a loop until the dialog box has gone away */
    do
    {
	XtAppProcessEvent(XtWidgetToApplicationContext(dialog_wgt),
	    (XtInputMask)XtIMAll);
    } while (XtIsManaged(dialog_wgt));

    XtDestroyWidget(dialog_wgt);
    XmStringFree(dirstring);

    if (browse_fname == NULL)
	return NULL;
    return vim_strsave((char_u *)browse_fname);
}

/*
 * The code below was originally taken from
 *	/usr/examples/motif/xmsamplers/xmeditor.c
 * on Digital Unix 4.0d, but heavily modified.
 */

/*
 * Process callback from Dialog cancel actions.
 */
/* ARGSUSED */
    static void
DialogCancelCB(w, client_data, call_data)
    Widget	w;		/*  widget id		*/
    XtPointer	client_data;	/*  data from application   */
    XtPointer	call_data;	/*  data from widget class  */
{
    if (browse_fname != NULL)
    {
	XtFree(browse_fname);
	browse_fname = NULL;
    }
    XtUnmanageChild(dialog_wgt);
}

/*
 * Process callback from Dialog actions.
 */
/* ARGSUSED */
    static void
DialogAcceptCB(w, client_data, call_data)
    Widget	w;		/*  widget id		*/
    XtPointer	client_data;	/*  data from application   */
    XtPointer	call_data;	/*  data from widget class  */
{
    XmFileSelectionBoxCallbackStruct *fcb;

    if (browse_fname != NULL)
    {
	XtFree(browse_fname);
	browse_fname = NULL;
    }
    fcb = (XmFileSelectionBoxCallbackStruct *)call_data;

    /* get the filename from the file selection box */
    XmStringGetLtoR(fcb->value, charset, &browse_fname);

    /* popdown the file selection box */
    XtUnmanageChild(dialog_wgt);
}

#endif /* USE_BROWSE */

#if defined(GUI_DIALOG) || defined(PROTO)

static int	dialogStatus;

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
    XmString		label;
    int			butcount;
    static Widget	dialogbb = NULL;
    static Widget	dialogmessage = NULL;
    Widget		*dialogButton;
    int			vertical;

    if (title == NULL)
	title = (char_u *)"Vim dialog";

    /* if our pointer is currently hidden, then we should show it. */
    gui_mch_mousehide(FALSE);

    /* First time called: create the dialog and the message */
    if (dialogbb == NULL)
    {
	dialogbb = XmCreateFormDialog(vimShell,
		(char *)"dialog", NULL, 0);

	dialogmessage = XtVaCreateManagedWidget("dialogMessage",
		xmLabelWidgetClass, dialogbb,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);
    }

    /* Set the message string */
    label = XmStringLtoRCreate((char *)message, STRING_TAG);
    if (label == NULL)
	return -1;
    XtVaSetValues(dialogmessage,
	    XmNlabelString, label,
	    NULL);
    XmStringFree(label);

    /* Check 'v' flag in 'guioptions': vertical button placement. */
    vertical = (vim_strchr(p_go, GO_VERTICAL) != NULL);

    /* Set the title of the Dialog window */
    label = XmStringCreateSimple((char *)title);
    if (label == NULL)
	return -1;
    XtVaSetValues(dialogbb,
	    XmNdialogTitle, label,
	    XmNhorizontalSpacing, 20,
	    XmNverticalSpacing, vertical ? 0 : 20,
	    NULL);
    XmStringFree(label);

    /* make a copy, so that we can insert NULs */
    buts = vim_strsave(buttons);
    if (buts == NULL)
	return -1;

    /* Count the number of buttons and allocate dialogButton[]. */
    butcount = 1;
    for (p = buts; *p; ++p)
	if (*p == DLG_BUTTON_SEP)
	    ++butcount;
    dialogButton = (Widget *)alloc((unsigned)(butcount * sizeof(Widget)));
    if (dialogButton == NULL)
    {
	vim_free(buts);
	return -1;
    }

    /*
     * Create the buttons.
     */
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
	label = XmStringCreate((char *)p, STRING_TAG);
	if (label == NULL)
	    break;

	dialogButton[butcount] = XtVaCreateManagedWidget("button",
		xmPushButtonWidgetClass, dialogbb,
		XmNlabelString, label,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, dialogmessage,
		NULL);
	XmStringFree(label);
	if (butcount)
	{
	    if (vertical)
		XtVaSetValues(dialogButton[butcount],
			XmNtopWidget, dialogButton[butcount - 1],
			NULL);
	    else
		XtVaSetValues(dialogButton[butcount],
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, dialogButton[butcount - 1],
			NULL);
	}

	XtAddCallback(dialogButton[butcount], XmNactivateCallback,
						butproc, (XtPointer)butcount);
	p = next;
    }
    vim_free(buts);

    if (dfltbutton < 1)
	dfltbutton = 1;
    if (dfltbutton > butcount)
	dfltbutton = butcount;
    XtVaSetValues(dialogbb,
	    XmNdefaultButton, dialogButton[dfltbutton - 1], NULL);
    XtManageChild(dialogbb);

    app = XtWidgetToApplicationContext(dialogbb);

    dialogStatus = -1;

    while (1)
    {
	XtAppNextEvent(app, &event);
	XtDispatchEvent(&event);
	if (dialogStatus >= 0)
	    break;
    }

    XtUnmanageChild(dialogbb);

    while (butcount)
    {
	--butcount;
	XtUnmanageChild(dialogButton[butcount]);
	XtDestroyWidget(dialogButton[butcount]);
    }

    vim_free(dialogButton);

    return dialogStatus;
}
#endif /* GUI_DIALOG */
