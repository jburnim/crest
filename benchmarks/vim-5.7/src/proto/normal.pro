/* normal.c */
void normal_cmd __ARGS((OPARG *oap, int toplevel));
void do_pending_operator __ARGS((CMDARG *cap, char_u *searchbuff, int *command_busy, int old_col, int gui_yank, int dont_adjust_op_end));
int do_mouse __ARGS((OPARG *oap, int c, int dir, long count, int fix_indent));
void check_visual_highlight __ARGS((void));
void end_visual_mode __ARGS((void));
int find_ident_under_cursor __ARGS((char_u **string, int find_type));
void clear_showcmd __ARGS((void));
int add_to_showcmd __ARGS((int c));
void add_to_showcmd_c __ARGS((int c));
void push_showcmd __ARGS((void));
void pop_showcmd __ARGS((void));
void do_check_scrollbind __ARGS((int check));
void check_scrollbind __ARGS((linenr_t topline_diff, long leftcol_diff));
void scroll_redraw __ARGS((int up, long count));
void start_selection __ARGS((void));
void may_start_select __ARGS((int c));
