/* $Id$ */
#include "bbs.h"

static char    * const sex[8] = {
    MSG_BIG_BOY, MSG_BIG_GIRL, MSG_LITTLE_BOY, MSG_LITTLE_GIRL,
    MSG_MAN, MSG_WOMAN, MSG_PLANT, MSG_MIME
};

#ifdef CHESSCOUNTRY
static const char * const chess_photo_name[2] = {
    "photo_fivechess", "photo_cchess"
};

static const char * const chess_type[2] = {
    "五子棋", "象棋"
};
#endif

int
kill_user(int num)
{
  userec_t u;
  memset(&u, 0, sizeof(userec_t));
  log_usies("KILL", getuserid(num));
  setuserid(num, "");
  passwd_update(num, &u);
  return 0;
}
int
u_loginview(void)
{
    int             i;
    unsigned int    pbits = cuser.loginview;

    clear();
    move(4, 0);
    for (i = 0; i < NUMVIEWFILE; i++)
	prints("    %c. %-20s %-15s \n", 'A' + i,
	       loginview_file[i][1], ((pbits >> i) & 1 ? "ˇ" : "Ｘ"));

    clrtobot();
    while ((i = getkey("請按 [A-N] 切換設定，按 [Return] 結束："))!='\r')
       {
	i = i - 'a';
	if (i >= NUMVIEWFILE || i < 0)
	    bell();
	else {
	    pbits ^= (1 << i);
	    move(i + 4, 28);
	    outs((pbits >> i) & 1 ? "ˇ" : "Ｘ");
	}
    }

    if (pbits != cuser.loginview) {
	cuser.loginview = pbits;
	passwd_update(usernum, &cuser);
    }
    return 0;
}

void
user_display(const userec_t * u, int real)
{
    int             diff = 0;
    char            genbuf[200];

    clrtobot();
    prints(
	   "        \033[30;41m┴┬┴┬┴┬\033[m  \033[1;30;45m    使 用 者"
	   " 資 料        "
	   "     \033[m  \033[30;41m┴┬┴┬┴┬\033[m\n");
    prints("                代號暱稱: %s(%s)\n"
	   "                真實姓名: %s"
#if FOREIGN_REG_DAY > 0
	   " %s%s"
#elif defined(FOREIGN_REG)
	   " %s"
#endif
	   "\n"
	   "                居住住址: %s\n"
	   "                電子信箱: %s\n"
	   "                性    別: %s\n"
	   "                銀行帳戶: %d 銀兩\n",
	   u->userid, u->username, u->realname,
#if FOREIGN_REG_DAY > 0
	   u->uflag2 & FOREIGN ? "(外籍: " : "",
	   u->uflag2 & FOREIGN ?
		(u->uflag2 & LIVERIGHT) ? "永久居留)" : "未取得居留權)"
		: "",
#elif defined(FOREIGN_REG)
	   u->uflag2 & FOREIGN ? "(外籍)" : "",
#endif
	   u->address, u->email,
	   sex[u->sex % 8], u->money);

    sethomedir(genbuf, u->userid);
    prints("                私人信箱: %d 封  (購買信箱: %d 封)\n"
	   "                手機號碼: %010d\n"
	   "                生    日: %02i/%02i/%02i\n"
	   "                小雞名字: %s\n",
	   get_num_records(genbuf, sizeof(fileheader_t)),
	   u->exmailbox, u->mobile,
	   u->month, u->day, u->year % 100, u->mychicken.name);
#ifdef PLAY_ANGEL
    if (real)
	prints("                小 天 使: %s\n",
		u->myangel[0] ? u->myangel : "無");
#endif
    prints("                註冊日期: %s", ctime4(&u->firstlogin));
    prints("                前次光臨: %s", ctime4(&u->lastlogin));
    prints("                前次點歌: %s", ctime4(&u->lastsong));
    prints("                上站文章: %d 次 / %d 篇\n",
	   u->numlogins, u->numposts);

#ifdef CHESSCOUNTRY
    {
	int i, j;
	FILE* fp;
	for(i = 0; i < 2; ++i){
	    sethomefile(genbuf, u->userid, chess_photo_name[i]);
	    fp = fopen(genbuf, "r");
	    if(fp != NULL){
		for(j = 0; j < 11; ++j)
		    fgets(genbuf, 200, fp);
		fgets(genbuf, 200, fp);
		prints("%12s棋國自我描述: %s", chess_type[i], genbuf + 11);
	    }
	}
    }
#endif

    if (real) {
	strcpy(genbuf, "bTCPRp#@XWBA#VSM0123456789ABCDEF");
	for (diff = 0; diff < 32; diff++)
	    if (!(u->userlevel & (1 << diff)))
		genbuf[diff] = '-';
	prints("                認證資料: %s\n"
	       "                user權限: %s\n",
	       u->justify, genbuf);
    } else {
	diff = (now - login_start_time) / 60;
	prints("                停留期間: %d 小時 %2d 分\n",
	       diff / 60, diff % 60);
    }

    /* Thor: 想看看這個 user 是那些板的板主 */
    if (u->userlevel >= PERM_BM) {
	int             i;
	boardheader_t  *bhdr;

	outs("                擔任板主: ");

	for (i = 0, bhdr = bcache; i < numboards; i++, bhdr++) {
	    if (is_uBM(bhdr->BM, u->userid)) {
		outs(bhdr->brdname);
		outc(' ');
	    }
	}
	outc('\n');
    }
    outs("        \033[30;41m┴┬┴┬┴┬┴┬┴┬┴┬┴┬┴┬┴┬┴┬┴┬┴"
	 "┬┴┬┴┬┴┬\033[m");

    outs((u->userlevel & PERM_LOGINOK) ?
	 "\n您的註冊程序已經完成，歡迎加入本站" :
	 "\n如果要提昇權限，請參考本站公佈欄辦理註冊");

#ifdef NEWUSER_LIMIT
    if ((u->lastlogin - u->firstlogin < 3 * 86400) && !HAS_PERM(PERM_POST))
	outs("\n新手上路，三天後開放權限");
#endif
}

void
mail_violatelaw(const char *crime, const char *police, const char *reason, const char *result)
{
    char            genbuf[200];
    fileheader_t    fhdr;
    FILE           *fp;

    sethomepath(genbuf, crime);
    stampfile(genbuf, &fhdr);
    if (!(fp = fopen(genbuf, "w")))
	return;
    fprintf(fp, "作者: [Ptt法院]\n"
	    "標題: [報告] 違法判決報告\n"
	    "時間: %s\n"
	    "\033[1;32m%s\033[m判決：\n     \033[1;32m%s\033[m"
	    "因\033[1;35m%s\033[m行為，\n違反本站站規，處以\033[1;35m%s\033[m，特此通知"
	    "\n請到 PttLaw 查詢相關法規資訊，並到 Play-Pay-ViolateLaw 繳交罰單",
	    ctime4(&now), police, crime, reason, result);
    fclose(fp);
    strcpy(fhdr.title, "[報告] 違法判決報告");
    strcpy(fhdr.owner, "[Ptt法院]");
    sethomedir(genbuf, crime);
    append_record(genbuf, &fhdr, sizeof(fhdr));
}

static void
violate_law(userec_t * u, int unum)
{
    char            ans[4], ans2[4];
    char            reason[128];
    move(1, 0);
    clrtobot();
    move(2, 0);
    outs("(1)Cross-post (2)亂發廣告信 (3)亂發連鎖信\n");
    outs("(4)騷擾站上使用者 (8)其他以罰單處置行為\n(9)砍 id 行為\n");
    getdata(5, 0, "(0)結束", ans, 3, DOECHO);
    switch (ans[0]) {
    case '1':
	strcpy(reason, "Cross-post");
	break;
    case '2':
	strcpy(reason, "亂發廣告信");
	break;
    case '3':
	strcpy(reason, "亂發連鎖信");
	break;
    case '4':
	while (!getdata(7, 0, "請輸入被檢舉理由以示負責：", reason, 50, DOECHO));
	strcat(reason, "[騷擾站上使用者]");
	break;
    case '8':
    case '9':
	while (!getdata(6, 0, "請輸入理由以示負責：", reason, 50, DOECHO));
	break;
    default:
	return;
    }
    getdata(7, 0, msg_sure_ny, ans2, 3, LCECHO);
    if (*ans2 != 'y')
	return;
    if (ans[0] == '9') {
	char            src[STRLEN], dst[STRLEN];
	sethomepath(src, u->userid);
	snprintf(dst, sizeof(dst), "tmp/%s", u->userid);
	friend_delete_all(u->userid, FRIEND_ALOHA);
	Rename(src, dst);
	post_violatelaw(u->userid, cuser.userid, reason, "砍除 ID");
        kill_user(unum);

    } else {
	u->userlevel |= PERM_VIOLATELAW;
	u->vl_count++;
	passwd_update(unum, u);
	post_violatelaw(u->userid, cuser.userid, reason, "罰單處份");
	mail_violatelaw(u->userid, cuser.userid, reason, "罰單處份");
    }
    pressanykey();
}

static void Customize(void)
{
    char    done = 0, mindbuf[5];
    int     key;
    char    *wm[3] = {"一般", "進階", "未來"};
#ifdef PLAY_ANGEL
    char    *am[4] = {"男女皆可", "限女生", "限男生", "暫不接受新的小主人"};
#endif

    showtitle("個人化設定", "個人化設定");
    memcpy(mindbuf, &currutmp->mind, 4);
    mindbuf[4] = 0;
    while( !done ){
	move(2, 0);
	outs("您目前的個人化設定: ");
	move(4, 0);
	prints("%-30s%10s\n", "A. 水球模式",
	       wm[(cuser.uflag2 & WATER_MASK)]);
	prints("%-30s%10s\n", "B. 接受站外信", REJECT_OUTTAMAIL ? "否" : "是");
	prints("%-30s%10s\n", "C. 新板自動進我的最愛",
	       ((cuser.uflag2 & FAVNEW_FLAG) ? "是" : "否"));
	prints("%-30s%10s\n", "D. 目前的心情", mindbuf);
	prints("%-30s%10s\n", "E. 高亮度顯示我的最愛", 
	       ((cuser.uflag2 & FAVNOHILIGHT) ? "否" : "是"));
#ifdef PLAY_ANGEL
	if( HAS_PERM(PERM_ANGEL) ){
	    prints("%-30s%10s\n", "F. 開放小主人詢問", 
		    (REJECT_QUESTION ? "否" : "是"));
	    prints("%-30s%10s\n", "G. 接受的小主人性別", am[ANGEL_STATUS()]);
	    key = getkey("請按 [A-G] 切換設定，按 [Return] 結束：");
	}else
#endif
	    key = getkey("請按 [A-E] 切換設定，按 [Return] 結束：");

	switch (key) {
	case 'a':{
	    int     currentset = cuser.uflag2 & WATER_MASK;
	    currentset = (currentset + 1) % 3;
	    cuser.uflag2 &= ~WATER_MASK;
	    cuser.uflag2 |= currentset;
	    vmsg("修正水球模式後請正常離線再重新上線");
	}
	    break;
	case 'b':
	    cuser.uflag2 ^= REJ_OUTTAMAIL;
	    break;
	case 'c':
	    cuser.uflag2 ^= FAVNEW_FLAG;
	    break;
	case 'd':{
	    getdata(b_lines - 1, 0, "現在的心情? ",
		    mindbuf, sizeof(mindbuf), DOECHO);
	    if (strcmp(mindbuf, "通緝") == 0)
		vmsg("不可以把自己設通緝啦!");
	    else if (strcmp(mindbuf, "壽星") == 0)
		vmsg("你不是今天生日欸!");
	    else
		memcpy(currutmp->mind, mindbuf, 4);
	}
	    break;
	case 'e':
	    cuser.uflag2 ^= FAVNOHILIGHT;
	    break;

#ifdef PLAY_ANGEL
	case 'f':
	    if( HAS_PERM(PERM_ANGEL) ){
		SwitchBeingAngel();
		break;
	    }
	    done = 1;
	    break;

	case 'g':
	    if( HAS_PERM(PERM_ANGEL) ){
		SwitchAngelSex(ANGEL_STATUS() + 1);
		break;
	    }
#endif

	default:
	    done = 1;
	}
	passwd_update(usernum, &cuser);
    }
    vmsg("設定完成");
}

void
uinfo_query(userec_t * u, int real, int unum)
{
    userec_t        x;
    register int    i = 0, fail, mail_changed;
    int             uid, ans;
    char            buf[STRLEN], *p;
    char            genbuf[200], reason[50];
    int money = 0;
    int             flag = 0, temp = 0, money_change = 0;

    fail = mail_changed = 0;

    memcpy(&x, u, sizeof(userec_t));
    ans = getans(real ?
	    "(1)改資料(2)設密碼(3)設權限(4)砍帳號(5)改ID"
	    "(6)殺/復活寵物(7)審判 [0]結束 " :
	    "請選擇 (1)修改資料 (2)設定密碼 (C) 個人化設定 ==> [0]結束 ");

    if (ans > '2' && ans != 'C' && ans != 'c' && !real)
	ans = '0';

    if (ans == '1' || ans == '3') {
	clear();
	i = 1;
	move(i++, 0);
	outs(msg_uid);
	outs(x.userid);
    }
    switch (ans) {
    case 'C':
    case 'c':
	Customize();
	return;
    case '7':
	violate_law(&x, unum);
	return;
    case '1':
	move(0, 0);
	outs("請逐項修改。");

	getdata_buf(i++, 0, " 暱 稱  ：", x.username,
		    sizeof(x.username), DOECHO);
	if (real) {
	    getdata_buf(i++, 0, "真實姓名：",
			x.realname, sizeof(x.realname), DOECHO);
	    getdata_buf(i++, 0, "居住地址：",
			x.address, sizeof(x.address), DOECHO);
	}
	snprintf(buf, sizeof(buf), "%010d", x.mobile);
	getdata_buf(i++, 0, "手機號碼：", buf, 11, LCECHO);
	x.mobile = atoi(buf);
	getdata_str(i++, 0, "電子信箱[變動要重新認證]：", buf, 50, DOECHO,
		    x.email);
	if (strcmp(buf, x.email) && strchr(buf, '@')) {
	    strlcpy(x.email, buf, sizeof(x.email));
	    mail_changed = 1 - real;
	}
	snprintf(genbuf, sizeof(genbuf), "%i", (u->sex + 1) % 8);
	getdata_str(i++, 0, "性別 (1)葛格 (2)姐接 (3)底迪 (4)美眉 (5)薯叔 "
		    "(6)阿姨 (7)植物 (8)礦物：",
		    buf, 3, DOECHO, genbuf);
	if (buf[0] >= '1' && buf[0] <= '8')
	    x.sex = (buf[0] - '1') % 8;
	else
	    x.sex = u->sex % 8;

	while (1) {
	    int             len;

	    snprintf(genbuf, sizeof(genbuf), "%02i/%02i/%02i",
		     u->month, u->day, u->year % 100);
	    len = getdata_str(i, 0, "生日 月月/日日/西元：", buf, 9,
			      DOECHO, genbuf);
	    if (len && len != 8)
		continue;
	    if (!len) {
		x.month = u->month;
		x.day = u->day;
		x.year = u->year;
	    } else if (len == 8) {
		x.month = (buf[0] - '0') * 10 + (buf[1] - '0');
		x.day = (buf[3] - '0') * 10 + (buf[4] - '0');
		x.year = (buf[6] - '0') * 10 + (buf[7] - '0');
	    } else
		continue;
	    if (!real && (x.month > 12 || x.month < 1 || x.day > 31 ||
			  x.day < 1 || x.year > 90 || x.year < 40))
		continue;
	    i++;
	    break;
	}

#ifdef PLAY_ANGEL
	if (real)
	    while (1) {
	        userec_t xuser;
		getdata_str(i, 0, "小天使：", buf, IDLEN + 1, DOECHO,
			x.myangel);
		if(buf[0] == 0 || (getuser(buf, &xuser) &&
			    (xuser.userlevel & PERM_ANGEL))){
		    strlcpy(x.myangel, xuser.userid, IDLEN + 1);
		    ++i;
		    break;
		}
	    }
#endif

#ifdef CHESSCOUNTRY
	{
	    int j, k;
	    FILE* fp;
	    for(j = 0; j < 2; ++j){
		sethomefile(genbuf, u->userid, chess_photo_name[j]);
		fp = fopen(genbuf, "r");
		if(fp != NULL){
		    FILE* newfp;
		    char mybuf[200];
		    for(k = 0; k < 11; ++k)
			fgets(genbuf, 200, fp);
		    fgets(genbuf, 200, fp);
		    chomp(genbuf);

		    snprintf(mybuf, 200, "%s棋國自我描述：", chess_type[j]);
		    getdata_buf(i, 0, mybuf, genbuf + 11, 80 - 11, DOECHO);
		    ++i;

		    sethomefile(mybuf, u->userid, chess_photo_name[j]);
		    strcat(mybuf, ".new");
		    if((newfp = fopen(mybuf, "w")) != NULL){
			rewind(fp);
			for(k = 0; k < 11; ++k){
			    fgets(mybuf, 200, fp);
			    fputs(mybuf, newfp);
			}
			fputs(genbuf, newfp);
			fputc('\n', newfp);

			fclose(newfp);

			sethomefile(genbuf, u->userid, chess_photo_name[j]);
			sethomefile(mybuf, u->userid, chess_photo_name[j]);
			strcat(mybuf, ".new");
			
			Rename(mybuf, genbuf);
		    }
		    fclose(fp);
		}
	    }
	}
#endif

	if (real) {
	    int l;
	    if (HAS_PERM(PERM_BBSADM)) {
		snprintf(genbuf, sizeof(genbuf), "%d", x.money);
		if (getdata_str(i++, 0, "銀行帳戶：", buf, 10, DOECHO, genbuf))
		    if ((l = atol(buf)) != 0) {
			if (l != x.money) {
			    money_change = 1;
			    money = x.money;
			    x.money = l;
			}
		    }
	    }
	    snprintf(genbuf, sizeof(genbuf), "%d", x.exmailbox);
	    if (getdata_str(i++, 0, "購買信箱數：", buf, 6,
			    DOECHO, genbuf))
		if ((l = atol(buf)) != 0)
		    x.exmailbox = (int)l;

	    getdata_buf(i++, 0, "認證資料：", x.justify,
			sizeof(x.justify), DOECHO);
	    getdata_buf(i++, 0, "最近光臨機器：",
			x.lasthost, sizeof(x.lasthost), DOECHO);

	    // XXX 一變數不要多用途亂用 "fail"
	    snprintf(genbuf, sizeof(genbuf), "%d", x.numlogins);
	    if (getdata_str(i++, 0, "上線次數：", buf, 10, DOECHO, genbuf))
		if ((fail = atoi(buf)) >= 0)
		    x.numlogins = fail;
	    snprintf(genbuf, sizeof(genbuf), "%d", u->numposts);
	    if (getdata_str(i++, 0, "文章數目：", buf, 10, DOECHO, genbuf))
		if ((fail = atoi(buf)) >= 0)
		    x.numposts = fail;
	    snprintf(genbuf, sizeof(genbuf), "%d", u->goodpost);
	    if (getdata_str(i++, 0, "優良文章數:", buf, 10, DOECHO, genbuf))
		if ((fail = atoi(buf)) >= 0)
		    x.goodpost = fail;
	    snprintf(genbuf, sizeof(genbuf), "%d", u->badpost);
	    if (getdata_str(i++, 0, "惡劣文章數:", buf, 10, DOECHO, genbuf))
		if ((fail = atoi(buf)) >= 0)
		    x.badpost = fail;
	    snprintf(genbuf, sizeof(genbuf), "%d", u->vl_count);
	    if (getdata_str(i++, 0, "違法記錄：", buf, 10, DOECHO, genbuf))
		if ((fail = atoi(buf)) >= 0)
		    x.vl_count = fail;

	    snprintf(genbuf, sizeof(genbuf),
		     "%d/%d/%d", u->five_win, u->five_lose, u->five_tie);
	    if (getdata_str(i++, 0, "五子棋戰績 勝/敗/和：", buf, 16, DOECHO,
			    genbuf))
		while (1) {
		    p = strtok(buf, "/\r\n");
		    if (!p)
			break;
		    x.five_win = atoi(p);
		    p = strtok(NULL, "/\r\n");
		    if (!p)
			break;
		    x.five_lose = atoi(p);
		    p = strtok(NULL, "/\r\n");
		    if (!p)
			break;
		    x.five_tie = atoi(p);
		    break;
		}
	    snprintf(genbuf, sizeof(genbuf),
		     "%d/%d/%d", u->chc_win, u->chc_lose, u->chc_tie);
	    if (getdata_str(i++, 0, "象棋戰績 勝/敗/和：", buf, 16, DOECHO,
			    genbuf))
		while (1) {
		    p = strtok(buf, "/\r\n");
		    if (!p)
			break;
		    x.chc_win = atoi(p);
		    p = strtok(NULL, "/\r\n");
		    if (!p)
			break;
		    x.chc_lose = atoi(p);
		    p = strtok(NULL, "/\r\n");
		    if (!p)
			break;
		    x.chc_tie = atoi(p);
		    break;
		}
#ifdef FOREIGN_REG
	    if (getdata_str(i++, 0, "住在 1)台灣 2)其他：", buf, 2, DOECHO, x.uflag2 & FOREIGN ? "2" : "1"))
		if ((fail = atoi(buf)) > 0){
		    if (fail == 2){
			x.uflag2 |= FOREIGN;
		    }
		    else
			x.uflag2 &= ~FOREIGN;
		}
	    if (x.uflag2 & FOREIGN)
		if (getdata_str(i++, 0, "永久居留權 1)是 2)否：", buf, 2, DOECHO, x.uflag2 & LIVERIGHT ? "1" : "2")){
		    if ((fail = atoi(buf)) > 0){
			if (fail == 1){
			    x.uflag2 |= LIVERIGHT;
			    x.userlevel |= (PERM_LOGINOK | PERM_POST);
			}
			else{
			    x.uflag2 &= ~LIVERIGHT;
			    x.userlevel &= ~(PERM_LOGINOK | PERM_POST);
			}
		    }
		}
#endif
	    fail = 0;
	}
	break;

    case '2':
	i = 19;
	if (!real) {
	    if (!getdata(i++, 0, "請輸入原密碼：", buf, PASSLEN, NOECHO) ||
		!checkpasswd(u->passwd, buf)) {
		outs("\n\n您輸入的密碼不正確\n");
		fail++;
		break;
	    }
	} else {
	    char            witness[3][32];
	    for (i = 0; i < 3; i++) {
		if (!getdata(19 + i, 0, "請輸入協助證明之使用者：",
			     witness[i], sizeof(witness[i]), DOECHO)) {
		    outs("\n不輸入則無法更改\n");
		    fail++;
		    break;
		} else if (!(uid = searchuser(witness[i], NULL))) {
		    outs("\n查無此使用者\n");
		    fail++;
		    break;
		} else {
		    userec_t        atuser;
		    passwd_query(uid, &atuser);
		    if (now - atuser.firstlogin < 6 * 30 * 24 * 60 * 60) {
			outs("\n註冊未超過半年，請重新輸入\n");
			i--;
		    }
		}
	    }
	    if (i < 3)
		break;
	    else
		i = 20;
	}

	if (!getdata(i++, 0, "請設定新密碼：", buf, PASSLEN, NOECHO)) {
	    outs("\n\n密碼設定取消, 繼續使用舊密碼\n");
	    fail++;
	    break;
	}
	strncpy(genbuf, buf, PASSLEN);

	getdata(i++, 0, "請檢查新密碼：", buf, PASSLEN, NOECHO);
	if (strncmp(buf, genbuf, PASSLEN)) {
	    outs("\n\n新密碼確認失敗, 無法設定新密碼\n");
	    fail++;
	    break;
	}
	buf[8] = '\0';
	strncpy(x.passwd, genpasswd(buf), PASSLEN);
	if (real)
	    x.userlevel &= (!PERM_LOGINOK);
	break;

    case '3':
	i = setperms(x.userlevel, str_permid);
	if (i == x.userlevel)
	    fail++;
	else {
	    flag = 1;
	    temp = x.userlevel;
	    x.userlevel = i;
	}
	break;

    case '4':
	i = QUIT;
	break;

    case '5':
	if (getdata_str(b_lines - 3, 0, "新的使用者代號：", genbuf, IDLEN + 1,
			DOECHO, x.userid)) {
	    if (searchuser(genbuf, NULL)) {
		outs("錯誤! 已經有同樣 ID 的使用者");
		fail++;
	    } else
		strlcpy(x.userid, genbuf, sizeof(x.userid));
	}
	break;
    case '6':
	if (x.mychicken.name[0])
	    x.mychicken.name[0] = 0;
	else
	    strlcpy(x.mychicken.name, "[死]", sizeof(x.mychicken.name));
	break;
    default:
	return;
    }

    if (fail) {
	pressanykey();
	return;
    }
    if (getans(msg_sure_ny) == 'y') {
	if (flag) {
	    post_change_perm(temp, i, cuser.userid, x.userid);
#ifdef PLAY_ANGEL
	    if (i & ~temp & PERM_ANGEL)
		mail_id(x.userid, "翅膀長出來了！", "etc/angel_notify", "[上帝]");
#endif
	}
	if (strcmp(u->userid, x.userid)) {
	    char            src[STRLEN], dst[STRLEN];

	    sethomepath(src, u->userid);
	    sethomepath(dst, x.userid);
	    Rename(src, dst);
	    setuserid(unum, x.userid);
	}
	memcpy(u, &x, sizeof(x));
	if (mail_changed) {
#ifdef EMAIL_JUSTIFY
	    x.userlevel &= ~PERM_LOGINOK;
	    mail_justify();
#endif
	}
	if (i == QUIT) {
	    char            src[STRLEN], dst[STRLEN];

	    sethomepath(src, x.userid);
	    snprintf(dst, sizeof(dst), "tmp/%s", x.userid);
	    friend_delete_all(x.userid, FRIEND_ALOHA);
	    Rename(src, dst);	/* do not remove user home */
            kill_user(unum);
	    return;
	} else
	    log_usies("SetUser", x.userid);
	if (money_change)
	    setumoney(unum, x.money);
	passwd_update(unum, &x);
	if (money_change) {
	    char title[TTLEN+1];
	    char msg[200];
	    clrtobot();
	    clear();
	    // XXX 此時斷線則修改資料沒 log
	    while (!getdata(5, 0, "請輸入理由以示負責：",
			    reason, sizeof(reason), DOECHO));

	    snprintf(msg, sizeof(msg),
		    "   站長\033[1;32m%s\033[m把\033[1;32m%s\033[m的錢"
		    "從\033[1;35m%d\033[m改成\033[1;35m%d\033[m\n"
		    "   \033[1;37m站長%s修改錢理由是：%s\033[m",
		    cuser.userid, x.userid, money, x.money,
		    cuser.userid, reason);
	    snprintf(title, sizeof(title),
		    "[公安報告] 站長%s修改%s錢報告", cuser.userid,
		    x.userid);
	    post_msg("Security", title, msg, "[系統安全局]");
	}
    }
}

int
u_info(void)
{
    move(2, 0);
    user_display(&cuser, 0);
    uinfo_query(&cuser, 0, usernum);
    strlcpy(currutmp->username, cuser.username, sizeof(currutmp->username));
    return 0;
}

int
u_cloak(void)
{
    outs((currutmp->invisible ^= 1) ? MSG_CLOAKED : MSG_UNCLOAK);
    return XEASY;
}

void
showplans(const char *uid)
{
    char            genbuf[200];

#ifdef CHESSCOUNTRY
    if (user_query_mode) {
	int    i = 0;
	FILE  *fp;
	userec_t xuser;

	sethomefile(genbuf, uid, chess_photo_name[user_query_mode - 1]);
	if ((fp = fopen(genbuf, "r")) != NULL)
	{
	    char   photo[6][256];
	    int    kingdom_bid = 0;
	    int    win = 0, lost = 0;

	    move(7, 0);
	    while (i < 12 && fgets(genbuf, 256, fp))
	    {
		chomp(genbuf);
		if (i < 6)  /* 讀照片檔 */
		    strcpy(photo[i], genbuf);
		else if (i == 6)
		    kingdom_bid = atoi(genbuf);
		else
		    prints("%s %s\n", photo[i - 7], genbuf);

		i++;
	    }

	    getuser(uid, &xuser);
	    if (user_query_mode == 1) {
		win = xuser.five_win;
		lost = xuser.five_lose;
	    } else if(user_query_mode == 2) {
		win = xuser.chc_win;
		lost = xuser.chc_lose;
	    }
	    prints("%s <總共戰績> %d 勝 %d 敗\n", photo[5], win, lost);


	    /* 棋國國徽 */
	    setapath(genbuf, bcache[kingdom_bid - 1].brdname);
	    strlcat(genbuf, "/chess_ensign", sizeof(genbuf));
	    show_file(genbuf, 13, 10, ONLY_COLOR);
	    return;
	}
    }
#endif /* defined(CHESSCOUNTRY) */

    sethomefile(genbuf, uid, fn_plans);
    if (!show_file(genbuf, 7, MAX_QUERYLINES, ONLY_COLOR))
	prints("《個人名片》%s 目前沒有名片", uid);
}

int
showsignature(char *fname, int *j)
{
    FILE           *fp;
    char            buf[256];
    int             i, num = 0;
    char            ch;

    clear();
    move(2, 0);
    setuserfile(fname, "sig.0");
    *j = strlen(fname) - 1;

    for (ch = '1'; ch <= '9'; ch++) {
	fname[*j] = ch;
	if ((fp = fopen(fname, "r"))) {
	    prints("\033[36m【 簽名檔.%c 】\033[m\n", ch);
	    for (i = 0; i < MAX_SIGLINES && fgets(buf, sizeof(buf), fp); i++)
		outs(buf);
	    num++;
	    fclose(fp);
	}
    }
    return num;
}

int
u_editsig(void)
{
    int             aborted;
    char            ans[4];
    int             j;
    char            genbuf[200];

    showsignature(genbuf, &j);

    getdata(0, 0, "簽名檔 (E)編輯 (D)刪除 (Q)取消？[Q] ",
	    ans, sizeof(ans), LCECHO);

    aborted = 0;
    if (ans[0] == 'd')
	aborted = 1;
    if (ans[0] == 'e')
	aborted = 2;

    if (aborted) {
	if (!getdata(1, 0, "請選擇簽名檔(1-9)？[1] ", ans, sizeof(ans), DOECHO))
	    ans[0] = '1';
	if (ans[0] >= '1' && ans[0] <= '9') {
	    genbuf[j] = ans[0];
	    if (aborted == 1) {
		unlink(genbuf);
		outs(msg_del_ok);
	    } else {
		setutmpmode(EDITSIG);
		aborted = vedit(genbuf, NA, NULL);
		if (aborted != -1)
		    outs("簽名檔更新完畢");
	    }
	}
	pressanykey();
    }
    return 0;
}

int
u_editplan(void)
{
    char            genbuf[200];

    getdata(b_lines - 1, 0, "名片 (D)刪除 (E)編輯 [Q]取消？[Q] ",
	    genbuf, 3, LCECHO);

    if (genbuf[0] == 'e') {
	int             aborted;

	setutmpmode(EDITPLAN);
	setuserfile(genbuf, fn_plans);
	aborted = vedit(genbuf, NA, NULL);
	if (aborted != -1)
	    outs("名片更新完畢");
	pressanykey();
	return 0;
    } else if (genbuf[0] == 'd') {
	setuserfile(genbuf, fn_plans);
	unlink(genbuf);
	outmsg("名片刪除完畢");
    }
    return 0;
}

int
u_editcalendar(void)
{
    char            genbuf[200];

    getdata(b_lines - 1, 0, "行事曆 (D)刪除 (E)編輯 [Q]取消？[Q] ",
	    genbuf, 3, LCECHO);

    if (genbuf[0] == 'e') {
	int             aborted;

	setutmpmode(EDITPLAN);
	sethomefile(genbuf, cuser.userid, "calendar");
	aborted = vedit(genbuf, NA, NULL);
	if (aborted != -1)
	    vmsg("行事曆更新完畢");
	return 0;
    } else if (genbuf[0] == 'd') {
	sethomefile(genbuf, cuser.userid, "calendar");
	unlink(genbuf);
	vmsg("行事曆刪除完畢");
    }
    return 0;
}

/* 使用者填寫註冊表格 */
static void
getfield(int line, const char *info, const char *desc, char *buf, int len)
{
    char            prompt[STRLEN];
    char            genbuf[200];

    move(line, 2);
    prints("原先設定：%-30.30s (%s)", buf, info);
    snprintf(prompt, sizeof(prompt), "%s：", desc);
    if (getdata_str(line + 1, 2, prompt, genbuf, len, DOECHO, buf))
	strcpy(buf, genbuf);
    move(line, 2);
    prints("%s：%s", desc, buf);
    clrtoeol();
}

static int
removespace(char *s)
{
    int             i, index;

    for (i = 0, index = 0; s[i]; i++) {
	if (s[i] != ' ')
	    s[index++] = s[i];
    }
    s[index] = '\0';
    return index;
}

static char    *
getregcode(char *buf)
{
    strcpy(buf, crypt(cuser.userid, "02"));
    return buf;
}

static int
isvalidemail(const char *email)
{
    FILE           *fp;
    char            buf[128], *c;
    if (!strstr(email, "@"))
	return 0;
    for (c = strstr(email, "@"); *c != 0; ++c)
	if ('A' <= *c && *c <= 'Z')
	    *c += 32;

    if ((fp = fopen("etc/banemail", "r"))) {
	while (fgets(buf, sizeof(buf), fp)) {
	    if (buf[0] == '#')
		continue;
	    chomp(buf);
	    if (buf[0] == 'A' && strcasecmp(&buf[1], email) == 0)
		return 0;
	    if (buf[0] == 'P' && strcasestr(email, &buf[1]))
		return 0;
	    if (buf[0] == 'S' && strcasecmp(strstr(email, "@") + 1, &buf[1]) == 0)
		return 0;
	}
	fclose(fp);
    }
    return 1;
}

static void
toregister(char *email, char *genbuf, char *phone, char *career,
	   char *ident, char *rname, char *addr, char *mobile)
{
    FILE           *fn;
    char            buf[128];

    sethomefile(buf, cuser.userid, "justify.wait");
    if (phone[0] != 0) {
	fn = fopen(buf, "w");
	assert(fn);
	fprintf(fn, "%s\n%s\ndummy\n%s\n%s\n%s\n",
		phone, career, rname, addr, mobile);
	fclose(fn);
    }
    clear();
    stand_title("認證設定");
    if (cuser.userlevel & PERM_NOREGCODE){
	strcpy(email, "x");
	goto REGFORM2;
    }
    move(2, 0);
    outs("您好, 本站認證認證的方式有:\n"
	 "  1.若您有 E-Mail  (本站不接受 yahoo, kimo等免費的 E-Mail)\n"
	 "    請輸入您的 E-Mail , 我們會寄發含有認證碼的信件給您\n"
	 "    收到後請到 (U)ser => (R)egister 輸入認證碼, 即可通過認證\n"
	 "\n"
	 "  2.若您沒有 E-Mail , 請輸入 x ,\n"
	 "    我們會由站長親自審核您的註冊資料\n"
	 "************************************************************\n"
	 "* 注意!                                                    *\n"
	 "* 您應該會在輸入完成後十分鐘內收到認證信, 若過久未收到,    *\n"
	 "* 或輸入後發生認證碼錯誤, 麻煩重填一次 E-Mail 或改手動認證 *\n"
	 "************************************************************\n");

#ifdef HAVEMOBILE
    outs("  3.若您有手機門號且想採取手機簡訊認證的方式 , 請輸入 m \n"
	 "    我們將會寄發含有認證碼的簡訊給您 \n"
	 "    收到後請到(U)ser => (R)egister 輸入認證碼, 即可通過認證\n");
#endif

    while (1) {
	email[0] = 0;
	getfield(15, "身分認證用", "E-Mail Address", email, 50);
	if (strcmp(email, "x") == 0 || strcmp(email, "X") == 0)
	    break;
#ifdef HAVEMOBILE
	else if (strcmp(email, "m") == 0 || strcmp(email, "M") == 0) {
	    if (isvalidmobile(mobile)) {
		char            yn[3];
		getdata(16, 0, "請再次確認您輸入的手機號碼正確嘛? [y/N]",
			yn, sizeof(yn), LCECHO);
		if (yn[0] == 'Y' || yn[0] == 'y')
		    break;
	    } else {
		move(17, 0);
		outs("指定的手機號碼不合法,"
		       "若您無手機門號請選擇其他方式認證");
	    }

	}
#endif
	else if (isvalidemail(email)) {
	    char            yn[3];
	    getdata(16, 0, "請再次確認您輸入的 E-Mail 位置正確嘛? [y/N]",
		    yn, sizeof(yn), LCECHO);
	    if (yn[0] == 'Y' || yn[0] == 'y')
		break;
	} else {
	    move(17, 0);
	    outs("指定的 E-Mail 不合法,"
		   "若您無 E-Mail 請輸入 x由站長手動認證");
	}
    }
    strncpy(cuser.email, email, sizeof(cuser.email));
 REGFORM2:
    if (strcasecmp(email, "x") == 0) {	/* 手動認證 */
	if ((fn = fopen(fn_register, "a"))) {
	    fprintf(fn, "num: %d, %s", usernum, ctime4(&now));
	    fprintf(fn, "uid: %s\n", cuser.userid);
	    fprintf(fn, "ident: \n");
	    fprintf(fn, "name: %s\n", rname);
	    fprintf(fn, "career: %s\n", career);
	    fprintf(fn, "addr: %s\n", addr);
	    fprintf(fn, "phone: %s\n", phone);
	    fprintf(fn, "mobile: %s\n", mobile);
	    fprintf(fn, "email: %s\n", email);
	    fprintf(fn, "----\n");
	    fclose(fn);
	}
    } else {
	char            tmp[IDLEN + 1];
	if (phone != NULL) {
#ifdef HAVEMOBILE
	    if (strcmp(email, "m") == 0 || strcmp(email, "M") == 0)
		sprintf(genbuf, sizeof(genbuf),
			"%s:%s:<Mobile>", phone, career);
	    else
#endif
		snprintf(genbuf, sizeof(genbuf),
			 "%s:%s:<Email>", phone, career);
	    strncpy(cuser.justify, genbuf, REGLEN);
	    sethomefile(buf, cuser.userid, "justify");
	}
	snprintf(buf, sizeof(buf),
		 "您在 " BBSNAME " 的認證碼: %s", getregcode(genbuf));
	strlcpy(tmp, cuser.userid, sizeof(tmp));
	strlcpy(cuser.userid, str_sysop, sizeof(cuser.userid));
#ifdef HAVEMOBILE
	if (strcmp(email, "m") == 0 || strcmp(email, "M") == 0)
	    mobile_message(mobile, buf);
	else
#endif
	    bsmtp("etc/registermail", buf, email, 0);
	strlcpy(cuser.userid, tmp, sizeof(cuser.userid));
	outs("\n\n\n我們即將寄出認證信 (您應該會在 10 分鐘內收到)\n"
	     "收到後您可以根據認證信標題的認證碼\n"
	     "輸入到 (U)ser -> (R)egister 後就可以完成註冊");
	pressanykey();
	return;
    }
}

static int HaveRejectStr(const char *s, const char **rej)
{
    int     i;
    char    *ptr, *rejectstr[] =
	{"幹", "阿", "不", "你媽", "某", "笨", "呆", "..", "xx",
	 "你管", "管我", "猜", "天才", "超人", 
	 "ㄅ", "ㄆ", "ㄇ", "ㄈ", "ㄉ", "ㄊ", "ㄋ", "ㄌ", "ㄍ", "ㄎ", "ㄏ",
	 "ㄐ", "ㄑ", "ㄒ", "ㄓ",/*"ㄔ",*/    "ㄕ", "ㄖ", "ㄗ", "ㄘ", "ㄙ",
	 "ㄧ", "ㄨ", "ㄩ", "ㄚ", "ㄛ", "ㄜ", "ㄝ", "ㄞ", "ㄟ", "ㄠ", "ㄡ",
	 "ㄢ", "ㄣ", "ㄤ", "ㄥ", "ㄦ", NULL};

    if( rej != NULL )
	for( i = 0 ; rej[i] != NULL ; ++i )
	    if( strstr(s, rej[i]) )
		return 1;

    for( i = 0 ; rejectstr[i] != NULL ; ++i )
	if( strstr(s, rejectstr[i]) )
	    return 1;

    if( (ptr = strstr(s, "ㄔ")) != NULL ){
	if( ptr != s && strncmp(ptr - 1, "都市", 4) == 0 )
	    return 0;
	return 1;
    }
    return 0;
}

static char *isvalidname(char *rname)
{
#ifdef FOREIGN_REG
    return NULL;
#else
    const char    *rejectstr[] =
	{"肥", "胖", "豬頭", "小白", "小明", "路人", "老王", "老李", "寶貝",
	 "先生", "帥哥", "老頭", "小姊", "小姐", "美女", "小妹", "大頭", 
	 "公主", "同學", "寶寶", "公子", "大頭", "小小", "小弟", "小妹",
	 "妹妹", "嘿", "嗯", "爺爺", "大哥", "無",
	 NULL};
    if( removespace(rname) && rname[0] < 0 &&
	strlen(rname) >= 4 &&
	!HaveRejectStr(rname, rejectstr) &&
	strncmp(rname, "小", 2) != 0   && //起頭是「小」
	strncmp(rname, "我是", 4) != 0 && //起頭是「我是」
	!(strlen(rname) == 4 && strncmp(&rname[2], "兒", 2) == 0) &&
	!(strlen(rname) >= 4 && strncmp(&rname[0], &rname[2], 2) == 0))
	return NULL;
    return "您的輸入不正確";
#endif

}

static char *isvalidcareer(char *career)
{
#ifndef FOREIGN_REG
    const char    *rejectstr[] = {NULL};
    if (!(removespace(career) && career[0] < 0 && strlen(career) >= 6) ||
	strcmp(career, "家裡") == 0 || HaveRejectStr(career, rejectstr) )
	return "您的輸入不正確";
    if (strcmp(&career[strlen(career) - 2], "大") == 0 ||
	strcmp(&career[strlen(career) - 4], "大學") == 0 ||
	strcmp(career, "學生大學") == 0)
	return "麻煩請加學校系所";
    if (strcmp(career, "學生高中") == 0)
	return "麻煩輸入學校名稱";
#else
    if( strlen(career) < 6 )
	return "您的輸入不正確";
#endif
    return NULL;
}

static char *isvalidaddr(char *addr)
{
    const char    *rejectstr[] =
	{"地球", "銀河", "火星", NULL};

    if (!removespace(addr) || addr[0] > 0 || strlen(addr) < 15) 
	return "這個地址並不合法";
    if (strstr(addr, "信箱") != NULL || strstr(addr, "郵政") != NULL) 
	return "抱歉我們不接受郵政信箱";
    if ((strstr(addr, "市") == NULL && strstr(addr, "巿") == NULL &&
	 strstr(addr, "縣") == NULL && strstr(addr, "室") == NULL) ||
	HaveRejectStr(addr, rejectstr)             ||
	strcmp(&addr[strlen(addr) - 2], "段") == 0 ||
	strcmp(&addr[strlen(addr) - 2], "路") == 0 ||
	strcmp(&addr[strlen(addr) - 2], "巷") == 0 ||
	strcmp(&addr[strlen(addr) - 2], "弄") == 0 ||
	strcmp(&addr[strlen(addr) - 2], "區") == 0 ||
	strcmp(&addr[strlen(addr) - 2], "市") == 0 ||
	strcmp(&addr[strlen(addr) - 2], "街") == 0    )
	return "這個地址並不合法";
    return NULL;
}

static char *isvalidphone(char *phone)
{
    int     i;
    for( i = 0 ; phone[i] != 0 ; ++i )
	if( !isdigit((int)phone[i]) )
	    return "請不要加分隔符號";
    if (!removespace(phone) || 
	strlen(phone) < 9 || 
	strstr(phone, "00000000") != NULL ||
	strstr(phone, "22222222") != NULL    ) {
	return "這個電話號碼並不合法(請含區碼)" ;
    }
    return NULL;
}

int
u_register(void)
{
    char            rname[20], addr[50], ident[11], mobile[16];
#ifdef FOREIGN_REG
    char            fore[2];
#endif
    char            phone[20], career[40], email[50], birthday[9], sex_is[2],
                    year, mon, day;
    char            inregcode[14], regcode[50];
    char            ans[3], *ptr, *errcode;
    char            genbuf[200];
    FILE           *fn;

    if (cuser.userlevel & PERM_LOGINOK) {
	outs("您的身份確認已經完成，不需填寫申請表");
	return XEASY;
    }
    if ((fn = fopen(fn_register, "r"))) {
	while (fgets(genbuf, STRLEN, fn)) {
	    if ((ptr = strchr(genbuf, '\n')))
		*ptr = '\0';
	    if (strncmp(genbuf, "uid: ", 5) == 0 &&
		strcmp(genbuf + 5, cuser.userid) == 0) {
		fclose(fn);
		outs("您的註冊申請單尚在處理中，請耐心等候");
		return XEASY;
	    }
	}
	fclose(fn);
    }
    strlcpy(rname, cuser.realname, sizeof(rname));
    strlcpy(addr, cuser.address, sizeof(addr));
    strlcpy(email, cuser.email, sizeof(email));
    snprintf(mobile, sizeof(mobile), "0%09d", cuser.mobile);
    if (cuser.month == 0 && cuser.day && cuser.year == 0)
	birthday[0] = 0;
    else
	snprintf(birthday, sizeof(birthday), "%02i/%02i/%02i",
		 cuser.month, cuser.day, cuser.year % 100);
    sex_is[0] = (cuser.sex % 8) + '1';
    sex_is[1] = 0;
    career[0] = phone[0] = '\0';
    sethomefile(genbuf, cuser.userid, "justify.wait");
    if ((fn = fopen(genbuf, "r"))) {
	fgets(genbuf, sizeof(genbuf), fn);
	chomp(genbuf);
	strlcpy(phone, genbuf, sizeof(phone));

	fgets(genbuf, sizeof(genbuf), fn);
	chomp(genbuf);
	strlcpy(career, genbuf, sizeof(career));

	fgets(genbuf, sizeof(genbuf), fn); // old version compatible

	fgets(genbuf, sizeof(genbuf), fn);
	chomp(genbuf);
	strlcpy(rname, genbuf, sizeof(rname));

	fgets(genbuf, sizeof(genbuf), fn);
	chomp(genbuf);
	strlcpy(addr, genbuf, sizeof(addr));

	fgets(genbuf, sizeof(genbuf), fn);
	chomp(genbuf);
	strlcpy(mobile, genbuf, sizeof(mobile));

	fclose(fn);
    }

    if (cuser.userlevel & PERM_NOREGCODE) {
	vmsg("您不被允許\使用認證碼認證。請填寫註冊申請單");
	goto REGFORM;
    }

    if (cuser.year != 0 &&	/* 已經第一次填過了~ ^^" */
	strcmp(cuser.email, "x") != 0 &&	/* 上次手動認證失敗 */
	strcmp(cuser.email, "X") != 0) {
	clear();
	stand_title("EMail認證");
	move(2, 0);
	prints("%s(%s) 您好，請輸入您的認證碼。\n"
	       "或您可以輸入 x來重新填寫 E-Mail 或改由站長手動認證\n",
	       cuser.userid, cuser.username);
	inregcode[0] = 0;
	do{
	    getdata(10, 0, "您的輸入: ", inregcode, sizeof(inregcode), DOECHO);
	    if( strcmp(inregcode, "x") == 0 ||
		strcmp(inregcode, "X") == 0 ||
		strlen(inregcode) == 13 )
		break;
	    if( strlen(inregcode) != 13 )
		vmsg("認證碼輸入不完全，應該一共有十三碼。");
	} while( 1 );

	if (strcmp(inregcode, getregcode(regcode)) == 0) {
	    int             unum;
	    if ((unum = searchuser(cuser.userid, NULL)) == 0) {
		vmsg("系統錯誤，查無此人！");
		u_exit("getuser error");
		exit(0);
	    }
	    mail_muser(cuser, "[註冊成功\囉]", "etc/registeredmail");
#if FOREIGN_REG_DAY > 0
	    if(cuser.uflag2 & FOREIGN)
		mail_muser(cuser, "[出入境管理局]", "etc/foreign_welcome");
#endif
	    cuser.userlevel |= (PERM_LOGINOK | PERM_POST);
	    outs("\n註冊成功\, 重新上站後將取得完整權限\n"
		   "請按下任一鍵跳離後重新上站~ :)");
	    sethomefile(genbuf, cuser.userid, "justify.wait");
	    unlink(genbuf);
	    snprintf(cuser.justify, sizeof(cuser.justify),
		     "%s:%s:auto", phone, career);
	    sethomefile(genbuf, cuser.userid, "justify");
	    log_file(genbuf, LOG_CREAT, cuser.justify);
	    pressanykey();
	    u_exit("registed");
	    exit(0);
	    return QUIT;
	} else if (strcmp(inregcode, "x") != 0 &&
		   strcmp(inregcode, "X") != 0) {
	    vmsg("認證碼錯誤！");
	} else {
	    toregister(email, genbuf, phone, career,
		       ident, rname, addr, mobile);
	    return FULLUPDATE;
	}
    }

    REGFORM:
    getdata(b_lines - 1, 0, "您確定要填寫註冊單嗎(Y/N)？[N] ",
	    ans, 3, LCECHO);
    if (ans[0] != 'y')
	return FULLUPDATE;

    move(2, 0);
    clrtobot();
    while (1) {
	clear();
	move(1, 0);
	prints("%s(%s) 您好，請據實填寫以下的資料:",
	       cuser.userid, cuser.username);
#ifdef FOREIGN_REG
	fore[0] = 'y';
	fore[1] = 0;
	getfield(2, "Y/n", "是否現在住在台灣", fore, 2);
    	if (fore[0] == 'n')
	    fore[0] |= FOREIGN;
	else
	    fore[0] = 0;
#endif
	while (1) {
	    getfield(8, 
#ifdef FOREIGN_REG
                     "請用本名",
#else
                     "請用中文",
#endif
                     "真實姓名", rname, 20);
	    if( (errcode = isvalidname(rname)) == NULL )
		break;
	    else
		vmsg(errcode);
	}

	move(11, 0);
	outs("  請盡量詳細的填寫您的服務單位，大專院校請麻煩"
	     "加\033[1;33m系所\033[m，公司單位請加\033[1;33m職稱\033[m，\n"
	     "  暫無工作請麻煩填寫\033[1;33m畢業學校\033[m。\n");
	while (1) {
	    getfield(9, "(畢業)學校(含\033[1;33m系所年級\033[m)或單位職稱",
		     "服務單位", career, 40);
	    if( (errcode = isvalidcareer(career)) == NULL )
		break;
	    else
		vmsg(errcode);
	}
	while (1) {
	    getfield(11, "含\033[1;33m縣市\033[m及門寢號碼"
		     "(台北請加\033[1;33m行政區\033[m)",
		     "目前住址", addr, sizeof(addr));
	    if( (errcode = isvalidaddr(addr)) == NULL
#ifdef FOREIGN_REG
                || fore[0] 
#endif
		)
		break;
	    else
		vmsg(errcode);
	}
	while (1) {
	    getfield(13, "不加-(), 包括長途區號", "連絡電話", phone, 11);
	    if( (errcode = isvalidphone(phone)) == NULL )
		break;
	    else
		vmsg(errcode);
	}
	getfield(15, "只輸入數字 如:0912345678 (可不填)",
		 "手機號碼", mobile, 20);
	while (1) {
	    int             len;

	    getfield(17, "月月/日日/西元 如:09/27/76", "生日", birthday, 9);
	    len = strlen(birthday);
	    if (!len) {
		snprintf(birthday, 9, "%02i/%02i/%02i",
			 cuser.month, cuser.day, cuser.year % 100);
		mon = cuser.month;
		day = cuser.day;
		year = cuser.year;
	    } else if (len == 8) {
		mon = (birthday[0] - '0') * 10 + (birthday[1] - '0');
		day = (birthday[3] - '0') * 10 + (birthday[4] - '0');
		year = (birthday[6] - '0') * 10 + (birthday[7] - '0');
	    } else{
		vmsg("您的輸入不正確");
		continue;
	    }
	    if (mon > 12 || mon < 1 || day > 31 || day < 1 || 
		year < 40){
		vmsg("您的輸入不正確");
		continue;
	    }
	    break;
	}
	getfield(19, "1.葛格 2.姐接 ", "性別", sex_is, 2);
	getdata(20, 0, "以上資料是否正確(Y/N)？(Q)取消註冊 [N] ",
		ans, 3, LCECHO);
	if (ans[0] == 'q')
	    return 0;
	if (ans[0] == 'y')
	    break;
    }
    strlcpy(cuser.realname, rname, sizeof(cuser.realname));
    strlcpy(cuser.address, addr, sizeof(cuser.address));
    strlcpy(cuser.email, email, sizeof(cuser.email));
    cuser.mobile = atoi(mobile);
    cuser.sex = (sex_is[0] - '1') % 8;
    cuser.month = mon;
    cuser.day = day;
    cuser.year = year;
#ifdef FOREIGN_REG
    if (fore[0])
	cuser.uflag2 |= FOREIGN;
    else
	cuser.uflag2 &= ~FOREIGN;
#endif
    trim(career);
    trim(addr);
    trim(phone);

    toregister(email, genbuf, phone, career, ident, rname, addr, mobile);

    clear();
    move(9, 3);
    outs("最後Post一篇\033[32m自我介紹文章\033[m給大家吧，"
	   "告訴所有老骨頭\033[31m我來啦^$。\\n\n\n\n");
    pressanykey();
    cuser.userlevel |= PERM_POST;
    brc_initial_board("WhoAmI");
    set_board();
    do_post();
    cuser.userlevel &= ~PERM_POST;
    return 0;
}

/* 列出所有註冊使用者 */
static int      usercounter, totalusers;
static unsigned short u_list_special;

static int
u_list_CB(int num, userec_t * uentp)
{
    static int      i;
    char            permstr[8], *ptr;
    register int    level;

    if (uentp == NULL) {
	move(2, 0);
	clrtoeol();
	prints("\033[7m  使用者代號   %-25s   上站  文章  %s  "
	       "最近光臨日期     \033[0m\n",
	       "綽號暱稱",
	       HAS_PERM(PERM_SEEULEVELS) ? "等級" : "");
	i = 3;
	return 0;
    }
    if (bad_user_id(uentp->userid))
	return 0;

    if ((uentp->userlevel & ~(u_list_special)) == 0)
	return 0;

    if (i == b_lines) {
	prints("\033[34;46m  已顯示 %d/%d 人(%d%%)  \033[31;47m  "
	       "(Space)\033[30m 看下一頁  \033[31m(Q)\033[30m 離開  \033[m",
	       usercounter, totalusers, usercounter * 100 / totalusers);
	i = igetch();
	if (i == 'q' || i == 'Q')
	    return QUIT;
	i = 3;
    }
    if (i == 3) {
	move(3, 0);
	clrtobot();
    }
    level = uentp->userlevel;
    strlcpy(permstr, "----", 8);
    if (level & PERM_SYSOP)
	permstr[0] = 'S';
    else if (level & PERM_ACCOUNTS)
	permstr[0] = 'A';
    else if (level & PERM_SYSOPHIDE)
	permstr[0] = 'p';

    if (level & (PERM_BOARD))
	permstr[1] = 'B';
    else if (level & (PERM_BM))
	permstr[1] = 'b';

    if (level & (PERM_XEMPT))
	permstr[2] = 'X';
    else if (level & (PERM_LOGINOK))
	permstr[2] = 'R';

    if (level & (PERM_CLOAK | PERM_SEECLOAK))
	permstr[3] = 'C';

    ptr = (char *)Cdate(&uentp->lastlogin);
    ptr[18] = '\0';
    prints("%-14s %-27.27s%5d %5d  %s  %s\n",
	   uentp->userid,
	   uentp->username,
	   uentp->numlogins, uentp->numposts,
	   HAS_PERM(PERM_SEEULEVELS) ? permstr : "", ptr);
    usercounter++;
    i++;
    return 0;
}

int
u_list(void)
{
    char            genbuf[3];

    setutmpmode(LAUSERS);
    u_list_special = usercounter = 0;
    totalusers = SHM->number;
    if (HAS_PERM(PERM_SEEULEVELS)) {
	getdata(b_lines - 1, 0, "觀看 [1]特殊等級 (2)全部？",
		genbuf, 3, DOECHO);
	if (genbuf[0] != '2')
	    u_list_special = PERM_BASIC | PERM_CHAT | PERM_PAGE | PERM_POST | PERM_LOGINOK | PERM_BM;
    }
    u_list_CB(0, NULL);
    if (passwd_apply(u_list_CB) == -1) {
	outs(msg_nobody);
	return XEASY;
    }
    move(b_lines, 0);
    clrtoeol();
    prints("\033[34;46m  已顯示 %d/%d 的使用者(系統容量無上限)  "
	   "\033[31;47m  (請按任意鍵繼續)  \033[m", usercounter, totalusers);
    igetch();
    return 0;
}

