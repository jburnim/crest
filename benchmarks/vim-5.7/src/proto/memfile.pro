/* memfile.c */
MEMFILE *mf_open __ARGS((char_u *fname, int trunc_file));
int mf_open_file __ARGS((MEMFILE *mfp, char_u *fname));
void mf_close __ARGS((MEMFILE *mfp, int del_file));
void mf_close_file __ARGS((BUF *buf, int getlines));
BHDR *mf_new __ARGS((MEMFILE *mfp, int negative, int page_count));
BHDR *mf_get __ARGS((MEMFILE *mfp, blocknr_t nr, int page_count));
void mf_put __ARGS((MEMFILE *mfp, BHDR *hp, int dirty, int infile));
void mf_free __ARGS((MEMFILE *mfp, BHDR *hp));
int mf_sync __ARGS((MEMFILE *mfp, int flags));
int mf_release_all __ARGS((void));
blocknr_t mf_trans_del __ARGS((MEMFILE *mfp, blocknr_t old_nr));
void mf_set_ffname __ARGS((MEMFILE *mfp));
void mf_fullname __ARGS((MEMFILE *mfp));
int mf_need_trans __ARGS((MEMFILE *mfp));
