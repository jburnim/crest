/* mark.c */
int setmark __ARGS((int c));
void setpcmark __ARGS((void));
void checkpcmark __ARGS((void));
FPOS *movemark __ARGS((int count));
FPOS *getmark __ARGS((int c, int changefile));
void fmarks_check_names __ARGS((BUF *buf));
int check_mark __ARGS((FPOS *pos));
void clrallmarks __ARGS((BUF *buf));
char_u *fm_getname __ARGS((struct filemark *fmark, int lead_len));
void do_marks __ARGS((char_u *arg));
void do_jumps __ARGS((void));
void mark_adjust __ARGS((linenr_t line1, linenr_t line2, long amount, long amount_after));
void set_last_cursor __ARGS((WIN *win));
int read_viminfo_filemark __ARGS((char_u *line, FILE *fp, int force));
void write_viminfo_filemarks __ARGS((FILE *fp));
int removable __ARGS((char_u *name));
int write_viminfo_marks __ARGS((FILE *fp_out));
void copy_viminfo_marks __ARGS((char_u *line, FILE *fp_in, FILE *fp_out, int count, int eof));
