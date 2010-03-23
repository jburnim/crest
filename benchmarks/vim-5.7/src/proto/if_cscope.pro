/* if_cscope.c */
void do_cscope __ARGS((EXARG *eap));
void do_cstag __ARGS((EXARG *eap));
int cs_fgets __ARGS((char_u *buf, int size));
void cs_free_tags __ARGS((void));
void cs_print_tags __ARGS((void));
