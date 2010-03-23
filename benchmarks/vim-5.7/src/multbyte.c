/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 * Multibyte extensions by Sung-Hoon Baek
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */
/*
 *	file : multbyte.c
 */

#include "vim.h"
#include "globals.h"
#include "option.h"
#ifdef WIN32
# include <windows.h>
#ifndef __MINGW32__
# include <winnls.h>
#endif
#endif
#ifdef USE_GUI_X11
# include <X11/Intrinsic.h>
#endif
#ifdef X_LOCALE
#include <X11/Xlocale.h>
#endif

#if defined(MULTI_BYTE) || defined(PROTO)

/*
 * Is 'c' a lead byte of multi-byte character?
 */
    int
IsLeadByte(c)
    int		c;
{
#ifdef WIN32
    /* is_dbcs is set by setting 'fileencoding'.  It becomes a Windows
     * CodePage identifier, which we can pass directly in to Windows API*/
    return IsDBCSLeadByteEx(is_dbcs, (BYTE)c);
#else
# if defined(BROKEN_LOCALE) || defined(macintosh)
    /*
     * if mblen is not available, character which MSB is turned on are treated
     * as leading byte character. (note : This assumption is not always true.)
     */
    return (c & 0x80);
# else
    char buf[2];
#  ifdef X_LOCALE
    extern int _Xmblen __ARGS((char *, size_t));
#   ifndef mblen
#    define mblen _Xmblen
#   endif
#  endif

    if (c == 0)
	return 0;
    buf[0] = c;
    buf[1] = 0;
    /*
     * mblen should return -1 for invalid (means the leading multibyte)
     * character.  However there are some platform that mblen returns 0 for
     * invalid character.  Therefore, following condition includes 0.
     */
    return mblen(buf, (size_t)1) <= 0;
# endif
#endif
}

/*
 * Is *p a trail byte of multi-byte character?  base : string pointer to line
 */
    int
IsTrailByte(base, p)
    char_u *base;
    char_u *p;
{
    char_u *q = base;

    if (p < base)
	return 0;
    while (q < p)
    {
	if (IsLeadByte(*q))
	    q++;
        q++;
    }
    return (q != p);
}

/*
 * if the cursor moves on an trail byte, set the cursor on the lead byte.
 */
    int
AdjustCursorForMultiByteChar()
{
    char_u *p;

    if (curwin->w_cursor.col > 0)
    {
	p = ml_get(curwin->w_cursor.lnum);
	if (IsTrailByte(p, p + curwin->w_cursor.col))
	{
	    --curwin->w_cursor.col;
	    return 1;
	}
    }
    return 0;
}

/*
 * count the length of the str which has multi-byte characters.  two-byte
 * character counts as one character.
 */
    int
MultiStrLen(str)
    char_u	*str;
{
    int count;

    if (str == NULL)
	return 0;
    for (count = 0; *str != NUL; count++)
    {
	if (IsLeadByte(*str))
	{
	    str++;
	    if (*str != NUL)
		str++;
	}
	else
	    str++;
    }
    return count;
}

    int
mb_dec(lp)
    FPOS  *lp;
{
    char_u *p = ml_get(lp->lnum);

    if (lp->col > 0)
    {		/* still within line */
	lp->col--;
	if ( lp->col > 0 && IsTrailByte(p, p + lp->col))
	    lp->col--;
	return 0;
    }
    if (lp->lnum > 1)
    {		/* there is a prior line */
	lp->lnum--;
	lp->col = STRLEN(ml_get(lp->lnum));
	if ( lp->col > 0 && IsTrailByte(p, p + lp->col))
	    lp->col--;
	return 1;
    }
    return -1;			/* at start of file */
}

    int
mb_isbyte1(buf, x)
    char_u	*buf;
    int		x;
{
    if (IsLeadByte(*(buf + x)))
    {
	return !IsTrailByte(buf, buf + x);
    }
    return 0;
}

    int
mb_isbyte2(buf, x)
    char_u	*buf;
    int		x;
{
    if (x > 0 && IsLeadByte(*(buf + x - 1)))
    {
	return IsTrailByte(buf, buf + x);
    }
    return 0;
}
#endif /* MULTI_BYTE */

#if defined(USE_XIM) || defined(PROTO)

static int	xim_has_focus = 0;
#ifdef USE_GUI_X11
static XIMStyle	input_style;
static int	status_area_enabled = TRUE;
#endif

#ifdef USE_GUI_GTK
static int xim_input_style;
static gboolean	use_status_area = 0;
#endif

    void
xim_set_focus(int focus)
{
    if (!xic)
	return;

    if (focus)
    {
	/* In Normal mode, only connect to IM if user uses over-the-spot. */
	if (!xim_has_focus
		&& (!(State & NORMAL)
#ifdef USE_GUI_GTK
		    || (xim_input_style & GDK_IM_PREEDIT_POSITION)
#else
		    || (input_style & XIMPreeditPosition)
#endif
		   ))
	{
	    xim_has_focus = 1;
#ifdef USE_GUI_GTK
	    gdk_im_begin(xic, gui.drawarea->window);
#else
	    XSetICFocus(xic);
#endif
	}
    }
    else
    {
	if (xim_has_focus)
	{
	    xim_has_focus = 0;
#ifdef USE_GUI_GTK
	    gdk_im_end();
#else
	    XUnsetICFocus(xic);
#endif
	}
    }
}

    void
xim_set_preedit()
{
    if (!xic)
	return;

    xim_set_focus(TRUE);

#ifdef USE_GUI_GTK
    if (gdk_im_ready())
    {
	GdkICAttributesType attrmask;
	GdkICAttr	    *attr;

	if (!xic_attr)
	    return;

	attr = xic_attr;
	attrmask = 0;

# ifdef USE_FONTSET
	if ((xim_input_style & GDK_IM_PREEDIT_POSITION)
	    && gui.fontset && gui.fontset->type == GDK_FONT_FONTSET)
	{
	    if (!xim_has_focus)
	    {
		if (attr->spot_location.y >= 0)
		{
		    attr->spot_location.x = 0;
		    attr->spot_location.y = -100;
		    attrmask |= GDK_IC_SPOT_LOCATION;
		}
	    }
	    else
	    {
		gint	width, height;

		if (attr->spot_location.x != TEXT_X(gui.col)
		    || attr->spot_location.y != TEXT_Y(gui.row))
		{
		    attr->spot_location.x = TEXT_X(gui.col);
		    attr->spot_location.y = TEXT_Y(gui.row);
		    attrmask |= GDK_IC_SPOT_LOCATION;
		}

		gdk_window_get_size(gui.drawarea->window, &width, &height);
		width -= 2 * gui.border_offset;
		height -= 2 * gui.border_offset;
		if (xim_input_style & GDK_IM_STATUS_AREA)
		    height -= gui.char_height;
		if (attr->preedit_area.width != width
		    || attr->preedit_area.height != height)
		{
		    attr->preedit_area.x = gui.border_offset;
		    attr->preedit_area.y = gui.border_offset;
		    attr->preedit_area.width = width;
		    attr->preedit_area.height = height;
		    attrmask |= GDK_IC_PREEDIT_AREA;
		}

		if (attr->preedit_fontset != gui.current_font)
		{
		    attr->preedit_fontset = gui.current_font;
		    attrmask |= GDK_IC_PREEDIT_FONTSET;
		}
	    }
	}
# endif /* USE_FONTSET */

	if (xim_fg_color < 0)
	{
	    xim_fg_color = gui.def_norm_pixel;
	    xim_bg_color = gui.def_back_pixel;
	}
	if (attr->preedit_foreground.pixel != xim_fg_color)
	{
	    attr->preedit_foreground.pixel = xim_fg_color;
	    attrmask |= GDK_IC_PREEDIT_FOREGROUND;
	}
	if (attr->preedit_background.pixel != xim_bg_color)
	{
	    attr->preedit_background.pixel = xim_bg_color;
	    attrmask |= GDK_IC_PREEDIT_BACKGROUND;
	}

	if (attrmask)
	    gdk_ic_set_attr(xic, attr, attrmask);
    }
#else /* USE_GUI_GTK */
    {
	XVaNestedList attr_list;
	XRectangle spot_area;
	XPoint over_spot;
	int line_space;

	if (!xim_has_focus)
	{
	    /* hide XIM cursor */
	    over_spot.x = 0;
	    over_spot.y = -100; /* arbitrary invisible position */
	    attr_list = (XVaNestedList) XVaCreateNestedList(0,
							    XNSpotLocation,
							    &over_spot,
							    NULL);
	    XSetICValues(xic, XNPreeditAttributes, attr_list, NULL);
	    XFree(attr_list);
	    return;
	}

	if (input_style & XIMPreeditPosition)
	{
	    if (xim_fg_color < 0)
	    {
		xim_fg_color = gui.def_norm_pixel;
		xim_bg_color = gui.def_back_pixel;
	    }
	    over_spot.x = TEXT_X(gui.col);
	    over_spot.y = TEXT_Y(gui.row);
	    spot_area.x = 0;
	    spot_area.y = 0;
	    spot_area.height = gui.char_height * Rows;
	    spot_area.width  = gui.char_width * Columns;
	    line_space = gui.char_height;
	    attr_list = (XVaNestedList) XVaCreateNestedList(0,
					    XNSpotLocation, &over_spot,
					    XNForeground, (Pixel) xim_fg_color,
					    XNBackground, (Pixel) xim_bg_color,
					    XNArea, &spot_area,
					    XNLineSpace, line_space,
					    NULL);
	    if (XSetICValues(xic, XNPreeditAttributes, attr_list, NULL))
		EMSG("Cannot set IC values");
	    XFree(attr_list);
	}
    }
#endif /* USE_GUI_GTK */
}

/*
 * Set up the status area.
 *
 * This should use a separate Widget, but that seems not possible, because
 * preedit_area and status_area should be set to the same window as for the
 * text input.  Unfortunately this means the status area pollutes the text
 * window...
 */
    void
xim_set_status_area()
{
    if (!xic)
	return;

#ifdef USE_GUI_GTK
# if defined(USE_FONTSET)
    if (use_status_area)
    {
	GdkICAttr   *attr;
	GdkIMStyle  style;
	gint	    width, height;
	GtkWidget   *widget;
	GdkICAttributesType attrmask;

	if (!xic_attr)
	    return;

	attr = xic_attr;
	attrmask = 0;
	style = gdk_ic_get_style(xic);
	if ((style & GDK_IM_STATUS_MASK) == GDK_IM_STATUS_AREA)
	{
	    if (gui.fontset && gui.fontset->type == GDK_FONT_FONTSET)
	    {
		widget = gui.mainwin;
		gdk_window_get_size(widget->window, &width, &height);

		attrmask |= GDK_IC_STATUS_AREA;
		attr->status_area.x = 0;
		attr->status_area.y = height - gui.char_height - 1;
		attr->status_area.width = width;
		attr->status_area.height = gui.char_height;
	    }
	}
	if (attrmask)
	    gdk_ic_set_attr(xic, attr, attrmask);
    }
# endif
#else
    {
	XVaNestedList preedit_list = 0, status_list = 0, list = 0;
	XRectangle pre_area, status_area;

	if (input_style & XIMStatusArea)
	{
	    if (input_style & XIMPreeditArea)
	    {
		XRectangle *needed_rect;

		/* to get status_area width */
		status_list = XVaCreateNestedList(0, XNAreaNeeded,
						  &needed_rect, NULL);
		XGetICValues(xic, XNStatusAttributes, status_list, NULL);
		XFree(status_list);

		status_area.width = needed_rect->width;
	    }
	    else
		status_area.width = gui.char_width * Columns;

	    status_area.x = 0;
	    status_area.y = gui.char_height * Rows + gui.border_offset;
	    if (gui.which_scrollbars[SBAR_BOTTOM])
		status_area.y += gui.scrollbar_height;
#if defined(WANT_MENU) && !defined(USE_GUI_GTK)
	    if (gui.menu_is_active)
		status_area.y += gui.menu_height;
#endif
	    status_area.height = gui.char_height;
	    status_list = XVaCreateNestedList(0, XNArea, &status_area, NULL);
	}
	else
	{
	    status_area.x = 0;
	    status_area.y = gui.char_height * Rows + gui.border_offset;
	    if (gui.which_scrollbars[SBAR_BOTTOM])
		status_area.y += gui.scrollbar_height;
#ifdef WANT_MENU
	    if (gui.menu_is_active)
		status_area.y += gui.menu_height;
#endif
	    status_area.width = 0;
	    status_area.height = gui.char_height;
	}

	if (input_style & XIMPreeditArea)   /* off-the-spot */
	{
	    pre_area.x = status_area.x + status_area.width;
	    pre_area.y = gui.char_height * Rows + gui.border_offset;
	    pre_area.width = gui.char_width * Columns - pre_area.x;
	    if (gui.which_scrollbars[SBAR_BOTTOM])
		pre_area.y += gui.scrollbar_height;
#ifdef WANT_MENU
	    if (gui.menu_is_active)
		pre_area.y += gui.menu_height;
#endif
	    pre_area.height = gui.char_height;
	    preedit_list = XVaCreateNestedList(0, XNArea, &pre_area, NULL);
	}
	else if (input_style & XIMPreeditPosition)   /* over-the-spot */
	{
	    pre_area.x = 0;
	    pre_area.y = 0;
	    pre_area.height = gui.char_height * Rows;
	    pre_area.width = gui.char_width * Columns;
	    preedit_list = XVaCreateNestedList(0, XNArea, &pre_area, NULL);
	}

	if (preedit_list && status_list)
	    list = XVaCreateNestedList(0, XNPreeditAttributes, preedit_list,
				       XNStatusAttributes, status_list, NULL);
	else if (preedit_list)
	    list = XVaCreateNestedList(0, XNPreeditAttributes, preedit_list,
				       NULL);
	else if (status_list)
	    list = XVaCreateNestedList(0, XNStatusAttributes, status_list,
				       NULL);
	else
	    list = NULL;

	if (list)
	{
	    XSetICValues(xic, XNVaNestedList, list, NULL);
	    XFree(list);
	}
	if (status_list)
	    XFree(status_list);
	if (preedit_list)
	    XFree(preedit_list);
    }
#endif
}

#if defined(USE_GUI_X11) || defined(PROTO)
# if defined(XtSpecificationRelease) && XtSpecificationRelease >= 6
#  define USE_X11R6_XIM
# endif

static int xim_real_init __ARGS((Window x11_window, Display *x11_display));

#ifdef USE_X11R6_XIM
static void xim_instantiate_cb __ARGS((Display *display, XPointer client_data, XPointer	call_data));
static void xim_destroy_cb __ARGS((XIM im, XPointer client_data, XPointer call_data));

/*ARGSUSED*/
    static void
xim_instantiate_cb(display, client_data, call_data)
    Display	*display;
    XPointer	client_data;
    XPointer	call_data;
{
    Window	x11_window;
    Display	*x11_display;

    gui_get_x11_windis(&x11_window, &x11_display);
    if (display != x11_display)
	return;

    xim_real_init(x11_window, x11_display);
    gui_set_winsize(FALSE);
    if (xic != NULL)
	XUnregisterIMInstantiateCallback(x11_display, NULL, NULL, NULL,
					 xim_instantiate_cb, NULL);
}

/*ARGSUSED*/
    static void
xim_destroy_cb(im, client_data, call_data)
    XIM		im;
    XPointer	client_data;
    XPointer	call_data;
{
    Window	x11_window;
    Display	*x11_display;

    gui_get_x11_windis(&x11_window, &x11_display);

    xic = NULL;
    status_area_enabled = FALSE;

    gui_set_winsize(FALSE);

    XRegisterIMInstantiateCallback(x11_display, NULL, NULL, NULL,
				   xim_instantiate_cb, NULL);
}
#endif

    void
xim_init()
{
    Window	x11_window;
    Display	*x11_display;

    gui_get_x11_windis(&x11_window, &x11_display);

    xic = NULL;

    if (xim_real_init(x11_window, x11_display))
	return;

    gui_set_winsize(FALSE);

#ifdef USE_X11R6_XIM
    XRegisterIMInstantiateCallback(x11_display, NULL, NULL, NULL,
				   xim_instantiate_cb, NULL);
#endif
}

    static int
xim_real_init(x11_window, x11_display)
    Window  x11_window;
    Display *x11_display;
{
    int		i;
    char	*p,
		*s,
		*ns,
		*end,
		tmp[1024],
		buf[32];
    XIM		xim = NULL;
    XIMStyles	*xim_styles;
    XIMStyle	this_input_style = 0;
    Boolean	found;
    XPoint	over_spot;
    XVaNestedList preedit_list, status_list;

    input_style = 0;
    status_area_enabled = FALSE;

#ifdef USE_FONTSET
    if (!gui.fontset)
    {
	/* This message is annoying when Vim was compiled with XIM support but
	 * it's not being used.
	 * EMSG("XIM requires guifontset setting");
	 */
	return FALSE;
    }
#else
    EMSG("XIM requires VIM compiled with +fontset feature.");
    return FALSE;
#endif

    if (xic != NULL)
	return FALSE;

    if (!gui.input_method || !*gui.input_method)
    {
	if ((p = XSetLocaleModifiers("")) != NULL && *p)
	    xim = XOpenIM(x11_display, NULL, NULL, NULL);
    }
    else
    {
	strcpy(tmp, gui.input_method);
	for (ns = s = tmp; ns && *s;)
	{
	    while (*s && isspace((unsigned char)*s))
		s++;
	    if (!*s)
		break;
	    if ((ns = end = strchr(s, ',')) == 0)
		end = s + strlen(s);
	    while (isspace((unsigned char)*end))
		end--;
	    *end = '\0';

	    strcpy(buf, "@im=");
	    strcat(buf, s);
	    if ((p = XSetLocaleModifiers(buf)) != NULL
		&& *p
		&& (xim = XOpenIM(x11_display, NULL, NULL, NULL)) != NULL)
		break;

	    s = ns + 1;
	}
    }

    if (xim == NULL && (p = XSetLocaleModifiers("")) != NULL && *p)
	xim = XOpenIM(x11_display, NULL, NULL, NULL);

    if (!xim)
	xim = XOpenIM(x11_display, NULL, NULL, NULL);

    if (!xim)
    {
	EMSG("Failed to open input method");
	return FALSE;
    }

#ifdef USE_X11R6_XIM
    {
	XIMCallback destroy_cb;

	destroy_cb.callback = xim_destroy_cb;
	destroy_cb.client_data = NULL;
	if (XSetIMValues(xim, XNDestroyCallback, &destroy_cb, NULL))
	    EMSG("Warning: Could not set destroy callback to IM");
    }
#endif

    if (XGetIMValues(xim, XNQueryInputStyle, &xim_styles, NULL) || !xim_styles)
    {
	EMSG("input method doesn't support any style");
	XCloseIM(xim);
	return FALSE;
    }

    found = False;
    strcpy(tmp, gui.preedit_type);
    for (s = tmp; s && !found; )
    {
	while (*s && isspace((unsigned char)*s))
	    s++;
	if (!*s)
	    break;
	if ((ns = end = strchr(s, ',')) != 0)
	    ns++;
	else
	    end = s + strlen(s);
	while (isspace((unsigned char)*end))
	    end--;
	*end = '\0';

	if (!strcmp(s, "OverTheSpot"))
	    this_input_style = (XIMPreeditPosition | XIMStatusArea);
	else if (!strcmp(s, "OffTheSpot"))
	    this_input_style = (XIMPreeditArea | XIMStatusArea);
	else if (!strcmp(s, "Root"))
	    this_input_style = (XIMPreeditNothing | XIMStatusNothing);

	for (i = 0; (unsigned short)i < xim_styles->count_styles; i++)
	{
	    if (this_input_style == xim_styles->supported_styles[i])
	    {
		found = True;
		break;
	    }
	    if ((xim_styles->supported_styles[i] & this_input_style)
			== (this_input_style & ~XIMStatusArea))
	    {
		this_input_style &= ~XIMStatusArea;
		found = True;
		break;
	    }
	}

	s = ns;
    }
    XFree(xim_styles);

    if (!found)
    {
	EMSG("input method doesn't support my preedit type");
	XCloseIM(xim);
	return FALSE;
    }

    over_spot.x = TEXT_X(gui.col);
    over_spot.y = TEXT_Y(gui.row);
    input_style = this_input_style;
    preedit_list = XVaCreateNestedList(0,
			    XNSpotLocation, &over_spot,
			    XNForeground, (Pixel)gui.def_norm_pixel,
			    XNBackground, (Pixel)gui.def_back_pixel,
			    XNFontSet, (XFontSet)gui.norm_font,
			    NULL);
    status_list = XVaCreateNestedList(0,
			    XNForeground, (Pixel)gui.def_norm_pixel,
			    XNBackground, (Pixel)gui.def_back_pixel,
			    XNFontSet, (XFontSet)gui.norm_font,
			    NULL);
    xic = XCreateIC(xim,
		    XNInputStyle, input_style,
		    XNClientWindow, x11_window,
		    XNFocusWindow, gui.wid,
		    XNPreeditAttributes, preedit_list,
		    XNStatusAttributes, status_list,
		    NULL);
    XFree(status_list);
    XFree(preedit_list);
    if (xic)
    {
	if (input_style & XIMStatusArea)
	{
	    xim_set_status_area();
	    status_area_enabled = TRUE;
	}
	else
	    gui_set_winsize(FALSE);
    }
    else
    {
	EMSG("Failed to create input context");
	XCloseIM(xim);
	return FALSE;
    }

    return TRUE;
}

#endif /* USE_GUI_X11 */

#if defined(USE_GUI_GTK) || defined(PROTO)

void
xim_decide_input_style()
{
    GdkIMStyle supported_style = GDK_IM_PREEDIT_NONE |
				 GDK_IM_PREEDIT_NOTHING |
				 GDK_IM_PREEDIT_POSITION |
				 GDK_IM_STATUS_AREA |
				 GDK_IM_STATUS_NONE |
				 GDK_IM_STATUS_NOTHING;

    if (!gdk_im_ready()) {
	xim_input_style = 0;
    } else {
	if (gtk_major_version > 1
	    || (gtk_major_version == 1
		&& (gtk_minor_version > 2
		    || (gtk_minor_version == 2 && gtk_micro_version >= 3))))
	    use_status_area = TRUE;
	else
	{
	    EMSG("Your GTK+ is older than 1.2.3. Status area disabled");
	    use_status_area = FALSE;
	}
#ifdef USE_FONTSET
	if (!gui.fontset || gui.fontset->type != GDK_FONT_FONTSET)
#endif
	    supported_style &= ~(GDK_IM_PREEDIT_POSITION | GDK_IM_STATUS_AREA);
	if (!use_status_area)
	    supported_style &= ~GDK_IM_STATUS_AREA;
	xim_input_style = gdk_im_decide_style(supported_style);
    }
}

    void
xim_init()
{
    xic = NULL;
    xic_attr = NULL;

    if (!gdk_im_ready())
    {
	EMSG("Input Method Server is not running");
	return;
    }
    if ((xic_attr = gdk_ic_attr_new()) != NULL)
    {
	gint width, height;
	GdkEventMask mask;
	GdkColormap *colormap;
	GdkICAttr *attr = xic_attr;
	GdkICAttributesType attrmask = GDK_IC_ALL_REQ;
	GtkWidget *widget = gui.drawarea;

	attr->style = xim_input_style;
	attr->client_window = gui.mainwin->window;

	if ((colormap = gtk_widget_get_colormap(widget)) !=
	    gtk_widget_get_default_colormap())
	{
	    attrmask |= GDK_IC_PREEDIT_COLORMAP;
	    attr->preedit_colormap = colormap;
	}
	attrmask |= GDK_IC_PREEDIT_FOREGROUND;
	attrmask |= GDK_IC_PREEDIT_BACKGROUND;
	attr->preedit_foreground = widget->style->fg[GTK_STATE_NORMAL];
	attr->preedit_background = widget->style->base[GTK_STATE_NORMAL];

#ifdef USE_FONTSET
	if ((xim_input_style & GDK_IM_PREEDIT_MASK) == GDK_IM_PREEDIT_POSITION)
	{
	    if (!gui.fontset || gui.fontset->type != GDK_FONT_FONTSET)
	    {
		EMSG("over-the-spot style requires fontset");
	    }
	    else
	    {
		gdk_window_get_size(widget->window, &width, &height);

		attrmask |= GDK_IC_PREEDIT_POSITION_REQ;
		attr->spot_location.x = TEXT_X(0);
		attr->spot_location.y = TEXT_Y(0);
		attr->preedit_area.x = gui.border_offset;
		attr->preedit_area.y = gui.border_offset;
		attr->preedit_area.width = width - 2*gui.border_offset;
		attr->preedit_area.height = height - 2*gui.border_offset;
		attr->preedit_fontset = gui.fontset;
	    }
	}

	if ((xim_input_style & GDK_IM_STATUS_MASK) == GDK_IM_STATUS_AREA)
	{
	    if (!gui.fontset || gui.fontset->type != GDK_FONT_FONTSET)
	    {
		EMSG("over-the-spot style requires fontset");
	    }
	    else
	    {
		gdk_window_get_size(gui.mainwin->window, &width, &height);
		attrmask |= GDK_IC_STATUS_AREA_REQ;
		attr->status_area.x = 0;
		attr->status_area.y = height - gui.char_height - 1;
		attr->status_area.width = width;
		attr->status_area.height = gui.char_height;
		attr->status_fontset = gui.fontset;
	    }
	}
#endif

	xic = gdk_ic_new(attr, attrmask);

	if (xic == NULL)
	    EMSG("Can't create input context.");
	else
	{
	    mask = gdk_window_get_events(widget->window);
	    mask |= gdk_ic_get_events(xic);
	    gdk_window_set_events(widget->window, mask);
	}
    }
}
#endif /* USE_GUI_GTK */

    int
xim_get_status_area_height(void)
{
#if defined(USE_FONTSET)
# ifdef USE_GUI_GTK
    if (xim_input_style & GDK_IM_STATUS_AREA)
	return gui.char_height;
# else
    if (status_area_enabled)
	return gui.char_height;
# endif
#endif
    return 0;
}

#endif /* USE_XIM */
