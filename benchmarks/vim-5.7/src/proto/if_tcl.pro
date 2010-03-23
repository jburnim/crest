/* if_tcl.c */
int do_tcl __ARGS((EXARG *eap));
int do_tclfile __ARGS((EXARG *eap));
int do_tcldo __ARGS((EXARG *eap));
void tcl_buffer_free __ARGS((BUF *buf));
void tcl_window_free __ARGS((WIN *win));
