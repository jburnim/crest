/* quickfix.c */
int qf_init __ARGS((char_u *efile, char_u *errorformat));
void qf_jump __ARGS((int dir, int errornr, int forceit));
void qf_list __ARGS((char_u *arg, int all));
void qf_older __ARGS((int count));
void qf_newer __ARGS((int count));
void qf_mark_adjust __ARGS((linenr_t line1, linenr_t line2, long amount, long amount_after));
