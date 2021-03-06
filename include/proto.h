/* $Id$ */
#ifndef INCLUDE_PROTO_H
#define INCLUDE_PROTO_H

#ifdef __GNUC__
#define GCC_CHECK_FORMAT(a,b) __attribute__ ((format (printf, a, b)))
#define GCC_NORETURN          __attribute__ ((__noreturn__))
#else
#define GCC_CHECK_FORMAT(a,b)
#define GCC_NORETURN
#endif

#ifdef __dietlibc__
#define random     glibc_random
#define srandom            glibc_srandom
#define initstate   glibc_initstate
#define setstate    glibc_setstate
long int random(void);
void srandom(unsigned int seed);
char *initstate(unsigned int seed, char *state, size_t n);
char *setstate(char *state);
#endif
 
/* admin */
int m_loginmsg(void);
int m_mod_board(char *bname);
int m_newbrd(int recover);
int scan_register_form(const char *regfile, int automode, int neednum);
int m_user(void);
int search_user_bypwd(void);
int search_user_bybakpwd(void);
int m_board(void);
int m_register(void);
int cat_register(void);
unsigned int setperms(unsigned int pbits, const char * const pstring[]);
void setup_man(const boardheader_t * board, const boardheader_t * oldboard);
void delete_symbolic_link(boardheader_t *bh, int bid);
int make_symbolic_link(const char *bname, int gid);
int make_symbolic_link_interactively(int gid);
void merge_dir(const char *dir1, const char *dir2, int isoutter);

/* announce */
int a_menu(const char *maintitle, char *path, int lastlevel, char *trans_buffer);
void a_copyitem(const char* fpath, const char* title, const char* owner, int mode);
int Announce(void);
void gem(char* maintitle, item_t* path, int update);
#ifdef BLOG
void BlogMain(int);
#endif

/* args */
void initsetproctitle(int argc, char **argv, char **envp);
void setproctitle(const char* format, ...) GCC_CHECK_FORMAT(1,2);

/* assess */
int inc_goodpost(const char *, int num);
int inc_badpost(const char *, int num);
int inc_goodsale(const char *, int num);
int inc_badsale(const char *, int num);
//void set_assess(int uid, unsigned char num, int type);

/* bbs */
int invalid_brdname(const char *brd);
void chomp(char *src);
int del_range(int ent, const fileheader_t *fhdr, const char *direct);
int cmpfowner(fileheader_t *fhdr);
int b_note_edit_bname(int bid);
int Read(void);
int CheckPostPerm(void);
void anticrosspost(void);
int Select(void);
void do_reply_title(int row, const char *title);
void outgo_post(const fileheader_t *fh, const char *board, const char *userid, const char *username);
int edit_title(int ent, fileheader_t *fhdr, const char *direct);
int whereami(int ent, const fileheader_t *fhdr, const char *direct);
void set_board(void);
int do_post(void);
void ReadSelect(void);
int save_violatelaw(void);
int board_select(void);
int board_digest(void);
int do_limitedit(int ent, fileheader_t * fhdr, const char *direct);

/* board */
#define setutmpbid(bid) currutmp->brc_id=bid;
int HasPerm(boardheader_t *bptr);
int New(void);
int Boards(void);
int root_board(void);
void save_brdbuf(void);
void init_brdbuf(void);
#ifdef CRITICAL_MEMORY
void sigfree(int);
#endif

/* brc */
int brc_initialize(void);
void brc_finalize(void);
int brc_unread(const char *fname, int bnum, const time4_t *blist);
int brc_unread_time(time4_t ftime, int bnum, const time4_t *blist);
int brc_initial_board(const char *boardname);
void brc_update(void);
int brc_read_record(int bid, int *num, time4_t *list);
time4_t * brc_find_record(int bid, int *num);
void brc_trunc(int bid, time4_t ftime);
void brc_addlist(const char* fname);

/* cache */
#define demoney(money) deumoney(usernum, money)
#define search_ulist(uid) search_ulistn(uid, 1)
#define getbcache(bid) (bcache + bid - 1)
#define moneyof(uid) SHM->money[uid - 1]
#define getbtotal(bid) SHM->total[bid - 1]
#define getbottomtotal(bid) SHM->n_bottom[bid-1]
void sort_bcache(void);
int getuser(const char *userid, userec_t *xuser);
void setuserid(int num, const char *userid);
int searchuser(const char *userid, char *rightid);
int getbnum(const char *bname);
void reset_board(int bid);
void touch_boards(void);
void addbrd_touchcache(void);
void setapath(char *buf, const char *boardname);
void setutmpmode(unsigned int mode);
void setadir(char *buf, const char *path);
int apply_boards(int (*func)(boardheader_t *));
int haspostperm(const char *bname);
void setbtotal(int bid);
void setbottomtotal(int bid);
unsigned int safe_sleep(unsigned int seconds);
int apply_ulist(int (*fptr)(const userinfo_t *));
userinfo_t *search_ulistn(int uid, int unum);
void purge_utmp(userinfo_t *uentp);
void getnewutmpent(const userinfo_t *up);
void resolve_garbage(void);
void resolve_boards(void);
void resolve_fcache(void);
void sem_init(int semkey,int *semid);
void sem_lock(int op,int semid);
char *u_namearray(char buf[][IDLEN + 1], int *pnum, char *tag);
char *getuserid(int num);
int searchnewuser(int mode);
int count_logins(int uid, int show);
void remove_from_uhash(int n);
void add_to_uhash(int n, const char *id);
int setumoney(int uid, int money);
userinfo_t *search_ulist_pid(int pid);
userinfo_t *search_ulist_userid(const char *userid);
void hbflreload(int bid);
int hbflcheck(int bid, int uid);
void *attach_shm(int shmkey, int shmsize);
void attach_SHM(void);
int is_BM_cache(int);
void buildBMcache(int);
void reload_bcache(void);
void reload_fcache(void);
#ifdef USE_COOLDOWN
#define cooldowntimeof(uid) SHM->cooldowntime[uid - 1]
void add_cooldowntime(int uid, int min);
#endif

/* cal */
int give_tax(int money);
int vice(int money, const char* item);
#define reload_money()  cuser.money=moneyof(usernum)
int deumoney(int uid, int money);
int lockutmpmode(int unmode, int state);
int unlockutmpmode(void);
int x_file(void);
int give_money(void);
int p_sysinfo(void);
int p_give(void);
int p_cloak(void);
int p_from(void);
int ordersong(void);
int p_exmail(void);
void mail_redenvelop(const char* from, const char* to, int money, char mode);

/* card */
int g_card_jack(void);
int g_ten_helf(void);
int card_99(void);

/* chat */
int t_chat(void);

/* chc */
void chc(int s, int mode);
int chc_main(void);
int chc_personal(void);
int chc_watch(void);

/* chicken */
void ch_buyitem(int money, const char *picture, int *item, int haveticket);
int chicken_main(void);
int chickenpk(int fd);
void time_diff(chicken_t *thechicken);
int isdeadth(const chicken_t *thechicken);
void show_chicken_data(chicken_t *thechicken, chicken_t *pkchicken);
int reload_chicken(void);

/* dark */
int main_dark(int fd,userinfo_t *uin);

/* dice */
int dice_main(void);

/* edit */
int vedit(char *fpath, int saveheader, int *islocal);
void write_header(FILE *fp, int ifuseanony);
void addsignature(FILE *fp, int ifuseanony);
void auto_backup(void);
void restore_backup(void);
char *ask_tmpbuf(int y);

/* fav */
void fav_set_old_folder(fav_t *fp);
int get_data_number(fav_t *fp);
int get_current_fav_level(void);
fav_t *get_current_fav(void);
int get_item_type(fav_type_t *ft);
char *get_item_title(fav_type_t *ft);
char *get_folder_title(int fid);
void set_attr(fav_type_t *ft, int bit, char bool);
void fav_sort_by_name(void);
void fav_sort_by_class(void);
int fav_load(void);
int fav_save(void);
void fav_remove_item(short id, char type);
fav_type_t *getadmtag(short bid);
fav_type_t *getboard(short bid);
fav_type_t *getfolder(short fid);
char getbrdattr(short bid);
time4_t getbrdtime(short bid);
void setbrdtime(short bid, time4_t t);
int fav_getid(fav_type_t *ft);
void fav_tag(short id, char type, char bool);
void move_in_current_folder(int from, int to);
void fav_move(int from, int to);
fav_type_t *fav_add_line(void);
fav_type_t *fav_add_folder(void);
fav_type_t *fav_add_board(int bid);
fav_type_t *fav_add_admtag(int bid);
void fav_remove_all_tagged_item(void);
void fav_add_all_tagged_item(void);
void fav_remove_all_tag(void);
void fav_set_folder_title(fav_type_t *ft, char *title);
int fav_stack_full(void);
void fav_folder_in(short fid);
void fav_folder_out(void);
void fav_free(void);
int fav_v3_to_v4(void);
int is_set_attr(fav_type_t *ft, char bit);
void fav_cleanup(void);
void fav_clean_invisible(void);
fav_t *get_fav_folder(fav_type_t *ft);
fav_t *get_fav_root(void);
void updatenewfav(int mode);
void subscribe_newfav(void);

/* file */
int file_count_line(const char *file);
int file_append_line(const char *file, const char *string);
int file_delete_line(const char *file, const char *string, int  case_sensitive);
int file_exist_record(const char *file, const char *string);

/* friend */
void friend_edit(int type);
void friend_load(int);
int t_override(void);
int t_reject(void);
void friend_add(const char *uident, int type, const char *des);
void friend_delete(const char *uident, int type);
void friend_delete_all(const char *uident, int type);
void friend_special(void);
void setfriendfile(char *fpath, int type);

/* gamble */
int ticket_main(void);
int openticket(int bid);
int ticket(int bid);

/* go */
int gochess(int fd);
int GoBot(void);

/* gomo */
int gomoku(int fd);

/* guess */
int guess_main(void);

/* indict */
int x_dict(void);
int use_dict(char *dict,char *database);

/* convert */
void set_converting_type(int which);

/* io */
int getdata(int line, int col, const char *prompt, char *buf, int len, int echo);
int igetch(void);
int getdata_str(int line, int col, const char *prompt, char *buf, int len, int echo, const char *defaultstr);
int getdata_buf(int line, int col, const char *prompt, char *buf, int len, int echo);
void add_io(int fd, int timeout);
void oflush(void);
int strip_ansi(char *buf, const char *str, int mode);
void strip_nonebig5(unsigned char *str, int maxlen);
int oldgetdata(int line, int col, const char *prompt, char *buf, int len, int echo);
void output(const char *s, int len);
int num_in_buf(void);
int ochar(int c);

/* kaede */
int Rename(const char* src, const char* dst);
int Copy(const char *src, const char *dst);
int Link(const char* src, const char* dst);
char *Ptt_prints(char *str, int mode);
char *my_ctime(const time4_t *t, char *ans, int len);

/* lovepaper */
int x_love(void);

/* mail */
int load_mailalert(const char *userid);
int mail_muser(const userec_t muser, const char *title, const char *filename);
int mail_id(const char* id, const char *title, const char *filename, const char *owner);
int m_read(void);
int doforward(const char *direct, const fileheader_t *fh, int mode);
int mail_reply(int ent, const fileheader_t *fhdr, const char *direct);
int bsmtp(const char *fpath, const char *title, const char *rcpt, int method);
void hold_mail(const char *fpath, const char *receiver);
void m_init(void);
int chkmailbox(void);
int mail_man(void);
int m_new(void);
int m_send(void);
int mail_list(void);
int setforward(void);
int m_internet(void);
int mail_mbox(void);
int built_mail_index(void);
int mail_all(void);
int invalidaddr(const char *addr);
int do_send(const char *userid, const char *title);
void my_send(const char *uident);
void setupmailusage(void);

/* mbbsd */
void show_call_in(int save, int which);
void write_request (int sig);
void log_usies(const char *mode, const char *mesg);
void system_abort(void);
void abort_bbs(int sig) GCC_NORETURN;
void del_distinct(const char *fname, const char *line);
void add_distinct(const char *fname, const char *line);
void u_exit(const char *mode);
void talk_request(int sig);
int reply_connection_request(const userinfo_t *uip);
int establish_talk_connection(const userinfo_t *uip);
void my_talk(userinfo_t * uin, int fri_stat, char defact);

/* menu */
void showtitle(const char *title, const char *mid);
void movie(int i);
void domenu(int cmdmode, const char *cmdtitle, int cmd, const commands_t cmdtable[]);
int admin(void);
int Mail(void);
int Talk(void);
int User(void);
int Xyz(void);
int Play_Play(void);
int Name_Menu(void);

#ifdef MERGEBBS
/* merge */
int m_sob(void);
void m_sob_brd(char *bname,char *fromdir);
#endif

/* more */
int more(char *fpath, int promptend);

/* name */
typedef int (*gnc_comp_func)(int, const char*, int);
typedef int (*gnc_perm_func)(int);
typedef char* (*gnc_getname_func)(int);

void usercomplete(const char *prompt, char *data);
void namecomplete(const char *prompt, char *data);
void AddNameList(const char *name);
void FreeNameList(void);
void CreateNameList(void);
int chkstr(char *otag, const char *tag, const char *name);
int InNameList(const char *name);
void ShowNameList(int row, int column, const char *prompt);
int RemoveNameList(const char *name);
void ToggleNameList(int *reciper, const char *listfile, const char *msg);
int generalnamecomplete(const char *prompt, char *data, int len, size_t nmemb,
		       gnc_comp_func compar, gnc_perm_func permission,
		       gnc_getname_func getname);
int completeboard_compar(int where, const char *str, int len);
int completeboard_permission(int where);
int complete_board_and_group_permission(int where);
char *completeboard_getname(int where);
int completeutmp_compar(int where, const char *str, int len);
int completeutmp_permission(int where);
char *completeutmp_getname(int where);

#define CompleteBoard(MSG,BUF) \
    generalnamecomplete(MSG, BUF, sizeof(BUF), SHM->Bnumber, \
      	&completeboard_compar, &completeboard_permission, \
	&completeboard_getname)
#define CompleteBoardAndGroup(MSG,BUF) \
    generalnamecomplete(MSG, BUF, sizeof(BUF), SHM->Bnumber, \
	&completeboard_compar, &complete_board_and_group_permission, \
	&completeboard_getname)
#define CompleteOnlineUser(MSG,BUF) \
    generalnamecomplete(MSG, BUF, sizeof(BUF), SHM->UTMPnumber, \
	&completeutmp_compar, &completeutmp_permission, \
	&completeutmp_getname)

/* osdep */
int cpuload(char *str);
double swapused(int *total, int *used);

#ifdef NEED_FLOCK
    #define LOCK_EX 1
    #define LOCK_UN 2

    int flock(int, int);
#endif

#ifdef NEED_UNSETENV
    void unsetenv(char *name);
#endif

#ifdef NEED_STRCASESTR
    char *strcasestr(const char *big, const char *little);
#endif

#ifdef NEED_STRLCPY
    size_t strlcpy(char *dst, const char *src, size_t size);
#endif

#ifdef NEED_STRLCAT
    size_t strlcat(char *dst, const char *src, size_t size);
#endif

#ifdef NEED_SCANDIR
    int scandir(const char *dirname, struct dirent ***namelist, int (*select)(struct dirent *), int (*compar)(const void *, const void *));
    int alphasort(const void *d1, const void *d2);
#endif

#ifdef NEED_INET_PTON
    int inet_pton(int af, const char *src, void *dst);
#endif

/* othello */
int othello_main(void);

/* page */
int main_railway(void);

/* read */
void i_read(int cmdmode, const char *direct, void (*dotitle)(), void (*doentry)(), const onekey_t *rcmdlist, int bidcache);
void fixkeep(const char *s, int first);
keeploc_t *getkeep(const char *s, int def_topline, int def_cursline);
int Tagger(time4_t chrono, int recno, int mode);
void EnumTagFhdr(fileheader_t *fhdr, char *direct, int locus);
void UnTagger (int locus);
/* record */
int substitute_record(const char *fpath, const void *rptr, int size, int id);
int lock_substitute_record(const char *fpath, void *rptr, int size, int id, int);
int get_record(const char *fpath, void *rptr, int size, int id);
int get_record_keep(const char *fpath, void *rptr, int size, int id, int *fd);
int append_record(const char *fpath, const fileheader_t *record, int size);
int stampfile(char *fpath, fileheader_t *fh);
void stampdir(char *fpath, fileheader_t *fh);
int get_num_records(const char *fpath, int size);
int get_records(const char *fpath, void *rptr, int size, int id, int number);
void stamplink(char *fpath, fileheader_t *fh);
int delete_record(const char fpath[], int size, int id);
int delete_files(const char* dirname, int (*filecheck)(), int record);
#ifdef SAFE_ARTICLE_DELETE
#ifndef _BBS_UTIL_C_
void safe_delete_range(const char *fpath, int id1, int id2);
#endif
int safe_article_delete(int ent, const fileheader_t *fhdr, const char *direct);
int safe_article_delete_range(const char *direct, int from, int to);
#endif
int delete_file(const char *dirname, int size, int ent, int (*filecheck)());
int delete_range(const char *fpath, int id1, int id2);
int apply_record(const char *fpath, int (*fptr)(void*,void*), int size,void *arg);
int search_rec(const char* dirname, int (*filecheck)());
int append_record_forward(char *fpath, fileheader_t *record, int size);
int get_sum_records(const char* fpath, int size);
int substitute_ref_record(const char* direct, fileheader_t *fhdr, int ent);
int getindex(const char *fpath, fileheader_t *fh, int start);

/* register */
int getnewuserid(void);
int bad_user_id(const char *userid);
void new_register(void);
int checkpasswd(const char *passwd, char *test);
void check_register(void);
char *genpasswd(char *pw);

/* screen */
void mouts(int y, int x, const char *str);
void move(int y, int x);
void outs(const char *str);
void outs_n(const char *str, int n);
void clrtoeol(void);
void clear(void);
void refresh(void);
void clrtobot(void);
void outmsg(const char *msg);
void prints(const char *fmt, ...) GCC_CHECK_FORMAT(1,2);
void region_scroll_up(int top, int bottom);
void outc(unsigned char ch);
void redoscr(void);
void redoln(void);
void clrtoline(int line);
void standout(void);
void standend(void);
void edit_outs(const char *text);
void edit_outs_n(const char *text, int n);
void outch(unsigned char c);
void rscroll(void);
void scroll(void);
void getyx(int *y, int *x);
void initscr(void);
void out_lines(const char *str, int line);
void screen_backup(int len, const screenline_t *bp, void *buf);
size_t screen_backupsize(int len, const screenline_t *bp);
void screen_restore(int len, screenline_t *bp, const void *buf);

/* stuff */
#define isprint2(ch) ((ch & 0x80) || isprint(ch))
#define not_alpha(ch) (ch < 'A' || (ch > 'Z' && ch < 'a') || ch > 'z')
#define not_alnum(ch) (ch < '0' || (ch > '9' && ch < 'A') || (ch > 'Z' && ch < 'a') || ch > 'z')
#define pressanykey() vmsg_lines(b_lines, NULL)
int vmsg_lines(int lines, const char *msg);
int log_user(const char *fmt, ...) GCC_CHECK_FORMAT(1,2);
unsigned int ipstr2int(const char *ip);
time4_t gettime(int line, time4_t dt, const char* head);
void setcalfile(char *buf, char *userid);
void stand_title(const char *title);
char getans(const char *fmt,...) GCC_CHECK_FORMAT(1,2);
int getkey(const char *fmt,...) GCC_CHECK_FORMAT(1,2);
int vmsg(const char *fmt,...) GCC_CHECK_FORMAT(1,2);
void trim(char *buf);
int show_file(const char *filename, int y, int lines, int mode);
void bell(void);
void setbpath(char *buf, const char *boardname);
int dashf(const char *fname);
void sethomepath(char *buf, const char *userid);
void sethomedir(char *buf, const char *userid);
char *Cdate(const time4_t *clock);
void sethomefile(char *buf, const char *userid, const char *fname);
int log_file(const char *fn, int flag, const char *fmt,...);
void str_lower(char *t, const char *s);
int cursor_key(int row, int column);
int search_num(int ch, int max);
void setuserfile(char *buf, const char *fname);
int is_BM(const char *list);
time4_t dasht(const char *fname);
int dashd(const char *fname);
int invalid_pname(const char *str);
void setbdir(char *buf, const char *boardname);
void setbfile(char *buf, const char *boardname, const char *fname);
void setbnfile(char *buf, const char *boardname, const char *fname, int n);
int dashl(const char *fname);
char *subject(char *title);
void setdirpath(char *buf, const char *direct, const char *fname);
int str_checksum(const char *str);
void show_help(const char * const helptext[]);
void show_helpfile(const char * helpfile);
int copy_file(const char *src, const char *dst);
int belong(const char *filelist, const char *key);
char *Cdatedate(const time4_t *clock);
void sethomeman(char *buf, const char *userid);
off_t dashs(const char *fname);
void cursor_clear(int row, int column);
void cursor_show(int row, int column);
void printdash(const char *mesg);
char *Cdatelite(const time4_t *clock);
int valid_ident(const char *ident);
int userid_is_BM(const char *userid, const char *list);
int is_uBM(const char *list, const char *id);
extern inline int *intbsearch(int key, const int *base0, int nmemb);
int qsort_intcompar(const void *a, const void *b);
#ifndef CRITICAL_MEMORY
    #define MALLOC(p)  malloc(p)
    #define FREE(p)    free(p)
#else
    void *MALLOC(int size);
    void FREE(void *ptr);
#endif
#ifdef OUTTACACHE
int tobind(const char *iface_ip, int port);
int toconnect(const char *host, int port);
int toread(int fd, void *buf, int len);
int towrite(int fd, const void *buf, int len);
#endif
#ifdef PLAY_ANGEL
void pressanykey_or_callangel(void);
#endif
#ifdef TIMET64
    struct tm *localtime4(const time4_t *);
    time4_t time4(time4_t *);
    char *ctime4(const time4_t *);
#else
    #define localtime4(a) localtime(a)
    #define time4(a)      time(a)
    #define ctime4(a)     ctime(a)
#endif

/* syspost */
int post_msg(const char* bname, const char* title, const char *msg, const char* author);
int post_file(const char *bname, const char *title, const char *filename, const char *author);
void post_newboard(const char *bgroup, const char *bname, const char *bms);
void post_violatelaw(const char *crime, const char *police, const char *reason, const char *result);
void post_change_perm(int oldperm, int newperm, const char *sysopid, const char *userid);
void give_money_post(const char *userid, int money); 

/* talk */
#define iswritable(uentp)    \
        (iswritable_stat(uentp, friend_stat(currutmp, uentp)))
#define isvisible(me, uentp) \
        (isvisible_stat(currutmp, uentp, friend_stat(me, uentp)))
     
int iswritable_stat(const userinfo_t *uentp, int fri_stat);
int isvisible_stat(const userinfo_t * me, const userinfo_t * uentp, int fri_stat);
int cmpwatermtime(const void *a, const void *b);
void getmessage(msgque_t msg);
void my_write2(void);
int t_idle(void);
const char *modestring(const userinfo_t * uentp, int simple);
int t_users(void);
int my_write(pid_t pid, const char *hint, const char *id, int flag, userinfo_t *);
void t_display_new(void);
void talkreply(void);
int t_pager(void);
int t_query(void);
int t_qchicken(void);
int t_talk(void);
int t_display(void);
int my_query(const char *uident);
int logout_friend_online(userinfo_t*);
void login_friend_online(void);
int isvisible_uid(int tuid);
int friend_stat(const userinfo_t *me, const userinfo_t * ui);
int call_in(const userinfo_t *uentp, int fri_stat);
int make_connection_to_somebody(userinfo_t *uin, int timeout);
#ifdef PLAY_ANGEL
int t_changeangel(void);
int t_angelmsg(void);
void CallAngel(void);
void SwitchBeingAngel(void);
void SwitchAngelSex(int);
int t_switchangel(void);
#endif

/* tmpjack */
int reg_barbq(void);
int p_ticket_main(void);
int j_ticket_main(void);

/* term */
void init_tty(void);
int term_init(void);
void save_cursor(void);
void restore_cursor(void);
void do_move(int destcol, int destline);
void scroll_forward(void);
void change_scroll_range(int top, int bottom);

/* topsong */
void sortsong(void);
int topsong(void);

/* user */
int kill_user(int num);
int u_editcalendar(void);
void user_display(const userec_t *u, int real);
void uinfo_query(userec_t *u, int real, int unum);
int showsignature(char *fname, int *j);
void mail_violatelaw(const char* crime, const char* police, const char* reason, const char* result);
void showplans(const char *uid);
int u_info(void);
int u_loginview(void);
int u_ansi(void);
int u_editplan(void);
int u_editsig(void);
int u_cloak(void);
int u_register(void);
int u_list(void);

/* vote */
void b_suckinfile(FILE *fp, char *fname);
int b_results(void);
int b_vote(void);
int b_vote_maintain(void);
void auto_close_polls(void);

/* vice */
int vice_main(void);

/* voteboard */
int do_voteboard(int);
void do_voteboardreply(const fileheader_t *fhdr);

/* xyz */
int m_sysop(void);
int x_boardman(void);
int x_note(void);
int x_login(void);
int x_week(void);
int x_issue(void);
int x_today(void);
int x_yesterday(void);
int x_user100(void);
int x_birth(void);
#if 0
int x_90(void);
int x_89(void);
int x_88(void);
int x_87(void);
int x_86(void);
#endif
int x_history(void);
int x_weather(void);
int x_stock(void);
int x_mrtmap(void);
int note(void);
int Goodbye(void);

/* toolkit */
unsigned StringHash(const unsigned char *s);

/* passwd */
int passwd_init(void);
int passwd_update(int num, userec_t *buf);
int passwd_query(int num, userec_t *buf);
int passwd_apply(int (*fptr)(int, userec_t *));
void passwd_lock(void);
void passwd_unlock(void);
int passwd_update_money(int num);
int initcuser(const char *userid);
int freecuser(void);


/* calendar */
int calendar(void);

/* util */
void touchbtotal(int bid);

/* util_cache.c */
void reload_pttcache(void);

#endif
