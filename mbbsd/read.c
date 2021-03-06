/* $Id$ */
#include "bbs.h"
#include "fnv_hash.h"

static fileheader_t *headers = NULL;
static int      last_line; // PTT: last_line 游標可指的最後一個

#include <sys/mman.h>

/* ----------------------------------------------------- */
/* Tag List 標籤                                         */
/* ----------------------------------------------------- */
static TagItem         *TagList;	/* ascending list */

/**
 * @param locus
 * @return void
 */
void
UnTagger(int locus)
{
    if (locus > TagNum)
	return;

    TagNum--;

    if (TagNum > locus)
	memmove(&TagList[locus], &TagList[locus + 1],
	       (TagNum - locus) * sizeof(TagItem));
}

int
Tagger(time4_t chrono, int recno, int mode)
{
    int             head, tail, posi = 0, comp;

    if(TagList == NULL) {
	TagList = malloc(sizeof(TagItem)*MAXTAGS);
    }

    for (head = 0, tail = TagNum - 1, comp = 1; head <= tail;) {
	posi = (head + tail) >> 1;
	comp = TagList[posi].chrono - chrono;
	if (!comp) {
	    break;
	} else if (comp < 0) {
	    head = posi + 1;
	} else {
	    tail = posi - 1;
	}
    }

    if (mode == TAG_NIN) {
	if (!comp && recno)	/* 絕對嚴謹：連 recno 一起比對 */
	    comp = recno - TagList[posi].recno;
	return comp;

    }
    if (!comp) {
	if (mode != TAG_TOGGLE)
	    return NA;

	TagNum--;
	memmove(&TagList[posi], &TagList[posi + 1],
	       (TagNum - posi) * sizeof(TagItem));
    } else if (TagNum < MAXTAGS) {
	TagItem        *tagp;

	memmove(&TagList[head+1], &TagList[head], sizeof(TagItem)*(TagNum-head));
	tagp = &TagList[head];
	tagp->chrono = chrono;
	tagp->recno = recno;
	TagNum++;
    } else {
	bell();
	return 0;		/* full */
    }
    return YEA;
}


static void
EnumTagName(char *fname, int locus) /* unused */
{
    snprintf(fname, sizeof(fname), "M.%d.A", (int)TagList[locus].chrono);
}

void
EnumTagFhdr(fileheader_t * fhdr, char *direct, int locus)
{
    get_record(direct, fhdr, sizeof(fileheader_t), TagList[locus].recno);
}

/* -1 : 取消 */
/* 0 : single article */
/* ow: whole tag list */

int
AskTag(const char *msg)
{
    int             num;

    num = TagNum;
    switch (getans("◆ %s A)文章 T)標記 Q)uit?", msg)) {
    case 'q':
	num = -1;
	break;
    case 'a':
	num = 0;
    }
    return num;
}


#include <sys/mman.h>

#define BATCH_SIZE      65536

static char           *
f_map(const char *fpath, int *fsize)
{
    int             fd, size;
    struct stat     st;
    char *map;

    if ((fd = open(fpath, O_RDONLY)) < 0)
	return (char *)-1;

    if (fstat(fd, &st) || !S_ISREG(st.st_mode) || (size = st.st_size) <= 0) {
	close(fd);
	return (char *)-1;
    }
    map = (char *)mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
    close(fd);
    *fsize = size;
    return map;
}


static int
TagThread(const char *direct)
{
    int             fsize, count;
    char           *title, *fimage;
    fileheader_t   *head, *tail;

    fimage = f_map(direct, &fsize);
    if (fimage == (char *)-1)
	return DONOTHING;

    head = (fileheader_t *) fimage;
    tail = (fileheader_t *) (fimage + fsize);
    count = 0;
    do {
	count++;
	title = subject(head->title);
	if (!strncmp(currtitle, title, TTLEN)) {
	    if (!Tagger(atoi(head->filename + 2), count, TAG_INSERT))
		break;
	}
    } while (++head < tail);

    munmap(fimage, fsize);
    return FULLUPDATE;
}


int
TagPruner(int bid)
{
    boardheader_t  *bp=NULL;
    assert(bid >= 0);   /* bid == 0 means in mailbox */
    if (bid){
	bp = getbcache(bid);
	if (strcmp(bp->brdname, "Security") == 0)
	    return DONOTHING;
    }
    if (TagNum && ((currstat != READING) || (currmode & MODE_BOARD))) {
	if (getans("刪除所有標記[N]?") != 'y')
	    return READ_REDRAW;
#ifdef SAFE_ARTICLE_DELETE
        if(bp && !(currmode & MODE_DIGEST) && bp->nuser>30 )
            safe_delete_range(currdirect, 0, 0);
        else
#endif
	    delete_range(currdirect, 0, 0);
	TagNum = 0;
	if (bid)
	    setbtotal(bid);
        else if(currstat == RMAIL)
            setupmailusage();

	return NEWDIRECT;
    }
    return DONOTHING;
}


/* ----------------------------------------------------- */
/* cursor & reading record position control              */
/* ----------------------------------------------------- */
keeploc_t      *
getkeep(const char *s, int def_topline, int def_cursline)
{
    /* 為省記憶體, 且避免 malloc/free 不成對, getkeep 最好不要 malloc,
     * 只記 s 的 hash 值,
     * fvn1a-32bit collision 機率約小於十萬分之一 */
    /* 原本使用 link list, 可是一方面會造成 malloc/free 不成對, 
     * 一方面 size 小, malloc space overhead 就高, 因此改成 link block,
     * 以 KEEPSLOT 為一個 block 的 link list.
     * 只有第一個 block 可能沒滿. */
    /* TODO LRU recycle? 麻煩在於別處可能把 keeploc_t pointer 記著... */
#define KEEPSLOT 10
    struct keepsome {
	unsigned char used;
	keeploc_t arr[KEEPSLOT];
	struct keepsome *next;
    };
    static struct keepsome preserv_keepblock;
    static struct keepsome *keeplist = &preserv_keepblock;
    struct keeploc_t *p;
    unsigned int key=fnv1a_32_str(s, FNV1_32_INIT);
    int i;

    if (def_cursline >= 0) {
	struct keepsome *kl=keeplist;
	while(kl) {
	    for(i=0; i<kl->used; i++)
		if(key == kl->arr[i].hashkey) {
		    p = &kl->arr[i];
		    if (p->crs_ln < 1)
			p->crs_ln = 1;
		    return p;
		}
	    kl=kl->next;
	}
    } else
	def_cursline = -def_cursline;

    if(keeplist->used==KEEPSLOT) {
	struct keepsome *kl;
	kl = (struct keepsome*)malloc(sizeof(struct keepsome));
	memset(kl, 0, sizeof(struct keepsome));
	kl->next = keeplist;
	keeplist = kl;
    }
    p = &keeplist->arr[keeplist->used];
    keeplist->used++;
    p->hashkey = key;
    p->top_ln = def_topline;
    p->crs_ln = def_cursline;
    return p;
}

void
fixkeep(const char *s, int first)
{
    keeploc_t      *k;

    k = getkeep(s, 1, 1);
    if (k->crs_ln >= first) {
	k->crs_ln = (first == 1 ? 1 : first - 1);
	k->top_ln = (first < 11 ? 1 : first - 10);
    }
}

/* calc cursor pos and show cursor correctly */
static int
cursor_pos(keeploc_t * locmem, int val, int from_top, int isshow)
{
    int  top=locmem->top_ln;
    if (!last_line){
	cursor_show(3 , 0);
	return DONOTHING;
    }
    if (val > last_line)
	val = last_line;
    if (val <= 0)
	val = 1;
    if (val >= top && val < top + p_lines) {
        if(isshow){
	    cursor_clear(3 + locmem->crs_ln - top, 0);
	    cursor_show(3 + val - top, 0);
	}
	locmem->crs_ln = val;
	return DONOTHING;
    }
    locmem->top_ln = val - from_top;
    if (locmem->top_ln <= 0)
	locmem->top_ln = 1;
    locmem->crs_ln = val;
    return isshow ? PARTUPDATE : HEADERS_RELOAD;
}

/**
 * 根據 stypen 選擇上/下一篇文章
 *
 * @param locmem  用來存在某看板游標位置的 structure。
 * @param stypen  游標移動的方法
 *           CURSOR_FIRST, CURSOR_NEXT, CURSOR_PREV:
 *             與游標目前位置的文章同標題 的 第一篇/下一篇/前一篇 文章。
 *           RELATE_FIRST, RELATE_NEXT, RELATE_PREV:
 *             與目前正閱讀的文章同標題 的 第一篇/下一篇/前一篇 文章。
 *           NEWPOST_NEXT, NEWPOST_PREV:
 *             下一個/前一個 thread 的第一篇。
 *           AUTHOR_NEXT, AUTHOR_PREV:
 *             XXX 這功能目前好像沒用到?
 *
 * @return 新的游標位置
 */
static int
thread(const keeploc_t * locmem, int stypen)
{
    fileheader_t fh;
    int     pos = locmem->crs_ln, jump = 200, new_ln;
    int     fd = -1, amatch = -1;
    int     step = (stypen & RS_FORWARD) ? 1 : -1;
    char    *key;
    
    STATINC(STAT_THREAD);
    if (stypen & RS_AUTHOR)
	key = headers[pos - locmem->top_ln].owner;
    else if (stypen & RS_CURRENT)
     	key = subject(currtitle);
    else
	key = subject(headers[pos - locmem->top_ln].title );

    for( new_ln = pos + step ;
	 new_ln > 0 && new_ln <= last_line && --jump > 0;
	 new_ln += step ) {
	get_record_keep(currdirect, &fh, sizeof(fileheader_t), new_ln, &fd);
        if( stypen & RS_TITLE ){
            if( stypen & RS_FIRST ){
		if( !strncmp(fh.title, key, PROPER_TITLE_LEN) )
		    break;
		else if( !strncmp(&fh.title[4], key, PROPER_TITLE_LEN) ) {
		    amatch = new_ln;
		    jump = 200; /* 當搜尋同主題第一篇, 連續找不到 200 篇才停 */
		}
	    }
            else if( !strncmp(subject(fh.title), key, PROPER_TITLE_LEN) )
		break;
	}
        else if( stypen & RS_NEWPOST ){
            if( strncmp(fh.title, "Re:", 3) )
		break;
	}
        else{  // RS_AUTHOR
            if( !strcmp(subject(fh.owner), key) )
		break;
	}
    }
    if( fd != -1 )
	close(fd);
    if( jump <= 0 || new_ln <= 0 || new_ln > last_line )
	new_ln = (amatch == -1 ? pos : amatch); //didn't find
    return new_ln;
}

#ifdef INTERNET_EMAIL
static void
mail_forward(const fileheader_t * fhdr, const char *direct, int mode)
{
    int             i;
    char            buf[STRLEN];
    char           *p;

    strncpy(buf, direct, sizeof(buf));
    if ((p = strrchr(buf, '/')))
	*p = '\0';
    switch (i = doforward(buf, fhdr, mode)) {
    case 0:
	vmsg(msg_fwd_ok);
	break;
    case -1:
	vmsg(msg_fwd_err1);
	break;
    case -2:
	vmsg(msg_fwd_err2);
	break;
    default:
	break;
    }
}
#endif

static int
select_read(const keeploc_t * locmem, int sr_mode)
{
#define READSIZE 64  // 8192 / sizeof(fileheader_t)
   fileheader_t    fhs[READSIZE];
   char newdirect[MAXPATHLEN];
   char keyword[TTLEN + 1] = "";
   char   genbuf[MAXPATHLEN], *p = strstr(currdirect, "SR.");
   static int _mode = 0;
   int    len, fd, fr, i, count=0, reference = 0;
   fileheader_t *fh = &headers[locmem->crs_ln - locmem->top_ln]; 

   STATINC(STAT_SELECTREAD);
   if(sr_mode & RS_AUTHOR)
           {
	     if(!getdata(b_lines, 0,
                 currmode & MODE_SELECT ? "增加條件 作者:":"搜尋作者:",
                  keyword, IDLEN+1, LCECHO))
                return READ_REDRAW; 
           }
   else if(sr_mode  & RS_KEYWORD)
          {
             if(!getdata(b_lines, 0, 
                 currmode & MODE_SELECT ? "增加條件 標題:":"搜尋標題:",
                 keyword, TTLEN, DOECHO))
                return READ_REDRAW;
#ifdef KEYWORD_LOG
             log_file("keyword_search_log", LOG_CREAT | LOG_VF,
		      "%s:%s\n", currboard, keyword);
#endif
          }
   else 
    {
     if(p && _mode & sr_mode & (RS_TITLE | RS_NEWPOST | RS_MARK))
            return DONOTHING;
                // Ptt: only once for these modes.
     if(sr_mode & RS_TITLE)
       strcpy(keyword, subject(fh->title));           
    }

   if(p == NULL)
      _mode = sr_mode;
   else
      _mode |= sr_mode;
   
   snprintf(genbuf, sizeof(genbuf), "%s%X.%X.%X",
            p ? p : "SR.",
            sr_mode, (int)strlen(keyword), StringHash(keyword));
   if( strlen(genbuf) > MAXPATHLEN - 50 )
       return  READ_REDRAW; // avoid overflow

   if (currstat == RMAIL)
       sethomefile(newdirect, cuser.userid, genbuf);
   else
       setbfile(newdirect, currboard, genbuf);

   if( now - dasht(newdirect) <  3600 )
       count = dashs(newdirect) / sizeof(fileheader_t);
   else {
       if( (fd = open(newdirect, O_CREAT | O_RDWR, 0600)) == -1 )
	   return READ_REDRAW;
       if( (fr = open(currdirect, O_RDONLY, 0)) != -1 ) {
	   while( (len = read(fr, fhs, sizeof(fhs))) > 0 ){
	       len /= sizeof(fileheader_t);
	       for( i = 0 ; i < len ; ++i ){
		   reference++;
		   if( sr_mode & RS_MARK &&
		       !(fhs[i].filemode & FILE_MARKED) )
		       continue;
		   else if(sr_mode & RS_NEWPOST &&
		      !strncmp(fhs[i].title,  "Re:", 3))
		       continue;
		   else if(sr_mode & RS_AUTHOR &&
		      !strcasestr(fhs[i].owner, keyword))
		       continue;
		   else if(sr_mode & RS_KEYWORD &&
		      !strcasestr(fhs[i].title, keyword))
		       continue;
		   else if(sr_mode & RS_TITLE &&          
		      strcmp(subject(fhs[i].title), keyword))
		       continue;
		   ++count;
                   if(p == NULL)
		   {
		       fhs[i].multi.refer.flag = 1;
		       fhs[i].multi.refer.ref = reference;
		   }
		   write(fd, &fhs[i], sizeof(fileheader_t));
	       }
	   } // end while
           close(fr);
       }
       ftruncate(fd, count*sizeof(fileheader_t));
       close(fd);
   }

   if(count) {
       strlcpy(currdirect, newdirect, sizeof(currdirect));
       currmode |= MODE_SELECT;
       return NEWDIRECT;
   }
   return READ_REDRAW;
}

static int
i_read_key(const onekey_t * rcmdlist, keeploc_t * locmem, 
           int bid, int bottom_line)
{
    int     mode = DONOTHING, num, new_top=10;
    int     ch, new_ln = locmem->crs_ln, lastmode=0;
    char    direct[60];
    static  char default_ch = 0;
    
    do {
	if( (mode = cursor_pos(locmem, new_ln, new_top, default_ch ? 0 : 1))
	    != DONOTHING )
	    return mode;
	
	if( !default_ch )
	    ch = igetch();
	else{
	    if(new_ln != locmem->crs_ln) {// move fault
		default_ch=0;
		return FULLUPDATE;
	    }
	    ch = default_ch;
	}

	new_top = 10; // default 10 
	switch (ch) {
        case '0':    case '1':    case '2':    case '3':    case '4':
	case '5':    case '6':    case '7':    case '8':    case '9':
	    if( (num = search_num(ch, last_line)) != -1 )
		new_ln = num + 1; 
	    break;
    	case 'q':
    	case 'e':
    	case KEY_LEFT:
	    if(currmode & MODE_SELECT){
		char genbuf[64];
		fileheader_t *fhdr = &headers[locmem->crs_ln - locmem->top_ln];
		board_select();
		setbdir(genbuf, currboard);
		locmem = getkeep(genbuf, 0, 1);
		locmem->crs_ln = fhdr->multi.refer.ref;
		num = locmem->crs_ln - p_lines + 1;
		locmem->top_ln = num < 1 ? 1 : num;
		mode =  NEWDIRECT;
	    }
	    else
		mode =  
		    (currmode & MODE_DIGEST) ? board_digest() : DOQUIT;
	    break;
        case Ctrl('L'):
	    redoscr();
	    break;

        case Ctrl('H'):
	    mode = select_read(locmem, RS_NEWPOST);
	    break;

        case 'a':
        case 'A':
	    mode = select_read(locmem, RS_AUTHOR);
	    break;

        case 'G':
	    mode = select_read(locmem, RS_MARK);
	    break;

        case '/':
        case '?':
	    mode = select_read(locmem, RS_KEYWORD);
	    break;

        case 'S':
	    mode = select_read(locmem, RS_TITLE);
	    break;

        case '=':
	    new_ln = thread(locmem, RELATE_FIRST);
	    break;

        case '\\':
	    new_ln = thread(locmem, CURSOR_FIRST);
	    break;

        case ']':
	    new_ln = thread(locmem, RELATE_NEXT);
	    break;

        case '+':
	    new_ln = thread(locmem, CURSOR_NEXT);
	    break;

        case '[':
	    new_ln = thread(locmem, RELATE_PREV);
	    break;

     	case '-':
	    new_ln = thread(locmem, CURSOR_PREV);
	    break;

    	case '<':
    	case ',':
	    new_ln = thread(locmem, NEWPOST_PREV);
	    break;

    	case '.':
    	case '>':
	    new_ln = thread(locmem, NEWPOST_NEXT);
	    break;

    	case 'p':
    	case 'k':
	case KEY_UP:
	    new_ln = locmem->crs_ln - 1; 
	    new_top = p_lines - 2;
	    break;

	case 'n':
	case 'j':
	case KEY_DOWN:
	    new_ln = locmem->crs_ln + 1; 
	    new_top = 1;
	    break;

	case ' ':
	case KEY_PGDN:
	case 'N':
	case Ctrl('F'):
	    new_ln = locmem->top_ln + p_lines; 
	    new_top = 0;
	    break;

	case KEY_PGUP:
	case Ctrl('B'):
	case 'P':
	    new_ln = locmem->top_ln - p_lines; 
	    new_top = 0;
	    break;

	case KEY_END:
	case '$':
	    new_ln = last_line;
	    new_top = p_lines-1;
	    break;

	case 'F':
	case 'U':
	    if (HAS_PERM(PERM_FORWARD)) {
		mail_forward(&headers[locmem->crs_ln - locmem->top_ln],
			     currdirect, ch /* == 'U' */ );
		/* by CharlieL */
		mode = READ_REDRAW;
	    }
	    break;

	case Ctrl('Q'):
	    mode = my_query(headers[locmem->crs_ln - locmem->top_ln].owner);
	    break;

	case Ctrl('S'):
	    if (HAS_PERM(PERM_ACCOUNTS)) {
		int             id;
		userec_t        muser;

		strlcpy(currauthor,
			headers[locmem->crs_ln - locmem->top_ln].owner,
			sizeof(currauthor));
		stand_title("使用者設定");
		move(1, 0);
		if ((id = getuser(headers[locmem->crs_ln - locmem->top_ln].owner, &muser))) {
		    user_display(&muser, 1);
		    uinfo_query(&muser, 1, id);
		}
		mode = FULLUPDATE;
	    }
	    break;

	    /* rocker.011018: 採用新的tag模式 */
	case 't':
	    /* 將原本在 Read() 裡面的 "TagNum = 0" 移至此處 */
	    if ((currstat & RMAIL && TagBoard != 0) ||
		(!(currstat & RMAIL) && TagBoard != bid)) {
		if (currstat & RMAIL)
		    TagBoard = 0;
		else
		    TagBoard = bid;
		TagNum = 0;
	    }
	    /* rocker.011112: 解決再select mode標記文章的問題 */
	    if (Tagger(atoi(headers[locmem->crs_ln - locmem->top_ln].filename + 2),
		       (currmode & MODE_SELECT) ?
		       (headers[locmem->crs_ln - locmem->top_ln].multi.refer.ref) :
		       locmem->crs_ln, TAG_TOGGLE))
		locmem->crs_ln = locmem->crs_ln + 1; 
	    mode = PART_REDRAW;
	    break;

    case Ctrl('C'):
	if (TagNum) {
	    TagNum = 0;
	    mode = FULLUPDATE;
	}
        break;

    case Ctrl('T'):
	mode = TagThread(currdirect);
        break;

    case Ctrl('D'):
	mode = TagPruner(bid);
        break;

    case '\n':
    case '\r':
    case 'l':
    case KEY_RIGHT:
	ch = 'r';
    default:
	if( ch == 'h' && currmode & (MODE_DIGEST) )
	    break;
	if (ch > 0 && ch <= onekey_size) {
	    int (*func)() = rcmdlist[ch - 1];
	    if (func != NULL){
		num  = locmem->crs_ln - bottom_line;
                   
		if( num > 0 ){
                    sprintf(direct,"%s.bottom", currdirect);
		    mode= (*func)(num, &headers[locmem->crs_ln-locmem->top_ln],
				  direct);
		}
		else
                    mode = (*func)(locmem->crs_ln, 
				   &headers[locmem->crs_ln - locmem->top_ln],
				   currdirect);
		if(mode == READ_SKIP)
                    mode = lastmode;

		// 以下這幾種 mode 要再處理游標
                if(mode == READ_PREV || mode == READ_NEXT || 
                   mode == RELATE_PREV || mode == RELATE_FIRST || 
                   mode == AUTHOR_NEXT || mode ==  AUTHOR_PREV ||
                   mode == RELATE_NEXT){
		    lastmode = mode;

		    switch(mode){
                    case READ_PREV:
                        new_ln =  locmem->crs_ln - 1;
                        break;
                    case READ_NEXT:
                        new_ln =  locmem->crs_ln + 1;
                        break;
	            case RELATE_PREV:
                        new_ln = thread(locmem, RELATE_PREV);
			break;
                    case RELATE_NEXT:
                        new_ln = thread(locmem, RELATE_NEXT);
			/* XXX: 讀到最後一篇要跳出來 */
			if( new_ln == locmem->crs_ln ){
			    default_ch = 0;
			    return FULLUPDATE;
			}
		        break;
                    case RELATE_FIRST:
                        new_ln = thread(locmem, RELATE_FIRST);
		        break;
                    case AUTHOR_PREV:
                        new_ln = thread(locmem, AUTHOR_PREV);
		        break;
                    case AUTHOR_NEXT:
                        new_ln = thread(locmem, AUTHOR_NEXT);
		        break;
		    }
		    mode = DONOTHING; default_ch = 'r';
                }
		else {
		    default_ch = 0;
		    lastmode = 0;
		}
	    } //end if (func != NULL)
	} // ch > 0 && ch <= onekey_size 
    	break;
	} // end switch
    } while (mode == DONOTHING);
    return mode;
}

int
get_records_and_bottom(char *direct,  fileheader_t* headers,
                     int recbase, int p_lines, int last_line, int bottom_line)
{
    int     n = bottom_line - recbase + 1, rv;
    char    directbottom[60];

    if( !last_line )
	return 0;
    if( n >= p_lines || (currmode & (MODE_SELECT | MODE_DIGEST)) )
	return get_records(direct, headers, sizeof(fileheader_t), recbase, 
			   p_lines);

    sprintf(directbottom, "%s.bottom", direct);
    if( n <= 0 )
	return get_records(directbottom, headers, sizeof(fileheader_t), 1-n, 
			   last_line-recbase + 1);
      
    rv = get_records(direct, headers, sizeof(fileheader_t), recbase, n);

    if( bottom_line < last_line )
	rv += get_records(directbottom, headers+n, sizeof(fileheader_t), 1, 
			  p_lines - n );
    return rv;
}

void
i_read(int cmdmode, const char *direct, void (*dotitle) (),
       void (*doentry) (), const onekey_t * rcmdlist, int bidcache)
{
    keeploc_t      *locmem = NULL;
    int             recbase = 0, mode;
    int             num = 0, entries = 0, n_bottom=0;
    int             i;
    char            currdirect0[64];
    int             last_line0 = last_line;
    int             bottom_line = 0;
    fileheader_t   *headers0 = headers;

    strlcpy(currdirect0, currdirect, sizeof(currdirect0));
#define FHSZ    sizeof(fileheader_t)
    /* Ptt: 這邊 headers 可以針對看板的最後 60 篇做 cache */
    headers = (fileheader_t *) calloc(p_lines, FHSZ);
    strlcpy(currdirect, direct, sizeof(currdirect));
    mode = NEWDIRECT;

    do {
	/* 依據 mode 顯示 fileheader */
	setutmpmode(cmdmode);
	switch (mode) {
	case NEWDIRECT:	/* 第一次載入此目錄 */
	case DIRCHANGED:
	    if (bidcache > 0 && !(currmode & (MODE_SELECT | MODE_DIGEST))){
		if( (last_line = getbtotal(currbid)) == 0 ){
		    setbtotal(currbid);
                    setbottomtotal(currbid);
		    last_line = getbtotal(currbid);
		}
                bottom_line = last_line;
                last_line += (n_bottom = getbottomtotal(currbid)); 
	    }
	    else
		bottom_line = last_line = get_num_records(currdirect, FHSZ);

	    if (mode == NEWDIRECT) {
		num = last_line - p_lines + 1;
		locmem = getkeep(currdirect, num < 1 ? 1 : num, last_line);
	    }
	    recbase = -1;

	case FULLUPDATE:
	    (*dotitle) ();

	case PARTUPDATE:
	    if (last_line < locmem->top_ln + p_lines) {
		if (bidcache > 0 && !(currmode & (MODE_SELECT | MODE_DIGEST))){
		    bottom_line = getbtotal(currbid);
		    num = bottom_line+getbottomtotal(currbid);
		}
		else
		    num = get_num_records(currdirect, FHSZ);

		if (last_line != num) {
		    last_line = num;
		    recbase = -1;
		}
	    }
	    if (recbase != locmem->top_ln) { //headers reload
		recbase = locmem->top_ln;
		if (recbase > last_line) {
		    recbase = last_line - p_lines + 1;
		    if (recbase < 1)
			recbase = 1;
		    locmem->top_ln = recbase;
		}
                entries=get_records_and_bottom(currdirect,
                           headers, recbase, p_lines, last_line, bottom_line);
	    }
	    if (locmem->crs_ln > last_line)
		locmem->crs_ln = last_line;
	    move(3, 0);
	    clrtobot();
	case PART_REDRAW:
	    move(3, 0);
            if( last_line == 0 )
                  outs("    沒有文章...");
            else
		for( i = 0; i < entries ; i++ )
		    (*doentry) (locmem->top_ln + i, &headers[i]);
	case READ_REDRAW:
	    outmsg(curredit & EDIT_ITEM ?
		   "\033[44m 私人收藏 \033[30;47m 繼續? \033[m" :
		   curredit & EDIT_MAIL ? msg_mailer : MSG_POSTER);
	    break;
	case TITLE_REDRAW:
	    (*dotitle) ();
            break;
        
        case HEADERS_RELOAD:
	    if (recbase != locmem->top_ln) {
		recbase = locmem->top_ln;
		if (recbase > last_line) {
		    recbase = last_line - p_lines + 1;
		    if (recbase < 1)
			recbase = 1;
		    locmem->top_ln = recbase;
		}
                entries = 
		    get_records_and_bottom(currdirect, headers, recbase,
					   p_lines, last_line, bottom_line);
	    }
            break;
	} //end switch
	mode = i_read_key(rcmdlist, locmem, currbid, bottom_line);
    } while (mode != DOQUIT);
#undef  FHSZ

    free(headers);
    last_line = last_line0;
    headers = headers0;
    strlcpy(currdirect, currdirect0, sizeof(currdirect));
    return;
}
