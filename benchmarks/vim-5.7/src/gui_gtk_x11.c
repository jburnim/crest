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

#include "vim.h"

#include <gdk/gdkkeysyms.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>

#include <gtk/gtk.h>

#include "gui_gtk_f.h"

#define VIM_NAME	"vim"
#define VIM_CLASS	"Vim"

/* slection distinguishers */
enum {
    SELECTION_TYPE_NONE,
    SELECTION_STRING,
    SELECTION_TEXT,
    SELECTION_COMPOUND_TEXT,
    SELECTION_CLIPBOARD
};

/*
 * Enable DND feature.  Disable this if it causes problems.
 */
#define GTK_DND

#ifdef GTK_DND
/* DND specification constants. */
enum {
  TARGET_STRING
};

static GtkTargetEntry target_table[] = {
    { "text/uri-list", 0, TARGET_STRING },
    { "STRING",     0, TARGET_STRING },
    { "text/plain", 0, TARGET_STRING }
};
static guint n_targets = sizeof(target_table) / sizeof(target_table[0]);
#endif

/* This is the single only fixed width font in X11, which seems to be present
 * on all servers and available in all the variants we need.
 *
 * Don't try to tell me that X11 is a wonderfull technology.  If You need to
 * look for a nice GUI design look for the PalmOS instead!
 */

#define DFLT_FONT		"-adobe-courier-medium-r-normal-*-14-*-*-*-m-*-*-*"

/*
 * Atom used to communicate save yourself from the X11 session manager. There
 * is no need to move this into the GUI struct, since this should be always
 * constant.
 */
static GdkAtom save_yourself_atom = GDK_NONE;

/*
 * Atom used to recognize requests for on the fly GTK+ style configuration
 * changes.
 */
static GdkAtom reread_rcfiles_atom = GDK_NONE;

/*
 * Keycodes recognized by vim.
 */
static struct special_key {
    guint key_sym;
    char_u code0;
    char_u code1;
} special_keys[] =
{
    { GDK_Up,		'k', 'u' },
    { GDK_Down,		'k', 'd' },
    { GDK_Left,		'k', 'l' },
    { GDK_Right,	'k', 'r' },
    { GDK_F1,		'k', '1' },
    { GDK_F2,		'k', '2' },
    { GDK_F3,		'k', '3' },
    { GDK_F4,		'k', '4' },
    { GDK_F5,		'k', '5' },
    { GDK_F6,		'k', '6' },
    { GDK_F7,		'k', '7' },
    { GDK_F8,		'k', '8' },
    { GDK_F9,		'k', '9' },
    { GDK_F10,		'k', ';' },
    { GDK_F11,		'F', '1' },
    { GDK_F12,		'F', '2' },
    { GDK_F13,		'F', '3' },
    { GDK_F14,		'F', '4' },
    { GDK_F15,		'F', '5' },
    { GDK_F16,		'F', '6' },
    { GDK_F17,		'F', '7' },
    { GDK_F18,		'F', '8' },
    { GDK_F19,		'F', '9' },
    { GDK_F20,		'F', 'A' },
    { GDK_F21,		'F', 'B' },
    { GDK_F22,		'F', 'C' },
    { GDK_F23,		'F', 'D' },
    { GDK_F24,		'F', 'E' },
    { GDK_F25,		'F', 'F' },
    { GDK_F26,		'F', 'G' },
    { GDK_F27,		'F', 'H' },
    { GDK_F28,		'F', 'I' },
    { GDK_F29,		'F', 'J' },
    { GDK_F30,		'F', 'K' },
    { GDK_F31,		'F', 'L' },
    { GDK_F32,		'F', 'M' },
    { GDK_F33,		'F', 'N' },
    { GDK_F34,		'F', 'O' },
    { GDK_F35,		'F', 'P' },
    { GDK_Help,		'%', '1' },
    { GDK_Undo,		'&', '8' },
    { GDK_BackSpace,	'k', 'b' },
    { GDK_Insert,	'k', 'I' },
    { GDK_Delete,	'k', 'D' },
    { GDK_Home,		'k', 'h' },
    { GDK_End,		'@', '7' },
    { GDK_Prior,	'k', 'P' },
    { GDK_Next,		'k', 'N' },
    { GDK_Print,	'%', '9' },
    /* Keypad keys: */
    { GDK_KP_Left,	'k', 'l' },
    { GDK_KP_Right,	'k', 'r' },
    { GDK_KP_Up,	'k', 'u' },
    { GDK_KP_Down,	'k', 'd' },
    { GDK_KP_Insert,	'k', 'I' },
    { GDK_KP_Delete,	'k', 'D' },
    { GDK_KP_Home,	'k', 'h' },
    { GDK_KP_End,	'@', '7' },
    { GDK_KP_Prior,	'k', 'P' },
    { GDK_KP_Next,	'k', 'N' },
    /* End of list marker: */
    { 0, 0, 0 }
};

/*
 * This table holds all the X GUI command line options allowed.  This includes
 * the standard ones so that we can skip them when vim is started without the
 * GUI (but the GUI might start up later).
 * When changing this, also update doc/gui_x11.txt and the usage message!!!
 */
static XrmOptionDescRec cmdline_options[] =
{
    /* We handle these options ourselfs */
    {"-fn",		".font",	    XrmoptionSepArg,	NULL},
    {"-font",		".font",	    XrmoptionSepArg,	NULL},
    {"-geom",		".geometry",	    XrmoptionSepArg,	NULL},
    {"-geometry",	".geometry",	    XrmoptionSepArg,	NULL},
    {"-reverse",	"*reverseVideo",    XrmoptionNoArg,	"True"},
    {"-rv",		"*reverseVideo",    XrmoptionNoArg,	"True"},
#if 0	/* TBD */
    {"-bg",		".background",	    XrmoptionSepArg,	NULL},
    {"-background",	".background",	    XrmoptionSepArg,	NULL},
    {"-fg",		".foreground",	    XrmoptionSepArg,	NULL},
    {"-foreground",	".foreground",	    XrmoptionSepArg,	NULL},
    {"-boldfont",	".boldFont",	    XrmoptionSepArg,	NULL},
    {"-italicfont",	".italicFont",	    XrmoptionSepArg,	NULL},
    {"+reverse",	"*reverseVideo",    XrmoptionNoArg,	"False"},
    {"+rv",		"*reverseVideo",    XrmoptionNoArg,	"False"},
    {"-iconic",		"*iconic",	    XrmoptionNoArg,	"True"},
    {"-name",		".name",	    XrmoptionSepArg,	NULL},
    {"-bw",		".borderWidth",	    XrmoptionSepArg,	NULL},
    {"-borderwidth",	".borderWidth",	    XrmoptionSepArg,	NULL},
    {"-sw",             ".scrollbarWidth",  XrmoptionSepArg,    NULL},
    {"-scrollbarwidth", ".scrollbarWidth",  XrmoptionSepArg,    NULL},
    {"-mh",             ".menuHeight",      XrmoptionSepArg,    NULL},
    {"-menuheight",     ".menuHeight",      XrmoptionSepArg,    NULL},
    {"-xrm",		NULL,		    XrmoptionResArg,	NULL}
#endif
};

/*
 * Arguments handled by GTK internally.
 */
static char *gtk_cmdline_options[] = {
    "--sync",
    "--gdk-debug=",
    "--gdk-no-debug=",
    "--no-xshm",
    "--xim-preedit",
    "--xim-status",
    "--gtk-debug=",
    "--gtk-no-debug=",
    "--g-fatal-warnings",
    "--gtk-module=",
    "-display",
    "--display",
    NULL
};

static int gui_argc = 0;
static char **gui_argv = NULL;

/*
 * Parse the GUI related command-line arguments.  Any arguments used are
 * deleted from argv, and *argc is decremented accordingly.  This is called
 * when vim is started, whether or not the GUI has been started.
 */
void
gui_mch_prepare(int *argc, char **argv)
{
    int arg;
    int i;
    char **gtk_option;

    /*
     * Move all the entries in argv which are relevant to X into gui_argv.
     */
    gui_argc = 0;
    gui_argv = (char **) lalloc((long_u) (*argc * sizeof(char *)), FALSE);
    if (gui_argv == NULL)
	return;

    gui_argv[gui_argc++] = argv[0];
    arg = 1;
    while (arg < *argc) {
	/* Look for argv[arg] in cmdline_options[] table */
	for (i = 0; i < XtNumber(cmdline_options); i++)
	    if (strcmp(argv[arg], cmdline_options[i].option) == 0)
		break;

	if (i < XtNumber(cmdline_options)) {
	    /* Remember finding "-rv" or "-reverse" */
	    if (strcmp("-rv", argv[arg]) == 0 ||
		strcmp("-reverse", argv[arg]) == 0) {
		found_reverse_arg = TRUE;
	    } else if ((strcmp("-fn", argv[arg]) == 0 ||
		        strcmp("-font", argv[arg]) == 0) && arg + 1 < *argc) {
		font_opt = argv[arg + 1];
	    } else if ((strcmp("-geometry", argv[arg]) == 0 ||
			strcmp("-geom", argv[arg]) == 0) && arg + 1 < *argc) {
		gui.geom = (char_u *)g_strdup((const char *)argv[arg + 1]);
	    }

	    /* Found match in table, so move it into gui_argv */
	    gui_argv[gui_argc++] = argv[arg];
	    if (--*argc > arg) {
		mch_memmove(&argv[arg], &argv[arg + 1],
			    (*argc - arg) * sizeof(char *));
		if (cmdline_options[i].argKind != XrmoptionNoArg) {
		    /* Move the option argument as well. */
		    gui_argv[gui_argc++] = argv[arg];
		    if (--*argc > arg)
			mch_memmove(&argv[arg], &argv[arg + 1],
				    (*argc - arg) * sizeof(char *));
		}
	    }
	} else
	    ++arg;
    }

    /* extract the gtk internal options */
    gtk_option = gtk_cmdline_options;
    while (*gtk_option)
    {
	arg = 1;
	while (arg < *argc)
	{
	    if (strncmp(argv[arg], *gtk_option, strlen(*gtk_option)) == 0)
	    {
		/* Replace the standard X argument "-display" with the GTK
		 * argument "--display". */
		if (strncmp(*gtk_option, "-display", 8) == 0)
		    gui_argv[gui_argc++] =
				   (char *)vim_strsave((char_u *)"--display");
		else
		    gui_argv[gui_argc++] = argv[arg];
		if (--*argc > arg)
		{
		    mch_memmove(&argv[arg], &argv[arg + 1],
					      (*argc - arg) * sizeof(char *));
		    /* Move the option argument as well, if there is one. */
		    if (strncmp(*gtk_option, "--xim", 5) == 0
			    || strncmp(*gtk_option, "-display", 8) == 0
			    || strncmp(*gtk_option, "--display", 9) == 0)
		    {
			gui_argv[gui_argc++] = argv[arg];
			if (--*argc > arg)
			    mch_memmove(&argv[arg], &argv[arg + 1],
				    (*argc - arg) * sizeof(char *));
		    }
		}
		break;
	    }
	    else
	        arg++;
	}
	gtk_option++;
    }
}

#ifdef GTK_HAVE_FEATURES_1_1_0
/*
 * This should be maybe compleatly removed.
 */
/*ARGSUSED*/
static gint
visibility_event(GtkWidget * widget, GdkEventVisibility * event)
{
    /* Just remember it for the creation of GCs before drawing
     * operations.
     */
    gui.visibility = event->state;

    return FALSE;
}
#endif

/*
 * Redraw the corresponding portions of the screen.
 */
/*ARGSUSED*/
static gint
expose_event(GtkWidget * widget, GdkEventExpose * event)
{
    /* Skip this when the GUI isn't set up yet, will redraw later. */
    if (gui.starting)
	return FALSE;

    out_flush();		/* make sure all output has been processed */
    gui_redraw(event->area.x, event->area.y,
	       event->area.width, event->area.height);

    /* Clear the border areas if needed */
    if (event->area.x < FILL_X(0))
	gdk_window_clear_area(gui.drawarea->window, 0, 0, FILL_X(0), 0);
    if (event->area.y < FILL_Y(0))
	gdk_window_clear_area(gui.drawarea->window, 0, 0, 0, FILL_Y(0));
    if (event->area.x > FILL_X(Columns))
	gdk_window_clear_area(gui.drawarea->window,
			      FILL_X((int)Columns), 0, 0, 0);
    if (event->area.y > FILL_Y(Rows))
	gdk_window_clear_area(gui.drawarea->window, 0, FILL_Y((int)Rows), 0, 0);

    return FALSE;
}


/****************************************************************************
 * Focus handlers:
 */


/*
 * This is a simple state machine:
 * BLINK_NONE   not blinking at all
 * BLINK_OFF    blinking, cursor is not shown
 * BLINK_ON     blinking, cursor is shown
 */

#define BLINK_NONE  0
#define BLINK_OFF   1
#define BLINK_ON    2

static int blink_state = BLINK_NONE;
static long_u blink_waittime = 700;
static long_u blink_ontime = 400;
static long_u blink_offtime = 250;
static guint blink_timer = 0;

void
gui_mch_set_blinking(long waittime, long on, long off)
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
    if (blink_timer) {
	gtk_timeout_remove(blink_timer);
	blink_timer = 0;
    }
    if (blink_state == BLINK_OFF)
	gui_update_cursor(TRUE, FALSE);
    blink_state = BLINK_NONE;
}

/*ARGSUSED*/
static gint
blink_cb(gpointer data)
{
    if (blink_state == BLINK_ON) {
	gui_undraw_cursor();
	blink_state = BLINK_OFF;
	blink_timer = gtk_timeout_add((guint32)blink_offtime,
				   (GtkFunction) blink_cb, NULL);
    } else {
	gui_update_cursor(TRUE, FALSE);
	blink_state = BLINK_ON;
	blink_timer = gtk_timeout_add((guint32)blink_ontime,
				   (GtkFunction) blink_cb, NULL);
    }

    return FALSE;		/* don't happen again */
}

/*
 * Start the cursor blinking.  If it was already blinking, this restarts the
 * waiting time and shows the cursor.
 */
void
gui_mch_start_blink()
{
    if (blink_timer)
	gtk_timeout_remove(blink_timer);
    /* Only switch blinking on if none of the times is zero */
    if (blink_waittime && blink_ontime && blink_offtime && gui.in_focus) {
	blink_timer = gtk_timeout_add((guint32)blink_waittime,
				   (GtkFunction) blink_cb, NULL);
	blink_state = BLINK_ON;
	gui_update_cursor(TRUE, FALSE);
    }
}

/*ARGSUSED*/
static void
enter_notify_event(GtkContainer * container, gpointer data)
{
    if (blink_state == BLINK_NONE)
	gui_mch_start_blink();

    /* make sure keybord input goes there */
    gtk_widget_grab_focus(gui.drawarea);
}

/*ARGSUSED*/
static void
leave_notify_event(GtkContainer * container, gpointer data)
{
    if (blink_state != BLINK_NONE)
	gui_mch_stop_blink();
}

/*ARGSUSED*/
static gint
focus_in_event(GtkWidget *widget, GdkEventFocus *focus, gpointer data)
{
    gui_focus_change(TRUE);

    if (blink_state == BLINK_NONE)
	gui_mch_start_blink();

    /* make sure keybord input goes there */
    gtk_widget_grab_focus(gui.drawarea);

#ifdef USE_XIM
    xim_set_focus(TRUE);
#endif

    return TRUE;
}

/*ARGSUSED*/
static gint
focus_out_event(GtkWidget * widget, GdkEventFocus *focus, gpointer data)
{
    gui_focus_change(FALSE);

    if (blink_state != BLINK_NONE)
	gui_mch_stop_blink();

#ifdef USE_XIM
    xim_set_focus(FALSE);
#endif

    return TRUE;
}


/****************************************************************************
 * Main keyboard handler:
 */

/*ARGSUSED*/
static gint
key_press_event(GtkWidget * widget, GdkEventKey * event, gpointer data)
{
    char_u string[256], string2[256];
    guint key_sym;
    int len;
    int i;
    int modifiers;
    int key;
    guint state;

    key_sym = event->keyval;
    len = event->length;
    state = event->state;
    g_assert(len <= sizeof(string));

#ifdef HANGUL_INPUT
    if (event->keyval == GDK_space && (state & GDK_SHIFT_MASK))
    {
	hangul_input_state_toggle();
	return TRUE;
    }
#endif

    for (i = 0; i < len; ++i)
	string[i] = event->string[i];

    /* Shift-Tab results in Left_Tab, but we want <S-Tab> */
    if (key_sym == GDK_ISO_Left_Tab)
	key_sym = GDK_Tab;

    if ((key_sym == GDK_2 || key_sym == GDK_at) && (state & GDK_CONTROL_MASK))
    {
	string[0] = NUL;	/* CTRL-2 and CTRL-@ is NUL */
	len = 1;
    }
    else if (len == 0 && (key_sym == GDK_space || key_sym == GDK_Tab))
    {
	/* When there are modifiers, these keys get zero length; we need the
	 * original key here to be able to add a modifier below. */
	string[0] = (key_sym & 0xff);
	len = 1;
    }

#ifdef WANT_MENU
    /* If there is a menu and 'wak' is "yes", or 'wak' is "menu" and the key
     * is a menu shortcut, we ignore everything with the ALT modifier. */
    if ((state & GDK_MOD1_MASK)
	    && gui.menu_is_active
	    && (*p_wak == 'y'
		|| (*p_wak == 'm'
		    && len == 1
		    && gui_is_menu_shortcut(string[0]))))
	return TRUE;
#endif

    /* Check for Alt/Meta key (Mod1Mask) */
    if (len == 1 && (state & GDK_MOD1_MASK)
	&& !(key_sym == GDK_BackSpace || key_sym == GDK_Delete)) {
	/* Don't do this for <S-M-Tab>, that should become K_S_TAB with ALT. */
	if (!(key_sym == GDK_Tab && (state & GDK_SHIFT_MASK)))
	{
	    string[0] |= 0x80;
	    state &= ~GDK_MOD1_MASK;	/* don't use it again */
	}
    }

    /* Check for special keys, making sure BS and DEL are recognised. */
    if (len == 0 || key_sym == GDK_BackSpace || key_sym == GDK_Delete) {
	for (i = 0; special_keys[i].key_sym != 0; i++) {
	    if (special_keys[i].key_sym == key_sym) {
		string[0] = CSI;
		string[1] = special_keys[i].code0;
		string[2] = special_keys[i].code1;
		len = 3;
		break;
	    }
	}
    }

    if (len == 0)   /* Unrecognised key */
	return TRUE;

    /* Special keys (and a few others) may have modifiers */
    if (len == 3 || key_sym == GDK_space || key_sym == GDK_Tab ||
	key_sym == GDK_Return || key_sym == GDK_Linefeed ||
	key_sym == GDK_Escape) {
	modifiers = 0;
	if (state & GDK_SHIFT_MASK)
	    modifiers |= MOD_MASK_SHIFT;
	if (state & GDK_CONTROL_MASK)
	    modifiers |= MOD_MASK_CTRL;
	if (state & GDK_MOD1_MASK)
	    modifiers |= MOD_MASK_ALT;
#if defined(USE_XIM) && defined(MULTI_BYTE)
	/* It seems GDK returns GDK_VoidSymbol if the len is 3 and it
	 * contains multi-byte characters. Is next is right?
	 */
	if (!is_dbcs || key_sym != GDK_VoidSymbol)
#endif
	{
	    /*
	     * For some keys a shift modifier is translated into another key
	     * code. Do we need to handle the case where len != 1 and
	     * string[0] != CSI?
	     */
	    if (string[0] == CSI && len == 3)
		key = TO_SPECIAL(string[1], string[2]);
	    else
		key = string[0];

	    key = simplify_key(key, &modifiers);
	    if (IS_SPECIAL(key)) {
		string[0] = CSI;
		string[1] = K_SECOND(key);
		string[2] = K_THIRD(key);
		len = 3;
	    } else {
		string[0] = key;
		len = 1;
	    }
	}

	if (modifiers) {
	    string2[0] = CSI;
	    string2[1] = KS_MODIFIER;
	    string2[2] = modifiers;
	    add_to_input_buf(string2, 3);
	}
    } /* special keys */

    if (len == 1 && (string[0] == Ctrl('C') || string[0] == intr_char)) {
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

    /* blank out the pointer if necessary */
    if (p_mh)
	gui_mch_mousehide(TRUE);

    if (gtk_main_level() > 0)
	gtk_main_quit();

    return TRUE;
}


/****************************************************************************
 * Selection handlers:
 */


/*ARGSUSED*/
static gint
selection_clear_event(GtkWidget * widget, GdkEventSelection * event)
{
    clip_lose_selection();

    if (gtk_main_level() > 0)
	gtk_main_quit();

    return TRUE;
}

#define RS_NONE	0	/* selection_received_event() not called yet */
#define RS_OK	1	/* selection_received_event() called and OK */
#define RS_FAIL	2	/* selection_received_event() called and failed */
static int received_selection;

/*ARGSUSED*/
static void
selection_received_event(GtkWidget * widget, GtkSelectionData * data)
{
    int		motion_type;
    long_u	len;
    char_u	*p;
    int		free_p = FALSE;

    if ((!data->data) || (data->length <= 0)) {
	received_selection = RS_FAIL;
	/* clip_free_selection(); ??? */
	if (gtk_main_level() > 0)
	    gtk_main_quit();

	return;
    }

    motion_type = MCHAR;
    if (data->type == gdk_atom_intern("COMPOUND_TEXT", FALSE)
	    || data->type == gdk_atom_intern("TEXT", FALSE))
    {
	int	count, i;
	char	**list;
	GString *str = g_string_new(NULL);

	count = gdk_text_property_to_text_list(data->type, data->format,
					     data->data, data->length, &list);
	len = 0;
	for (i = 0; i < count; i++)
	    g_string_append(str, list[i]);

	p = (char_u *)str->str;
	len = str->len;
	g_string_free(str, FALSE);
	gdk_free_text_list(list);
	free_p = TRUE;
    }
    else
    {
	p = (char_u *)data->data;
	len = data->length;
    }

    if (data->type == clipboard.atom) {
	motion_type = *p++;
	len--;
    }
    clip_yank_selection(motion_type, p, (long) len);
	received_selection = RS_OK;
    if (gtk_main_level() > 0)
	gtk_main_quit();

    if (free_p)
	g_free(p);
}

#ifdef GTK_HAVE_FEATURES_1_1_4
/*
 * Prepare our selection data for passing it to the external selection
 * client.
 */
/*ARGSUSED*/
static void
selection_get_event(GtkWidget *widget,
		    GtkSelectionData *selection_data,
		    guint      info,
		    guint      time_,
		    gpointer   data)
{
    char_u	*string;
    char_u	*result;
    long_u	length;
    int		motion_type;
    GdkAtom	type = GDK_NONE;

    if (!clipboard.owned)
	return;			/* Shouldn't ever happen */

    if (info != SELECTION_STRING && info != SELECTION_CLIPBOARD
	    && info != SELECTION_COMPOUND_TEXT && info != SELECTION_TEXT)
	return;

    clip_get_selection();

    /* get the selection from the * register */
    motion_type = clip_convert_selection(&string, &length);
    if (motion_type < 0)
	return;

    /* For our own format, the first byte contains the motion type */
    if (info == SELECTION_CLIPBOARD)
	length++;

    result = lalloc((long_u)(2 * length), FALSE);
    if (result == NULL)
    {
	vim_free(string);
	return;
    }

    if (info == SELECTION_CLIPBOARD)
    {
	result[0] = motion_type;
	mch_memmove(result + 1, string, (size_t)(length - 1));
	type = clipboard.atom;
    }
    else if (info == SELECTION_COMPOUND_TEXT || info == SELECTION_TEXT)
    {
	char *str;
	gint format, new_len;

	vim_free(result);
	str = g_new(char, length + 1);
	mch_memmove(str, string, (size_t) length);
	vim_free(string);
	str[length] = '\0';
	gdk_string_to_compound_text(str, &type, &format, &result, &new_len);
	g_free(str);
	selection_data->type = type;
	selection_data->format = format;
	gtk_selection_data_set(selection_data, type, format, result, new_len);
	gdk_free_compound_text(result);
	return;
    }
    else
    {
	mch_memmove(result, string, (size_t)length);
	type = GDK_TARGET_STRING;
    }
    selection_data->type = selection_data->target;
    selection_data->format = 8;	/* 8 bits per char */

    gtk_selection_data_set(selection_data, type, 8, result, (gint)length);
    vim_free(string);
    vim_free(result);
}

#else	/* not GTK_HAVE_FEATURES_1_1_4 */

static void
selection_handler(GtkWidget *widget, GtkSelectionData *selection_data,
		  gpointer data)
{
    char_u	*string;
    char_u	*result;
    long_u	length;
    int		motion_type;

    if (!clipboard.owned)
	return;			/* Shouldn't ever happen */

    if (selection_data->target != clipboard.atom
	    && selection_data->target != GDK_TARGET_STRING)
	return;

    clip_get_selection();

    /* get the selection from the * register */
    motion_type = clip_convert_selection(&string, (long_u *)&length);
    if (motion_type < 0)
	return;

    /* For our own format, the first byte contains the motion type */
    if (selection_data->target == clipboard.atom)
	length++;

    result = (char_u *)malloc(2 * length);
    if (result == NULL)
    {
	vim_free(string);
	return;
    }

    if (selection_data->target == clipboard.atom)
    {
	result[0] = motion_type;
	mch_memmove(result + 1, string, (size_t)(length - 1));
    }
    else
	mch_memmove(result, string, (size_t)length);

    selection_data->type = selection_data->target;
    selection_data->format = 8; /* 8 bits per char */

    gtk_selection_data_set(selection_data, selection_data->target, 8,
	    result, length);
    vim_free(string);
    vim_free(result);
}

#endif	/* GTK_HAVE_FEATURES_1_1_0 */

/*
 * Check if the GUI can be started.  Called before gvimrc is sourced.
 * Return OK or FAIL.
 */
int
gui_mch_init_check(void)
{
#if 1
    /* This is needed to make the locale handling consistant between
     * the GUI and the rest of VIM.
     *
     * FIXME: This is required for correct dead key handling.
     * However for some as of jet unknow reason this appears to collide
     * with the setlocale(LC_ALL, "") calls in other parts of VIM.
     */
    gtk_set_locale();
#endif

    /* Don't use gtk_init(), it exits on failure. */
    if (!gtk_init_check(&gui_argc, &gui_argv))
    {
	gui.dying = TRUE;
	EMSG("cannot open display");
	return FAIL;
    }
    vim_free(gui_argv);

    /* Somehow I can't resist the feeling that we should move the
     * corrseponding code out of the terminal section and
     * thus aleviate the need for tis field later entierly.
     */
    gui.dpy = GDK_DISPLAY();
    return OK;
}


/****************************************************************************
 * Mouse handling callbacks
 */


static guint mouse_click_timer = 0;
static int mouse_timed_out = TRUE;

/*
 * Timer used to recognize multiple clicks of the mouse button
 */
static gint
mouse_click_timer_cb(gpointer data)
{
    /* we don't use this information currently */
    int *timed_out = (int *) data;

    *timed_out = TRUE;
    return FALSE;		/* don't happen again */
}

static guint motion_repeat_timer = 0;
static int motion_repeat_offset = FALSE;

static gint motion_notify_event(GtkWidget *, GdkEventMotion *);

/*
 * Timer used to recognize multiple clicks of the mouse button.
 */
/*ARGSUSED*/
static gint
motion_repeat_timer_cb(gpointer data)
{
    gint x, y;
    GdkModifierType state;
    GdkEventMotion event;

    gdk_window_get_pointer(gui.drawarea->window, &x, &y, &state);

    if (!(state & (GDK_BUTTON1_MASK | GDK_BUTTON2_MASK |
		    GDK_BUTTON3_MASK | GDK_BUTTON4_MASK |
		    GDK_BUTTON5_MASK)))
    {
	motion_repeat_timer = 0;
	return FALSE;
    }

    /* If there already is a mouse click in the input buffer, wait another
     * time (otherwise we would create a backlog of clicks) */
    if (vim_used_in_input_buf() > 10)
	return TRUE;

    motion_repeat_timer = 0;

    /* Fake a motion event.
     * Trick: Pretend the mouse moved to the next character on every other
     * event, otherwise drag events will be discarded, because they are still
     * in the same character. */
    event.is_hint = FALSE;
    if (motion_repeat_offset)
    {
	event.x = x + gui.char_width;
	motion_repeat_offset = FALSE;
    }
    else
    {
	event.x = x;
	motion_repeat_offset = TRUE;
    }
    event.y = y;
    event.state = state;
    motion_notify_event(gui.drawarea, &event);

    /* Don't happen again. We will get reinstalled in the synthetic event if
     * needed - thus repeating should still work.
     */
    return FALSE;
}

/*ARGSUSED*/
static gint
motion_notify_event(GtkWidget * widget, GdkEventMotion * event)
{
    gint x, y;
    GdkModifierType state;
    int_u vim_modifiers;
    int button;

    if (event->is_hint)
	gdk_window_get_pointer(event->window, &x, &y, &state);
    else {
	x = event->x;
	y = event->y;
	state = event->state;
    }

    button = (event->state & (GDK_BUTTON1_MASK | GDK_BUTTON2_MASK |
			      GDK_BUTTON3_MASK | GDK_BUTTON4_MASK |
			      GDK_BUTTON5_MASK))
	      ? MOUSE_DRAG : ' ';

    /* If our pointer is currently hidden, then we should show it. */
    gui_mch_mousehide(FALSE);

    /* Just moving the rodent above the drawing area without any button beeing
     * pressed. */
    if (button != MOUSE_DRAG) {
	gui_mouse_moved(y);
	return TRUE;
    }

    /* translate modifier coding between the main engine and GTK */
    vim_modifiers = 0x0;
    if (state & GDK_SHIFT_MASK)
	vim_modifiers |= MOUSE_SHIFT;
    if (state & GDK_CONTROL_MASK)
	vim_modifiers |= MOUSE_CTRL;
    if (state & GDK_MOD1_MASK)
	vim_modifiers |= MOUSE_ALT;

    /* inform the editor egine about the occurence of this event */
    gui_send_mouse_event(button, x, y, FALSE, vim_modifiers);
    if (gtk_main_level() > 0)
	gtk_main_quit();

    /*
     * Auto repeat timer handling.
     */
    if (x < 0 || y < 0
	    || x >= gui.drawarea->allocation.width
	    || y >= gui.drawarea->allocation.height) {

	int dx;
	int dy;
	int offshoot;
	int delay = 10;

	/* Calculate the maximal distance of the cursor from the drawing area.
	 * (offshoot can't become negative here!).
	 */
	dx = x < 0 ? -x : x - gui.drawarea->allocation.width;
	dy = y < 0 ? -y : y - gui.drawarea->allocation.height;

	offshoot = dx > dy ? dx : dy;

	/* Make a linearly declaying timer delay with a threshold of 5 at a
	 * distance of 127 pixels from the main window.
	 *
	 * One could think endlessly about the most ergonomic variant here.
	 * For example it could make sense to calculate the distance from the
	 * drags start instead...
	 *
	 * Maybe a parabolic interpolation would suite us better here too...
	 */
	if (offshoot > 127) {
	    /* 5 appears to be somehow near to my perceptual limits :-). */
	    delay = 5;
	} else {
	    delay = (130 * (127 - offshoot)) / 127 + 5;
	}

	/* shoot again */
	if (!motion_repeat_timer)
	    motion_repeat_timer = gtk_timeout_add((guint32)delay,
						motion_repeat_timer_cb, NULL);
    }

    return TRUE; /* handled */
}


/*
 * Mouse button handling.  Note please that we are capturing multiple click's
 * by our own timeout mechanism instead of the one provided by GTK+ istelf.
 * This is due to the way the generic VIM code is recognizing multiple clicks.
 */
/*ARGSUSED*/
static gint
button_press_event(GtkWidget * widget, GdkEventButton * event)
{
    int button;
    int repeated_click = FALSE;
    int x, y;
    int_u vim_modifiers;

    /*
     * Don't let additional events about multiple clicks send by GTK to us
     * after the inital button press event confuse us.
     */
    if (event->type != GDK_BUTTON_PRESS)
	return FALSE;

    x = event->x;
    y = event->y;

    /* Handle multiple clicks */
    if (!mouse_timed_out && mouse_click_timer) {
	gtk_timeout_remove(mouse_click_timer);
	mouse_click_timer = 0;
	repeated_click = TRUE;
    }

    mouse_timed_out = FALSE;
    mouse_click_timer = gtk_timeout_add((guint32)p_mouset,
				  mouse_click_timer_cb, &mouse_timed_out);

    switch (event->button) {
    case 1:
	button = MOUSE_LEFT;
	break;
    case 2:
	button = MOUSE_MIDDLE;
	break;
    case 3:
	button = MOUSE_RIGHT;
	break;
    case 4:
	button = MOUSE_4;
	break;
    case 5:
	button = MOUSE_5;
	break;
    default:
	return FALSE;		/* Unknown button */
    }

    vim_modifiers = 0x0;
    if (event->state & GDK_SHIFT_MASK)
	vim_modifiers |= MOUSE_SHIFT;
    if (event->state & GDK_CONTROL_MASK)
	vim_modifiers |= MOUSE_CTRL;
    if (event->state & GDK_MOD1_MASK)
	vim_modifiers |= MOUSE_ALT;

    gui_send_mouse_event(button, x, y, repeated_click, vim_modifiers);
    if (gtk_main_level() > 0)
	gtk_main_quit();	/* make sure the above will be handled immediately */

    return TRUE;
}

/*ARGSUSED*/
static gint
button_release_event(GtkWidget * widget, GdkEventButton * event)
{
    int x, y;
    int_u vim_modifiers;

    /* Remove any motion "mashine gun" timers used for automatic further
       extension of allocation areas if outside of the applications window
       area .*/
    if (motion_repeat_timer) {
	gtk_timeout_remove(motion_repeat_timer);
	motion_repeat_timer = 0;
    }

    x = event->x;
    y = event->y;

    vim_modifiers = 0x0;
    if (event->state & GDK_SHIFT_MASK)
	vim_modifiers |= MOUSE_SHIFT;
    if (event->state & GDK_CONTROL_MASK)
	vim_modifiers |= MOUSE_CTRL;
    if (event->state & GDK_MOD1_MASK)
	vim_modifiers |= MOUSE_ALT;

    gui_send_mouse_event(MOUSE_RELEASE, x, y, FALSE, vim_modifiers);
    if (gtk_main_level() > 0)
	gtk_main_quit();		/* make sure it will be handled immediately */

    return TRUE;
}


#ifdef GTK_DND
/****************************************************************************
 * Drag aNd Drop support handlers.
 */

/*
 * DND receiver.
 */
/*ARGSUSED*/
static void
drag_data_received(GtkWidget *widget, GdkDragContext *context,
	gint x, gint y,
	GtkSelectionData *data,
	guint info, guint time)
{
    char_u	**fnames;
    int		redo_dirs = FALSE;
    int		i;
    int		n;
    char	*start;
    char	*stop;
    char	*copy;
    int		nfiles;
    GdkModifierType current_modifiers;

    /* Get the current modifier state for proper distinguishment between
     * different operations later. */
    current_modifiers = 0;
    gdk_window_get_pointer(NULL, NULL, NULL, &current_modifiers);

    /* guard against trash */
    if ((data->length <=  0)
	    || (data->format != 8)
	    || (((char *)data->data)[data->length] != '\0')) {
	gtk_drag_finish(context, FALSE, FALSE, time);

	return;
    }

    /* Count how meny items there may be there and normalize
     * delimiters.
     */
    n = 1;
    copy = strdup((char *)data->data);
    for (i = 0; i < data->length; ++i) {
	if (copy[i] == '\n')
	    ++n;
	else if(copy[i] == '\r') {
	    copy[i] = '\n';
	    ++n;
	}
    }

    fnames = (char_u **) alloc((n + 1) * sizeof(char_u *));

    start = copy;
    stop = copy;
    nfiles = 0;
    for (i = 0; i < n; ++i) {
	stop = strchr(start, '\n');
	if (stop)
	    *stop = '\0';

	if (!strlen(start))
	    continue;

	if (strncmp(start, "file:", 5)) {
	    int j;

	    free(copy);
	    for (j = 0; j < nfiles; ++j)
		free(fnames[j]);
	    gtk_drag_finish(context, FALSE, FALSE, time);

	    return;
	}

	if (!strncmp(start, "file://localhost", 16)) {
	    fnames[nfiles] = (char_u *)strdup(start + 16);
	    ++nfiles;
	} else {
	    fnames[nfiles] = (char_u *)strdup(start + 5);
	    ++nfiles;
	}
	start = stop + 2;
    }
    free(copy);

    /* accept */
    gtk_drag_finish (context, TRUE, FALSE, time);

    /*
     * Handle dropping a directory on Vim.
     */
    if (nfiles == 1) {
	if (mch_isdir(fnames[0])) {
	    if (mch_chdir((char *)fnames[0]) == 0) {
		free(fnames[0]);
		fnames[0] = NULL;
		redo_dirs = TRUE;
	    }
	}
    } else {
	/* Ignore any directories */
	for (i = 0; i < nfiles; ++i) {
	    if (mch_isdir(fnames[i])) {
		free(fnames[i]);
		fnames[i] = NULL;
	    }
	}
    }

    if (current_modifiers & GDK_SHIFT_MASK) {
	/* Shift held down, change to first file's directory */
	if (fnames[0] != NULL && vim_chdirfile(fnames[0]) == 0)
	    redo_dirs = TRUE;
    }

    /* Handle the drop, :edit or :split to get to the file */
    handle_drop(nfiles, fnames, current_modifiers & GDK_CONTROL_MASK);

    if (redo_dirs)
	shorten_fnames(TRUE);

    /* Update the screen display */
    update_screen(NOT_VALID);
    setcursor();
    out_flush();
}
#endif /* GTK_DND */

#if 0
/* Not used yet, because I don't know how to catch the WM_SAVE_YOURSELF event.
 */
/*
 * Setup the WM_PROTOCOLS to indicate we want the WM_SAVE_YOURSELF event.
 * This is an ugly use of X functions.  GTK doesn't offer an alternative.
 */
static void
setup_save_yourself(void)
{
    Atom	*existing;
    Atom	*new;
    int		count;
    int		i;

    /* first get the existing value */
    if (XGetWMProtocols(GDK_DISPLAY(),
		  GDK_WINDOW_XWINDOW(gui.mainwin->window), &existing, &count))
    {
	/* check if WM_SAVE_YOURSELF isn't there yet */
	for (i = 0; i < count; ++i)
	    if (existing[i] == save_yourself_atom)
		break;
	if (i == count)
	{
	    /* allocate an Atoms array which is one item longer */
	    new = (Atom *)alloc((count + 1) * sizeof(Atom));
	    if (new != NULL)
	    {
		for (i = 0; i < count; ++i)
		    new[i] = existing[i];
		new[count] = save_yourself_atom;
		XSetWMProtocols(GDK_DISPLAY(),
		     GDK_WINDOW_XWINDOW(gui.mainwin->window), new, count + 1);
		vim_free(new);
	    }
	}
	XFree(existing);
    }
}
#endif

/*
 * Setup the window icon after the main window has bee realized.
 */
/*ARGSUSED*/
static void
mainwin_realize(GtkWidget *widget)
{
/* If you get an error message here, you still need to unpack the runtime
 * archive! */
#include "../runtime/vim32x32.xpm"
    static GdkPixmap *icon = NULL;
    static GdkBitmap *icon_mask = NULL;

    if (!icon)
	icon = gdk_pixmap_create_from_xpm_d(gui.mainwin->window,
						    &icon_mask, NULL, magick);
    gdk_window_set_icon(gui.mainwin->window, NULL, icon, icon_mask);
#if 0

    /* Setup to indicate to the window manager that we want to catch the
     * WM_SAVE_YOURSELF event. */
    setup_save_yourself();
#endif
}

/*
 * After the drawing area comes up, we calculate all colors and create the
 * dummy blank cursor.
 *
 * Don't try to set any VIM scrollbar sizes anywhere here. I'm relying on the
 * fact that the main VIM engine doesn't take them into account anywhere.
 */
static void
drawarea_realize_cb(GtkWidget *widget)
{
    char blank_data[] = {0x0};
    GdkPixmap *blank_mask;
    GdkColor color;
    GtkWidget *sbar;

#ifdef USE_XIM
    xim_init();
#endif
    gui_mch_new_colors();

    blank_mask = gdk_bitmap_create_from_data(NULL, blank_data, 1, 1);
    gdk_color_white(gdk_colormap_get_system(), &color);
    gui.blank_pointer = gdk_cursor_new_from_pixmap(blank_mask, blank_mask,
							&color, &color, 0, 0);
    gdk_bitmap_unref(blank_mask);
    if (gui.pointer_hidden)
        gdk_window_set_cursor(widget->window, gui.blank_pointer);

    /* get the actual size of the scrollbars, if they are realized */
    sbar = firstwin->w_scrollbars[SBAR_LEFT].id;
    if (!sbar || (!gui.which_scrollbars[SBAR_LEFT]
				    && firstwin->w_scrollbars[SBAR_RIGHT].id))
	sbar = firstwin->w_scrollbars[SBAR_RIGHT].id;
    if (sbar && GTK_WIDGET_REALIZED(sbar) && sbar->allocation.width)
	gui.scrollbar_width = sbar->allocation.width;

    sbar = gui.bottom_sbar.id;
    if (sbar && GTK_WIDGET_REALIZED(sbar) && sbar->allocation.height)
	gui.scrollbar_height = sbar->allocation.height;
}

/*
 * Callback routine for the "delete_event" signal on the toplevel window.
 * Tries to vim gracefully, or refuses to exit with changed buffers.
 */
/*ARGSUSED*/
static int
delete_event_cb(GtkWidget *wgt, gpointer cbdata)
{
    gui_window_closed();
    return TRUE;
}

#define VIM_ATOM_NAME "_VIM_TEXT"
static const GtkTargetEntry primary_targets[] = {
    {VIM_ATOM_NAME, 0, SELECTION_CLIPBOARD},
    {"STRING", 0, SELECTION_STRING},
    {"TEXT", 0, SELECTION_TEXT},
    {"COMPOUND_TEXT", 0, SELECTION_COMPOUND_TEXT}
};

/*
 * Initialise the X GUI.  Create all the windows, set up all the call-backs etc.
 * Returns OK for success, FAIL when the GUI can't be started.
 */
int
gui_mch_init()
{
    GtkWidget *vbox;

    /* Initialize values */
    gui.rev_video = FALSE;
    gui.border_width = 2;
    gui.scrollbar_width = SB_DEFAULT_WIDTH;
    gui.scrollbar_height = SB_DEFAULT_WIDTH;
    gui.fgcolor = g_new0(GdkColor, 1);
    gui.bgcolor = g_new0(GdkColor, 1);

#ifdef WANT_MENU
    /* Don't change the menu height values used in gui.c at runtime */
    gui.menu_height_fixed = TRUE;
#endif

    /* Set default foreground and background colours. */
    gui.norm_pixel = gui.def_norm_pixel;
    gui.back_pixel = gui.def_back_pixel;

    gui.mainwin = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_policy(GTK_WINDOW(gui.mainwin), TRUE, TRUE, TRUE);
    gtk_container_border_width(GTK_CONTAINER(gui.mainwin), 0);
    gtk_widget_set_events(gui.mainwin, GDK_VISIBILITY_NOTIFY_MASK);
    (void)gtk_signal_connect(GTK_OBJECT(gui.mainwin), "delete_event",
			     GTK_SIGNAL_FUNC(delete_event_cb), NULL);

    /* Add an icon to the main window. For fun and convenience of the user. */
    if (vim_strchr(p_go, GO_ICON) != NULL)
	gtk_signal_connect(GTK_OBJECT(gui.mainwin), "realize",
				      GTK_SIGNAL_FUNC(mainwin_realize), NULL);


#ifdef GTK_HAVE_FEATURES_1_1_0
    /* FIXME: this should eventually get the accelgroup of the gui.mainwin */
    gui.accel_group = gtk_accel_group_get_default();
#endif

    vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(gui.mainwin), vbox);
    gtk_widget_show(vbox);

#ifdef WANT_MENU
    /* create the menubar and handle */
    gui.menubar = gtk_menu_bar_new();
    gtk_widget_show(gui.menubar);
    gtk_box_pack_start(GTK_BOX(vbox), gui.menubar, FALSE, TRUE, 0);
#endif

#ifdef USE_TOOLBAR
    /* create the toolbar */
    if (p_toolbar) {
	if (strstr((const char *)p_toolbar, "text")
		&& strstr((const char *)p_toolbar, "icons"))
	    gui.toolbar =
		gtk_toolbar_new(GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_BOTH);
	else if (strstr((const char *)p_toolbar, "text"))
	    gui.toolbar =
		gtk_toolbar_new(GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_TEXT);
	else
	    gui.toolbar =
		gtk_toolbar_new(GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_ICONS);
    } else
	gui.toolbar =
	    gtk_toolbar_new(GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_ICONS);

    gtk_widget_show(gui.toolbar);
# ifdef GTK_HAVE_FEATURES_1_1_0
    /* some aesthetics on the toolbar */
    gtk_toolbar_set_button_relief(GTK_TOOLBAR(gui.toolbar), GTK_RELIEF_NONE);
# endif
    gtk_container_border_width(GTK_CONTAINER(gui.toolbar), 1);
    gtk_box_pack_start(GTK_BOX(vbox), gui.toolbar, FALSE, TRUE, 0);
#endif

    gui.formwin = gtk_form_new();
    gtk_container_border_width(GTK_CONTAINER(gui.formwin), 0);
    gtk_widget_set_events(gui.formwin, GDK_EXPOSURE_MASK);

    gui.drawarea = gtk_drawing_area_new();

    /* Determine which events we will filter. */
    gtk_widget_set_events(gui.drawarea,
			  GDK_EXPOSURE_MASK |
			  GDK_ENTER_NOTIFY_MASK |
			  GDK_LEAVE_NOTIFY_MASK |
			  GDK_BUTTON_PRESS_MASK |
			  GDK_BUTTON_RELEASE_MASK |
			  GDK_POINTER_MOTION_MASK |
			  GDK_POINTER_MOTION_HINT_MASK);

    gtk_widget_show(gui.drawarea);
    gtk_form_put(GTK_FORM(gui.formwin), gui.drawarea, 0, 0);
    gtk_widget_show(gui.formwin);
    gtk_box_pack_start(GTK_BOX(vbox), gui.formwin, TRUE, TRUE, 0);

    gtk_signal_connect(GTK_OBJECT(gui.mainwin), "key_press_event",
		       (GtkSignalFunc) key_press_event, NULL);
    gtk_signal_connect(GTK_OBJECT(gui.drawarea), "realize",
		       GTK_SIGNAL_FUNC(drawarea_realize_cb), NULL);

    /* Check if reverse video needs to be applied (on Sun it's done by X) */
    if (gui.rev_video && gui_mch_get_lightness(gui.back_pixel)
	> gui_mch_get_lightness(gui.norm_pixel)) {
	gui.norm_pixel = gui.def_back_pixel;
	gui.back_pixel = gui.def_norm_pixel;
	gui.def_norm_pixel = gui.norm_pixel;
	gui.def_back_pixel = gui.back_pixel;
    }
    gui.visibility = GDK_VISIBILITY_UNOBSCURED;
    clipboard.atom = gdk_atom_intern(VIM_ATOM_NAME, FALSE);
    save_yourself_atom = gdk_atom_intern("WM_SAVE_YOURSELF", FALSE);
    reread_rcfiles_atom = gdk_atom_intern("_GTK_READ_RCFILES", FALSE);

    /*
     * Start out by adding the configured border width into the border offset.
     */
    gui.border_offset = gui.border_width;

#ifdef GTK_HAVE_FEATURES_1_1_0
    gtk_signal_connect(GTK_OBJECT(gui.mainwin), "visibility_notify_event",
		       GTK_SIGNAL_FUNC(visibility_event), NULL);
#endif
    gtk_signal_connect(GTK_OBJECT(gui.drawarea), "expose_event",
		       GTK_SIGNAL_FUNC(expose_event), NULL);

    /*
     * Only install these enter/leave callbacks when 'p' in 'guioptions'.
     * Only needed for some window managers.
     */
    if (vim_strchr(p_go, GO_POINTER) != NULL) {
	gtk_signal_connect(GTK_OBJECT(gui.drawarea), "leave_notify_event",
			   GTK_SIGNAL_FUNC(leave_notify_event), NULL);
	gtk_signal_connect(GTK_OBJECT(gui.drawarea), "enter_notify_event",
			   GTK_SIGNAL_FUNC(enter_notify_event), NULL);
    }

    gtk_signal_connect(GTK_OBJECT(gui.mainwin), "focus_out_event",
			   GTK_SIGNAL_FUNC(focus_out_event), NULL);
    gtk_signal_connect(GTK_OBJECT(gui.mainwin), "focus_in_event",
			   GTK_SIGNAL_FUNC(focus_in_event), NULL);

    gtk_signal_connect(GTK_OBJECT(gui.drawarea), "motion_notify_event",
		       GTK_SIGNAL_FUNC(motion_notify_event), NULL);
    gtk_signal_connect(GTK_OBJECT(gui.drawarea), "button_press_event",
		       GTK_SIGNAL_FUNC(button_press_event), NULL);
    gtk_signal_connect(GTK_OBJECT(gui.drawarea), "button_release_event",
		       GTK_SIGNAL_FUNC(button_release_event), NULL);

    /*
     * Add selection handler functions.
     */
    gtk_signal_connect(GTK_OBJECT(gui.drawarea), "selection_clear_event",
		       GTK_SIGNAL_FUNC(selection_clear_event), NULL);
    gtk_signal_connect(GTK_OBJECT(gui.drawarea), "selection_received",
		       GTK_SIGNAL_FUNC(selection_received_event), NULL);

    /* gtk_selection_add_target() is not in GTK 1.1.2 */
#ifdef GTK_HAVE_FEATURES_1_1_4
    gtk_selection_add_targets(gui.drawarea, GDK_SELECTION_PRIMARY,
			      primary_targets,
			      sizeof(primary_targets)/sizeof(primary_targets[0]));
    gtk_signal_connect(GTK_OBJECT(gui.drawarea), "selection_get",
		       GTK_SIGNAL_FUNC(selection_get_event), NULL);
#else
    gtk_selection_add_handler(gui.drawarea, GDK_SELECTION_PRIMARY,
			      GDK_TARGET_STRING, selection_handler, NULL);
    gtk_selection_add_handler(gui.drawarea, GDK_SELECTION_PRIMARY,
			      clipboard.atom, selection_handler, NULL);
#endif

    /* Pretend we don't have input focus, we will get an event if we do. */
    gui.in_focus = FALSE;

    return OK;
}


/*
 * Called when the foreground or background color has been changed.
 */
void
gui_mch_new_colors()
{
    /* This used to change the graphics contexts directly but we are currently
     * manipulating them where desired.
     */
    if (gui.drawarea && gui.drawarea->window) {
	GdkColor color;
	color.pixel = gui.back_pixel;
	gdk_window_set_background(gui.drawarea->window, &color);
    }
}

#ifdef GTK_HAVE_FEATURES_1_1_6
# define USE_GEOMETRY_FOR_HINTS		1
#endif

static void
update_window_manager_hints(void)
{
    int width;
    int height;
#ifdef USE_GEOMETRY_FOR_HINTS
    GdkGeometry geometry;
    GdkWindowHints geometry_mask;
    static int old_width = 0;
    static int old_height = 0;
    static int old_char_width = 0;
    static int old_char_height = 0;

    /* This also needs to be done when the main window isn't there yet,
     * otherwise the hints don't work. */
    width = gui_get_base_width();
    height = gui_get_base_height();

    /* Avoid an expose event when the size didn't change. */
    if (width != old_width
	    || height != old_height
	    || gui.char_width != old_char_width
	    || gui.char_height != old_char_height)
    {
	geometry_mask =
		     GDK_HINT_BASE_SIZE|GDK_HINT_RESIZE_INC|GDK_HINT_MIN_SIZE;
	geometry.width_inc = gui.char_width;
	geometry.height_inc = gui.char_height;
	geometry.base_width = width;
	geometry.base_height = height;
	geometry.min_width = width + MIN_COLUMNS * gui.char_width;
	geometry.min_height = height + MIN_LINES * gui.char_height;
	gtk_window_set_geometry_hints(GTK_WINDOW(gui.mainwin), gui.formwin,
				      &geometry, geometry_mask);
	old_width = width;
	old_height = height;
	old_char_width = gui.char_width;
	old_char_height = gui.char_height;
    }
#else
    XSizeHints size_hints;

    /* Don't set the size until the main window is really there */
    if (!GTK_WIDGET_REALIZED(gui.mainwin))
	return;

    width = gui_get_base_width();
    height = gui_get_base_height();

    /* The hints don't automatically take the menubar and toolbar into
     * account.  Need to add their height here, if they are visible. */
# ifdef USE_TOOLBAR
    if (gui.toolbar && GTK_WIDGET_REALIZED(gui.toolbar)
	    && GTK_WIDGET_VISIBLE(gui.toolbar))
	height += gui.toolbar->allocation.height;
# endif
# ifdef WANT_MENU
    if (gui.menubar && GTK_WIDGET_REALIZED(gui.menubar)
	    && GTK_WIDGET_VISIBLE(gui.menubar))
	height += gui.menubar->allocation.height;
# endif

    /*
     * Argh!!! Once again we need to deal with an ommission in GTK+ by
     * resorting to direct Xlib calls. Fortunatly I know how to do it :-).
     */
    size_hints.flags = (PResizeInc | PBaseSize | PMinSize | PSize);
    size_hints.width_inc = gui.char_width;
    size_hints.height_inc = gui.char_height;
    size_hints.base_width = width;
    size_hints.base_height = height;
    size_hints.min_width = width + MIN_COLUMNS * gui.char_width;
    size_hints.min_height = height + MIN_LINES * gui.char_height;

    /* This is only needed for "older" window managers.  See a corresposning
     * comment in the X11 headers. */
    size_hints.width = width + Columns * gui.char_width;
    size_hints.height = height + Rows * gui.char_height;

    XSetWMNormalHints(GDK_DISPLAY(),
		      GDK_WINDOW_XWINDOW(gui.mainwin->window),
		      &size_hints);
#endif	/* USE_GEOMETRY_FOR_HINTS */
}

/*
 * This signal informs us about the need to rearrange our subwidgets.
 */
/*ARGSUSED*/
static gint
form_configure_event(GtkWidget * widget, GdkEventConfigure * event)
{
    gtk_form_freeze(GTK_FORM(gui.formwin));
    gui_resize_window(event->width, event->height);
    gtk_form_thaw(GTK_FORM(gui.formwin));

    return TRUE;
}

/*
 * X11 based inter client communication handler.
 */
/*ARGSUSED*/
static gint
client_event_cb(GtkWidget *widget, GdkEventClient *event)
{
    if (event->message_type == save_yourself_atom) {
	/* NOTE: this is never reached! */
	out_flush();
	ml_sync_all(FALSE, FALSE);      /* preserve all swap files */
	return TRUE;
    } else if (event->message_type == reread_rcfiles_atom) {
	gui_mch_new_colors();
	return TRUE;
    }
    return FALSE;
}

/*
 * Function called when window already closed.
 * We can't do much more here than to trying to preserve what had been done,
 * since the window is already inevitably going away.
 */
static void
destroy_callback(void)
{
    /* preserve files and exit */
    preserve_exit();
    if (gtk_main_level() > 0)
	gtk_main_quit();
}


/*
 * Open the GUI window which was created by a call to gui_mch_init().
 */
int
gui_mch_open()
{
    int x = -1, y = -1;

    /* Determine user specified geometry, if present. */
    if (gui.geom) {
	int mask;
	unsigned w, h;

	mask = XParseGeometry((char *)gui.geom, &x, &y, &w, &h);
	if (mask & WidthValue)
	    Columns = w;
	if (mask & HeightValue)
	    Rows = h;
	if (mask & (XValue | YValue))
	    gtk_widget_set_uposition(gui.mainwin, x, y);
	g_free(gui.geom);
	gui.geom = NULL;
    }

    gtk_form_set_size(GTK_FORM(gui.formwin),
	    (guint)(gui_get_base_width() + Columns * gui.char_width),
	    (guint)(gui_get_base_height() + Rows * gui.char_height));
    update_window_manager_hints();

    if (found_reverse_arg )
    {
	gui.def_norm_pixel = gui_mch_get_color((char_u *)"White");
	gui.def_back_pixel = gui_mch_get_color((char_u *)"Black");
    }
    else
    {
	gui.def_norm_pixel = gui_mch_get_color((char_u *)"Black");
	gui.def_back_pixel = gui_mch_get_color((char_u *)"White");
    }

    /* Get the colors from the "Normal" and "Menu" group (set in syntax.c or
     * in a vimrc file)
     */
    set_normal_colors();

    /* Check that none of the colors are the same as the background color */
    gui_check_colors();

    /* Get the colors for the highlight groups (gui_check_colors() might have
     * changed them).
     */
    highlight_gui_started();	/* re-init colors and fonts */

    gtk_signal_connect(GTK_OBJECT(gui.mainwin), "destroy",
		       GTK_SIGNAL_FUNC(destroy_callback), NULL);

    /* Make this run after any internal handling of the client event happaned
     * to make sure that all changes implicated by it are already in place and
     * we thus can make our own adjustments.
     */
    gtk_signal_connect_after(GTK_OBJECT(gui.mainwin), "client_event",
                       GTK_SIGNAL_FUNC(client_event_cb), NULL);

#ifdef HANGUL_INPUT
    hangul_keyboard_set();
#endif

    /*
     * Notify the fixed area about the need to resize the contents of the
     * gui.formwin, which we use for random positioning of the included
     * components.
     *
     * We connect this signal deferred finally after anything is in place,
     * since this is intended to handle resizements coming from the window
     * manager upon us and should not interfere with what VIM is requesting
     * upon startup.
     */
    gtk_signal_connect(GTK_OBJECT(gui.formwin), "configure_event",
		       GTK_SIGNAL_FUNC(form_configure_event), NULL);

#ifdef GTK_DND
    /*
     * Set up for receiving DND items.
     */
    gtk_drag_dest_set(gui.drawarea,
	    GTK_DEST_DEFAULT_ALL,
	    target_table, n_targets,
	    GDK_ACTION_COPY | GDK_ACTION_MOVE);

    gtk_signal_connect(GTK_OBJECT(gui.drawarea), "drag_data_received",
	    GTK_SIGNAL_FUNC(drag_data_received), NULL);
#endif

    gtk_widget_show(gui.mainwin);

    return OK;
}


/*ARGSUSED*/
void
gui_mch_exit(int rc)
{
    gtk_exit(0);
}

/*
 * Get the position of the top left corner of the window.
 */
int
gui_mch_get_winpos(int *x, int *y)
{
    /* For some people this must be gdk_window_get_origin() for a correct
     * result.  Where is the documentation! */
#ifdef GTK_HAVE_FEATURES_1_1_4
    gdk_window_get_root_origin(gui.mainwin->window, x, y);
#else
    gdk_window_get_origin(gui.mainwin->window, x, y);
#endif
    return OK;
}

/*
 * Set the position of the top left corner of the window to the given
 * coordinates.
 */
void
gui_mch_set_winpos(int x, int y)
{
    gdk_window_move(gui.mainwin->window, x, y);
}

/*
 * Set the windows size.
 */
/*ARGSUSED*/
void
gui_mch_set_winsize(int width, int height,
		    int min_width, int min_height,
		    int base_width, int base_height)
{
    gtk_form_set_size(GTK_FORM(gui.formwin), width, height);

    /* give GTK+ a chance to put all widget's into place */
    gui_mch_update();

    /* this will cause the proper resizement to happen too */
    update_window_manager_hints();
}


/*
 * The screen size is used to make sure the initial window doesn't get bigger
 * then the screen.  This subtracts some room for menubar, toolbar and window
 * decoreations.
 */
void
gui_mch_get_screen_dimensions(int *screen_w, int *screen_h)
{
    *screen_w = gdk_screen_width();
    /* Subtract 'guihearroom' from the height to allow some room for the
     * window manager (task list and window title bar). */
    *screen_h = gdk_screen_height() - p_ghr;

    /*
     * FIXME: dirty trick: Because the gui_get_base_height() doesn't include
     * the toolbar and menubar for GTK, we subtract them from the screen
     * hight, so that the window size can be made to fit on the screen.
     * This should be completely changed later.
     */
#ifdef USE_TOOLBAR
    if (gui.toolbar && GTK_WIDGET_REALIZED(gui.toolbar)
	    && GTK_WIDGET_VISIBLE(gui.toolbar))
	*screen_h -= gui.toolbar->allocation.height;
#endif
#ifdef WANT_MENU
    if (gui.menubar && GTK_WIDGET_REALIZED(gui.menubar)
	    && GTK_WIDGET_VISIBLE(gui.menubar))
	*screen_h -= gui.menubar->allocation.height;
#endif
}

#if defined(WANT_MENU) || defined(PROTO)
void
gui_mch_enable_menu(int flag)
{
    if (flag)
	gtk_widget_show(gui.menubar);
    else
	gtk_widget_hide(gui.menubar);

    update_window_manager_hints();
}
#endif


#if defined(USE_TOOLBAR) || defined(PROTO)
void
gui_mch_show_toolbar(int showit)
{
    if (gui.toolbar == NULL)
	return;

    if (!showit) {
	if (GTK_WIDGET_VISIBLE(gui.toolbar)) {
	    gtk_widget_hide(gui.toolbar);
	    /* wait util this gets done on the server side. */
	    update_window_manager_hints();
	}
    } else {
	g_assert(p_toolbar != NULL);
	if (strstr((const char *)p_toolbar, "text")
		&& strstr((const char *)p_toolbar, "icons")) {
	    gtk_toolbar_set_style(GTK_TOOLBAR(gui.toolbar), GTK_TOOLBAR_BOTH);
	} else if (strstr((const char *)p_toolbar, "text")) {
	    gtk_toolbar_set_style(GTK_TOOLBAR(gui.toolbar), GTK_TOOLBAR_TEXT);
	} else if (strstr((const char *)p_toolbar, "icons")) {
	    gtk_toolbar_set_style(GTK_TOOLBAR(gui.toolbar), GTK_TOOLBAR_ICONS);
	}

	if (!GTK_WIDGET_VISIBLE(gui.toolbar)) {
	    gtk_widget_show(gui.toolbar);
	    update_window_manager_hints();
	}

	if (strstr((const char *)p_toolbar, "tooltips"))
	    gtk_toolbar_set_tooltips(GTK_TOOLBAR(gui.toolbar), TRUE);
	else
	    gtk_toolbar_set_tooltips(GTK_TOOLBAR(gui.toolbar), FALSE);
    }
}
#endif


#ifdef USE_FONTSET
static GuiFont
gui_mch_get_fontset(char_u * name, int report_error)
{
    GdkFont *font;

    if (!gui.in_use || name == NULL)
	return (GuiFont) 0;

    font = gdk_fontset_load((gchar *) name);

    if (font == NULL) {
	if (report_error)
	    EMSG2("Unknown fontset: %s", name);
	return (GuiFont) 0;
    }

    /* reference this font as beeing in use */
    gdk_font_ref(font);

    return (GuiFont) font;
}
#endif

#ifdef GTK_HAVE_FEATURES_1_1_0
/*
 * Get a font structure for highlighting.
 * "cbdata" is a pointer to the global gui structure.
 */
/*ARGSUSED*/
static void
font_sel_ok(GtkWidget *wgt, gpointer cbdata)
{
    Gui *vw = (Gui *)cbdata;
    GtkFontSelectionDialog *fs = (GtkFontSelectionDialog *)vw->fontdlg;

    if (vw->fontname)
	g_free(vw->fontname);

    vw->fontname = (char_u *)g_strdup(
				gtk_font_selection_dialog_get_font_name(fs));
    gtk_widget_hide(vw->fontdlg);
    if (gtk_main_level() > 0)
	gtk_main_quit();
}

/*ARGSUSED*/
static void
font_sel_cancel(GtkWidget *wgt, gpointer cbdata)
{
    Gui *vw = (Gui *)cbdata;

    gtk_widget_hide(vw->fontdlg);
    if (gtk_main_level() > 0)
	gtk_main_quit();
}

/*ARGSUSED*/
static void
font_sel_destroy(GtkWidget *wgt, gpointer cbdata)
{
    Gui *vw = (Gui *)cbdata;

    vw->fontdlg = NULL;
    if (gtk_main_level() > 0)
	gtk_main_quit();
}
#endif

/*
 * Initialise vim to use the font with the given name.
 * Return FAIL if the font could not be loaded, OK otherwise.
 */
int
gui_mch_init_font(char_u * font_name)
{
    GdkFont	*font = NULL;
    char	*chunk[28], *sdup, *tmp;
    int		len, i;

#ifdef USE_FONTSET
    {
	static char_u *dflt_fontset = NULL;

	if (gui.fontset)
	{
	    /* If fontset is active, VIM treat all the font as a fontset. */
	    if (font_name == NULL || vim_strchr(font_name, ',') == NULL)
		font_name = dflt_fontset;
	    font = gui_mch_get_fontset(font_name, FALSE);
	    if (font == NULL)
		return FAIL;
	}
	else if (font_name == NULL && *p_guifontset)
	{
	    font = gui_mch_get_fontset(p_guifontset, FALSE);
	    if (font)
	    {
		font_name = p_guifontset;
		dflt_fontset = alloc(STRLEN(p_guifontset) + 1);
		STRCPY(dflt_fontset, p_guifontset);
	    }
	}
    }

    if (font == NULL)
#endif
    {
	/*
	 * If none of the fonts in 'font' could be loaded, try the default,
	 * which should be present on all X11 servers.
	 */
	if (font_name == NULL)
	    font_name = (char_u *) DFLT_FONT;

#ifdef GTK_HAVE_FEATURES_1_1_0
	if (STRCMP(font_name, "*") == 0) {
	    /*
	     * Request for a font handling dialog.
	     * Not quite sure we should handle this here...
	     */
	    /*
	     * NOTE about font selection widget: this can easily be backported
	     * to gtk-1.0.x.
	     */
	    if (!gui.fontdlg)
	    {
		GtkFontSelectionDialog	*fsd = NULL;

		gui.fontdlg = gtk_font_selection_dialog_new("Font Selection");
		fsd = GTK_FONT_SELECTION_DIALOG(gui.fontdlg);
		if (p_guifont != NULL)
		    gtk_font_selection_dialog_set_font_name(fsd,
							    (char *)p_guifont);
# ifdef GTK_HAVE_FEATURES_1_1_4
		gtk_window_set_modal(GTK_WINDOW(gui.fontdlg), TRUE);
		gtk_window_set_transient_for(GTK_WINDOW(gui.fontdlg),
						     GTK_WINDOW(gui.mainwin));
# endif
		gtk_signal_connect(GTK_OBJECT(gui.fontdlg), "destroy",
				   GTK_SIGNAL_FUNC(font_sel_destroy), &gui);
		gtk_signal_connect(GTK_OBJECT(fsd->ok_button), "clicked",
				   GTK_SIGNAL_FUNC(font_sel_ok), &gui);
		gtk_signal_connect(GTK_OBJECT(fsd->cancel_button), "clicked",
				   GTK_SIGNAL_FUNC(font_sel_cancel), &gui);
	    }
	    if (gui.fontname)
	    {
		g_free(gui.fontname);
		gui.fontname = NULL;
	    }
	    gtk_window_position(GTK_WINDOW(gui.fontdlg), GTK_WIN_POS_MOUSE);
	    gtk_widget_show(gui.fontdlg);
# ifdef GTK_HAVE_FEATURES_1_1_4
	    {
		static gchar		*spacings[] = {"c", "m", NULL};

		/* In GTK 1.2.3 this must be after the gtk_widget_show() call,
		 * otherwise everything is blocked for ten seconds. */
		gtk_font_selection_dialog_set_filter(
			GTK_FONT_SELECTION_DIALOG(gui.fontdlg),
			GTK_FONT_FILTER_BASE,
			GTK_FONT_ALL, NULL, NULL,
			NULL, NULL, spacings, NULL);
	    }
# endif

	    while (gui.fontdlg && GTK_WIDGET_VISIBLE(gui.fontdlg))
		gtk_main_iteration_do(TRUE);

	    if (gui.fontname == NULL)
		return FAIL;
	    vim_free(p_guifont);
	    p_guifont = vim_strsave(gui.fontname);
	    font_name = p_guifont;
	}
#endif
	if (font == NULL)
	    font = gui_mch_get_font(font_name, FALSE);

	if (font == NULL)
	    return FAIL;
    }

    if (gui.norm_font != 0)
	gdk_font_unref(font);
    gui.norm_font = font;
#ifdef USE_FONTSET
    if (font->type == GDK_FONT_FONTSET)
    {
	gui.fontset = (GuiFont) font;
	gui.char_width = gdk_string_width(font, " ");
    }
    else
#endif
    {
	gui.char_width = ((XFontStruct *)
				      GDK_FONT_XFONT(font))->max_bounds.width;
    }

    gui.char_height = font->ascent + font->descent;
    gui.char_ascent = font->ascent;

    /* Set the fontname, which will be used for information purposes */
    hl_set_font_name(font_name);

    if (font->type != GDK_FONT_FONTSET)
    {
	/* There is only one excuse I can give for the following attempt
	 * to manage font styles:
	 *
	 * I HATE THE BRAIN DEAD WAY X11 IS HANDLING FONTS (--mdcki)
	 */
	if ((sdup = g_strdup((const char *)font_name)) == NULL)
	    return FAIL;

	/* split up the whole */
	i = 0;
	for (tmp = sdup; *tmp != '\0'; ++tmp)
	    if (*tmp == '-')
	    {
		*tmp = '\0';
		chunk[i] = tmp + 1;
		++i;
	    }

	if (i == 14)
	{
	    char *bold_name = NULL;
	    char *ital_name = NULL;
	    char *italbold_name = NULL;

	    /* font name was compleate */
	    len = strlen((const char *)font_name) + 32;
	    bold_name = (char *)alloc(len);
	    ital_name = (char *)alloc(len);
	    italbold_name = (char *)alloc(len);
	    if (bold_name == NULL || ital_name == NULL || italbold_name == NULL)
	    {
		vim_free(bold_name);
		vim_free(ital_name);
		vim_free(italbold_name);
		g_free(sdup);
		return FAIL;
	    }

	    *bold_name = '\0';
	    *ital_name = '\0';
	    *italbold_name = '\0';

	    for (i = 0; i < 14; ++i)
	    {
		strcat(bold_name, "-");
		strcat(ital_name, "-");
		strcat(italbold_name, "-");
		strcat(bold_name, (i != 2) ? chunk[i] : "bold");
		strcat(ital_name, (i != 3) ? chunk[i] : "o");

		if (i != 2 && i != 3)
		    strcat(italbold_name, chunk[i]);
		else
		{
		    if (i == 2)
			strcat(italbold_name, "bold");
		    else if (i == 3)
			strcat(italbold_name, "o");
		}
	    }

	    font = gui_mch_get_font((char_u *)bold_name, FALSE);
	    if (font != NULL)
		gui.bold_font = font;
	    else if (gui.bold_font)
	    {
		gdk_font_unref(gui.bold_font);
		gui.bold_font = NULL;
	    }

	    font = gui_mch_get_font((char_u *)ital_name, FALSE);
	    if (font != NULL)
		gui.ital_font = font;
	    else if (gui.ital_font)
	    {
		gdk_font_unref(gui.ital_font);
		gui.ital_font = NULL;
	    }

	    font = gui_mch_get_font((char_u *)italbold_name, FALSE);
	    if (font != NULL)
		gui.boldital_font = font;
	    else if (gui.boldital_font)
	    {
		gdk_font_unref(gui.boldital_font);
		gui.boldital_font = NULL;
	    }

	    vim_free(bold_name);
	    vim_free(ital_name);
	    vim_free(italbold_name);
	}

	g_free(sdup);
    }

    /* Synchronize the fonts used in user input dialogs, since otherwise
     * search/replace will be esp annoyig in case of international font usage.
     */
    gui_gtk_synch_fonts();

#ifdef USE_XIM
    /* Adjust input management behaviour to the capabilities of the new
     * fontset */
    xim_decide_input_style();
    if (xim_get_status_area_height())
    {
	/* Status area is required.  Just create the empty label so that
	 * mainwin will allocate the extra space for status area. */
	GtkWidget *label = gtk_label_new("       ");

	gtk_widget_set_usize(label, 20, gui.char_height + 2);
	gtk_box_pack_end(GTK_BOX(GTK_BIN(gui.mainwin)->child), label,
							     FALSE, FALSE, 0);
	gtk_widget_show(label);
    }
#endif

    /* Preserve the logical dimensions of the screen. */
    update_window_manager_hints();

    return OK;
}


GuiFont
gui_mch_get_font(char_u * name, int report_error)
{
    GdkFont *font;

    if (!gui.in_use || name == NULL)	/* can't do this when GUI not running */
	return (GuiFont) 0;

#ifdef USE_FONTSET
    if (gui.fontset)
	/* If fontset is active, VIM treat all the font as a fontset */
	return gui_mch_get_fontset(name, report_error);
    else if (vim_strchr(name, ','))
	return (GuiFont)0;
#endif

    font = gdk_font_load((const gchar *) name);

    if (font == NULL) {
	if (report_error)
	    EMSG2("Unknown font: %s", name);
	return (GuiFont) 0;
    }

    /* reference this font as beeing in use */
    gdk_font_ref(font);
    if (((XFontStruct *) GDK_FONT_XFONT(font))->max_bounds.width
	!= ((XFontStruct *) GDK_FONT_XFONT(font))->min_bounds.width) {
	EMSG2("Font \"%s\" is not fixed-width", name);
	gdk_font_unref(font);

	return (GuiFont) 0;
    }
    return (GuiFont) font;
}

/*
 * Set the current text font.
 * Since we create all GC on demand, we use just gui.current_font to
 * indicate the desired current font.
 */
void
gui_mch_set_font(GuiFont font)
{
    gui.current_font = font;
}

#if 0 /* not used */
/*
 * Return TRUE if the two fonts given are equivalent.
 */
int
gui_mch_same_font(GuiFont f1, GuiFont f2)
{
    return gdk_font_equal((GdkFont *) f1, (GdkFont *) f2);
}
#endif

/*
 * If a font is not going to be used, free its structure.
 */
void
gui_mch_free_font(GuiFont font)
{
    if (font)
	gdk_font_unref((GdkFont *) font);
}


/*
 * Return the Pixel value (color) for the given color name.  This routine was
 * pretty much taken from example code in the Silicon Graphics OSF/Motif
 * Programmer's Guide.
 * Return -1 for error.
 */
GuiColor
gui_mch_get_color(char_u * name)
{
    int i;
    static char *(vimnames[][2]) =
    {
    /* A number of colors that some X11 systems don't have */
	{"LightRed", "#FFA0A0"},
	{"LightGreen", "#80FF80"},
	{"LightMagenta", "#FFA0FF"},
	{"DarkCyan", "#008080"},
	{"DarkBlue", "#0000C0"},
	{"DarkRed", "#C00000"},
	{"DarkMagenta", "#C000C0"},
	{"DarkGrey", "#C0C0C0"},
	{NULL, NULL}
    };

    if (!gui.in_use)		/* can't do this when GUI not running */
	return (GuiColor)(-1);

    while (name != NULL) {
	GdkColor color;

	if (gdk_color_parse((const gchar *)name, &color)) {
	    GdkColormap *colormap;
	    colormap = gtk_widget_get_colormap(gui.drawarea);
	    gdk_color_alloc(colormap, &color);

	    return (GuiColor) color.pixel;
	}
	/* add a few builtin names */
	for (i = 0;; ++i) {
	    if (vimnames[i][0] == NULL) {
		name = NULL;
		break;
	    }
	    if (STRICMP(name, vimnames[i][0]) == 0) {
		name = (char_u *) vimnames[i][1];
		break;
	    }
	}
    }

    return (GuiColor)(-1);
}

/*
 * Set the current text foreground color.
 */
void
gui_mch_set_fg_color(GuiColor color)
{
    gui.fgcolor->pixel = (Pixel) color;
}

/*
 * Set the current text background color.
 */
void
gui_mch_set_bg_color(GuiColor color)
{
    gui.bgcolor->pixel = (Pixel) color;
}

/*
 * Use the blank mouse pointer or not.
 *
 * hide: TRUE = use blank ptr, FALSE = use parent ptr
 */
void
gui_mch_mousehide(int hide)
{
    if (gui.pointer_hidden != hide) {
	if (gui.drawarea->window && gui.blank_pointer) {
	    if (hide)
		gdk_window_set_cursor(gui.drawarea->window, gui.blank_pointer);
	    else
		gdk_window_set_cursor(gui.drawarea->window, NULL);
	}
	gui.pointer_hidden = hide;
    }
}

void
gui_mch_draw_string(int row, int col, char_u * s, int len, int flags)
{
    GdkGC *gc;

    if (gui.current_font == NULL || gui.drawarea->window == NULL)
	return;

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

    gc = gdk_gc_new(gui.drawarea->window);
    gdk_gc_set_exposures(gc, gui.visibility != GDK_VISIBILITY_UNOBSCURED);

    if (flags & DRAW_TRANSP) {
	gdk_gc_set_foreground(gc, gui.fgcolor);
	gdk_draw_text(gui.drawarea->window,
		      gui.current_font,
		      gc,
		      TEXT_X(col), TEXT_Y(row),
		      (const gchar *)s, len);
    } else {
	int width;
	int height;
	width = gdk_text_width(gui.current_font, (const gchar *)s, len);
	height = gui.char_height;

	gdk_gc_set_foreground(gc, gui.bgcolor);
	gdk_draw_rectangle(gui.drawarea->window,
			   gc,
			   TRUE,
			   FILL_X(col), FILL_Y(row), width, height);
	gdk_gc_set_foreground(gc, gui.fgcolor);
	gdk_draw_text(gui.drawarea->window,
		      gui.current_font,
		      gc,
		      TEXT_X(col), TEXT_Y(row),
		      (const gchar *)s, len);
    }

    /* redraw the contents with an offset of 1 to emulate bold */
    if (flags & DRAW_BOLD) {
	gdk_draw_text(gui.drawarea->window,
		      gui.current_font,
		      gc,
		      TEXT_X(col) + 1, TEXT_Y(row),
		      (const gchar *)s, len);
    }

    if (flags & DRAW_UNDERL) {
	gdk_draw_line(gui.drawarea->window,
		      gc, FILL_X(col),
	FILL_Y(row + 1) - 1, FILL_X(col + len) - 1, FILL_Y(row + 1) - 1);
    }
    gdk_gc_destroy(gc);
}

/*
 * Return OK if the key with the termcap name "name" is supported.
 */
int
gui_mch_haskey(char_u * name)
{
    int i;

    for (i = 0; special_keys[i].key_sym != 0; i++)
	if (name[0] == special_keys[i].code0 &&
	    name[1] == special_keys[i].code1)
	    return OK;
    return FAIL;
}

#if defined(WANT_TITLE) || defined(PROTO)
/*
 * Return the text window-id and display.  Only required for X-based GUI's
 */
int
gui_get_x11_windis(Window * win, Display ** dis)
{
    /*
     * This is currently only used by the code which is handling the
     * window manager title of this program.
     */
    *dis = GDK_DISPLAY();
    if (gui.mainwin->window) {
	*win = GDK_WINDOW_XWINDOW(gui.mainwin->window);
	return OK;
    }
    *win = 0;
    return FAIL;
}
#endif

void
gui_mch_beep()
{
    gdk_beep();
}

void
gui_mch_flash(int msec)
{
    GdkGCValues values;
    GdkGC *invert_gc;
    GdkColor foreground;
    GdkColor background;

    if (gui.drawarea->window == NULL)
	return;

    foreground.pixel = gui.norm_pixel ^ gui.back_pixel;
    background.pixel = gui.norm_pixel ^ gui.back_pixel;

    values.foreground = foreground;
    values.background = background;
    values.function = GDK_XOR;
    invert_gc = gdk_gc_new_with_values(gui.drawarea->window,
				       &values,
				       GDK_GC_FOREGROUND |
				       GDK_GC_BACKGROUND |
				       GDK_GC_FUNCTION);
    gdk_gc_set_exposures(invert_gc, gui.visibility != GDK_VISIBILITY_UNOBSCURED);

    /* Do a visual beep by changing back and forth in some undetermined way,
     * the foreground and background colors.  This is due to the fact that
     * there can't be really any prediction about the effects of XOR on
     * arbitrary X11 servers. However this seems to be enought for what we
     * intend it to do.
     */
    gdk_draw_rectangle(gui.drawarea->window, invert_gc,
		       TRUE,
		       0, 0,
		       FILL_X((int) Columns) + gui.border_offset,
		       FILL_Y((int) Rows) + gui.border_offset);

    gdk_flush();
    ui_delay((long)msec, TRUE);	/* wait so many msec */
    gdk_draw_rectangle(gui.drawarea->window, invert_gc,
		       TRUE,
		       0, 0,
		       FILL_X((int) Columns) + gui.border_offset,
		       FILL_Y((int) Rows) + gui.border_offset);

    gdk_gc_destroy(invert_gc);
}

/*
 * Invert a rectangle from row r, column c, for nr rows and nc columns.
 */
void
gui_mch_invert_rectangle(int r, int c, int nr, int nc)
{
    GdkGCValues values;
    GdkGC *invert_gc;
    GdkColor foreground;
    GdkColor background;

    if (gui.drawarea->window == NULL)
	return;

    foreground.pixel = gui.norm_pixel ^ gui.back_pixel;
    background.pixel = gui.norm_pixel ^ gui.back_pixel;

    values.foreground = foreground;
    values.background = background;
    values.function = GDK_XOR;
    invert_gc = gdk_gc_new_with_values(gui.drawarea->window,
				       &values,
				       GDK_GC_FOREGROUND |
				       GDK_GC_BACKGROUND |
				       GDK_GC_FUNCTION);
    gdk_gc_set_exposures(invert_gc, gui.visibility != GDK_VISIBILITY_UNOBSCURED);
    gdk_draw_rectangle(gui.drawarea->window, invert_gc,
		       TRUE,
		       FILL_X(c), FILL_Y(r),
		       (nc) * gui.char_width, (nr) * gui.char_height);
    gdk_gc_destroy(invert_gc);
}

/*
 * Iconify the GUI window.
 */
void
gui_mch_iconify()
{
    XIconifyWindow(GDK_DISPLAY(),
		   GDK_WINDOW_XWINDOW(gui.mainwin->window),
		   DefaultScreen(GDK_DISPLAY()));
}

/*
 * Draw a cursor without focus.
 */
void
gui_mch_draw_hollow_cursor(GuiColor color)
{
    GdkGC *gc;

    if (gui.drawarea->window == NULL)
	return;

    gui_mch_set_fg_color(color);

    gc = gdk_gc_new(gui.drawarea->window);
    gdk_gc_set_exposures(gc, gui.visibility != GDK_VISIBILITY_UNOBSCURED);
    gdk_gc_set_foreground(gc, gui.fgcolor);
#if defined(USE_FONTSET) && defined(MULTI_BYTE)
    if (gui.fontset)
    {
	if (IsLeadByte(LinePointers[gui.row][gui.col])
# ifdef HANGUL_INPUT
		|| composing_hangul
# endif
	   )
	    gdk_draw_rectangle(gui.drawarea->window, gc,
			       FALSE,
			       FILL_X(gui.col), FILL_Y(gui.row),
			       2*gui.char_width - 1, gui.char_height - 1);
	else
	    gdk_draw_rectangle(gui.drawarea->window, gc,
			       FALSE,
			       FILL_X(gui.col), FILL_Y(gui.row),
			       gui.char_width - 1, gui.char_height - 1);
    }
    else
#endif
    gdk_draw_rectangle(gui.drawarea->window, gc,
		       FALSE,
		       FILL_X(gui.col), FILL_Y(gui.row),
		       gui.char_width - 1, gui.char_height - 1);
    gdk_gc_destroy(gc);
}

/*
 * Draw part of a cursor, "w" pixels wide, and "h" pixels high, using
 * color "color".
 */
void
gui_mch_draw_part_cursor(int w, int h, GuiColor color)
{
    GdkGC *gc;

    if (gui.drawarea->window == NULL)
	return;

    gui_mch_set_fg_color(color);

    gc = gdk_gc_new(gui.drawarea->window);
    gdk_gc_set_exposures(gc, gui.visibility != GDK_VISIBILITY_UNOBSCURED);
    gdk_gc_set_foreground(gc, gui.fgcolor);
    gdk_draw_rectangle(gui.drawarea->window, gc,
		       TRUE,
#ifdef RIGHTLEFT
    /* vertical line should be on the right of current point */
	   State != CMDLINE && curwin->w_p_rl ? FILL_X(gui.col + 1) - w :
#endif
		       FILL_X(gui.col),
		       FILL_Y(gui.row) + gui.char_height - h,
		       w, h);
    gdk_gc_destroy(gc);
}

#ifndef GTK_HAVE_FEATURES_1_1_0
static gint
idle_function(GtkWidget * label)
{
    if (gtk_main_level() > 0)
	gtk_main_quit();

    return FALSE;
}
#endif


/*
 * Catch up with any queued X11 events.  This may put keyboard input into the
 * input buffer, call resize call-backs, trigger timers etc.  If there is
 * nothing in the X11 event queue (& no timers pending), then we return
 * immediately.
 */
void
gui_mch_update()
{
#ifdef GTK_HAVE_FEATURES_1_1_0
    while (gtk_events_pending() && !vim_is_input_buf_full())
	gtk_main_iteration_do(FALSE);
#else
    int pending;

    /* Somehow the above loop hangs on GTK 1.0.6.  Use the idle_function() to
     * work around this weird problem. */
    while (((pending = gtk_events_pending()) > 1) && !vim_is_input_buf_full())
	gtk_main_iteration();

    if ((pending == 1) && !vim_is_input_buf_full()) {
	gtk_idle_add((GtkFunction)idle_function, gui.mainwin);
	gtk_main_iteration_do(FALSE);
    }
#endif
}

static gint
input_timer_cb(gpointer data)
{
    int *timed_out = (int *) data;

    /* Just inform the caller about the accurence of it */
    *timed_out = TRUE;

    if (gtk_main_level() > 0)
	gtk_main_quit();

    return FALSE;		/* don't happen again */
}

#ifdef USE_SNIFF
/*
 * Callback function, used when data is available on the SNiFF connection.
 */
/* ARGSUSED */
    static void
sniff_request_cb(
    gpointer	data,
    gint	source_fd,
    GdkInputCondition condition)
{
    static char_u bytes[3] = {CSI, (int)KS_EXTRA, (int)KE_SNIFF};

    add_to_input_buf(bytes, 3);

    if (gtk_main_level() > 0)
	gtk_main_quit();
}
#endif

/*
 * GUI input routine called by gui_wait_for_chars().  Waits for a character
 * from the keyboard.
 *  wtime == -1     Wait forever.
 *  wtime == 0      This should never happen.
 *  wtime > 0       Wait wtime milliseconds for a character.
 * Returns OK if a character was found to be available within the given time,
 * or FAIL otherwise.
 */
int
gui_mch_wait_for_chars(long wtime)
{
    int focus;
    guint timer;
    static int timed_out;
#ifdef USE_SNIFF
    static int	sniff_on = 0;
    static gint	sniff_input_id = 0;
#endif

#ifdef USE_SNIFF
    if (sniff_on && !want_sniff_request)
    {
	if (sniff_input_id)
	    gdk_input_remove(sniff_input_id);
	sniff_on = 0;
    }
    else if (!sniff_on && want_sniff_request)
    {
	/* Add fd_from_sniff to watch for available data in main loop. */
	sniff_input_id = gdk_input_add(fd_from_sniff,
			       GDK_INPUT_READ, sniff_request_cb, (gpointer)0);
	sniff_on = 1;
    }
#endif

    timed_out = FALSE;

    /* this timeout makes sure that we will return if no characters arrived in
     * time */

    if (wtime > 0)
	timer = gtk_timeout_add((guint32)wtime, input_timer_cb, &timed_out);
    else
	timer = 0;

    focus = gui.in_focus;

    do {
	/* Stop or start blinking when focus changes */
	if (gui.in_focus != focus) {
	    if (gui.in_focus)
		gui_mch_start_blink();
	    else
		gui_mch_stop_blink();
	    focus = gui.in_focus;
	}

	/*
	 * Loop in GTK+ processing  until a timeout or input occurs.
	 */
	gtk_main();

	/* Got char, return immediately */
	if (!vim_is_input_buf_empty()) {
	    if (timer != 0 && !timed_out)
		gtk_timeout_remove(timer);
	    return OK;
	}
    } while (wtime < 0 || !timed_out);

    /*
     * Flush all eventually pending (drawing) events.
     */
    gui_mch_update();

    return FAIL;
}


/****************************************************************************
 * Output drawing routines.
 ****************************************************************************/


/* Flush any output to the screen */
void
gui_mch_flush()
{
    gdk_flush();
}

/*
 * Clear a rectangular region of the screen from text pos (row1, col1) to
 * (row2, col2) inclusive.
 */
void
gui_mch_clear_block(int row1, int col1, int row2, int col2)
{
    GdkGC *gc;
    GdkColor color;

    if (gui.drawarea->window == NULL)
	return;

    color.pixel = gui.back_pixel;

    gc = gdk_gc_new(gui.drawarea->window);
    gdk_gc_set_exposures(gc, gui.visibility != GDK_VISIBILITY_UNOBSCURED);
    gdk_gc_set_foreground(gc, &color);

    /*
     * Clear one extra pixel at the right, for when bold characters have
     * spilled over to the next column.  This can erase part of the next
     * character however in the usage context of this function it will be
     * overriden immediately by the correct character again.
     */

    gdk_draw_rectangle(gui.drawarea->window, gc, TRUE,
		       FILL_X(col1), FILL_Y(row1),
		       (col2 - col1 + 1) * gui.char_width + 1,
		       (row2 - row1 + 1) * gui.char_height);
    gdk_gc_destroy(gc);
}

void
gui_mch_clear_all(void)
{
    if (gui.drawarea->window == NULL)
	return;

    gdk_window_clear(gui.drawarea->window);
}

/*
 * Scroll the text between gui.scroll_region_top & gui.scroll_region_bot by the
 * number of lines given.  Positive scrolls down (text goes up) and negative
 * scrolls up (text goes down).
 */
static void check_copy_area(void)
{
    XEvent event;
    XGraphicsExposeEvent *gevent;

    if (gui.visibility != GDK_VISIBILITY_PARTIAL)
	return;

    gdk_flush();

    /* Wait to check whether the scroll worked or not. */
    for (;;) {
	if (XCheckTypedEvent(GDK_DISPLAY(), NoExpose, &event))
	    return;		/* The scroll worked. */

	if (XCheckTypedEvent(GDK_DISPLAY(), GraphicsExpose, &event)) {
	    gevent = (XGraphicsExposeEvent *) & event;
	    gui_redraw(gevent->x, gevent->y, gevent->width, gevent->height);
	    if (gevent->count == 0)
		return;		/* This was the last expose event */
	}
	gdk_flush();
    }
}

/*
 * Delete the given number of lines from the given row, scrolling up any
 * text further down within the scroll region.
 */
void
gui_mch_delete_lines(int row, int num_lines)
{
    if (gui.visibility == GDK_VISIBILITY_FULLY_OBSCURED)
	return;			/* Can't see the window */

    if (num_lines <= 0)
	return;

    if (row + num_lines > gui.scroll_region_bot) {
	/* Scrolled out of region, just blank the lines out */
	gui_clear_block(row, 0, gui.scroll_region_bot, (int) Columns - 1);
    } else {
	GdkGC *gc;

	gc = gdk_gc_new(gui.drawarea->window);
	gdk_gc_set_exposures(gc, gui.visibility != GDK_VISIBILITY_UNOBSCURED);
	gdk_gc_set_foreground(gc, gui.fgcolor);
	gdk_gc_set_background(gc, gui.bgcolor);

	/* copy one extra pixel, for when bold has spilled over */
	gdk_window_copy_area(gui.drawarea->window, gc,
			     FILL_X(0), FILL_Y(row),
			     gui.drawarea->window,
			     FILL_X(0), FILL_Y(row + num_lines),
			     gui.char_width * (int) Columns + 1,
			     gui.char_height *
				(gui.scroll_region_bot - row - num_lines + 1));
	gdk_gc_destroy(gc);

	/* Update gui.cursor_row if the cursor scrolled or copied over */
	if (gui.cursor_row >= row) {
	    if (gui.cursor_row < row + num_lines)
		gui.cursor_is_valid = FALSE;
	    else if (gui.cursor_row <= gui.scroll_region_bot)
		gui.cursor_row -= num_lines;
	}
	gui_clear_block(gui.scroll_region_bot - num_lines + 1, 0,
			gui.scroll_region_bot, (int) Columns - 1);
	check_copy_area();
    }
}

/*
 * Insert the given number of lines before the given row, scrolling down any
 * following text within the scroll region.
 */
void
gui_mch_insert_lines(int row, int num_lines)
{
    if (gui.visibility == GDK_VISIBILITY_FULLY_OBSCURED)
	return;			/* Can't see the window */

    if (num_lines <= 0)
	return;

    if (row + num_lines > gui.scroll_region_bot) {
	/* Scrolled out of region, just blank the lines out */
	gui_clear_block(row, 0, gui.scroll_region_bot, (int) Columns - 1);
    } else {
	GdkGC *gc;

	gc = gdk_gc_new(gui.drawarea->window);
	gdk_gc_set_exposures(gc, gui.visibility != GDK_VISIBILITY_UNOBSCURED);
	gdk_gc_set_foreground(gc, gui.fgcolor);
	gdk_gc_set_background(gc, gui.bgcolor);

	/* copy one extra pixel, for when bold has spilled over */
	gdk_window_copy_area(gui.drawarea->window, gc,
			     FILL_X(0), FILL_Y(row + num_lines),
			     gui.drawarea->window,
			     FILL_X(0), FILL_Y(row),
			     gui.char_width * (int) Columns + 1,
			     gui.char_height *
				(gui.scroll_region_bot - row - num_lines + 1));
	gdk_gc_destroy(gc);

	/* Update gui.cursor_row if the cursor scrolled or copied over */
	if (gui.cursor_row >= gui.row) {
	    if (gui.cursor_row <= gui.scroll_region_bot - num_lines)
		gui.cursor_row += num_lines;
	    else if (gui.cursor_row <= gui.scroll_region_bot)
		gui.cursor_is_valid = FALSE;
	}
	gui_clear_block(row, 0, row + num_lines - 1, (int) Columns - 1);
	check_copy_area();
    }
}

/*
 * X Selection stuff, for cutting and pasting text to other windows.
 */
void
clip_mch_request_selection()
{
    /* First try to get the content of our own special clipboard. */
    received_selection = RS_NONE;
    (void)gtk_selection_convert(gui.drawarea,
				    GDK_SELECTION_PRIMARY, clipboard.atom,
				    (guint32)GDK_CURRENT_TIME);
    while (received_selection == RS_NONE)
	gtk_main();		/* wait for selection_received_event */

    if (received_selection == RS_FAIL)
    {
	/* Now try to get it out of the usual string selection. */
	received_selection = RS_NONE;
	(void)gtk_selection_convert(gui.drawarea, GDK_SELECTION_PRIMARY,
				    GDK_TARGET_STRING,
				    (guint32)GDK_CURRENT_TIME);
	while (received_selection == RS_NONE)
	    gtk_main();		/* wait for selection_received_event */
    }
}

void
clip_mch_lose_selection()
{
    gtk_selection_owner_set(gui.drawarea,
			    GDK_SELECTION_PRIMARY, (guint32)GDK_CURRENT_TIME);
    gui_mch_update();
}

/*
 * Check whatever we allready own the selection.
 */
int
clip_mch_own_selection()
{
#if 0
    return gdk_selection_owner_get(
				GDK_SELECTION_PRIMARY) == gui.drawarea->window;
#else
    /* At this point we always already own the clipboard */

    return OK;
#endif
}

/*
 * Send the current selection to the clipboard.
 */
void
clip_mch_set_selection()
{
    gtk_selection_owner_set(gui.drawarea,
			    GDK_SELECTION_PRIMARY,
			    (guint32)GDK_CURRENT_TIME);
    gui_mch_update();
}


#if defined(WANT_MENU) || defined(PROTO)
/*
 * Make a menu item appear either active or not active (grey or not grey).
 */
void
gui_mch_menu_grey(VimMenu * menu, int grey)
{
    if (menu->id == 0)
	return;

    gui_mch_menu_hidden(menu, FALSE);
    gtk_widget_set_sensitive(menu->id, !grey);

    gui_mch_update();
}

/*
 * Make menu item hidden or not hidden.
 */
void
gui_mch_menu_hidden(VimMenu * menu, int hidden)
{
    if (menu->id == 0)
	return;

    if (hidden) {
	if (GTK_WIDGET_VISIBLE(menu->id))
	    gtk_widget_hide(menu->id);
    } else {
	if (!GTK_WIDGET_VISIBLE(menu->id))
	    gtk_widget_show(menu->id);
    }

    gui_mch_update();
}

/*
 * This is called after setting all the menus to grey/hidden or not.
 */
void
gui_mch_draw_menubar()
{
    /* just make sure that the visual changes get effect immediately */
    gui_mch_update();
}
#endif

/*
 * Scrollbar stuff.
 */
void
gui_mch_enable_scrollbar(GuiScrollbar * sb, int flag)
{
    if (sb->id == 0)
	return;
    if (flag)
	gtk_widget_show(sb->id);
    else
	gtk_widget_hide(sb->id);
    update_window_manager_hints();
}


/*
 * Return the lightness of a pixel.  White is 255.
 */
int
gui_mch_get_lightness(GuiColor pixel)
{
    XColor xc;
    GdkColormap *colormap;

    colormap = gtk_widget_get_colormap(gui.mainwin);
    xc.pixel = pixel;

    /* FIXME: this is crap in terms of actual accuracy */
    XQueryColor(GDK_DISPLAY(), GDK_COLORMAP_XCOLORMAP(colormap), &xc);

    return (int) (xc.red * 3 + xc.green * 6 + xc.blue) / (10 * 256);
}

#if (defined(SYNTAX_HL) && defined(WANT_EVAL)) || defined(PROTO)

/*
 * Return the RGB value of a pixel as "#RRGGBB".
 *
 * Unfortunately there appears to be no way to accomplish this entierly
 * without resorting to native X11 functions.
 */
char_u *
gui_mch_get_rgb(GuiColor pixel)
{
    XColor xc;
    GdkColormap *colormap;
    static char_u retval[10];

    colormap = gtk_widget_get_colormap(gui.mainwin);

    xc.pixel = pixel;
    XQueryColor(GDK_DISPLAY(), GDK_COLORMAP_XCOLORMAP(colormap), &xc);

    sprintf((char *) retval, "#%02x%02x%02x",
	    (unsigned) xc.red >> 8,
	    (unsigned) xc.green >> 8,
	    (unsigned) xc.blue >> 8);

    /* WOAH!!! Returning pointer to static string!  Could be overwritten when
     * this function is called recursively (e.g., when some event occurs). */
    return retval;
}
#endif

/*
 * Get current y mouse coordinate in text window.
 * Return -1 when unknown.
 */
int
gui_mch_get_mouse_x(void)
{
    int winx, winy;
    GdkModifierType mask;

    gdk_window_get_pointer(gui.drawarea->window, &winx, &winy, &mask);
    return winx;
}

int
gui_mch_get_mouse_y(void)
{
    int winx, winy;
    GdkModifierType mask;

    gdk_window_get_pointer(gui.drawarea->window, &winx, &winy, &mask);
    return winy;
}

void
gui_mch_setmouse(int x, int y)
{
    /* Sorry for the Xlib call, but we can't avoid it, since there is no
     * internal GDK mechanism present to accomplish this.
     */
    XWarpPointer(GDK_DISPLAY(), (Window) 0,
		 GDK_WINDOW_XWINDOW(gui.drawarea->window), 0, 0, 0, 0, x, y);
}
