/* if_python.c */
int do_python __ARGS((EXARG *eap));
int do_pyfile __ARGS((EXARG *eap));
void python_buffer_free __ARGS((BUF *buf));
void python_window_free __ARGS((WIN *win));
char *Py_GetProgramName __ARGS((void));
