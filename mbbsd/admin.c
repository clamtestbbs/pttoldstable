/* $Id$ */
#include "bbs.h"

/* 進站水球宣傳 */
int
m_loginmsg(void)
{
  char msg[100];
  move(21,0);
  clrtobot();
  if(SHM->loginmsg.pid && SHM->loginmsg.pid != currutmp->pid)
    {
      outs("目前已經有以下的 進站水球設定請先協調好再設定..");
      getmessage(SHM->loginmsg);
    }
  getdata(22, 0, 
     "進站水球:本站活動,不干擾使用者為限,設定者離站自動取消,確定要設?(y/N)",
          msg, 3, LCECHO);

  if(msg[0]=='y' &&

     getdata_str(23, 0, "設定進站水球:", msg, 56, DOECHO, SHM->loginmsg.last_call_in))
    {
          SHM->loginmsg.pid=currutmp->pid; /*站長不多 就不管race condition */
          strcpy(SHM->loginmsg.last_call_in, msg);
          strcpy(SHM->loginmsg.userid, cuser.userid);
    }
  return 0;
}

/* 使用者管理 */
int
m_user(void)
{
    userec_t        xuser;
    int             id;
    char            genbuf[200];

    stand_title("使用者設定");
    usercomplete(msg_uid, genbuf);
    if (*genbuf) {
	move(2, 0);
	if ((id = getuser(genbuf, &xuser))) {
	    user_display(&xuser, 1);
	    uinfo_query(&xuser, 1, id);
	} else {
	    outs(err_uid);
	    clrtoeol();
	    pressanykey();
	}
    }
    return 0;
}

static int
search_key_user(const char *passwdfile, int mode)
{
    userec_t        user;
    int             ch;
    int             coun = 0;
    FILE            *fp1 = fopen(passwdfile, "r");
    char            friendfile[128]="", key[22], genbuf[8],
                    *keymatch;


    assert(fp1);
    clear();
    getdata(0, 0, mode ? "請輸入使用者關鍵字[電話|地址|姓名|身份證|上站地點|"
	    "email|小雞id] :" : "請輸入id :", key, sizeof(key), DOECHO);
    if(!key[0]) {
	fclose(fp1);
	return 0;
    }
    while ((fread(&user, sizeof(user), 1, fp1)) > 0 && coun < MAX_USERS) {
	if (!(++coun & 15)) {
	    move(1, 0);
	    prints("第 [%d] 筆資料\n", coun);
	    refresh();
	}
        keymatch = NULL;
	if (!strcasecmp(user.userid, key))
             keymatch = user.userid; 
        else if(mode) {
             if(strstr(user.realname, key))
                 keymatch = user.realname; 
             else if(strstr(user.username, key))
                 keymatch = user.username; 
             else if(strstr(user.lasthost, key))
                 keymatch = user.lasthost; 
             else if(strcasestr(user.email, key))
                 keymatch = user.email; 
             else if(strstr(user.address, key))
                 keymatch = user.address; 
             else if(strstr(user.justify, key))
                 keymatch = user.justify; 
             else if(strstr(user.mychicken.name, key))
                 keymatch = user.mychicken.name; 
	}
        if(keymatch) {
	    move(1, 0);
	    prints("第 [%d] 筆資料\n", coun);
	    refresh();

	    user_display(&user, 1);
	    uinfo_query(&user, 1, coun);
	    outs("\033[44m               空白鍵\033[37m:搜尋下一個"
		 "          \033[33m Q\033[37m: 離開");
	    outs(mode ? 
                 "      A: add to namelist \033[m " :
		 "      S: 取用備份資料    \033[m ");
	    while (1) {
		while ((ch = igetch()) == 0);
                if (ch == 'a' || ch=='A' )
                  {
                   if(!friendfile[0])
                    {
                     friend_special();
                     setfriendfile(friendfile, FRIEND_SPECIAL);
                    }
                   friend_add(user.userid, FRIEND_SPECIAL, keymatch);
                   break;
                  }
		if (ch == ' ')
		    break;
		if (ch == 'q' || ch == 'Q') {
		    fclose(fp1);
		    return 0;
		}
		if (ch == 's' && !mode) {
		    if ((ch = searchuser(user.userid, user.userid))) {
			setumoney(ch, user.money);
			passwd_update(ch, &user);
			fclose(fp1);
			return 0;
		    } else {
			getdata(0, 0,
				"目前的 PASSWD 檔沒有此 ID，新增嗎？[y/N]",
				genbuf, 3, LCECHO);
			if (genbuf[0] != 'y') {
			    outs("目前的PASSWDS檔沒有此id "
				 "請先new一個這個id的帳號");
			} else {
			    int             allocid = getnewuserid();

			    if (allocid > MAX_USERS || allocid <= 0) {
				fprintf(stderr, "本站人口已達飽和！\n");
				exit(1);
			    }
			    if (passwd_update(allocid, &user) == -1) {
				fprintf(stderr, "客滿了，再見！\n");
				exit(1);
			    }
			    setuserid(allocid, user.userid);
			    if (!searchuser(user.userid, NULL)) {
				fprintf(stderr, "無法建立帳號\n");
				exit(1);
			    }
			    fclose(fp1);
			    return 0;
			}
		    }
		}
	    }
	}
    }

    fclose(fp1);
    return 0;
}

/* 以任意 key 尋找使用者 */
int
search_user_bypwd(void)
{
    search_key_user(FN_PASSWD, 1);
    return 0;
}

/* 尋找備份的使用者資料 */
int
search_user_bybakpwd(void)
{
    char           *choice[] = {
	"PASSWDS.NEW1", "PASSWDS.NEW2", "PASSWDS.NEW3",
	"PASSWDS.NEW4", "PASSWDS.NEW5", "PASSWDS.NEW6",
	"PASSWDS.BAK"
    };
    int             ch;

    clear();
    move(1, 1);
    outs("請輸入你要用來尋找備份的檔案 或按 'q' 離開\n");
    outs(" [\033[1;31m1\033[m]一天前, [\033[1;31m2\033[m]兩天前, "
	 "[\033[1;31m3\033[m]三天前\n");
    outs(" [\033[1;31m4\033[m]四天前, [\033[1;31m5\033[m]五天前, "
	 "[\033[1;31m6\033[m]六天前\n");
    outs(" [7]備份的\n");
    do {
	move(5, 1);
	outs("選擇 => ");
	ch = igetch();
	if (ch == 'q' || ch == 'Q')
	    return 0;
    } while (ch < '1' || ch > '7');
    ch -= '1';
    if( access(choice[ch], R_OK) != 0 )
	vmsg("檔案不存在");
    else
	search_key_user(choice[ch], 0);
    return 0;
}

static void
bperm_msg(const boardheader_t * board)
{
    prints("\n設定 [%s] 看板之(%s)權限：", board->brdname,
	   board->brdattr & BRD_POSTMASK ? "發表" : "閱\讀");
}

unsigned int
setperms(unsigned int pbits, const char * const pstring[])
{
    register int    i;

    move(4, 0);
    for (i = 0; i < NUMPERMS / 2; i++) {
	prints("%c. %-20s %-15s %c. %-20s %s\n",
	       'A' + i, pstring[i],
	       ((pbits >> i) & 1 ? "ˇ" : "Ｘ"),
	       i < 10 ? 'Q' + i : '0' + i - 10,
	       pstring[i + 16],
	       ((pbits >> (i + 16)) & 1 ? "ˇ" : "Ｘ"));
    }
    clrtobot();
    while (
       (i = getkey("請按 [A-5] 切換設定，按 [Return] 結束："))!='\r')
    {
	if (isdigit(i))
	    i = i - '0' + 26;
	else if (isalpha(i))
	    i = tolower(i) - 'a';
	else {
	    bell();
	    continue;
	}

	pbits ^= (1 << i);
	move(i % 16 + 4, i <= 15 ? 24 : 64);
	outs((pbits >> i) & 1 ? "ˇ" : "Ｘ");
    }
    return pbits;
}

#ifdef CHESSCOUNTRY
static void
AddingChessCountryFiles(const char* apath)
{
    char filename[256];
    char symbolicname[256];
    char adir[256];
    FILE* fp;
    fileheader_t fh;

    setadir(adir, apath);

    /* creating chess country regalia */
    snprintf(filename, sizeof(filename), "%s/chess_ensign", apath);
    close(open(filename, O_CREAT | O_WRONLY, 0644));

    strlcpy(symbolicname, apath, sizeof(symbolicname));
    stampfile(symbolicname, &fh);
    symlink("chess_ensign", symbolicname);

    strcpy(fh.title, "◇ 棋國國徽 (不能刪除，系統需要)");
    strcpy(fh.owner, str_sysop);
    append_record(adir, &fh, sizeof(fileheader_t));

    /* creating member list */
    snprintf(filename, sizeof(filename), "%s/chess_list", apath);
    if (!dashf(filename)) {
	fp = fopen(filename, "w");
	assert(fp);
	fputs("棋國國名\n"
		"帳號            階級    加入日期        等級或被誰俘虜\n"
		"──────    ───  ─────      ───────\n",
		fp);
	fclose(fp);
    }

    strlcpy(symbolicname, apath, sizeof(symbolicname));
    stampfile(symbolicname, &fh);
    symlink("chess_list", symbolicname);

    strcpy(fh.title, "◇ 棋國成員表 (不能刪除，系統需要)");
    strcpy(fh.owner, str_sysop);
    append_record(adir, &fh, sizeof(fileheader_t));

    /* creating profession photos' dir */
    snprintf(filename, sizeof(filename), "%s/chess_photo", apath);
    mkdir(filename, 0755);

    strlcpy(symbolicname, apath, sizeof(symbolicname));
    stampfile(symbolicname, &fh);
    symlink("chess_photo", symbolicname);

    strcpy(fh.title, "◆ 棋國照片檔 (不能刪除，系統需要)");
    strcpy(fh.owner, str_sysop);
    append_record(adir, &fh, sizeof(fileheader_t));
}
#endif /* defined(CHESSCOUNTRY) */

/* 自動設立精華區 */
void
setup_man(const boardheader_t * board, const boardheader_t * oldboard)
{
    char            genbuf[200];

    setapath(genbuf, board->brdname);
    mkdir(genbuf, 0755);

#ifdef CHESSCOUNTRY
    if (oldboard == NULL || oldboard->chesscountry != board->chesscountry)
	if (board->chesscountry != CHESSCODE_NONE)
	    AddingChessCountryFiles(genbuf);
	// else // doesn't remove files..
#endif
}

void delete_symbolic_link(boardheader_t *bh, int bid)
{
    memset(bh, 0, sizeof(boardheader_t));
    substitute_record(fn_board, bh, sizeof(boardheader_t), bid);
    reset_board(bid);
    sort_bcache(); 
    log_usies("DelLink", bh->brdname);
}

int dir_cmp(const void *a, const void *b)
{
  return (atoi( &((fileheader_t *)a)->filename[2] ) -
          atoi( &((fileheader_t *)b)->filename[2] ));
}

void merge_dir(const char *dir1, const char *dir2, int isoutter)
{
     int i, pn, sn;
     fileheader_t *fh;
     char *p1, *p2, bakdir[128], file1[128], file2[128];
     strcpy(file1,dir1);
     strcpy(file2,dir2);
     if((p1=strrchr(file1,'/')))
	 p1 ++;
     else
	 p1 = file1;
     if((p2=strrchr(file2,'/')))
	 p2 ++;
     else
	 p2 = file2;

     pn=get_num_records(dir1, sizeof(fileheader_t));
     sn=get_num_records(dir2, sizeof(fileheader_t));
     if(!sn) return;
     fh= (fileheader_t *)malloc( (pn+sn)*sizeof(fileheader_t));
     get_records(dir1, fh, sizeof(fileheader_t), 1, pn);
     get_records(dir2, fh+pn, sizeof(fileheader_t), 1, sn);
     if(isoutter)
         {
             for(i=0; i<sn; i++)
               if(fh[pn+i].owner[0])
                   strcat(fh[pn+i].owner, "."); 
         }
     qsort(fh, pn+sn, sizeof(fileheader_t), dir_cmp);
     sprintf(bakdir,"%s.bak", dir1);
     Rename(dir1, bakdir);
     for(i=1; i<=pn+sn; i++ )
        {
         if(!fh[i-1].filename[0]) continue;
         if(i == pn+sn ||  strcmp(fh[i-1].filename, fh[i].filename))
	 {
                fh[i-1].recommend =0;
		fh[i-1].filemode |= 1;
                append_record(dir1, &fh[i-1], sizeof(fileheader_t));
		strcpy(p1, fh[i-1].filename);
                if(!dashf(file1))
		      {
			  strcpy(p2, fh[i-1].filename);
			  Copy(file2, file1);
		      } 
	 }
         else
                fh[i].filemode |= fh[i-1].filemode;
        }
     
     free(fh);
}

int
m_mod_board(char *bname)
{
    boardheader_t   bh, newbh;
    int             bid;
    char            genbuf[256], ans[4];

    bid = getbnum(bname);
    if (!bid || !bname[0] || get_record(fn_board, &bh, sizeof(bh), bid) == -1) {
	vmsg(err_bid);
	return -1;
    }
    prints("看板名稱：%s\n看板說明：%s\n看板bid：%d\n看板GID：%d\n"
	   "板主名單：%s", bh.brdname, bh.title, bid, bh.gid, bh.BM);
    bperm_msg(&bh);

    /* Ptt 這邊斷行會檔到下面 */
    move(9, 0);
    snprintf(genbuf, sizeof(genbuf), "(E)設定 (V)違法/解除%s%s [Q]取消？",
	    HAS_PERM(PERM_SYSOP |
		     PERM_BOARD) ? " (B)Vote (S)救回 (C)合併 (G)賭盤解卡" : "",
	    HAS_PERM(PERM_SYSSUBOP | PERM_BOARD) ? " (D)刪除" : "");
    getdata(10, 0, genbuf, ans, 3, LCECHO);

    switch (*ans) {
    case 'g':
	if (HAS_PERM(PERM_SYSOP | PERM_BOARD)) {
	    char            path[256];
	    setbfile(genbuf, bname, FN_TICKET_LOCK);
	    setbfile(path, bname, FN_TICKET_END);
	    rename(genbuf, path);
	}
	break;
    case 's':
	if (HAS_PERM(PERM_SYSOP | PERM_BOARD)) {
	  snprintf(genbuf, sizeof(genbuf),
		   BBSHOME "/bin/buildir boards/%c/%s &",
		   bh.brdname[0], bh.brdname);
	    system(genbuf);
	}
	break;
    case 'c':
	if (HAS_PERM(PERM_SYSOP)) {
	   char frombname[20], fromdir[256];
#ifdef MERGEBBS
	   if(getans("是否匯入SOB看板? (y/N)")=='y')
	   { 
                 setbdir(genbuf, bname);
	         m_sob_brd(bname, fromdir);
		 if(!fromdir[0]) break;
                 merge_dir(genbuf, fromdir, 1);
           }
	   else{
#endif
	    CompleteBoard(MSG_SELECT_BOARD, frombname);
            if (frombname[0] == '\0' || !getbnum(frombname) ||
		!strcmp(frombname,bname))
	                     break;
            setbdir(genbuf, bname);
            setbdir(fromdir, frombname);
            merge_dir(genbuf, fromdir, 0);
#ifdef MERGEBBS
	   }
#endif
	    touchbtotal(bid);
	}
	break;
    case 'b':
	if (HAS_PERM(PERM_SYSOP | PERM_BOARD)) {
	    char            bvotebuf[10];

	    memcpy(&newbh, &bh, sizeof(bh));
	    snprintf(bvotebuf, sizeof(bvotebuf), "%d", newbh.bvote);
	    move(20, 0);
	    prints("看板 %s 原來的 BVote：%d", bh.brdname, bh.bvote);
	    getdata_str(21, 0, "新的 Bvote：", genbuf, 5, LCECHO, bvotebuf);
	    newbh.bvote = atoi(genbuf);
	    substitute_record(fn_board, &newbh, sizeof(newbh), bid);
	    reset_board(bid);
	    log_usies("SetBoardBvote", newbh.brdname);
	    break;
	} else
	    break;
    case 'v':
	memcpy(&newbh, &bh, sizeof(bh));
	outs("看板目前為");
	outs((bh.brdattr & BRD_BAD) ? "違法" : "正常");
	getdata(21, 0, "確定更改？", genbuf, 5, LCECHO);
	if (genbuf[0] == 'y') {
	    if (newbh.brdattr & BRD_BAD)
		newbh.brdattr = newbh.brdattr & (!BRD_BAD);
	    else
		newbh.brdattr = newbh.brdattr | BRD_BAD;
	    substitute_record(fn_board, &newbh, sizeof(newbh), bid);
	    reset_board(bid);
	    log_usies("ViolateLawSet", newbh.brdname);
	}
	break;
    case 'd':
	if (!HAS_PERM(PERM_SYSOP | PERM_BOARD))
	    break;
	getdata_str(9, 0, msg_sure_ny, genbuf, 3, LCECHO, "N");
	if (genbuf[0] != 'y' || !bname[0])
	    outs(MSG_DEL_CANCEL);
	else if (bh.brdattr & BRD_SYMBOLIC) {
	    delete_symbolic_link(&bh, bid);
	}
	else {
	    strlcpy(bname, bh.brdname, sizeof(bh.brdname));
	    snprintf(genbuf, sizeof(genbuf),
		    "/bin/tar zcvf tmp/board_%s.tgz boards/%c/%s man/boards/%c/%s >/dev/null 2>&1;"
		    "/bin/rm -fr boards/%c/%s man/boards/%c/%s",
		    bname, bname[0], bname, bname[0],
		    bname, bname[0], bname, bname[0], bname);
	    system(genbuf);
	    memset(&bh, 0, sizeof(bh));
	    snprintf(bh.title, sizeof(bh.title),
		     "%s 看板 %s 刪除", bname, cuser.userid);
	    post_msg("Security", bh.title, "請注意刪除的合法性", "[系統安全局]");
	    substitute_record(fn_board, &bh, sizeof(bh), bid);
	    reset_board(bid);
            sort_bcache(); 
	    log_usies("DelBoard", bh.title);
	    outs("刪板完畢");
	}
	break;
    case 'e':
	move(8, 0);
	outs("直接按 [Return] 不修改該項設定");
	memcpy(&newbh, &bh, sizeof(bh));

	while (getdata(9, 0, "新看板名稱：", genbuf, IDLEN + 1, DOECHO)) {
	    if (getbnum(genbuf)) {
		move(3, 0);
		outs("錯誤! 板名雷同");
	    } else if ( !invalid_brdname(genbuf) ){
		strlcpy(newbh.brdname, genbuf, sizeof(newbh.brdname));
		break;
	    }
	}

	do {
	    getdata_str(12, 0, "看板類別：", genbuf, 5, DOECHO, bh.title);
	    if (strlen(genbuf) == 4)
		break;
	} while (1);

	if (strlen(genbuf) >= 4)
	    strncpy(newbh.title, genbuf, 4);

	newbh.title[4] = ' ';

	getdata_str(14, 0, "看板主題：", genbuf, BTLEN + 1, DOECHO,
		    bh.title + 7);
	if (genbuf[0])
	    strlcpy(newbh.title + 7, genbuf, sizeof(newbh.title) - 7);
	if (getdata_str(15, 0, "新板主名單：", genbuf, IDLEN * 3 + 3, DOECHO,
			bh.BM)) {
	    trim(genbuf);
	    strlcpy(newbh.BM, genbuf, sizeof(newbh.BM));
	}
#ifdef CHESSCOUNTRY
	if (HAS_PERM(PERM_SYSOP)) {
	    snprintf(genbuf, sizeof(genbuf), "%d", bh.chesscountry);
	    if (getdata_str(16, 0, "設定棋國 (0)無 (1)五子棋 (2)象棋", ans,
			sizeof(ans), LCECHO, genbuf)){
		newbh.chesscountry = atoi(ans);
		if (newbh.chesscountry > CHESSCODE_MAX ||
			newbh.chesscountry < CHESSCODE_NONE)
		    newbh.chesscountry = bh.chesscountry;
	    }
	}
#endif /* defined(CHESSCOUNTRY) */
	if (HAS_PERM(PERM_SYSOP|PERM_BOARD)) {
	    move(1, 0);
	    clrtobot();
	    newbh.brdattr = setperms(newbh.brdattr, str_permboard);
	    move(1, 0);
	    clrtobot();
	}
	if (newbh.brdattr & BRD_GROUPBOARD)
	    strncpy(newbh.title + 5, "Σ", 2);
	else if (newbh.brdattr & BRD_NOTRAN)
	    strncpy(newbh.title + 5, "◎", 2);
	else
	    strncpy(newbh.title + 5, "●", 2);

	if (HAS_PERM(PERM_SYSOP|PERM_BOARD) && !(newbh.brdattr & BRD_HIDE)) {
	    getdata_str(14, 0, "設定讀寫權限(Y/N)？", ans, sizeof(ans), LCECHO, "N");
	    if (*ans == 'y') {
		getdata_str(15, 0, "限制 [R]閱\讀 (P)發表？", ans, sizeof(ans), LCECHO,
			    "R");
		if (*ans == 'p')
		    newbh.brdattr |= BRD_POSTMASK;
		else
		    newbh.brdattr &= ~BRD_POSTMASK;

		move(1, 0);
		clrtobot();
		bperm_msg(&newbh);
		newbh.level = setperms(newbh.level, str_permid);
		clear();
	    }
	}

	getdata(b_lines - 1, 0, "請您確定(Y/N)？[Y]", genbuf, 4, LCECHO);

	if ((*genbuf != 'n') && memcmp(&newbh, &bh, sizeof(bh))) {
	    if (strcmp(bh.brdname, newbh.brdname)) {
		char            src[60], tar[60];

		setbpath(src, bh.brdname);
		setbpath(tar, newbh.brdname);
		Rename(src, tar);

		setapath(src, bh.brdname);
		setapath(tar, newbh.brdname);
		Rename(src, tar);
	    }
	    setup_man(&newbh, &bh);
	    substitute_record(fn_board, &newbh, sizeof(newbh), bid);
	    reset_board(bid);
            sort_bcache(); 
	    log_usies("SetBoard", newbh.brdname);
	}
    }
    return 0;
}

/* 設定看板 */
int
m_board(void)
{
    char            bname[32];

    stand_title("看板設定");
    CompleteBoardAndGroup(msg_bid, bname);
    if (!*bname)
	return 0;
    m_mod_board(bname);
    return 0;
}

/* 設定系統檔案 */
int
x_file(void)
{
    int             aborted;
    char            ans[4], *fpath;

    move(b_lines - 7, 0);
    /* Ptt */
    outs("設定 (1)身份確認信 (4)post注意事項 (5)錯誤登入訊息 (6)註冊範例 (7)通過確認通知\n");
    outs("     (8)email post通知 (9)系統功\能精靈 (A)茶樓 (B)站長名單 (C)email通過確認\n");
    outs("     (D)新使用者需知 (E)身份確認方法 (F)歡迎畫面 (G)進站畫面"
#ifdef MULTI_WELCOME_LOGIN
	 "(X)刪除進站畫面"
#endif
	 "\n");
    outs("     (H)看板期限 (I)故鄉 (J)出站畫面 (K)生日卡 (L)節日 (M)外籍使用者認證通知\n");
    outs("     (N)外籍使用者過期警告通知 (O)看板列表 help (P)文章列表 help\n");
#ifdef PLAY_ANGEL
    outs(" (Y)小天使認證通知\n");
#endif
    getdata(b_lines - 1, 0, "[Q]取消[1-9 A-P]？", ans, sizeof(ans), LCECHO);

    switch (ans[0]) {
    case '1':
	fpath = "etc/confirm";
	break;
    case '4':
	fpath = "etc/post.note";
	break;
    case '5':
	fpath = "etc/goodbye";
	break;
    case '6':
	fpath = "etc/register";
	break;
    case '7':
	fpath = "etc/registered";
	break;
    case '8':
	fpath = "etc/emailpost";
	break;
    case '9':
	fpath = "etc/hint";
	break;
    case 'b':
	fpath = "etc/sysop";
	break;
    case 'c':
	fpath = "etc/bademail";
	break;
    case 'd':
	fpath = "etc/newuser";
	break;
    case 'e':
	fpath = "etc/justify";
	break;
    case 'f':
	fpath = "etc/Welcome";
	break;
    case 'g':
#ifdef MULTI_WELCOME_LOGIN
	getdata(b_lines - 1, 0, "第幾個進站畫面[0-4]", ans, sizeof(ans), LCECHO);
	if (ans[0] == '1') {
	    fpath = "etc/Welcome_login.1";
	} else if (ans[0] == '2') {
	    fpath = "etc/Welcome_login.2";
	} else if (ans[0] == '3') {
	    fpath = "etc/Welcome_login.3";
	} else if (ans[0] == '4') {
	    fpath = "etc/Welcome_login.4";
	} else {
	    fpath = "etc/Welcome_login.0";
	}
#else
	fpath = "etc/Welcome_login";
#endif
	break;

#ifdef MULTI_WELCOME_LOGIN
    case 'x':
	getdata(b_lines - 1, 0, "第幾個進站畫面[1-4]", ans, sizeof(ans), LCECHO);
	if (ans[0] == '1') {
	    unlink("etc/Welcome_login.1");
	    vmsg("ok");
	} else if (ans[0] == '2') {
	    unlink("etc/Welcome_login.2");
	    vmsg("ok");
	} else if (ans[0] == '3') {
	    unlink("etc/Welcome_login.3");
	    vmsg("ok");
	} else if (ans[0] == '4') {
	    unlink("etc/Welcome_login.4");
	    vmsg("ok");
	} else {
	    vmsg("所指定的進站畫面無法刪除");
	}
	return FULLUPDATE;

#endif

    case 'h':
	fpath = "etc/expire.conf";
	break;
    case 'i':
	fpath = "etc/domain_name_query.cidr";
	break;
    case 'j':
	fpath = "etc/Logout";
	break;
    case 'k':
	fpath = "etc/Welcome_birth";
	break;
    case 'l':
	fpath = "etc/feast";
	break;
    case 'm':
	fpath = "etc/foreign_welcome";
	break;
    case 'n':
	fpath = "etc/foreign_expired_warn";
	break;
    case 'o':
	fpath = "etc/boardlist.help";
	break;
    case 'p':
	fpath = "etc/board.help";
	break;

#ifdef PLAY_ANGEL
    case 'y':
	fpath = "etc/angel_notify";
	break;
#endif

    default:
	return FULLUPDATE;
    }
    aborted = vedit(fpath, NA, NULL);
    vmsg("\n\n系統檔案[%s]：%s", fpath,
	 (aborted == -1) ? "未改變" : "更新完畢");
    return FULLUPDATE;
}

static int add_board_record(const boardheader_t *board)
{
    int bid;
    if ((bid = getbnum("")) > 0) {
	substitute_record(fn_board, board, sizeof(boardheader_t), bid);
	reset_board(bid);
        sort_bcache(); 
    } else if (append_record(fn_board, (fileheader_t *)board, sizeof(boardheader_t)) == -1) {
	return -1;
    } else {
	addbrd_touchcache();
    }
    return 0;
}

int
m_newbrd(int recover)
{
    boardheader_t   newboard;
    char            ans[4];
    char            genbuf[200];

    stand_title("建立新板");
    memset(&newboard, 0, sizeof(newboard));

    newboard.gid = class_bid;
    if (newboard.gid == 0) {
	vmsg("請先選擇一個類別再開板!");
	return -1;
    }
    do {
	if (!getdata(3, 0, msg_bid, newboard.brdname,
		     sizeof(newboard.brdname), DOECHO))
	    return -1;
    } while (invalid_brdname(newboard.brdname));

    do {
	getdata(6, 0, "看板類別：", genbuf, 5, DOECHO);
	if (strlen(genbuf) == 4)
	    break;
    } while (1);

    strncpy(newboard.title, genbuf, 4);

    newboard.title[4] = ' ';

    getdata(8, 0, "看板主題：", genbuf, BTLEN + 1, DOECHO);
    if (genbuf[0])
	strlcpy(newboard.title + 7, genbuf, sizeof(newboard.title) - 7);
    setbpath(genbuf, newboard.brdname);

    if (!recover && 
        (getbnum(newboard.brdname) > 0 || mkdir(genbuf, 0755) == -1)) {
	vmsg("此看板已經存在! 請取不同英文板名");
	return -1;
    }
    newboard.brdattr = BRD_NOTRAN;

    if (HAS_PERM(PERM_SYSOP)) {
	move(1, 0);
	clrtobot();
	newboard.brdattr = setperms(newboard.brdattr, str_permboard);
	move(1, 0);
	clrtobot();
    }
    getdata(9, 0, "是看板? (N:目錄) (Y/n)：", genbuf, 3, LCECHO);
    if (genbuf[0] == 'n')
	newboard.brdattr |= BRD_GROUPBOARD;

    if (newboard.brdattr & BRD_GROUPBOARD)
	strncpy(newboard.title + 5, "Σ", 2);
    else if (newboard.brdattr & BRD_NOTRAN)
	strncpy(newboard.title + 5, "◎", 2);
    else
	strncpy(newboard.title + 5, "●", 2);

    newboard.level = 0;
    getdata(11, 0, "板主名單：", newboard.BM, sizeof(newboard.BM), DOECHO);
#ifdef CHESSCOUNTRY
    if (getdata_str(12, 0, "設定棋國 (0)無 (1)五子棋 (2)象棋", ans,
		sizeof(ans), LCECHO, "0")){
	newboard.chesscountry = atoi(ans);
	if (newboard.chesscountry > CHESSCODE_MAX ||
		newboard.chesscountry < CHESSCODE_NONE)
	    newboard.chesscountry = CHESSCODE_NONE;
    }
#endif /* defined(CHESSCOUNTRY) */

    if (HAS_PERM(PERM_SYSOP) && !(newboard.brdattr & BRD_HIDE)) {
	getdata_str(14, 0, "設定讀寫權限(Y/N)？", ans, sizeof(ans), LCECHO, "N");
	if (*ans == 'y') {
	    getdata_str(15, 0, "限制 [R]閱\讀 (P)發表？", ans, sizeof(ans), LCECHO, "R");
	    if (*ans == 'p')
		newboard.brdattr |= BRD_POSTMASK;
	    else
		newboard.brdattr &= (~BRD_POSTMASK);

	    move(1, 0);
	    clrtobot();
	    bperm_msg(&newboard);
	    newboard.level = setperms(newboard.level, str_permid);
	    clear();
	}
    }

    add_board_record(&newboard);
    getbcache(class_bid)->childcount = 0;
    pressanykey();
    setup_man(&newboard, NULL);
    outs("\n新板成立");
    post_newboard(newboard.title, newboard.brdname, newboard.BM);
    log_usies("NewBoard", newboard.title);
    pressanykey();
    return 0;
}

int make_symbolic_link(const char *bname, int gid)
{
    boardheader_t   newboard;
    int bid;
    
    bid = getbnum(bname);
    memset(&newboard, 0, sizeof(newboard));

    /*
     * known issue:
     *   These two stuff will be used for sorting.  But duplicated brdnames
     *   may cause wrong binary-search result.  So I replace the last 
     *   letters of brdname to '~'(ascii code 126) in order to correct the
     *   resuilt, thought I think it's a dirty solution.
     *
     *   Duplicate entry with same brdname may cause wrong result, if
     *   searching by key brdname.  But people don't need to know a board
     *   is symbolic, so just let SYSOP know it. You may want to read
     *   board.c:load_boards().
     */

    strlcpy(newboard.brdname, bname, sizeof(newboard.brdname));
    newboard.brdname[strlen(bname) - 1] = '~';
    strlcpy(newboard.title, bcache[bid - 1].title, sizeof(newboard.title));
    strcpy(newboard.title + 5, "＠看板連結");

    newboard.gid = gid;
    BRD_LINK_TARGET(&newboard) = bid;
    newboard.brdattr = BRD_NOTRAN | BRD_SYMBOLIC;

    if (add_board_record(&newboard) < 0)
	return -1;
    return bid;
}

int make_symbolic_link_interactively(int gid)
{
    char buf[32];

    CompleteBoard(msg_bid, buf);
    if (!buf[0])
	return -1;

    stand_title("建立看板連結");

    if (make_symbolic_link(buf, gid) < 0) {
	vmsg("看板連結建立失敗");
	return -1;
    }
    log_usies("NewSymbolic", buf);
    return 0;
}

static int
auto_scan(char fdata[][STRLEN], char ans[])
{
    int             good = 0;
    int             count = 0;
    int             i;
    char            temp[10];

    if (!strncmp(fdata[2], "小", 2) || strstr(fdata[2], "丫")
	|| strstr(fdata[2], "誰") || strstr(fdata[2], "不")) {
	ans[0] = '0';
	return 1;
    }
    strncpy(temp, fdata[2], 2);
    temp[2] = '\0';

    /* 疊字 */
    if (!strncmp(temp, &(fdata[2][2]), 2)) {
	ans[0] = '0';
	return 1;
    }
    if (strlen(fdata[2]) >= 6) {
	if (strstr(fdata[2], "陳水扁")) {
	    ans[0] = '0';
	    return 1;
	}
	if (strstr("趙錢孫李周吳鄭王", temp))
	    good++;
	else if (strstr("杜顏黃林陳官余辛劉", temp))
	    good++;
	else if (strstr("蘇方吳呂李邵張廖應蘇", temp))
	    good++;
	else if (strstr("徐謝石盧施戴翁唐", temp))
	    good++;
    }
    if (!good)
	return 0;

    if (!strcmp(fdata[3], fdata[4]) ||
	!strcmp(fdata[3], fdata[5]) ||
	!strcmp(fdata[4], fdata[5])) {
	ans[0] = '4';
	return 5;
    }
    if (strstr(fdata[3], "大")) {
	if (strstr(fdata[3], "台") || strstr(fdata[3], "淡") ||
	    strstr(fdata[3], "交") || strstr(fdata[3], "政") ||
	    strstr(fdata[3], "清") || strstr(fdata[3], "警") ||
	    strstr(fdata[3], "師") || strstr(fdata[3], "銘傳") ||
	    strstr(fdata[3], "中央") || strstr(fdata[3], "成") ||
	    strstr(fdata[3], "輔") || strstr(fdata[3], "東吳"))
	    good++;
    } else if (strstr(fdata[3], "女中"))
	good++;

    if (strstr(fdata[4], "地球") || strstr(fdata[4], "宇宙") ||
	strstr(fdata[4], "信箱")) {
	ans[0] = '2';
	return 3;
    }
    if (strstr(fdata[4], "市") || strstr(fdata[4], "縣")) {
	if (strstr(fdata[4], "路") || strstr(fdata[4], "街")) {
	    if (strstr(fdata[4], "號"))
		good++;
	}
    }
    for (i = 0; fdata[5][i]; i++) {
	if (isdigit((int)fdata[5][i]))
	    count++;
    }

    if (count <= 4) {
	ans[0] = '3';
	return 4;
    } else if (count >= 7)
	good++;

    if (good >= 3) {
	ans[0] = 'y';
	return -1;
    } else
	return 0;
}

/* 處理 Register Form */
int
scan_register_form(const char *regfile, int automode, int neednum)
{
    char            genbuf[200];
    char    *logfile = "register.log";
    char    *field[] = {
	"uid", "ident", "name", "career", "addr", "phone", "email", NULL
    };
    char    *finfo[] = {
	"帳號", "身分證號", "真實姓名", "服務單位", "目前住址",
	"連絡電話", "電子郵件信箱", NULL
    };
    char    *reason[] = {
	"輸入真實姓名",
	"詳填「(畢業)學校及『系』『級』」或「服務單位(含所屬縣市及職稱)」",
	"填寫完整的住址資料 (含縣市名稱, 台北市請含行政區域）",
	"詳填連絡電話 (含區域碼, 中間不用加 \"-\", \"(\", \")\"等符號",
	"確實填寫註冊申請表",
	"用中文填寫申請單",
	"輸入真實身分證字號",
	NULL
    };
    char    *autoid = "AutoScan";
    userec_t        muser;
    FILE           *fn, *fout, *freg;
    char            fdata[7][STRLEN];
    char            fname[STRLEN], buf[STRLEN];
    char            ans[4], *ptr, *uid;
    int             n = 0, unum = 0;
    int             nSelf = 0, nAuto = 0;

    uid = cuser.userid;
    snprintf(fname, sizeof(fname), "%s.tmp", regfile);
    move(2, 0);
    if (dashf(fname)) {
	if (neednum == 0) {	/* 自己進 Admin 來審的 */
	    vmsg("其他 SYSOP 也在審核註冊申請單");
	}
	return -1;
    }
    Rename(regfile, fname);
    if ((fn = fopen(fname, "r")) == NULL) {
	vmsg("系統錯誤，無法讀取註冊資料檔: %s", fname);
	return -1;
    }
    if (neednum) {		/* 被強迫審的 */
	move(1, 0);
	clrtobot();
	prints("各位具有站長權限的人，註冊單累積超過一百份了，麻煩您幫忙審 %d 份\n", neednum);
	outs("也就是大概二十分之一的數量，當然，您也可以多審\n沒審完之前，系統不會讓你跳出喲！謝謝");
	pressanykey();
    }
    while( fgets(genbuf, STRLEN, fn) ){
	memset(fdata, 0, sizeof(fdata));
	do {
	    if( genbuf[0] == '-' )
		break;
	    if ((ptr = (char *)strstr(genbuf, ": "))) {
		*ptr = '\0';
		for (n = 0; field[n]; n++) {
		    if (strcmp(genbuf, field[n]) == 0) {
			strlcpy(fdata[n], ptr + 2, sizeof(fdata[n]));
			if ((ptr = (char *)strchr(fdata[n], '\n')))
			    *ptr = '\0';
		    }
		}
	    }
	} while( fgets(genbuf, STRLEN, fn) );

	if ((unum = getuser(fdata[0], &muser)) == 0) {
	    move(2, 0);
	    clrtobot();
	    outs("系統錯誤，查無此人\n\n");
	    for (n = 0; field[n]; n++)
		prints("%s     : %s\n", finfo[n], fdata[n]);
	    pressanykey();
	    neednum--;
	} else {
	    neednum--;
	    if (automode)
		uid = autoid;

	    if ((!automode || !auto_scan(fdata, ans))) {
		uid = cuser.userid;

		move(1, 0);
		prints("帳號位置    ：%d\n", unum);
		user_display(&muser, 1);
		move(14, 0);
		prints("\033[1;32m------------- 請站長嚴格審核使用者資料，您還有 %d 份---------------\033[m\n", neednum);
	    	prints("  %-12s：%s\n", finfo[0], fdata[0]);
#ifdef FOREIGN_REG
		prints("0.%-12s：%s%s\n", finfo[2], fdata[2],
		       muser.uflag2 & FOREIGN ? " (外籍)" : "");
#else
		prints("0.%-12s：%s\n", finfo[2], fdata[2]);
#endif
		for (n = 3; field[n]; n++) {
		    prints("%d.%-12s：%s\n", n - 2, finfo[n], fdata[n]);
		}
		if (muser.userlevel & PERM_LOGINOK) {
		    ans[0] = getkey("此帳號已經完成註冊, "
				    "更新(Y/N/Skip)？[N] ");
		    if (ans[0] != 'y' && ans[0] != 's')
			ans[0] = 'd';
		} else {
		    if (search_ulist(unum) == NULL)
		        ans[0] = vmsg_lines(22, "是否接受此資料(Y/N/Q/Del/Skip)？[S])");
		    else
			ans[0] = 's';
		    if ('A' <= ans[0] && ans[0] <= 'Z')
			ans[0] += 32;
		    if (ans[0] != 'y' && ans[0] != 'n' && ans[0] != 'q' &&
			ans[0] != 'd' && !('0' <= ans[0] && ans[0] <= '4'))
			ans[0] = 's';
		    ans[1] = 0;
		}
		nSelf++;
	    } else
		nAuto++;
	    if (neednum > 0 && ans[0] == 'q') {
		move(2, 0);
		clrtobot();
		vmsg("沒審完不能退出");
		ans[0] = 's';
	    }
	    switch (ans[0]) {
	    case 'q':
		if ((freg = fopen(regfile, "a"))) {
		    for (n = 0; field[n]; n++)
			fprintf(freg, "%s: %s\n", field[n], fdata[n]);
		    fprintf(freg, "----\n");
		    while (fgets(genbuf, STRLEN, fn))
			fputs(genbuf, freg);
		    fclose(freg);
		}
	    case 'd':
		break;
	    case '0':
	    case '1':
	    case '2':
	    case '3':
	    case '4':
	    case 'n':
		if (ans[0] == 'n') {
		    for (n = 0; field[n]; n++)
			prints("%s: %s\n", finfo[n], fdata[n]);
		    move(9, 0);
		    outs("請提出退回申請表原因，按 <enter> 取消\n");
		    for (n = 0; reason[n]; n++)
			prints("%d) 請%s\n", n, reason[n]);
		} else
		    buf[0] = ans[0];
		if (ans[0] != 'n' ||
		    getdata(10 + n, 0, "退回原因：", buf, 60, DOECHO))
		    if ((buf[0] - '0') >= 0 && (buf[0] - '0') < n) {
			int             i;
			fileheader_t    mhdr;
			char            title[128], buf1[80];
			FILE           *fp;

			sethomepath(buf1, muser.userid);
			stampfile(buf1, &mhdr);
			strlcpy(mhdr.owner, cuser.userid, sizeof(mhdr.owner));
			strlcpy(mhdr.title, "[註冊失敗]", TTLEN);
			mhdr.filemode = 0;
			sethomedir(title, muser.userid);
			if (append_record(title, &mhdr, sizeof(mhdr)) != -1) {
			    fp = fopen(buf1, "w");
			    
			    for(i = 0; buf[i] && i < sizeof(buf); i++){
				if (!isdigit((int)buf[i]))
				    continue;
				fprintf(fp, "[退回原因] 請%s\n",
					reason[buf[i] - '0']);
			    }

			    fclose(fp);
			}
			if ((fout = fopen(logfile, "a"))) {
			    for (n = 0; field[n]; n++)
				fprintf(fout, "%s: %s\n", field[n], fdata[n]);
			    fprintf(fout, "Date: %s\n", Cdate(&now));
			    fprintf(fout, "Rejected: %s [%s]\n----\n",
				    uid, buf);
			    fclose(fout);
			}
			break;
		    }
		move(10, 0);
		clrtobot();
		outs("取消退回此註冊申請表");
	    case 's':
		if ((freg = fopen(regfile, "a"))) {
		    for (n = 0; field[n]; n++)
			fprintf(freg, "%s: %s\n", field[n], fdata[n]);
		    fprintf(freg, "----\n");
		    fclose(freg);
		}
		break;
	    default:
		outs("以下使用者資料已經更新:\n");
		mail_muser(muser, "[註冊成功\囉]", "etc/registered");
#if FOREIGN_REG_DAY > 0
		if(muser.uflag2 & FOREIGN)
		    mail_muser(muser, "[出入境管理局]", "etc/foreign_welcome");
#endif
		muser.userlevel |= (PERM_LOGINOK | PERM_POST);
		strlcpy(muser.realname, fdata[2], sizeof(muser.realname));
		strlcpy(muser.address, fdata[4], sizeof(muser.address));
		strlcpy(muser.email, fdata[6], sizeof(muser.email));
		snprintf(genbuf, sizeof(genbuf), "%s:%s:%s",
			 fdata[5], fdata[3], uid);
		strlcpy(muser.justify, genbuf, sizeof(muser.justify));
		passwd_update(unum, &muser);

		sethomefile(buf, muser.userid, "justify");
		log_file(buf, LOG_CREAT, genbuf);

		if ((fout = fopen(logfile, "a"))) {
		    for (n = 0; field[n]; n++)
			fprintf(fout, "%s: %s\n", field[n], fdata[n]);
		    fprintf(fout, "Date: %s\n", Cdate(&now));
		    fprintf(fout, "Approved: %s\n", uid);
		    fprintf(fout, "----\n");
		    fclose(fout);
		}
		sethomefile(genbuf, muser.userid, "justify.wait");
		unlink(genbuf);
		break;
	    }
	}
    }
    fclose(fn);
    unlink(fname);

    move(0, 0);
    clrtobot();

    move(5, 0);
    prints("您審了 %d 份註冊單，AutoScan 審了 %d 份", nSelf, nAuto);

    pressanykey();
    return (0);
}

int
m_register(void)
{
    FILE           *fn;
    int             x, y, wid, len;
    char            ans[4];
    char            genbuf[200];

    if ((fn = fopen(fn_register, "r")) == NULL) {
	outs("目前並無新註冊資料");
	return XEASY;
    }
    stand_title("審核使用者註冊資料");
    y = 2;
    x = wid = 0;

    while (fgets(genbuf, STRLEN, fn) && x < 65) {
	if (strncmp(genbuf, "uid: ", 5) == 0) {
	    move(y++, x);
	    outs(genbuf + 5);
	    len = strlen(genbuf + 5);
	    if (len > wid)
		wid = len;
	    if (y >= t_lines - 3) {
		y = 2;
		x += wid + 2;
	    }
	}
    }
    fclose(fn);
    getdata(b_lines - 1, 0, "開始審核嗎(Auto/Yes/No)？[N] ", ans, sizeof(ans), LCECHO);
    if (ans[0] == 'a')
	scan_register_form(fn_register, 1, 0);
    else if (ans[0] == 'y')
	scan_register_form(fn_register, 0, 0);

    return 0;
}

int
cat_register(void)
{
    if (system("cat register.new.tmp >> register.new") == 0 &&
	unlink("register.new.tmp") == 0)
	vmsg("OK 嚕~~ 繼續去奮鬥吧!!");
    else
	vmsg("沒辦法CAT過去呢 去檢查一下系統吧!!");
    return 0;
}

static void
give_id_money(const char *user_id, int money, FILE * log_fp, const char *mail_title, time4_t t)
{
    char            tt[TTLEN + 1] = {0};

    if (deumoney(searchuser(user_id, NULL), money) < 0) { // TODO if searchuser() return 0
	move(12, 0);
	clrtoeol();
	prints("id:%s money:%d 不對吧!!", user_id, money);
	pressanykey();
    } else {
	fprintf(log_fp, "%d %s %d", (int)t, user_id, money);
	snprintf(tt, sizeof(tt), "%s : %d ptt 幣", mail_title, money);
	mail_id(user_id, tt, "etc/givemoney.why", "[PTT 銀行]");
    }
}

int
give_money(void)
{
    FILE           *fp, *fp2;
    char           *ptr, *id, *mn;
    char            buf[200] = "", tt[TTLEN + 1] = "";
    struct tm      *pt = localtime4(&now);
    int             to_all = 0, money = 0;

    getdata(0, 0, "指定使用者(S) 全站使用者(A) 取消(Q)？[S]", buf, sizeof(buf), LCECHO);
    if (buf[0] == 'q')
	return 1;
    else if (buf[0] == 'a') {
	to_all = 1;
	getdata(1, 0, "發多少錢呢?", buf, 20, DOECHO);
	money = atoi(buf);
	if (money <= 0) {
	    move(2, 0);
	    vmsg("輸入錯誤!!");
	    return 1;
	}
    } else {
	if (vedit("etc/givemoney.txt", NA, NULL) < 0)
	    return 1;
    }

    clear();
    getdata(0, 0, "要發錢了嗎(Y/N)[N]", buf, 3, LCECHO);
    if (buf[0] != 'y')
	return 1;

    if (!(fp2 = fopen("etc/givemoney.log", "a")))
	return 1;
    strftime(buf, sizeof(buf), "%Y/%m/%d/%H:%M", pt);
    fprintf(fp2, "%s\n", buf);


    getdata(1, 0, "紅包袋標題 ：", tt, TTLEN, DOECHO);
    move(2, 0);

    vmsg("編紅包袋內容");
    if (vedit("etc/givemoney.why", NA, NULL) < 0) {
        fclose(fp2);
	return 1;
    }

    stand_title("發錢中...");
    if (to_all) {
	int             i, unum;
	for (unum = SHM->number, i = 0; i < unum; i++) {
	    if (bad_user_id(SHM->userid[i]))
		continue;
	    id = SHM->userid[i];
	    give_id_money(id, money, fp2, tt, now);
	}
	//something wrong @ _ @
	    //give_money_post("全站使用者", atoi(money));
    } else {
	if (!(fp = fopen("etc/givemoney.txt", "r+"))) {
	    fclose(fp2);
	    return 1;
	}
	while (fgets(buf, sizeof(buf), fp)) {
	    clear();
	    if (!(ptr = strchr(buf, ':')))
		continue;
	    *ptr = '\0';
	    id = buf;
	    mn = ptr + 1;
	    give_id_money(id, atoi(mn), fp2, tt, now);
	    give_money_post(id, atoi(mn));
	}
	fclose(fp);
    }

    fclose(fp2);
    pressanykey();
    return FULLUPDATE;
}
