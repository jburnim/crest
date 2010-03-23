/* regexp.c */
char_u *skip_regexp __ARGS((char_u *p, int dirc, int magic));
vim_regexp *vim_regcomp __ARGS((char_u *expr, int magic));
int vim_regcomp_had_eol __ARGS((void));
int vim_regexec __ARGS((vim_regexp *prog, char_u *string, int at_bol));
char_u *regtilde __ARGS((char_u *source, int magic));
void vim_regnewptr __ARGS((vim_regexp *prog, char_u *old_ptr, char_u *new_ptr));
int vim_regsub __ARGS((vim_regexp *prog, char_u *source, char_u *dest, int copy, int magic));
