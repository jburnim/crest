/* multbyte.c */
int IsLeadByte __ARGS((int c));
int IsTrailByte __ARGS((char_u *base, char_u *p));
int AdjustCursorForMultiByteChar __ARGS((void));
int MultiStrLen __ARGS((char_u *str));
int mb_dec __ARGS((FPOS *lp));
int mb_isbyte1 __ARGS((char_u *buf, int x));
int mb_isbyte2 __ARGS((char_u *buf, int x));
void xim_set_focus __ARGS((int focus));
void xim_set_preedit __ARGS((void));
void xim_set_status_area __ARGS((void));
void xim_init __ARGS((void));
void xim_decide_input_style __ARGS((void));
void xim_init __ARGS((void));
int xim_get_status_area_height __ARGS((void));
