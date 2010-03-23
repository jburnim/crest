/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved		by Bram Moolenaar
 *				GUI/Motif support by Robert Webb
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <X11/Shell.h>

#include "vim.h"

#ifdef USE_FONTSET
# ifdef X_LOCALE
#  include <X11/Xlocale.h>
# else
#  include <locale.h>
# endif
#endif

#define VIM_NAME	"vim"
#define VIM_CLASS	"Vim"

/* Default resource values */
#define DFLT_FONT		"7x13"
#define DFLT_MENU_BG_COLOR	"gray77"
#define DFLT_MENU_FG_COLOR	"black"
#define DFLT_SCROLL_BG_COLOR	"gray60"
#define DFLT_SCROLL_FG_COLOR	"gray77"

Widget vimShell = (Widget)0;

static Atom   wm_atoms[2];	/* Window Manager Atoms */
#define DELETE_WINDOW_IDX 0	/* index in wm_atoms[] for WM_DELETE_WINDOW */
#define SAVE_YOURSELF_IDX 1	/* index in wm_atoms[] for WM_SAVE_YOURSELF */

#ifdef USE_FONTSET
static XFontSet current_fontset;
#define XDrawString(dpy,win,gc,x,y,str,n) \
	    do \
	    { \
		if (gui.fontset) \
		    XmbDrawString(dpy,win,current_fontset,gc,x,y,str,n); \
		else \
		    XDrawString(dpy,win,gc,x,y,str,n); \
	    } while (0)
#ifndef SLOW_XSERVER
static Window workwid = 0;
static void create_workwindow __ARGS((void));
#endif

static int check_fontset_sanity __ARGS((XFontSet fs));
static int fontset_width __ARGS((XFontSet fs));
static int fontset_height __ARGS((XFontSet fs));
static int fontset_ascent __ARGS((XFontSet fs));
static GuiFont gui_mch_get_fontset __ARGS((char_u *name, int giveErrorIfMissing));
#endif

static GuiColor	prev_fg_color = (GuiColor)-1;
static GuiColor	prev_bg_color = (GuiColor)-1;

#ifdef USE_GUI_MOTIF
static XButtonPressedEvent last_mouse_event;
#endif

static void gui_x11_timer_cb __ARGS((XtPointer timed_out, XtIntervalId *interval_id));
static void gui_x11_visibility_cb __ARGS((Widget w, XtPointer dud, XEvent *event, Boolean *dum));
static void gui_x11_expose_cb __ARGS((Widget w, XtPointer dud, XEvent *event, Boolean *dum));
static void gui_x11_resize_window_cb __ARGS((Widget w, XtPointer dud, XEvent *event, Boolean *dum));
static void gui_x11_focus_change_cb __ARGS((Widget w, XtPointer data, XEvent *event, Boolean *dum));
static void gui_x11_enter_cb __ARGS((Widget w, XtPointer data, XEvent *event, Boolean *dum));
static void gui_x11_leave_cb __ARGS((Widget w, XtPointer data, XEvent *event, Boolean *dum));
static void gui_x11_mouse_cb __ARGS((Widget w, XtPointer data, XEvent *event, Boolean *dum));
#ifdef USE_SNIFF
static void gui_x11_sniff_request_cb __ARGS((XtPointer closure, int *source, XtInputId *id));
#endif
static void gui_x11_check_copy_area __ARGS((void));
static void gui_x11_wm_protocol_handler __ARGS((Widget, XtPointer, XEvent *, Boolean *));
static void gui_x11_blink_cb __ARGS((XtPointer timed_out, XtIntervalId *interval_id));
static Cursor gui_x11_create_blank_mouse __ARGS((void));


static struct specialkey
{
    KeySym  key_sym;
    char_u  vim_code0;
    char_u  vim_code1;
} special_keys[] =
{
    {XK_Up,	    'k', 'u'},
    {XK_Down,	    'k', 'd'},
    {XK_Left,	    'k', 'l'},
    {XK_Right,	    'k', 'r'},

    {XK_F1,	    'k', '1'},
    {XK_F2,	    'k', '2'},
    {XK_F3,	    'k', '3'},
    {XK_F4,	    'k', '4'},
    {XK_F5,	    'k', '5'},
    {XK_F6,	    'k', '6'},
    {XK_F7,	    'k', '7'},
    {XK_F8,	    'k', '8'},
    {XK_F9,	    'k', '9'},
    {XK_F10,	    'k', ';'},

    {XK_F11,	    'F', '1'},
    {XK_F12,	    'F', '2'},
    {XK_F13,	    'F', '3'},
    {XK_F14,	    'F', '4'},
    {XK_F15,	    'F', '5'},
    {XK_F16,	    'F', '6'},
    {XK_F17,	    'F', '7'},
    {XK_F18,	    'F', '8'},
    {XK_F19,	    'F', '9'},
    {XK_F20,	    'F', 'A'},

    {XK_F21,	    'F', 'B'},
    {XK_F22,	    'F', 'C'},
    {XK_F23,	    'F', 'D'},
    {XK_F24,	    'F', 'E'},
    {XK_F25,	    'F', 'F'},
    {XK_F26,	    'F', 'G'},
    {XK_F27,	    'F', 'H'},
    {XK_F28,	    'F', 'I'},
    {XK_F29,	    'F', 'J'},
    {XK_F30,	    'F', 'K'},

    {XK_F31,	    'F', 'L'},
    {XK_F32,	    'F', 'M'},
    {XK_F33,	    'F', 'N'},
    {XK_F34,	    'F', 'O'},
    {XK_F35,	    'F', 'P'},	    /* keysymdef.h defines up to F35 */

    {XK_Help,	    '%', '1'},
    {XK_Undo,	    '&', '8'},
    {XK_BackSpace,  'k', 'b'},
    {XK_Insert,	    'k', 'I'},
    {XK_Delete,	    'k', 'D'},
    {XK_Home,	    'k', 'h'},
    {XK_End,	    '@', '7'},
    {XK_Prior,	    'k', 'P'},
    {XK_Next,	    'k', 'N'},
    {XK_Print,	    '%', '9'},

    /* Keypad keys: */
#ifdef XK_KP_Left
    {XK_KP_Left,    'k', 'l'},
    {XK_KP_Right,   'k', 'r'},
    {XK_KP_Up,	    'k', 'u'},
    {XK_KP_Down,    'k', 'd'},
    {XK_KP_Insert,  'k', 'I'},
    {XK_KP_Delete,  'k', 'D'},
    {XK_KP_Home,    'k', 'h'},
    {XK_KP_End,	    '@', '7'},
    {XK_KP_Prior,   'k', 'P'},
    {XK_KP_Next,    'k', 'N'},
#endif

    /* End of list marker: */
    {(KeySym)0,	    0, 0}
};

#define XtNboldFont	    "boldFont"
#define XtCBoldFont	    "BoldFont"
#define XtNitalicFont	    "italicFont"
#define XtCItalicFont	    "ItalicFont"
#define XtNboldItalicFont   "boldItalicFont"
#define XtCBoldItalicFont   "BoldItalicFont"
#define XtNscrollbarWidth   "scrollbarWidth"
#define XtCScrollbarWidth   "ScrollbarWidth"
#define XtNmenuHeight	    "menuHeight"
#define XtCMenuHeight	    "MenuHeight"

/* Resources for setting the foreground and background colors of menus */
#define XtNmenuBackground   "menuBackground"
#define XtCMenuBackground   "MenuBackground"
#define XtNmenuForeground   "menuForeground"
#define XtCMenuForeground   "MenuForeground"

/* Resources for setting the foreground and background colors of scrollbars */
#define XtNscrollBackground "scrollBackground"
#define XtCScrollBackground "ScrollBackground"
#define XtNscrollForeground "scrollForeground"
#define XtCScrollForeground "ScrollForeground"

/*
 * X Resources:
 */
static XtResource vim_resources[] =
{
    {
	XtNforeground,
	XtCForeground,
	XtRPixel,
	sizeof(Pixel),
	XtOffsetOf(Gui, def_norm_pixel),
	XtRString,
	XtDefaultForeground
    },
    {
	XtNbackground,
	XtCBackground,
	XtRPixel,
	sizeof(Pixel),
	XtOffsetOf(Gui, def_back_pixel),
	XtRString,
	XtDefaultBackground
    },
    {
	XtNfont,
	XtCFont,
	XtRString,
	sizeof(String *),
	XtOffsetOf(Gui, dflt_font),
	XtRImmediate,
	XtDefaultFont
    },
    {
	XtNboldFont,
	XtCBoldFont,
	XtRString,
	sizeof(String *),
	XtOffsetOf(Gui, dflt_bold_fn),
	XtRImmediate,
	""
    },
    {
	XtNitalicFont,
	XtCItalicFont,
	XtRString,
	sizeof(String *),
	XtOffsetOf(Gui, dflt_ital_fn),
	XtRImmediate,
	""
    },
    {
	XtNboldItalicFont,
	XtCBoldItalicFont,
	XtRString,
	sizeof(String *),
	XtOffsetOf(Gui, dflt_boldital_fn),
	XtRImmediate,
	""
    },
    {
	XtNgeometry,
	XtCGeometry,
	XtRString,
	sizeof(String *),
	XtOffsetOf(Gui, geom),
	XtRImmediate,
	""
    },
    {
	XtNreverseVideo,
	XtCReverseVideo,
	XtRBool,
	sizeof(Bool),
	XtOffsetOf(Gui, rev_video),
	XtRImmediate,
	(XtPointer) False
    },
    {
	XtNborderWidth,
	XtCBorderWidth,
	XtRInt,
	sizeof(int),
	XtOffsetOf(Gui, border_width),
	XtRImmediate,
	(XtPointer) 2
    },
    {
	XtNscrollbarWidth,
	XtCScrollbarWidth,
	XtRInt,
	sizeof(int),
	XtOffsetOf(Gui, scrollbar_width),
	XtRImmediate,
	(XtPointer) SB_DEFAULT_WIDTH
    },
#ifdef WANT_MENU
    {
	XtNmenuHeight,
	XtCMenuHeight,
	XtRInt,
	sizeof(int),
	XtOffsetOf(Gui, menu_height),
	XtRImmediate,
	(XtPointer) MENU_DEFAULT_HEIGHT	    /* Should figure out at run time */
    },
#endif
    {
	XtNmenuForeground,
	XtCMenuForeground,
	XtRPixel,
	sizeof(Pixel),
	XtOffsetOf(Gui, menu_fg_pixel),
	XtRString,
	DFLT_MENU_FG_COLOR
    },
    {
	XtNmenuBackground,
	XtCMenuBackground,
	XtRPixel,
	sizeof(Pixel),
	XtOffsetOf(Gui, menu_bg_pixel),
	XtRString,
	DFLT_MENU_BG_COLOR
    },
    {
	XtNscrollForeground,
	XtCScrollForeground,
	XtRPixel,
	sizeof(Pixel),
	XtOffsetOf(Gui, scroll_fg_pixel),
	XtRString,
	DFLT_SCROLL_FG_COLOR
    },
    {
	XtNscrollBackground,
	XtCScrollBackground,
	XtRPixel,
	sizeof(Pixel),
	XtOffsetOf(Gui, scroll_bg_pixel),
	XtRString,
	DFLT_SCROLL_BG_COLOR
    },
#ifdef USE_XIM
    {
	"preeditType",
	"PreeditType",
	XtRString,
	sizeof(char*),
	XtOffsetOf(Gui, preedit_type),
	XtRString,
	(XtPointer)"OverTheSpot,OffTheSpot,Root"
    },
    {
	"inputMethod",
	"InputMethod",
	XtRString,
	sizeof(char*),
	XtOffsetOf(Gui, input_method),
	XtRString,
	NULL
    },
    {
	"openIM",
	"OpenIM",
	XtRBoolean,
	sizeof(Boolean),
	XtOffsetOf(Gui, open_im),
	XtRImmediate,
	(XtPointer)TRUE
    },
#endif /* USE_XIM */
};

/*
 * This table holds all the X GUI command line options allowed.  This includes
 * the standard ones so that we can skip them when vim is started without the
 * GUI (but the GUI might start up later).
 * When changing this, also update doc/vim_gui.txt and the usage message!!!
 */
static XrmOptionDescRec cmdline_options[] =
{
    /* We handle these options ourselves */
    {"-bg",		".background",	    XrmoptionSepArg,	NULL},
    {"-background",	".background",	    XrmoptionSepArg,	NULL},
    {"-fg",		".foreground",	    XrmoptionSepArg,	NULL},
    {"-foreground",	".foreground",	    XrmoptionSepArg,	NULL},
    {"-fn",		".font",	    XrmoptionSepArg,	NULL},
    {"-font",		".font",	    XrmoptionSepArg,	NULL},
    {"-boldfont",	".boldFont",	    XrmoptionSepArg,	NULL},
    {"-italicfont",	".italicFont",	    XrmoptionSepArg,	NULL},
    {"-geom",		".geometry",	    XrmoptionSepArg,	NULL},
    {"-geometry",	".geometry",	    XrmoptionSepArg,	NULL},
    {"-reverse",	"*reverseVideo",    XrmoptionNoArg,	"True"},
    {"-rv",		"*reverseVideo",    XrmoptionNoArg,	"True"},
    {"+reverse",	"*reverseVideo",    XrmoptionNoArg,	"False"},
    {"+rv",		"*reverseVideo",    XrmoptionNoArg,	"False"},
    {"-display",	".display",	    XrmoptionSepArg,	NULL},
    {"-iconic",		"*iconic",	    XrmoptionNoArg,	"True"},
    {"-name",		".name",	    XrmoptionSepArg,	NULL},
    {"-bw",		".borderWidth",	    XrmoptionSepArg,	NULL},
    {"-borderwidth",	".borderWidth",	    XrmoptionSepArg,	NULL},
    {"-sw",		".scrollbarWidth",  XrmoptionSepArg,	NULL},
    {"-scrollbarwidth",	".scrollbarWidth",  XrmoptionSepArg,	NULL},
    {"-mh",		".menuHeight",	    XrmoptionSepArg,	NULL},
    {"-menuheight",	".menuHeight",	    XrmoptionSepArg,	NULL},
    {"-xrm",		NULL,		    XrmoptionResArg,	NULL}
};

static int gui_argc = 0;
static char **gui_argv = NULL;

/*
 * Call-back routines.
 */

/* ARGSUSED */
    static void
gui_x11_timer_cb(timed_out, interval_id)
    XtPointer	    timed_out;
    XtIntervalId    *interval_id;
{
    *((int *)timed_out) = TRUE;
}

/* ARGSUSED */
    static void
gui_x11_visibility_cb(w, dud, event, dum)
    Widget	w;
    XtPointer	dud;
    XEvent	*event;
    Boolean	*dum;
{
    if (event->type != VisibilityNotify)
	return;

    gui.visibility = event->xvisibility.state;

    /*
     * When we do an XCopyArea(), and the window is partially obscured, we want
     * to receive an event to tell us whether it worked or not.
     */
    XSetGraphicsExposures(gui.dpy, gui.text_gc,
	gui.visibility != VisibilityUnobscured);
}

/* ARGSUSED */
    static void
gui_x11_expose_cb(w, dud, event, dum)
    Widget	w;
    XtPointer	dud;
    XEvent	*event;
    Boolean	*dum;
{
    XExposeEvent    *gevent;

    if (event->type != Expose)
	return;

    out_flush();	    /* make sure all output has been processed */

    gevent = (XExposeEvent *)event;
    gui_redraw(gevent->x, gevent->y, gevent->width, gevent->height);

    /* Clear the border areas if needed */
    if (gevent->x < FILL_X(0))
	XClearArea(gui.dpy, gui.wid, 0, 0, FILL_X(0), 0, False);
    if (gevent->y < FILL_Y(0))
	XClearArea(gui.dpy, gui.wid, 0, 0, 0, FILL_Y(0), False);
    if (gevent->x > FILL_X(Columns))
	XClearArea(gui.dpy, gui.wid, FILL_X((int)Columns), 0, 0, 0, False);
    if (gevent->y > FILL_Y(Rows))
	XClearArea(gui.dpy, gui.wid, 0, FILL_Y((int)Rows), 0, 0, False);
}

/* ARGSUSED */
    static void
gui_x11_resize_window_cb(w, dud, event, dum)
    Widget	w;
    XtPointer	dud;
    XEvent	*event;
    Boolean	*dum;
{
    if (event->type != ConfigureNotify)
	return;

    gui_resize_window(event->xconfigure.width, event->xconfigure.height
#ifdef USE_XIM
						- xim_get_status_area_height()
#endif
		     );
#ifdef USE_XIM
    xim_set_preedit();
#endif
#if defined(USE_FONTSET) && !defined(SLOW_XSERVER)
    if (gui.fontset)
	create_workwindow ();
#endif
}

/* ARGSUSED */
    static void
gui_x11_focus_change_cb(w, data, event, dum)
    Widget	w;
    XtPointer	data;
    XEvent	*event;
    Boolean	*dum;
{
    gui_focus_change(event->type == FocusIn);
#ifdef USE_XIM
    xim_set_focus(event->type == FocusIn);
#endif
}

/* ARGSUSED */
    static void
gui_x11_enter_cb(w, data, event, dum)
    Widget	w;
    XtPointer	data;
    XEvent	*event;
    Boolean	*dum;
{
    gui_focus_change(TRUE);
}

/* ARGSUSED */
    static void
gui_x11_leave_cb(w, data, event, dum)
    Widget	w;
    XtPointer	data;
    XEvent	*event;
    Boolean	*dum;
{
    gui_focus_change(FALSE);
}

/* ARGSUSED */
    void
gui_x11_key_hit_cb(w, dud, event, dum)
    Widget	w;
    XtPointer	dud;
    XEvent	*event;
    Boolean	*dum;
{
    XKeyPressedEvent	*ev_press;
#ifdef USE_XIM
    char_u		string2[256];
    char_u		string_shortbuf[256];
    char_u		*string=string_shortbuf;
    Boolean		string_alloced = False;
    Status		status;
#else
    char_u		string[3], string2[3];
#endif
    KeySym		key_sym, key_sym2;
    int			len, len2;
    int			i;
    int			modifiers;
    int			key;

    ev_press = (XKeyPressedEvent *)event;

#ifdef USE_XIM
    if (xic)
    {
	len = XmbLookupString(xic, ev_press, (char *)string, sizeof(string_shortbuf),
		&key_sym, &status);
	if (status == XBufferOverflow)
	{
	    string = (char_u *)XtMalloc(len + 1);
	    string_alloced = True;
	    len = XmbLookupString(xic, ev_press, (char *)string, len,
		    &key_sym, &status);
	}
	if (status == XLookupNone || status == XLookupChars)
	    key_sym = XK_VoidSymbol;
    }
    else
#endif
	len = XLookupString(ev_press, (char *)string, sizeof(string),
		&key_sym, NULL);

#ifdef HANGUL_INPUT
    if ((key_sym == XK_space) && (ev_press->state & ShiftMask))
    {
	hangul_input_state_toggle();
	return;
    }
#endif

    if (key_sym == XK_space)
	string[0] = ' ';	/* Otherwise Ctrl-Space doesn't work */

    /*
     * Only on some machines ^_ requires Ctrl+Shift+minus.  For consistency,
     * allow just Ctrl+minus too.
     */
    if (key_sym == XK_minus && (ev_press->state & ControlMask))
	string[0] = Ctrl('_');

#ifdef XK_ISO_Left_Tab
    /* why do we get XK_ISO_Left_Tab instead of XK_Tab for shift-tab? */
    if (key_sym == XK_ISO_Left_Tab)
    {
	key_sym = XK_Tab;
	string[0] = TAB;
	len = 1;
    }
#endif

    /* Check for Alt/Meta key (Mod1Mask) */
    if (len == 1 && (ev_press->state & Mod1Mask)
			&& !(key_sym == XK_BackSpace || key_sym == XK_Delete))
    {
#ifdef USE_GUI_MOTIF
	/* Ignore ALT keys when they are used for the menu only */
	if (gui.menu_is_active
		&& (p_wak[0] == 'y'
		    || (p_wak[0] == 'm' && gui_is_menu_shortcut(string[0]))))
	{
#ifdef USE_XIM
	    if (string_alloced)
		XtFree((char *)string);
#endif
	    return;
	}
#endif
	/*
	 * Before we set the 8th bit, check to make sure the user doesn't
	 * already have a mapping defined for this sequence. We determine this
	 * by checking to see if the input would be the same without the
	 * Alt/Meta key.
	 * Don't do this for <S-M-Tab>, that should become K_S_TAB with ALT.
	 */
	ev_press->state &= ~Mod1Mask;
	len2 = XLookupString(ev_press, (char *)string2, sizeof(string2),
							     &key_sym2, NULL);
	if (key_sym2 == XK_space)
	    string2[0] = ' ';	    /* Otherwise Meta-Ctrl-Space doesn't work */
	if (	   len2 == 1
		&& string[0] == string2[0]
		&& !(key_sym == XK_Tab && (ev_press->state & ShiftMask)))
	    string[0] |= 0x80;
	else
	    ev_press->state |= Mod1Mask;
    }

#if 0
    if (len == 1 && string[0] == CSI) /* this doesn't work yet */
    {
	string[1] = CSI;
	string[2] = CSI;
	len = 3;
    }
#endif

    /* Check for special keys, making sure BS and DEL are recognised. */
    if (len == 0 || key_sym == XK_BackSpace || key_sym == XK_Delete)
    {
	for (i = 0; special_keys[i].key_sym != (KeySym)0; i++)
	{
	    if (special_keys[i].key_sym == key_sym)
	    {
		string[0] = CSI;
		string[1] = special_keys[i].vim_code0;
		string[2] = special_keys[i].vim_code1;
		len = 3;
	    }
	}
    }

    /* Unrecognised key */
    if (len == 0)
    {
#ifdef USE_XIM
	if (string_alloced)
	    XtFree((char *)string);
#endif
	return;
    }

    /* Special keys (and a few others) may have modifiers */
    if (len == 3 || key_sym == XK_space || key_sym == XK_Tab
	|| key_sym == XK_Return || key_sym == XK_Linefeed
	|| key_sym == XK_Escape)
    {
	modifiers = 0;
	if (ev_press->state & ShiftMask)
	    modifiers |= MOD_MASK_SHIFT;
	if (ev_press->state & ControlMask)
	    modifiers |= MOD_MASK_CTRL;
	if (ev_press->state & Mod1Mask)
	    modifiers |= MOD_MASK_ALT;

#if defined(USE_XIM) && defined(MULTI_BYTE)
	if (!is_dbcs || key_sym != XK_VoidSymbol)
#endif
	{
	    /*
	     * For some keys a shift modifier is translated into another key
	     * code.  Do we need to handle the case where len != 1 and
	     * string[0] != CSI?
	     */
	    if (string[0] == CSI && len == 3)
		key = TO_SPECIAL(string[1], string[2]);
	    else
		key = string[0];
	    key = simplify_key(key, &modifiers);
	    if (IS_SPECIAL(key))
	    {
		string[0] = CSI;
		string[1] = K_SECOND(key);
		string[2] = K_THIRD(key);
		len = 3;
	    }
	    else
	    {
		string[0] = key;
		len = 1;
	    }
	}

	if (modifiers)
	{
	    string2[0] = CSI;
	    string2[1] = KS_MODIFIER;
	    string2[2] = modifiers;
	    add_to_input_buf(string2, 3);
	}
    }
    if (len == 1 && (string[0] == Ctrl('C')
#ifdef UNIX
	    || (intr_char != 0 && string[0] == intr_char)
#endif
	    ))
    {
	trash_input_buf();
	got_int = TRUE;
    }

    if (len == 1 && string[0] == CSI)
    {
	/* Turn CSI into K_CSI. */
	string[1] = KS_EXTRA;
	string[2] = KE_CSI;
	len = 3;
    }

    add_to_input_buf(string, len);

#ifdef USE_XIM
    if (string_alloced)
	XtFree((char *)string);
#endif

    /*
     * blank out the pointer if necessary
     */
    if (p_mh)
	gui_mch_mousehide(TRUE);
}

/* ARGSUSED */
    static void
gui_x11_mouse_cb(w, dud, event, dum)
    Widget	w;
    XtPointer	dud;
    XEvent	*event;
    Boolean	*dum;
{
    static XtIntervalId timer = (XtIntervalId)0;
    static int	timed_out = TRUE;

    int		button;
    int		repeated_click = FALSE;
    int		x, y;
    int_u	x_modifiers;
    int_u	vim_modifiers;

    if (event->type == MotionNotify)
    {
	x = event->xmotion.x;
	y = event->xmotion.y;
	x_modifiers = event->xmotion.state;
	button = (x_modifiers & (Button1Mask | Button2Mask | Button3Mask))
		? MOUSE_DRAG : ' ';

	/*
	 * if our pointer is currently hidden, then we should show it.
	 */
	gui_mch_mousehide(FALSE);

	if (button != MOUSE_DRAG)	/* just moving the rodent */
	{
#ifdef WANT_MENU
	    if (dud)			/* moved in vimForm */
		y -= gui.menu_height;
#endif
	    gui_mouse_moved(y);
	    return;
	}
    }
    else
    {
	x = event->xbutton.x;
	y = event->xbutton.y;
	if (event->type == ButtonPress)
	{
	    /* Handle multiple clicks */
	    if (!timed_out)
	    {
		XtRemoveTimeOut(timer);
		repeated_click = TRUE;
	    }
	    timed_out = FALSE;
	    timer = XtAppAddTimeOut(app_context, (long_u)p_mouset,
			gui_x11_timer_cb, &timed_out);
	    switch (event->xbutton.button)
	    {
		case Button1:	button = MOUSE_LEFT;	break;
		case Button2:	button = MOUSE_MIDDLE;	break;
		case Button3:	button = MOUSE_RIGHT;	break;
		case Button4:	button = MOUSE_4;	break;
		case Button5:	button = MOUSE_5;	break;
		default:
		    return;	/* Unknown button */
	    }
	}
	else if (event->type == ButtonRelease)
	    button = MOUSE_RELEASE;
	else
	    return;	/* Unknown mouse event type */

	x_modifiers = event->xbutton.state;
#ifdef USE_GUI_MOTIF
	last_mouse_event = event->xbutton;
#endif
    }

    vim_modifiers = 0x0;
    if (x_modifiers & ShiftMask)
	vim_modifiers |= MOUSE_SHIFT;
    if (x_modifiers & ControlMask)
	vim_modifiers |= MOUSE_CTRL;
    if (x_modifiers & Mod1Mask)	    /* Alt or Meta key */
	vim_modifiers |= MOUSE_ALT;

    gui_send_mouse_event(button, x, y, repeated_click, vim_modifiers);
}

#ifdef USE_SNIFF
/* ARGSUSED */
    static void
gui_x11_sniff_request_cb(closure, source, id)
    XtPointer	closure;
    int		*source;
    XtInputId	*id;
{
    static char_u bytes[3] = {CSI, (int)KS_EXTRA, (int)KE_SNIFF};

    add_to_input_buf(bytes, 3);
}
#endif

/*
 * End of call-back routines
 */

/*
 * Parse the GUI related command-line arguments.  Any arguments used are
 * deleted from argv, and *argc is decremented accordingly.  This is called
 * when vim is started, whether or not the GUI has been started.
 */
    void
gui_mch_prepare(argc, argv)
    int	    *argc;
    char    **argv;
{
    int	    arg;
    int	    i;

    /*
     * Move all the entries in argv which are relevant to X into gui_argv.
     */
    gui_argc = 0;
    gui_argv = (char **)lalloc((long_u)(*argc * sizeof(char *)), FALSE);
    if (gui_argv == NULL)
	return;
    gui_argv[gui_argc++] = argv[0];
    arg = 1;
    while (arg < *argc)
    {
	/* Look for argv[arg] in cmdline_options[] table */
	for (i = 0; i < XtNumber(cmdline_options); i++)
	    if (strcmp(argv[arg], cmdline_options[i].option) == 0)
		break;

	if (i < XtNumber(cmdline_options))
	{
	    /* Remember finding "-rv" or "-reverse" */
	    if (strcmp("-rv", argv[arg]) == 0
		    || strcmp("-reverse", argv[arg]) == 0)
		found_reverse_arg = TRUE;
	    else if ((strcmp("-fn", argv[arg]) == 0
			|| strcmp("-font", argv[arg]) == 0)
		    && arg + 1 < *argc)
		font_opt = argv[arg+1];

	    /* Found match in table, so move it into gui_argv */
	    gui_argv[gui_argc++] = argv[arg];
	    if (--*argc > arg)
	    {
		mch_memmove(&argv[arg], &argv[arg + 1], (*argc - arg)
						    * sizeof(char *));
		if (cmdline_options[i].argKind != XrmoptionNoArg)
		{
		    /* Move the options argument as well */
		    gui_argv[gui_argc++] = argv[arg];
		    if (--*argc > arg)
			mch_memmove(&argv[arg], &argv[arg + 1], (*argc - arg)
							    * sizeof(char *));
		}
	    }
	}
	else
	    arg++;
    }
}

#ifndef XtSpecificationRelease
# define CARDINAL (Cardinal *)
#else
# if XtSpecificationRelease == 4
# define CARDINAL (Cardinal *)
# else
# define CARDINAL (int *)
# endif
#endif

/*
 * Check if the GUI can be started.  Called before gvimrc is sourced.
 * Return OK or FAIL.
 */
    int
gui_mch_init_check()
{
#if defined(USE_FONTSET) && (defined(HAVE_LOCALE_H) || defined(X_LOCALE))
    setlocale(LC_ALL, "");
#endif
#ifdef USE_XIM
    XtSetLanguageProc(NULL, NULL, NULL);
#endif
    open_app_context();
    if (app_context != NULL)
	gui.dpy = XtOpenDisplay(app_context, 0, VIM_NAME, VIM_CLASS,
	    cmdline_options, XtNumber(cmdline_options),
	    CARDINAL &gui_argc, gui_argv);

    if (app_context == NULL || gui.dpy == NULL)
    {
	gui.dying = TRUE;
	EMSG("cannot open display");
	return FAIL;
    }
    return OK;
}

/*
 * Initialise the X GUI.  Create all the windows, set up all the call-backs etc.
 * Returns OK for success, FAIL when the GUI can't be started.
 */
    int
gui_mch_init()
{
    Widget	AppShell;
    long_u	gc_mask;
    XGCValues	gc_vals;
    int		x, y, mask;
    unsigned	w, h;

    /* Uncomment this to enable synchronous mode for debugging */
    /* XSynchronize(gui.dpy, True); */

    /*
     * So converters work.
     */
    XtInitializeWidgetClass(applicationShellWidgetClass);
    XtInitializeWidgetClass(topLevelShellWidgetClass);

    /*
     * The applicationShell is created as an unrealized
     * parent for multiple topLevelShells.  The topLevelShells
     * are created as popup children of the applicationShell.
     * This is a recommendation of Paul Asente & Ralph Swick in
     * _X_Window_System_Toolkit_ p. 677.
     */
    AppShell = XtVaAppCreateShell(VIM_NAME, VIM_CLASS,
	    applicationShellWidgetClass, gui.dpy,
	    NULL);

    /*
     * Get the application resources
     */
    XtVaGetApplicationResources(AppShell, (XtPointer)&gui,
	vim_resources, XtNumber(vim_resources), NULL);

    gui.scrollbar_height = gui.scrollbar_width;

#ifdef WANT_MENU
    /* If the menu height was set, don't change it at runtime */
    if (gui.menu_height != MENU_DEFAULT_HEIGHT)
	gui.menu_height_fixed = TRUE;
#endif

    /* Set default foreground and background colours */
    gui.norm_pixel = gui.def_norm_pixel;
    gui.back_pixel = gui.def_back_pixel;

    /* Create shell widget to put vim in */
    vimShell = XtVaCreatePopupShell("VIM",
	topLevelShellWidgetClass, AppShell,
	XtNborderWidth, 0,
	NULL);

    /* Check if reverse video needs to be applied (on Sun it's done by X) */
    if (gui.rev_video && gui_mch_get_lightness(gui.back_pixel)
				      > gui_mch_get_lightness(gui.norm_pixel))
    {
	gui.norm_pixel = gui.def_back_pixel;
	gui.back_pixel = gui.def_norm_pixel;
	gui.def_norm_pixel = gui.norm_pixel;
	gui.def_back_pixel = gui.back_pixel;
    }

    /* Get the colors from the "Normal" and "Menu" group (set in syntax.c or
     * in a vimrc file) */
    set_normal_colors();

    /*
     * Check that none of the colors are the same as the background color
     */
    gui_check_colors();

    /*
     * Set up the GCs.	The font attributes will be set in gui_init_font().
     */
    gc_mask = GCForeground | GCBackground;
    gc_vals.foreground = gui.norm_pixel;
    gc_vals.background = gui.back_pixel;
    gui.text_gc = XCreateGC(gui.dpy, DefaultRootWindow(gui.dpy), gc_mask,
	    &gc_vals);

    gc_vals.foreground = gui.back_pixel;
    gc_vals.background = gui.norm_pixel;
    gui.back_gc = XCreateGC(gui.dpy, DefaultRootWindow(gui.dpy), gc_mask,
	    &gc_vals);

    gc_mask |= GCFunction;
    gc_vals.foreground = gui.norm_pixel ^ gui.back_pixel;
    gc_vals.background = gui.norm_pixel ^ gui.back_pixel;
    gc_vals.function   = GXxor;
    gui.invert_gc = XCreateGC(gui.dpy, DefaultRootWindow(gui.dpy), gc_mask,
	    &gc_vals);

    gui.visibility = VisibilityUnobscured;
    x11_setup_atoms(gui.dpy);

    /* Now adapt the supplied(?) geometry-settings */
    /* Added by Kjetil Jacobsen <kjetilja@stud.cs.uit.no> */
    if (gui.geom != NULL && *gui.geom != NUL)
    {
	mask = XParseGeometry((char *)gui.geom, &x, &y, &w, &h);
	if (mask & WidthValue)
	    Columns = w;
	if (mask & HeightValue)
	    Rows = h;
	/*
	 * Set the (x,y) position of the main window only if specified in the
	 * users geometry, so we get good defaults when they don't. This needs
	 * to be done before the shell is popped up.
	 */
	if (mask & (XValue|YValue))
	    XtVaSetValues(vimShell, XtNgeometry, gui.geom, NULL);
    }

    gui_x11_create_widgets();

   /*
    * Add an icon to Vim (Marcel Douben: 11 May 1998).
    */
    if (vim_strchr(p_go, GO_ICON) != NULL)
    {
#include "vim_icon.xbm"
#include "vim_mask.xbm"

	Arg	arg[2];

	XtSetArg(arg[0], XtNiconPixmap,
		XCreateBitmapFromData(gui.dpy,
		    DefaultRootWindow(gui.dpy),
		    (char *)vim_icon_bits,
		    vim_icon_width,
		    vim_icon_height));
	XtSetArg(arg[1], XtNiconMask,
		XCreateBitmapFromData(gui.dpy,
		    DefaultRootWindow(gui.dpy),
		    (char *)vim_mask_icon_bits,
		    vim_mask_icon_width,
		    vim_mask_icon_height));
	XtSetValues(vimShell, arg, (Cardinal)2);
    }

    return OK;
}

/*
 * Called when starting the GUI fails after calling gui_mch_init().
 */
    void
gui_mch_uninit()
{
    gui_x11_destroy_widgets();
#ifndef LESSTIF_VERSION
    XtCloseDisplay(gui.dpy);
#endif
    gui.dpy = NULL;
    vimShell = (Widget)0;
}

/*
 * Called when the foreground or background color has been changed.
 */
    void
gui_mch_new_colors()
{
    long_u	gc_mask;
    XGCValues	gc_vals;

    gc_mask = GCForeground | GCBackground;
    gc_vals.foreground = gui.norm_pixel;
    gc_vals.background = gui.back_pixel;
    if (gui.text_gc != NULL)
	XChangeGC(gui.dpy, gui.text_gc, gc_mask, &gc_vals);

    gc_vals.foreground = gui.back_pixel;
    gc_vals.background = gui.norm_pixel;
    if (gui.back_gc != NULL)
	XChangeGC(gui.dpy, gui.back_gc, gc_mask, &gc_vals);

    gc_mask |= GCFunction;
    gc_vals.foreground = gui.norm_pixel ^ gui.back_pixel;
    gc_vals.background = gui.norm_pixel ^ gui.back_pixel;
    gc_vals.function   = GXxor;
    if (gui.invert_gc != NULL)
	XChangeGC(gui.dpy, gui.invert_gc, gc_mask, &gc_vals);

    gui_x11_set_back_color();
}

/*
 * Open the GUI window which was created by a call to gui_mch_init().
 */
    int
gui_mch_open()
{
    /* Actually open the window */
    XtPopup(vimShell, XtGrabNone);

    gui.wid = gui_x11_get_wid();
    gui.blank_pointer = gui_x11_create_blank_mouse();

    /*
     * Add a callback for the Close item on the window managers menu, and the
     * save-yourself event.
     */
    wm_atoms[SAVE_YOURSELF_IDX] =
			      XInternAtom(gui.dpy, "WM_SAVE_YOURSELF", False);
    wm_atoms[DELETE_WINDOW_IDX] =
			      XInternAtom(gui.dpy, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(gui.dpy, XtWindow(vimShell), wm_atoms, 2);
    XtAddEventHandler(vimShell, NoEventMask, True, gui_x11_wm_protocol_handler,
							     NULL);
/* #define ENABLE_EDITRES */
#ifdef ENABLE_EDITRES
    /*
     * Enable editres protocol (see editres(1))
     * Usually will need to add -lXmu to the linker line as well.
     */
    {
	extern void _XEditResCheckMessages();
	XtAddEventHandler(vimShell, 0, True, _XEditResCheckMessages,
		(XtPointer)NULL);
    }
#endif

#if defined(WANT_MENU) && defined(USE_GUI_ATHENA)
    /* The Athena GUI needs this again after opening the window */
    gui_position_menu();
#endif

    /* Get the colors for the highlight groups (gui_check_colors() might have
     * changed them) */
    highlight_gui_started();		/* re-init colors and fonts */

#ifdef HANGUL_INPUT
    hangul_keyboard_set();
#endif
#if defined(USE_FONTSET) && !defined(SLOW_XSERVER)
    if (gui.fontset)
	create_workwindow();
#endif
#ifdef USE_XIM
    xim_init();
#endif

    return OK;
}

/*ARGSUSED*/
    void
gui_mch_exit(rc)
    int		rc;
{
#if 0
    /* Lesstif gives an error message here, and so does Solaris.  The man page
     * says that this isn't needed when exiting, so just skip it. */
    XtCloseDisplay(gui.dpy);
#endif
}

/*
 * Get the position of the top left corner of the window.
 */
    int
gui_mch_get_winpos(x, y)
    int		*x, *y;
{
    Dimension	xpos, ypos;

    XtVaGetValues(vimShell,
	XtNx,	&xpos,
	XtNy,	&ypos,
	NULL);
    *x = xpos;
    *y = ypos;
    return OK;
}

/*
 * Set the position of the top left corner of the window to the given
 * coordinates.
 */
    void
gui_mch_set_winpos(x, y)
    int		x, y;
{
    XtVaSetValues(vimShell,
	XtNx,	x,
	XtNy,	y,
	NULL);
}

    void
gui_mch_set_winsize(width, height, min_width, min_height,
		    base_width, base_height)
    int		width;
    int		height;
    int		min_width;
    int		min_height;
    int		base_width;
    int		base_height;
{
    XtVaSetValues(vimShell,
	XtNwidthInc,	gui.char_width,
	XtNheightInc,	gui.char_height,
#if defined(XtSpecificationRelease) && XtSpecificationRelease >= 4
	XtNbaseWidth,	base_width,
	XtNbaseHeight,	base_height,
#endif
	XtNminWidth,	min_width,
	XtNminHeight,	min_height,
	XtNwidth,	width,
#ifdef USE_XIM
	XtNheight,	height + xim_get_status_area_height(),
#else
	XtNheight,	height,
#endif
	NULL);
}

/*
 * Allow 10 pixels for horizontal borders, 30 for vertical borders.
 * Is there no way in X to find out how wide the borders really are?
 */
    void
gui_mch_get_screen_dimensions(screen_w, screen_h)
    int	    *screen_w;
    int	    *screen_h;
{
    *screen_w = DisplayWidth(gui.dpy, DefaultScreen(gui.dpy)) - 10;
    *screen_h = DisplayHeight(gui.dpy, DefaultScreen(gui.dpy)) - p_ghr;
}

/*
 * Initialise vim to use the font with the given name.	Return FAIL if the font
 * could not be loaded, OK otherwise.
 */
    int
gui_mch_init_font(font_name)
    char_u	*font_name;
{
    XFontStruct	*font = NULL;

#ifdef USE_FONTSET
    XFontSet		fontset = NULL;
    static char_u	*dflt_fontset = NULL;

    if (gui.fontset)
    {
	/* If fontset is active, VIM treat all the font as a fontset */
	if (font_name == NULL)
	    font_name = dflt_fontset;

	fontset = (XFontSet)gui_mch_get_fontset(font_name, FALSE);
	if (fontset == NULL)
	    return FAIL;
    }
    else if (font_name == NULL && *p_guifontset)
    {
	fontset = (XFontSet)gui_mch_get_fontset(p_guifontset, FALSE);
	if (fontset != NULL)
	{
	    font_name = p_guifontset;
	    dflt_fontset = alloc(STRLEN(p_guifontset) + 1);
	    STRCPY(dflt_fontset, p_guifontset);
	}
    }

    if (fontset == NULL)
#endif
    {
	if (font_name == NULL)
	{
	    /*
	     * If none of the fonts in 'font' could be loaded, try the one set
	     * in the X resource, and finally just try using DFLT_FONT, which
	     * will hopefully always be there.
	     */
	    font_name = gui.dflt_font;
	    font = (XFontStruct *)gui_mch_get_font(font_name, FALSE);
	    if (font == NULL)
		font_name = (char_u *)DFLT_FONT;
	}
	if (font == NULL)
	    font = (XFontStruct *)gui_mch_get_font(font_name, FALSE);
	if (font == NULL)
	    return FAIL;
    }

#ifdef USE_FONTSET
    if (fontset != NULL)
    {
	if (gui.norm_font != 0)
	    XFreeFontSet(gui.dpy, (XFontSet)gui.norm_font);
	gui.norm_font = (GuiFont)fontset;
	gui.fontset = (GuiFont)fontset;
	gui.char_width = fontset_width((XFontSet)fontset);
	gui.char_height = fontset_height((XFontSet)fontset);
	gui.char_ascent = fontset_ascent((XFontSet)fontset);
    }
    else
#endif
    {
	if (gui.norm_font != 0)
	    XFreeFont(gui.dpy, (XFontStruct *)gui.norm_font);
	gui.norm_font = (GuiFont)font;
	gui.char_width = font->max_bounds.width;
	gui.char_height = font->ascent + font->descent;
	gui.char_ascent = font->ascent;
    }

    hl_set_font_name(font_name);

    /*
     * Try to load other fonts for bold, italic, and bold-italic.
     * We should also try to work out what font to use for these when they are
     * not specified by X resources, but we don't yet.
     */
    if (gui.bold_font == 0 &&
	    gui.dflt_bold_fn != NULL && *gui.dflt_bold_fn != NUL)
	gui.bold_font = gui_mch_get_font(gui.dflt_bold_fn, FALSE);
    if (gui.ital_font == 0 &&
	    gui.dflt_ital_fn != NULL && *gui.dflt_ital_fn != NUL)
	gui.ital_font = gui_mch_get_font(gui.dflt_ital_fn, FALSE);
    if (gui.boldital_font == 0 &&
	    gui.dflt_boldital_fn != NULL && *gui.dflt_boldital_fn != NUL)
	gui.boldital_font = gui_mch_get_font(gui.dflt_boldital_fn, FALSE);

    /* TODO: if (vimShell != (Widget)0 && XtIsRealized(vimShell)) */

    return OK;
}

/*
 * Get a font structure for highlighting.
 */
    GuiFont
gui_mch_get_font(name, giveErrorIfMissing)
    char_u	*name;
    int		giveErrorIfMissing;
{
    XFontStruct	*font;

    if (!gui.in_use || name == NULL)    /* can't do this when GUI not running */
	return (GuiFont)0;

#ifdef USE_FONTSET
    if (gui.fontset)
	/* If fontset is active, VIM treat all the font as a fontset */
	return gui_mch_get_fontset(name, giveErrorIfMissing);
    else if (vim_strchr(name, ','))
	return (GuiFont)0;
#endif

    font = XLoadQueryFont(gui.dpy, (char *)name);

    if (font == NULL)
    {
	if (giveErrorIfMissing)
	    EMSG2("Unknown font: %s", name);
	return (GuiFont)0;
    }

#ifdef DEBUG
    printf("Font Information for '%s':\n", name);
    printf("  w = %d, h = %d, ascent = %d, descent = %d\n",
	   font->max_bounds.width, font->ascent + font->descent,
	   font->ascent, font->descent);
    printf("  max ascent = %d, max descent = %d, max h = %d\n",
	   font->max_bounds.ascent, font->max_bounds.descent,
	   font->max_bounds.ascent + font->max_bounds.descent);
    printf("  min lbearing = %d, min rbearing = %d\n",
	   font->min_bounds.lbearing, font->min_bounds.rbearing);
    printf("  max lbearing = %d, max rbearing = %d\n",
	   font->max_bounds.lbearing, font->max_bounds.rbearing);
    printf("  leftink = %d, rightink = %d\n",
	   (font->min_bounds.lbearing < 0),
	   (font->max_bounds.rbearing > font->max_bounds.width));
    printf("\n");
#endif

    if (font->max_bounds.width != font->min_bounds.width)
    {
	EMSG2("Font \"%s\" is not fixed-width", name);
	XFreeFont(gui.dpy, font);
	return (GuiFont)0;
    }
    return (GuiFont)font;
}

/*
 * Set the current text font.
 */
    void
gui_mch_set_font(font)
    GuiFont	font;
{
#ifdef USE_FONTSET
    if (gui.fontset)
    {
	if (font)
	    current_fontset = (XFontSet)font;
    }
    else
#endif
    {
	static Font	prev_font = (Font) -1;
	Font		fid = ((XFontStruct *)font)->fid;

	if (fid != prev_font)
	{
	    XSetFont(gui.dpy, gui.text_gc, fid);
	    XSetFont(gui.dpy, gui.back_gc, fid);
	    prev_font = fid;
	}
    }
}

#if 0 /* not used */
/*
 * Return TRUE if the two fonts given are equivalent.
 */
    int
gui_mch_same_font(f1, f2)
    GuiFont	f1;
    GuiFont	f2;
{
#ifdef USE_FONTSET
    if (gui.fontset)
	return f1 == f2;
    else
#endif
    return ((XFontStruct *)f1)->fid == ((XFontStruct *)f2)->fid;
}
#endif

/*
 * If a font is not going to be used, free its structure.
 */
    void
gui_mch_free_font(font)
    GuiFont	font;
{
    if (font)
    {
#ifdef USE_FONTSET
	if (gui.fontset)
	    XFreeFontSet(gui.dpy, (XFontSet)font);
	else
#endif
	    XFreeFont(gui.dpy, (XFontStruct *)font);
    }
}

#ifdef USE_FONTSET

    static GuiFont
gui_mch_get_fontset(name, giveErrorIfMissing)
    char_u	*name;
    int		giveErrorIfMissing;
{
    XFontSet	fontset;
    char	**missing, *def_str;
    int		num_missing;

    if (!gui.in_use || name == NULL)
	return (GuiFont)0;

    fontset = XCreateFontSet(gui.dpy, (char *)name, &missing, &num_missing,
			     &def_str);
    if (num_missing > 0)
    {
	int i;

	if (giveErrorIfMissing)
	{
	    EMSG2("Error: During loading fontset %s", name);
	    EMSG("Font(s) for the following character sets are missing:");
	    for (i = 0; i < num_missing; i++)
		EMSG2("%s", missing[i]);
	}
	XFreeStringList(missing);
    }

    if (fontset == NULL)
    {
	if (giveErrorIfMissing)
	    EMSG2("Unknown fontset: %s", name);
	return (GuiFont)0;
    }

    if (check_fontset_sanity(fontset) == FAIL)
    {
	XFreeFontSet(gui.dpy, fontset);
	return (GuiFont)0;
    }
    return (GuiFont)fontset;
}

#ifndef SLOW_XSERVER
    static void
create_workwindow()
{
    Window	    root;
    unsigned int    dummy;
    unsigned int    depth;

    if (workwid)
	XFreePixmap(gui.dpy, workwid);
    XGetGeometry(gui.dpy, gui.wid, &root, (int *)&dummy, (int *)&dummy, &dummy,
						      &dummy, &dummy, &depth);
    workwid = XCreatePixmap(gui.dpy, gui.wid,
			    (unsigned int)(Columns * gui.char_width),
			    gui.char_height, depth);
}
#endif

    static int
check_fontset_sanity(fs)
    XFontSet fs;
{
    XFontStruct	**xfs;
    char	**font_name;
    int		fn;
    char	*base_name;
    int		i;
    int		min_width;
    int		min_font_idx = 0;

    base_name = XBaseFontNameListOfFontSet(fs);
    fn = XFontsOfFontSet(fs, &xfs, &font_name);
#if 0
    if (fn < 2)
    {
	char *locale = XLocaleOfFontSet(fs);

	if (!locale || locale[0] == 'C')
	{
	    EMSG("locale is not set correctly");
	    MSG("Set LANG environment variable to your locale");
	    MSG("For korean:");
	    MSG("   csh: setenv LANG ko");
	    MSG("   sh : export LANG=ko");
	}
	EMSG2("fontset name: %s", base_name);
	EMSG("Your language Font missing");
	EMSG2("loaded fontname: %s", font_name[0]);
	return FAIL;
    }
#endif
    for (i = 0; i < fn; i++)
    {
	if (xfs[i]->max_bounds.width != xfs[i]->min_bounds.width)
	{
	    EMSG2("Fontset name: %s", base_name);
	    EMSG2("Font '%s' is not fixed-width", font_name[i]);
	    return FAIL;
	}
    }
    /* scan base font width */
    min_width = 32767;
    for (i = 0; i < fn; i++)
    {
	if (xfs[i]->max_bounds.width<min_width)
	{
	    min_width = xfs[i]->max_bounds.width;
	    min_font_idx = i;
	}
    }
    for (i = 0; i < fn; i++)
    {
	if (	   xfs[i]->max_bounds.width != 2 * min_width
		&& xfs[i]->max_bounds.width != min_width)
	{
	    EMSG2("Fontset name: %s\n", base_name);
	    EMSG2("Font0: %s\n", font_name[min_font_idx]);
	    EMSG2("Font1: %s\n", font_name[i]);
	    EMSGN("Font%d width is not twice that of font0\n", i);
	    EMSGN("Font0 width: %ld\n", xfs[min_font_idx]->max_bounds.width);
	    EMSGN("Font1 width: %ld\n\n", xfs[i]->max_bounds.width);
	    return FAIL;
	}
    }
    /* it seems ok. Good Luck!! */
    return OK;
}

    static int
fontset_width(fs)
    XFontSet fs;
{
    return XmbTextEscapement(fs, "Vim", 3) / 3;
}

    static int
fontset_height(fs)
    XFontSet fs;
{
    XFontSetExtents *extents;

    extents = XExtentsOfFontSet(fs);
    return extents->max_logical_extent.height;
}

/* NOT USED YET
    static int
fontset_descent(fs)
    XFontSet fs;
{
    XFontSetExtents *extents;

    extents = XExtentsOfFontSet (fs);
    return extents->max_logical_extent.height + extents->max_logical_extent.y;
}
*/

    static int
fontset_ascent(fs)
    XFontSet fs;
{
    XFontSetExtents *extents;

    extents = XExtentsOfFontSet(fs);
    return -extents->max_logical_extent.y;
}

#endif /* USE_FONTSET */

/*
 * Return the Pixel value (color) for the given color name.  This routine was
 * pretty much taken from example code in the Silicon Graphics OSF/Motif
 * Programmer's Guide.
 * Return -1 for error.
 */
    GuiColor
gui_mch_get_color(name)
    char_u *name;
{
    XrmValue	from, to;
    int		i;
    static char *(vimnames[][2]) =
    {
	/* A number of colors that some X11 systems don't have */
	{"LightRed",	"#FFA0A0"},
	{"LightGreen",	"#80FF80"},
	{"LightMagenta","#FFA0FF"},
	{"DarkCyan",	"#008080"},
	{"DarkBlue",	"#0000c0"},
	{"DarkRed",	"#c00000"},
	{"DarkMagenta",	"#c000c0"},
	{"DarkGrey",	"#c0c0c0"},
	{NULL, NULL}
    };

    if (!gui.in_use)		    /* can't do this when GUI not running */
	return (GuiColor)-1;

    while (name != NULL)
    {
	from.size = STRLEN(name) + 1;
	if (from.size < sizeof(String))
	    from.size = sizeof(String);
	from.addr = (char *)name;
	to.addr = NULL;
	/* How do we get rid of the missing color warning? */
	XtConvert(vimShell, XtRString, &from, XtRPixel, &to);
	if (to.addr != NULL)
	    return (GuiColor)*((Pixel *)to.addr);

	/* add a few builtin names */
	for (i = 0; ; ++i)
	{
	    if (vimnames[i][0] == NULL)
	    {
		name = NULL;
		break;
	    }
	    if (STRICMP(name, vimnames[i][0]) == 0)
	    {
		name = (char_u *)vimnames[i][1];
		break;
	    }
	}
    }

    return (GuiColor)-1;
}

    void
gui_mch_set_fg_color(color)
    GuiColor	color;
{
    if (color != prev_fg_color)
    {
	XSetForeground(gui.dpy, gui.text_gc, (Pixel)color);
	prev_fg_color = color;
    }
}

/*
 * Set the current text background color.
 */
    void
gui_mch_set_bg_color(color)
    GuiColor	color;
{
    if (color != prev_bg_color)
    {
	XSetBackground(gui.dpy, gui.text_gc, (Pixel)color);
	prev_bg_color = color;
    }
}

/*
 * create a mouse pointer that is blank
 */
    static Cursor
gui_x11_create_blank_mouse()
{
    Pixmap blank_pixmap = XCreatePixmap(gui.dpy, gui.wid, 1, 1, 1);
    GC gc = XCreateGC(gui.dpy, blank_pixmap, (unsigned long)0, (XGCValues*)0);
    XDrawPoint(gui.dpy, blank_pixmap, gc, 0, 0);
    XFreeGC(gui.dpy, gc);
    return XCreatePixmapCursor(gui.dpy, blank_pixmap, blank_pixmap,
	    (XColor*)&gui.norm_pixel, (XColor*)&gui.norm_pixel, 0, 0);
}

/*
 * Use the blank mouse pointer or not.
 */
    void
gui_mch_mousehide(hide)
    int		hide;	/* TRUE = use blank ptr, FALSE = use parent ptr */
{
    if (gui.pointer_hidden != hide)
    {
	if (hide)
	    XDefineCursor(gui.dpy, gui.wid, gui.blank_pointer);
	else
	    XUndefineCursor(gui.dpy, gui.wid);
	gui.pointer_hidden = hide;
    }
}

    void
gui_mch_draw_string(row, col, s, len, flags)
    int	    row;
    int	    col;
    char_u  *s;
    int	    len;
    int	    flags;
{
#if defined(USE_FONTSET) && defined(MULTI_BYTE)
    if (gui.fontset)
    {
	if (col > 0 && mb_isbyte1(LinePointers[row], col - 1))
	{
	    col++;
	    len--;
	    s++;
	}
	if (len == 1 && IsLeadByte(*s))
	    len = 2;
    }
#endif
    if (flags & DRAW_TRANSP)
	XDrawString(gui.dpy, gui.wid, gui.text_gc, TEXT_X(col),
	    TEXT_Y(row), (char *)s, len);
#ifdef USE_FONTSET
    else if (gui.fontset)
    {
# ifdef SLOW_XSERVER
	/* XmbDrawImageString has bug. Don't use it */
	XSetForeground(gui.dpy, gui.text_gc, prev_bg_color);
	XFillRectangle(gui.dpy, gui.wid, gui.text_gc, FILL_X(col),
		       FILL_Y(row), gui.char_width * len, gui.char_height);
	XSetForeground(gui.dpy, gui.text_gc, prev_fg_color);
	XDrawString(gui.dpy, gui.wid, gui.text_gc, TEXT_X(col),
		    TEXT_Y(row), (char *)s, len);
# else
	XSetForeground(gui.dpy, gui.text_gc, prev_bg_color);
	XFillRectangle(gui.dpy, workwid, gui.text_gc, 0, 0,
		       gui.char_width * len, gui.char_height);
	XSetForeground (gui.dpy, gui.text_gc, prev_fg_color);
	XDrawString(gui.dpy, workwid, gui.text_gc, 0, gui.char_ascent,
		    (char *)s, len);
	XCopyArea(gui.dpy, workwid, gui.wid, gui.text_gc, 0, 0,
		   gui.char_width * len, gui.char_height,
		   FILL_X(col), FILL_Y(row));
# endif
    }
#endif
    else
	XDrawImageString(gui.dpy, gui.wid, gui.text_gc, TEXT_X(col),
	    TEXT_Y(row), (char *)s, len);

    if (flags & DRAW_BOLD)
	XDrawString(gui.dpy, gui.wid, gui.text_gc, TEXT_X(col) + 1,
		TEXT_Y(row), (char *)s, len);

    if (flags & DRAW_UNDERL)
	XDrawLine(gui.dpy, gui.wid, gui.text_gc, FILL_X(col),
	     FILL_Y(row + 1) - 1, FILL_X(col + len) - 1, FILL_Y(row + 1) - 1);
}

/*
 * Return OK if the key with the termcap name "name" is supported.
 */
    int
gui_mch_haskey(name)
    char_u  *name;
{
    int i;

    for (i = 0; special_keys[i].key_sym != (KeySym)0; i++)
	if (name[0] == special_keys[i].vim_code0 &&
					 name[1] == special_keys[i].vim_code1)
	    return OK;
    return FAIL;
}

/*
 * Return the text window-id and display.  Only required for X-based GUI's
 */
    int
gui_get_x11_windis(win, dis)
    Window  *win;
    Display **dis;
{
    *win = XtWindow(vimShell);
    *dis = gui.dpy;
    return OK;
}

    void
gui_mch_beep()
{
    XBell(gui.dpy, 0);
}

    void
gui_mch_flash(msec)
    int		msec;
{
    /* Do a visual beep by reversing the foreground and background colors */
    XFillRectangle(gui.dpy, gui.wid, gui.invert_gc, 0, 0,
	    FILL_X((int)Columns) + gui.border_offset,
	    FILL_Y((int)Rows) + gui.border_offset);
    XSync(gui.dpy, False);
    ui_delay((long)msec, TRUE);	/* wait for a few msec */
    XFillRectangle(gui.dpy, gui.wid, gui.invert_gc, 0, 0,
	    FILL_X((int)Columns) + gui.border_offset,
	    FILL_Y((int)Rows) + gui.border_offset);
}

/*
 * Invert a rectangle from row r, column c, for nr rows and nc columns.
 */
    void
gui_mch_invert_rectangle(r, c, nr, nc)
    int	    r;
    int	    c;
    int	    nr;
    int	    nc;
{
    XFillRectangle(gui.dpy, gui.wid, gui.invert_gc,
	FILL_X(c), FILL_Y(r), (nc) * gui.char_width, (nr) * gui.char_height);
}

/*
 * Iconify the GUI window.
 */
    void
gui_mch_iconify()
{
    XIconifyWindow(gui.dpy, XtWindow(vimShell), DefaultScreen(gui.dpy));
}

/*
 * Draw a cursor without focus.
 */
    void
gui_mch_draw_hollow_cursor(color)
    GuiColor color;
{
#if defined(USE_FONTSET) && defined(MULTI_BYTE)
    if (gui.fontset)
    {
	if (IsLeadByte(LinePointers[gui.row][gui.col])
#ifdef HANGUL_INPUT
		|| composing_hangul
#endif
	   )
	    XDrawRectangle(gui.dpy, gui.wid, gui.text_gc, FILL_X(gui.col),
		       FILL_Y(gui.row), 2*gui.char_width - 1, gui.char_height - 1);
	else
	    XDrawRectangle(gui.dpy, gui.wid, gui.text_gc, FILL_X(gui.col),
		       FILL_Y(gui.row), gui.char_width - 1, gui.char_height - 1);
    }
    else
#endif
    {
	gui_mch_set_fg_color(color);
	XDrawRectangle(gui.dpy, gui.wid, gui.text_gc, FILL_X(gui.col),
		       FILL_Y(gui.row), gui.char_width - 1, gui.char_height - 1);
    }
}

/*
 * Draw part of a cursor, "w" pixels wide, and "h" pixels high, using
 * color "color".
 */
    void
gui_mch_draw_part_cursor(w, h, color)
    int		w;
    int		h;
    GuiColor	color;
{
    gui_mch_set_fg_color(color);

    XFillRectangle(gui.dpy, gui.wid, gui.text_gc,
#ifdef RIGHTLEFT
	    /* vertical line should be on the right of current point */
	    State != CMDLINE && curwin->w_p_rl ? FILL_X(gui.col + 1) - w :
#endif
		FILL_X(gui.col),
	    FILL_Y(gui.row) + gui.char_height - h, w, h);
}

/*
 * Catch up with any queued X events.  This may put keyboard input into the
 * input buffer, call resize call-backs, trigger timers etc.  If there is
 * nothing in the X event queue (& no timers pending), then we return
 * immediately.
 */
    void
gui_mch_update()
{
    while (XtAppPending(app_context) && !vim_is_input_buf_full())
	XtAppProcessEvent(app_context, (XtInputMask)XtIMAll);
}

/*
 * GUI input routine called by gui_wait_for_chars().  Waits for a character
 * from the keyboard.
 *  wtime == -1	    Wait forever.
 *  wtime == 0	    This should never happen.
 *  wtime > 0	    Wait wtime milliseconds for a character.
 * Returns OK if a character was found to be available within the given time,
 * or FAIL otherwise.
 */
    int
gui_mch_wait_for_chars(wtime)
    long    wtime;
{
    int		    focus;

    /*
     * Make this static, in case gui_x11_timer_cb is called after leaving
     * this function (otherwise a random value on the stack may be changed).
     */
    static int	    timed_out;
    XtIntervalId    timer = (XtIntervalId)0;
#ifdef USE_SNIFF
    static int	    sniff_on = 0;
    static XtInputId sniff_input_id = 0;
#endif

    timed_out = FALSE;

#ifdef USE_SNIFF
    if (sniff_on && !want_sniff_request)
    {
	if (sniff_input_id)
	    XtRemoveInput(sniff_input_id);
	sniff_on = 0;
    }
    else if (!sniff_on && want_sniff_request)
    {
	sniff_input_id = XtAppAddInput(app_context, fd_from_sniff,
		     (XtPointer)XtInputReadMask, gui_x11_sniff_request_cb, 0);
	sniff_on = 1;
    }
#endif

    if (wtime > 0)
	timer = XtAppAddTimeOut(app_context, (long_u)wtime, gui_x11_timer_cb,
								  &timed_out);

    focus = gui.in_focus;
    while (!timed_out)
    {
	/* Stop or start blinking when focus changes */
	if (gui.in_focus != focus)
	{
	    if (gui.in_focus)
		gui_mch_start_blink();
	    else
		gui_mch_stop_blink();
	    focus = gui.in_focus;
	}

	/*
	 * Don't use gui_mch_update() because then we will spin-lock until a
	 * char arrives, instead we use XtAppProcessEvent() to hang until an
	 * event arrives.  No need to check for input_buf_full because we are
	 * returning as soon as it contains a single char.  Note that
	 * XtAppNextEvent() may not be used because it will not return after a
	 * timer event has arrived -- webb
	 */
	XtAppProcessEvent(app_context, (XtInputMask)XtIMAll);

	if (!vim_is_input_buf_empty())
	{
	    if (timer != (XtIntervalId)0 && !timed_out)
		XtRemoveTimeOut(timer);
	    return OK;
	}
    }
    return FAIL;
}

/*
 * Output routines.
 */

/* Flush any output to the screen */
    void
gui_mch_flush()
{
    XFlush(gui.dpy);
}

/*
 * Clear a rectangular region of the screen from text pos (row1, col1) to
 * (row2, col2) inclusive.
 */
    void
gui_mch_clear_block(row1, col1, row2, col2)
    int	    row1;
    int	    col1;
    int	    row2;
    int	    col2;
{
    /*
     * Clear one extra pixel at the right, for when bold characters have
     * spilled over to the next column.
     * Can this ever erase part of the next character? - webb
     */
    XFillRectangle(gui.dpy, gui.wid, gui.back_gc, FILL_X(col1),
	FILL_Y(row1), (col2 - col1 + 1) * gui.char_width + 1,
	(row2 - row1 + 1) * gui.char_height);
}

    void
gui_mch_clear_all()
{
    XClearArea(gui.dpy, gui.wid, 0, 0, 0, 0, False);
}

/*
 * Delete the given number of lines from the given row, scrolling up any
 * text further down within the scroll region.
 */
    void
gui_mch_delete_lines(row, num_lines)
    int	    row;
    int	    num_lines;
{
    if (gui.visibility == VisibilityFullyObscured)
	return;	    /* Can't see the window */

    if (num_lines <= 0)
	return;

    if (row + num_lines > gui.scroll_region_bot)
    {
	/* Scrolled out of region, just blank the lines out */
	gui_clear_block(row, 0, gui.scroll_region_bot, (int)Columns - 1);
    }
    else
    {
	/* copy one extra pixel, for when bold has spilled over */
	XCopyArea(gui.dpy, gui.wid, gui.wid, gui.text_gc,
	    FILL_X(0), FILL_Y(row + num_lines),
	    gui.char_width * (int)Columns + 1,
	    gui.char_height * (gui.scroll_region_bot - row - num_lines + 1),
	    FILL_X(0), FILL_Y(row));

	/* Update gui.cursor_row if the cursor scrolled or copied over */
	if (gui.cursor_row >= row)
	{
	    if (gui.cursor_row < row + num_lines)
		gui.cursor_is_valid = FALSE;
	    else if (gui.cursor_row <= gui.scroll_region_bot)
		gui.cursor_row -= num_lines;
	}

	gui_clear_block(gui.scroll_region_bot - num_lines + 1, 0,
	    gui.scroll_region_bot, (int)Columns - 1);
	gui_x11_check_copy_area();
    }
}

/*
 * Insert the given number of lines before the given row, scrolling down any
 * following text within the scroll region.
 */
    void
gui_mch_insert_lines(row, num_lines)
    int	    row;
    int	    num_lines;
{
    if (gui.visibility == VisibilityFullyObscured)
	return;	    /* Can't see the window */

    if (num_lines <= 0)
	return;

    if (row + num_lines > gui.scroll_region_bot)
    {
	/* Scrolled out of region, just blank the lines out */
	gui_clear_block(row, 0, gui.scroll_region_bot, (int)Columns - 1);
    }
    else
    {
	/* copy one extra pixel, for when bold has spilled over */
	XCopyArea(gui.dpy, gui.wid, gui.wid, gui.text_gc,
	    FILL_X(0), FILL_Y(row),
	    gui.char_width * (int)Columns + 1,
	    gui.char_height * (gui.scroll_region_bot - row - num_lines + 1),
	    FILL_X(0), FILL_Y(row + num_lines));

	/* Update gui.cursor_row if the cursor scrolled or copied over */
	if (gui.cursor_row >= gui.row)
	{
	    if (gui.cursor_row <= gui.scroll_region_bot - num_lines)
		gui.cursor_row += num_lines;
	    else if (gui.cursor_row <= gui.scroll_region_bot)
		gui.cursor_is_valid = FALSE;
	}

	gui_clear_block(row, 0, row + num_lines - 1, (int)Columns - 1);
	gui_x11_check_copy_area();
    }
}

/*
 * Scroll the text between gui.scroll_region_top & gui.scroll_region_bot by the
 * number of lines given.  Positive scrolls down (text goes up) and negative
 * scrolls up (text goes down).
 */
    static void
gui_x11_check_copy_area()
{
    XEvent		    event;
    XGraphicsExposeEvent    *gevent;

    if (gui.visibility != VisibilityPartiallyObscured)
	return;

    XFlush(gui.dpy);

    /* Wait to check whether the scroll worked or not */
    for (;;)
    {
	if (XCheckTypedEvent(gui.dpy, NoExpose, &event))
	    return;	/* The scroll worked. */

	if (XCheckTypedEvent(gui.dpy, GraphicsExpose, &event))
	{
	    gevent = (XGraphicsExposeEvent *)&event;
	    gui_redraw(gevent->x, gevent->y, gevent->width, gevent->height);
	    if (gevent->count == 0)
		return;		/* This was the last expose event */
	}
	XSync(gui.dpy, False);
    }
}

/*
 * X Selection stuff, for cutting and pasting text to other windows.
 */

    void
clip_mch_lose_selection()
{
    clip_x11_lose_selection(vimShell);
}

    int
clip_mch_own_selection()
{
    return clip_x11_own_selection(vimShell);
}

    void
clip_mch_request_selection()
{
    clip_x11_request_selection(vimShell, gui.dpy);
}

    void
clip_mch_set_selection()
{
    clip_x11_set_selection();
}

#if defined(WANT_MENU) || defined(PROTO)
/*
 * Menu stuff.
 */

/*
 * Make a menu either grey or not grey.
 */
    void
gui_mch_menu_grey(menu, grey)
    VimMenu *menu;
    int	    grey;
{
    if (menu->id != (Widget)0)
    {
	gui_mch_menu_hidden(menu, False);
	if (grey)
	    XtSetSensitive(menu->id, False);
	else
	    XtSetSensitive(menu->id, True);
    }
}

/*
 * Make menu item hidden or not hidden
 */
    void
gui_mch_menu_hidden(menu, hidden)
    VimMenu *menu;
    int	    hidden;
{
    if (menu->id != (Widget)0)
    {
	if (hidden)
	    XtUnmanageChild(menu->id);
	else
	    XtManageChild(menu->id);
    }
}

/*
 * This is called after setting all the menus to grey/hidden or not.
 */
    void
gui_mch_draw_menubar()
{
    /* Nothing to do in X */
}

/* ARGSUSED */
    void
gui_x11_menu_cb(w, client_data, call_data)
    Widget	w;
    XtPointer	client_data, call_data;
{
    gui_menu_cb((VimMenu *)client_data);
}

#endif /* WANT_MENU */

/*
 * Scrollbar stuff.
 */

    void
gui_mch_enable_scrollbar(sb, flag)
    GuiScrollbar    *sb;
    int		    flag;
{
    if (sb->id != (Widget)0)
    {
	if (flag)
	    XtManageChild(sb->id);
	else
	    XtUnmanageChild(sb->id);
    }
}


/*
 * Function called when window closed.	Works like ":qa".
 * Should put up a requester!
 */
/*ARGSUSED*/
    static void
gui_x11_wm_protocol_handler(w, client_data, event, dum)
    Widget	w;
    XtPointer	client_data;
    XEvent	*event;
    Boolean	*dum;
{
    /*
     * Only deal with Client messages.
     */
    if (event->type != ClientMessage)
	return;

    /*
     * The WM_SAVE_YOURSELF event arrives when the window manager wants to
     * exit.  That can be cancelled though, thus Vim shouldn't exit here.
     * Just sync our swap files.
     */
    if (((XClientMessageEvent *)event)->data.l[0] ==
						  wm_atoms[SAVE_YOURSELF_IDX])
    {
	out_flush();
	ml_sync_all(FALSE, FALSE);	/* preserve all swap files */

	/* Set the window's WM_COMMAND property, to let the window manager
	 * know we are done saving ourselves.  We don't want to be restarted,
	 * thus set argv to NULL. */
	XSetCommand(gui.dpy, XtWindow(vimShell), NULL, 0);
	return;
    }

    if (((XClientMessageEvent *)event)->data.l[0] !=
						  wm_atoms[DELETE_WINDOW_IDX])
	return;

    gui_window_closed();
}

/*
 * Cursor blink functions.
 *
 * This is a simple state machine:
 * BLINK_NONE	not blinking at all
 * BLINK_OFF	blinking, cursor is not shown
 * BLINK_ON	blinking, cursor is shown
 */

#define BLINK_NONE  0
#define BLINK_OFF   1
#define BLINK_ON    2

static int		blink_state = BLINK_NONE;
static long_u		blink_waittime = 700;
static long_u		blink_ontime = 400;
static long_u		blink_offtime = 250;
static XtIntervalId	blink_timer = (XtIntervalId)0;

    void
gui_mch_set_blinking(waittime, on, off)
    long    waittime, on, off;
{
    blink_waittime = waittime;
    blink_ontime = on;
    blink_offtime = off;
}

/*
 * Stop the cursor blinking.  Show the cursor if it wasn't shown.
 */
    void
gui_mch_stop_blink()
{
    if (blink_timer != (XtIntervalId)0)
    {
	XtRemoveTimeOut(blink_timer);
	blink_timer = (XtIntervalId)0;
    }
    if (blink_state == BLINK_OFF)
	gui_update_cursor(TRUE, FALSE);
    blink_state = BLINK_NONE;
}

/*
 * Start the cursor blinking.  If it was already blinking, this restarts the
 * waiting time and shows the cursor.
 */
    void
gui_mch_start_blink()
{
    if (blink_timer != (XtIntervalId)0)
	XtRemoveTimeOut(blink_timer);
    /* Only switch blinking on if none of the times is zero */
    if (blink_waittime && blink_ontime && blink_offtime && gui.in_focus)
    {
	blink_timer = XtAppAddTimeOut(app_context, blink_waittime,
						      gui_x11_blink_cb, NULL);
	blink_state = BLINK_ON;
	gui_update_cursor(TRUE, FALSE);
    }
}

/* ARGSUSED */
    static void
gui_x11_blink_cb(timed_out, interval_id)
    XtPointer	    timed_out;
    XtIntervalId    *interval_id;
{
    if (blink_state == BLINK_ON)
    {
	gui_undraw_cursor();
	blink_state = BLINK_OFF;
	blink_timer = XtAppAddTimeOut(app_context, blink_offtime,
						      gui_x11_blink_cb, NULL);
    }
    else
    {
	gui_update_cursor(TRUE, FALSE);
	blink_state = BLINK_ON;
	blink_timer = XtAppAddTimeOut(app_context, blink_ontime,
						      gui_x11_blink_cb, NULL);
    }
}

/*
 * Return the lightness of a pixel.  White is 255.
 */
    int
gui_mch_get_lightness(pixel)
    GuiColor	pixel;
{
    XColor	xc;
    Colormap	colormap;

    colormap = DefaultColormap(gui.dpy, XDefaultScreen(gui.dpy));

    xc.pixel = pixel;
    XQueryColor(gui.dpy, colormap, &xc);

    return (int)(xc.red * 3 + xc.green * 6 + xc.blue) / (10 * 256);
}

#if (defined(SYNTAX_HL) && defined(WANT_EVAL)) || defined(PROTO)
/*
 * Return the RGB value of a pixel as "#RRGGBB".
 */
    char_u *
gui_mch_get_rgb(pixel)
    GuiColor	pixel;
{
    XColor	xc;
    Colormap	colormap;
    static char_u retval[10];

    colormap = DefaultColormap(gui.dpy, XDefaultScreen(gui.dpy));

    xc.pixel = pixel;
    XQueryColor(gui.dpy, colormap, &xc);

    sprintf((char *)retval, "#%02x%02x%02x",
	    (unsigned)xc.red >> 8,
	    (unsigned)xc.green >> 8,
	    (unsigned)xc.blue >> 8);
    return retval;
}
#endif

/*
 * Add the callback functions.
 */
    void
gui_x11_callbacks(textArea, vimForm)
    Widget textArea;
    Widget vimForm;
{
    XtAddEventHandler(textArea, VisibilityChangeMask, FALSE,
	gui_x11_visibility_cb, (XtPointer)0);

    XtAddEventHandler(textArea, ExposureMask, FALSE, gui_x11_expose_cb,
	(XtPointer)0);

    XtAddEventHandler(vimForm, StructureNotifyMask, FALSE,
	gui_x11_resize_window_cb, (XtPointer)0);

    XtAddEventHandler(vimShell, FocusChangeMask, FALSE, gui_x11_focus_change_cb,
	(XtPointer)0);
    /*
     * Only install these enter/leave callbacks when 'p' in 'guioptions'.
     * Only needed for some window managers.
     */
    if (vim_strchr(p_go, GO_POINTER) != NULL)
    {
	XtAddEventHandler(vimShell, LeaveWindowMask, FALSE, gui_x11_leave_cb,
	    (XtPointer)0);
	XtAddEventHandler(textArea, LeaveWindowMask, FALSE, gui_x11_leave_cb,
	    (XtPointer)0);
	XtAddEventHandler(textArea, EnterWindowMask, FALSE, gui_x11_enter_cb,
	    (XtPointer)0);
	XtAddEventHandler(vimShell, EnterWindowMask, FALSE, gui_x11_enter_cb,
	    (XtPointer)0);
    }

    XtAddEventHandler(vimForm, KeyPressMask, FALSE, gui_x11_key_hit_cb,
	(XtPointer)0);
    XtAddEventHandler(textArea, KeyPressMask, FALSE, gui_x11_key_hit_cb,
	(XtPointer)0);

    /* get pointer moved events from scrollbar, needed for 'mousefocus' */
    XtAddEventHandler(vimForm, PointerMotionMask,
	FALSE, gui_x11_mouse_cb, (XtPointer)1);
    XtAddEventHandler(textArea, ButtonPressMask | ButtonReleaseMask |
					 ButtonMotionMask | PointerMotionMask,
	FALSE, gui_x11_mouse_cb, (XtPointer)0);
}

/*
 * Get current y mouse coordinate in text window.
 * Return -1 when unknown.
 */
    int
gui_mch_get_mouse_x()
{
    int		rootx, rooty, winx, winy;
    Window	root, child;
    unsigned int mask;

    if (XQueryPointer(gui.dpy, XtWindow(vimShell), &root, &child,
		&rootx, &rooty, &winx, &winy, &mask))
    {
	if (gui.which_scrollbars[SBAR_LEFT])
	    return winx - gui.scrollbar_width;
	return winx;
    }
    return -1;
}

    int
gui_mch_get_mouse_y()
{
    int		rootx, rooty, winx, winy;
    Window	root, child;
    unsigned int mask;

    if (XQueryPointer(gui.dpy, XtWindow(vimShell), &root, &child,
		&rootx, &rooty, &winx, &winy, &mask))
	return winy
#ifdef WANT_MENU
	    - gui.menu_height
#endif
	    ;
    return -1;
}

    void
gui_mch_setmouse(x, y)
    int		x;
    int		y;
{
    if (gui.which_scrollbars[SBAR_LEFT])
	x += gui.scrollbar_width;
    XWarpPointer(gui.dpy, (Window)0, XtWindow(vimShell), 0, 0, 0, 0,
						      x, y
#ifdef WANT_MENU
						      + gui.menu_height
#endif
						      );
}

#if defined(USE_GUI_MOTIF) || defined(PROTO)
    XButtonPressedEvent *
gui_x11_get_last_mouse_event()
{
    return &last_mouse_event;
}
#endif
