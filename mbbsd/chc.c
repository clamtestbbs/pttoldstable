#include "bbs.h"

#define CENTER(a, b)	(((a) + (b)) >> 1)
#define CHC_TIMEOUT	300
#define CHC_LOG		"chc_log"	/* log file name */

extern userinfo_t *uip;

typedef int     (*play_func_t) (int, chcusr_t *, chcusr_t *, board_t, board_t);

typedef struct drc_t {
    rc_t            from, to;
}               drc_t;

static rc_t	    chc_from, chc_to, chc_select, chc_cursor;
static int	    chc_lefttime;
static int	    chc_my, chc_turn, chc_selected, chc_firststep;
static char	    chc_mode;
static char	    chc_warnmsg[64];
static char	    chc_ipass = 0, chc_hepass = 0;
/* fp is for logging the step */
static FILE        *fp = NULL;
static board_t	   *chc_bp;
static chc_act_list *act_list = NULL;


/* some constant variable definition */

static const char *turn_str[2] = {"�ª�", "����"};

static const char *num_str[10] = {
    "", "�@", "�G", "�T", "�|", "��", "��", "�C", "�K", "�E"
};

static const char *chess_str[2][8] = {
    /* 0     1     2     3     4     5     6     7 */
    {"  ", "�N", "�h", "�H", "��", "��", "�]", "��"},
    {"  ", "��", "�K", "��", "��", "�X", "��", "�L"}
};

static const char *chess_brd[BRD_ROW * 2 - 1] = {
    /* 0   1   2   3   4   5   6   7   8 */
    "�z�w�s�w�s�w�s�w�s�w�s�w�s�w�s�w�{",	/* 0 */
    "�x  �x  �x  �x�@�x���x  �x  �x  �x",
    "�u�w�q�w�q�w�q�w�q�w�q�w�q�w�q�w�t",	/* 1 */
    "�x  �x  �x  �x���x�@�x  �x  �x  �x",
    "�u�w�q�w�q�w�q�w�q�w�q�w�q�w�q�w�t",	/* 2 */
    "�x  �x  �x  �x  �x  �x  �x  �x  �x",
    "�u�w�q�w�q�w�q�w�q�w�q�w�q�w�q�w�t",	/* 3 */
    "�x  �x  �x  �x  �x  �x  �x  �x  �x",
    "�u�w�r�w�r�w�r�w�r�w�r�w�r�w�r�w�t",	/* 4 */
    "�x  ��    �e          �~    ��  �x",
    "�u�w�s�w�s�w�s�w�s�w�s�w�s�w�s�w�t",	/* 5 */
    "�x  �x  �x  �x  �x  �x  �x  �x  �x",
    "�u�w�q�w�q�w�q�w�q�w�q�w�q�w�q�w�t",	/* 6 */
    "�x  �x  �x  �x  �x  �x  �x  �x  �x",
    "�u�w�q�w�q�w�q�w�q�w�q�w�q�w�q�w�t",	/* 7 */
    "�x  �x  �x  �x�@�x���x  �x  �x  �x",
    "�u�w�q�w�q�w�q�w�q�w�q�w�q�w�q�w�t",	/* 8 */
    "�x  �x  �x  �x���x�@�x  �x  �x  �x",
    "�|�w�r�w�r�w�r�w�r�w�r�w�r�w�r�w�}"	/* 9 */
};

static char *hint_str[] = {
    "  q      �{�����}",
    "  p      �n�D�M��",
    "��V��   ���ʹC��",
    "Enter    ���/����"
};

/*
 * Start of the network communication function.
 */
static int
chc_recvmove(int s)
{
    drc_t           buf;

    if (read(s, &buf, sizeof(buf)) != sizeof(buf))
	return 0;
    chc_from = buf.from, chc_to = buf.to;
    return 1;
}

static int
chc_sendmove(int s)
{
    drc_t           buf;

    buf.from = chc_from, buf.to = chc_to;
    if (write(s, &buf, sizeof(buf)) != sizeof(buf))
	return 0;
    return 1;
}

static void
chc_broadcast(chc_act_list *p, board_t board){
    while(p){
	if (chc_sendmove(p->sock) < 0) {
	    if (p->next->next == NULL)
		p = NULL;
	    else {
		chc_act_list *tmp = p->next->next;
		p->next = tmp;
	    }
	    free(p->next);
	}
	p = p->next;
    }
}

static int
chc_broadcast_recv(chc_act_list *act_list, board_t board){
    if (!chc_recvmove(act_list->sock))
	return 0;
    chc_broadcast(act_list->next, board);
    return 1;
}

static void
chc_broadcast_send(chc_act_list *act_list, board_t board){
    chc_broadcast(act_list, board);
}

/*
 * End of the network communication function.
 */

/*
 * Start of the drawing function.
 */
static void
chc_movecur(int r, int c)
{
    move(r * 2 + 3, c * 4 + 4);
}

static char *
getstep(board_t board, rc_t *from, rc_t *to)
{
    int             turn, fc, tc;
    char           *dir;
    static char	    buf[80];

    turn = CHE_O(board[from->r][from->c]);
    fc = (turn == (chc_my ^ 1) ? from->c + 1 : 9 - from->c);
    tc = (turn == (chc_my ^ 1) ? to->c + 1 : 9 - to->c);
    if (from->r == to->r)
	dir = "��";
    else {
	if (from->c == to->c)
	    tc = from->r - to->r;
	if (tc < 0)
	    tc = -tc;

	if ((turn == (chc_my ^ 1) && to->r > from->r) ||
	    (turn == chc_my && to->r < from->r))
	    dir = "�i";
	else
	    dir = "�h";
    }
    sprintf(buf, "%s%s%s%s",
	   chess_str[turn][CHE_P(board[from->r][from->c])],
	   num_str[fc], dir, num_str[tc]);
    return buf;
}

static void
showstep(board_t board)
{
    int		    eatten;

    prints("%s%s", CHE_O(board[chc_from.r][chc_from.c]) == 0 ? BLACK_COLOR : RED_COLOR, getstep(board, &chc_from, &chc_to));

    eatten = board[chc_to.r][chc_to.c];
    if (eatten)
	prints("�G %s%s",
	       CHE_O(eatten) == 0 ? BLACK_COLOR : RED_COLOR,
	       chess_str[CHE_O(eatten)][CHE_P(eatten)]);
    prints("\033[m");
}

static void
chc_drawline(board_t board, chcusr_t *user1, chcusr_t *user2, int line)
{
    int             i, j;

    move(line, 0);
    clrtoeol();
    if (line == 0) {
	prints("\033[1;46m   �H�ѹ��   \033[45m%30s VS %-20s%10s\033[m",
	       user1->userid, user2->userid, chc_mode & CHC_WATCH ? "[�[�ѼҦ�]" : "");
    } else if (line >= 3 && line <= 21) {
	outs("   ");
	for (i = 0; i < 9; i++) {
	    j = board[RTL(line)][i];
	    if ((line & 1) == 1 && j) {
		if (chc_selected &&
		    chc_select.r == RTL(line) && chc_select.c == i)
		    prints("%s%s\033[m",
			   CHE_O(j) == 0 ? BLACK_REVERSE : RED_REVERSE,
			   chess_str[CHE_O(j)][CHE_P(j)]);
		else
		    prints("%s%s\033[m",
			   CHE_O(j) == 0 ? BLACK_COLOR : RED_COLOR,
			   chess_str[CHE_O(j)][CHE_P(j)]);
	    } else
		prints("%c%c", chess_brd[line - 3][i * 4],
		       chess_brd[line - 3][i * 4 + 1]);
	    if (i != 8)
		prints("%c%c", chess_brd[line - 3][i * 4 + 2],
		       chess_brd[line - 3][i * 4 + 3]);
	}
	outs("        ");

	if (line >= 3 && line < 3 + (int)dim(hint_str)) {
	    outs(hint_str[line - 3]);
	} else if (line == SIDE_ROW) {
	    prints("\033[1m�A�O%s%s\033[m",
		   chc_my == 0 ? BLACK_COLOR : RED_COLOR,
		   turn_str[chc_my]);
	} else if (line == TURN_ROW) {
	    prints("%s%s\033[m",
		   TURN_COLOR,
		   chc_my == chc_turn ? "����A�U�ѤF" : "���ݹ��U��");
	} else if (line == STEP_ROW && !chc_firststep) {
	    showstep(board);
	} else if (line == TIME_ROW) {
	    prints("�Ѿl�ɶ� %d:%02d", chc_lefttime / 60, chc_lefttime % 60);
	} else if (line == WARN_ROW) {
	    outs(chc_warnmsg);
	} else if (line == MYWIN_ROW) {
	    prints("\033[1;33m%12.12s    "
		   "\033[1;31m%2d\033[37m�� "
		   "\033[34m%2d\033[37m�� "
		   "\033[36m%2d\033[37m�M\033[m",
		   user1->userid,
		   user1->win, user1->lose - 1, user1->tie);
	} else if (line == HISWIN_ROW) {
	    prints("\033[1;33m%12.12s    "
		   "\033[1;31m%2d\033[37m�� "
		   "\033[34m%2d\033[37m�� "
		   "\033[36m%2d\033[37m�M\033[m",
		   user2->userid,
		   user2->win, user2->lose - 1, user2->tie);
	}
    } else if (line == 2 || line == 22) {
	outs("   ");
	if (line == 2)
	    for (i = 1; i <= 9; i++)
		prints("%s  ", num_str[i]);
	else
	    for (i = 9; i >= 1; i--)
		prints("%s  ", num_str[i]);
    }
}

static void
chc_redraw(chcusr_t *user1, chcusr_t *user2, board_t board)
{
    int             i;
    for (i = 0; i <= 22; i++)
	chc_drawline(board, user1, user2, i);
}
/*
 * End of the drawing function.
 */


/*
 * Start of the log function.
 */
int
chc_log_open(chcusr_t *user1, chcusr_t *user2, char *file)
{
    char buf[128];
    if ((fp = fopen(file, "w")) == NULL)
	return -1;
    sprintf(buf, "%s V.S. %s\n", user1->userid, user2->userid);
    fputs(buf, fp);
    return 0;
}

void
chc_log_close(void)
{
    if (fp)
	fclose(fp);
}

int
chc_log(char *desc)
{
    if (fp)
	return fputs(desc, fp);
    return -1;
}

int
chc_log_step(board_t board, rc_t *from, rc_t *to)
{
    char buf[80];
    sprintf(buf, "  %s%s\033[m\n", CHE_O(board[from->r][from->c]) == 0 ? BLACK_COLOR : RED_COLOR, getstep(board, from, to));
    return chc_log(buf);
}

static int
chc_filter(struct dirent *dir)
{
    if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0 )
	return 0;
    return strstr(dir->d_name, ".poem") != NULL;
}

int
chc_log_poem(void)
{
    struct dirent **namelist;
    int n;

    // TODO use readdir(), don't use lots of memory
    n = scandir(BBSHOME"/etc/chess", &namelist, chc_filter, alphasort);
    if (n < 0)
	perror("scandir");
    else {
	char buf[80];
	FILE *fp; // XXX shadow global fp
	sprintf(buf, BBSHOME"/etc/chess/%s", namelist[rand() % n]->d_name);
	if ((fp = fopen(buf, "r")) == NULL)
	    return -1;

	while(fgets(buf, sizeof(buf), fp) != NULL)
	    chc_log(buf);
	while(n--)
	    free(namelist[n]);
	free(namelist);
	fclose(fp);
    }
    return 0;
}
/*
 * End of the log function.
 */


/*
 * Start of the rule function.
 */

static void
chc_init_board(board_t board)
{
    memset(board, 0, sizeof(board_t));
    board[0][4] = CHE(1, chc_my ^ 1);	/* �N */
    board[0][3] = board[0][5] = CHE(2, chc_my ^ 1);	/* �h */
    board[0][2] = board[0][6] = CHE(3, chc_my ^ 1);	/* �H */
    board[0][0] = board[0][8] = CHE(4, chc_my ^ 1);	/* �� */
    board[0][1] = board[0][7] = CHE(5, chc_my ^ 1);	/* �� */
    board[2][1] = board[2][7] = CHE(6, chc_my ^ 1);	/* �] */
    board[3][0] = board[3][2] = board[3][4] =
	board[3][6] = board[3][8] = CHE(7, chc_my ^ 1);	/* �� */

    board[9][4] = CHE(1, chc_my);	/* �� */
    board[9][3] = board[9][5] = CHE(2, chc_my);	/* �K */
    board[9][2] = board[9][6] = CHE(3, chc_my);	/* �� */
    board[9][0] = board[9][8] = CHE(4, chc_my);	/* �� */
    board[9][1] = board[9][7] = CHE(5, chc_my);	/* �X */
    board[7][1] = board[7][7] = CHE(6, chc_my);	/* �� */
    board[6][0] = board[6][2] = board[6][4] =
	board[6][6] = board[6][8] = CHE(7, chc_my);	/* �L */
}

static void
chc_movechess(board_t board)
{
    board[chc_to.r][chc_to.c] = board[chc_from.r][chc_from.c];
    board[chc_from.r][chc_from.c] = 0;
}

static int
dist(rc_t from, rc_t to, int rowcol)
{
    int             d;

    d = rowcol ? from.c - to.c : from.r - to.r;
    return d > 0 ? d : -d;
}

static int
between(board_t board, rc_t from, rc_t to, int rowcol)
{
    int             i, rtv = 0;

    if (rowcol) {
	if (from.c > to.c)
	    i = from.c, from.c = to.c, to.c = i;
	for (i = from.c + 1; i < to.c; i++)
	    if (board[to.r][i])
		rtv++;
    } else {
	if (from.r > to.r)
	    i = from.r, from.r = to.r, to.r = i;
	for (i = from.r + 1; i < to.r; i++)
	    if (board[i][to.c])
		rtv++;
    }
    return rtv;
}

static int
chc_canmove(board_t board, rc_t from, rc_t to)
{
    int             i;
    int             rd, cd, turn;

    rd = dist(from, to, 0);
    cd = dist(from, to, 1);
    turn = CHE_O(board[from.r][from.c]);

    /* general check */
    if (board[to.r][to.c] && CHE_O(board[to.r][to.c]) == turn)
	return 0;

    /* individual check */
    switch (CHE_P(board[from.r][from.c])) {
    case 1:			/* �N �� */
	if (!(rd == 1 && cd == 0) &&
	    !(rd == 0 && cd == 1))
	    return 0;
	if ((turn == (chc_my ^ 1) && to.r > 2) ||
	    (turn == chc_my && to.r < 7) ||
	    to.c < 3 || to.c > 5)
	    return 0;
	break;
    case 2:			/* �h �K */
	if (!(rd == 1 && cd == 1))
	    return 0;
	if ((turn == (chc_my ^ 1) && to.r > 2) ||
	    (turn == chc_my && to.r < 7) ||
	    to.c < 3 || to.c > 5)
	    return 0;
	break;
    case 3:			/* �H �� */
	if (!(rd == 2 && cd == 2))
	    return 0;
	if ((turn == (chc_my ^ 1) && to.r > 4) ||
	    (turn == chc_my && to.r < 5))
	    return 0;
	/* ��H�L */
	if (board[CENTER(from.r, to.r)][CENTER(from.c, to.c)])
	    return 0;
	break;
    case 4:			/* �� */
	if (!(rd > 0 && cd == 0) &&
	    !(rd == 0 && cd > 0))
	    return 0;
	if (between(board, from, to, rd == 0))
	    return 0;
	break;
    case 5:			/* �� �X */
	if (!(rd == 2 && cd == 1) &&
	    !(rd == 1 && cd == 2))
	    return 0;
	/* �䰨�} */
	if (rd == 2) {
	    if (board[CENTER(from.r, to.r)][from.c])
		return 0;
	} else {
	    if (board[from.r][CENTER(from.c, to.c)])
		return 0;
	}
	break;
    case 6:			/* �] �� */
	if (!(rd > 0 && cd == 0) &&
	    !(rd == 0 && cd > 0))
	    return 0;
	i = between(board, from, to, rd == 0);
	if ((i > 1) ||
	    (i == 1 && !board[to.r][to.c]) ||
	    (i == 0 && board[to.r][to.c]))
	    return 0;
	break;
    case 7:			/* �� �L */
	if (!(rd == 1 && cd == 0) &&
	    !(rd == 0 && cd == 1))
	    return 0;
	if (((turn == (chc_my ^ 1) && to.r < 5) ||
	     (turn == chc_my && to.r > 4)) &&
	    cd != 0)
	    return 0;
	if ((turn == (chc_my ^ 1) && to.r < from.r) ||
	    (turn == chc_my && to.r > from.r))
	    return 0;
	break;
    }
    return 1;
}

static void
findking(board_t board, int turn, rc_t * buf)
{
    int             i, r, c;

    r = (turn == (chc_my ^ 1)) ? 0 : 7;
    for (i = 0; i < 3; r++, i++)
	for (c = 3; c < 6; c++)
	    if (CHE_P(board[r][c]) == 1 &&
		CHE_O(board[r][c]) == turn) {
		buf->r = r, buf->c = c;
		return;
	    }
}

static int
chc_iskfk(board_t board)
{
    rc_t            from, to;

    findking(board, 0, &to);
    findking(board, 1, &from);
    if (from.c == to.c && between(board, from, to, 0) == 0)
	return 1;
    return 0;
}

static int
chc_ischeck(board_t board, int turn)
{
    rc_t            from, to;

    findking(board, turn, &to);
    for (from.r = 0; from.r < BRD_ROW; from.r++)
	for (from.c = 0; from.c < BRD_COL; from.c++)
	    if (board[from.r][from.c] &&
		CHE_O(board[from.r][from.c]) != turn)
		if (chc_canmove(board, from, to))
		    return 1;
    return 0;
}

/*
 * End of the rule function.
 */

static void
chcusr_put(userec_t *userec, chcusr_t *user)
{
    userec->chc_win = user->win;
    userec->chc_lose = user->lose;
    userec->chc_tie = user->tie;
}

static void
chcusr_get(userec_t *userec, chcusr_t *user)
{
    strlcpy(user->userid, userec->userid, sizeof(user->userid));
    user->win = userec->chc_win;
    user->lose = userec->chc_lose;
    user->tie = userec->chc_tie;
}

static int
hisplay(int s, chcusr_t *user1, chcusr_t *user2, board_t board, board_t tmpbrd)
{
    int             start_time;
    int             endgame = 0, endturn = 0;

    start_time = now;
    while (!endturn) {
	chc_lefttime = CHC_TIMEOUT - (now - start_time);
	if (chc_lefttime < 0) {
	    chc_lefttime = 0;

	    /* to make him break out igetkey() */
	    chc_from.r = -2;
	    chc_broadcast_send(act_list, board);
	}
	chc_drawline(board, user1, user2, TIME_ROW);
	move(1, 0);
	oflush();
	switch (igetkey()) {
	case 'q':
	    endgame = 2;
	    endturn = 1;
	    break;
	case 'p':
	    if (chc_hepass) {
		chc_from.r = -1;
		chc_broadcast_send(act_list, board);
		endgame = 3;
		endturn = 1;
	    }
	    break;
	case I_OTHERDATA:
	    if (!chc_broadcast_recv(act_list, board)) {	/* disconnect */
		endturn = 1;
		endgame = 1;
	    } else {
		if (chc_from.r == -1) {
		    chc_hepass = 1;
		    strlcpy(chc_warnmsg, "\033[1;33m�n�D�M��!\033[m", sizeof(chc_warnmsg));
		    chc_drawline(board, user1, user2, WARN_ROW);
		} else {
		    /* �y���ܴ�
		     *   (CHC_WATCH_PERSONAL �]�w��
		     *    ���[�Ѫ̬ݪ��ѧ�����H���Ъ��ѧ�)
		     *   �ѽL�ݭ˸m���M�p�~�n�ഫ
		     */
		    /* 1.�p�G�b�[�� �B�ѧ��O�O�H�b���� �B����A ��*/
		    if ( ((chc_mode & CHC_WATCH) && (chc_mode & CHC_WATCH_PERSONAL)) ||
			    /* 2.�ۤv�b���� */
			    (chc_mode & CHC_PERSONAL) ||
			    ((chc_mode & CHC_WATCH) && !chc_turn)
			  )
			; // do nothing
		    else {
			chc_from.r = 9 - chc_from.r, chc_from.c = 8 - chc_from.c;
			chc_to.r = 9 - chc_to.r, chc_to.c = 8 - chc_to.c;
		    }
		    chc_cursor = chc_to;
		    if (CHE_P(board[chc_to.r][chc_to.c]) == 1)
			endgame = 2;
		    endturn = 1;
		    chc_hepass = 0;
		    chc_drawline(board, user1, user2, STEP_ROW);
		    chc_log_step(board, &chc_from, &chc_to);
		    chc_movechess(board);
		    chc_drawline(board, user1, user2, LTR(chc_from.r));
		    chc_drawline(board, user1, user2, LTR(chc_to.r));
		}
	    }
	    break;
	}
    }
    return endgame;
}

static int
myplay(int s, chcusr_t *user1, chcusr_t *user2, board_t board, board_t tmpbrd)
{
    int             ch, start_time;
    int             endgame = 0, endturn = 0;

    chc_ipass = 0, chc_selected = 0;
    start_time = now;
    chc_lefttime = CHC_TIMEOUT - (now - start_time);
    bell();
    while (!endturn) {
	chc_drawline(board, user1, user2, TIME_ROW);
	chc_movecur(chc_cursor.r, chc_cursor.c);
	oflush();
	ch = igetkey();
	chc_lefttime = CHC_TIMEOUT - (now - start_time);
	if (chc_lefttime < 0)
	    ch = 'q';
	switch (ch) {
	case I_OTHERDATA:
	    if (!chc_broadcast_recv(act_list, board)) {	/* disconnect */
		endgame = 1;
		endturn = 1;
	    } else if (chc_from.r == -1 && chc_ipass) {
		endgame = 3;
		endturn = 1;
	    }
	    break;
	case KEY_UP:
	    chc_cursor.r--;
	    if (chc_cursor.r < 0)
		chc_cursor.r = BRD_ROW - 1;
	    break;
	case KEY_DOWN:
	    chc_cursor.r++;
	    if (chc_cursor.r >= BRD_ROW)
		chc_cursor.r = 0;
	    break;
	case KEY_LEFT:
	    chc_cursor.c--;
	    if (chc_cursor.c < 0)
		chc_cursor.c = BRD_COL - 1;
	    break;
	case KEY_RIGHT:
	    chc_cursor.c++;
	    if (chc_cursor.c >= BRD_COL)
		chc_cursor.c = 0;
	    break;
	case 'q':
	    endgame = 2;
	    endturn = 1;
	    break;
	case 'p':
	    chc_ipass = 1;
	    chc_from.r = -1;
	    chc_broadcast_send(act_list, board);
	    strlcpy(chc_warnmsg, "\033[1;33m�n�D�M��!\033[m", sizeof(chc_warnmsg));
	    chc_drawline(board, user1, user2, WARN_ROW);
	    bell();
	    break;
	case '\r':
	case '\n':
	case ' ':
	    if (chc_selected) {
		if (chc_cursor.r == chc_select.r &&
		    chc_cursor.c == chc_select.c) {
		    chc_selected = 0;
		    chc_drawline(board, user1, user2, LTR(chc_cursor.r));
		} else if (chc_canmove(board, chc_select, chc_cursor)) {
		    if (CHE_P(board[chc_cursor.r][chc_cursor.c]) == 1)
			endgame = 1;
		    chc_from = chc_select;
		    chc_to = chc_cursor;
		    if (!endgame) {
			memcpy(tmpbrd, board, sizeof(board_t));
			chc_movechess(tmpbrd);
		    }
		    if (endgame || !chc_iskfk(tmpbrd)) {
			chc_drawline(board, user1, user2, STEP_ROW);
			chc_log_step(board, &chc_from, &chc_to);
			chc_movechess(board);
			chc_broadcast_send(act_list, board);
			chc_selected = 0;
			chc_drawline(board, user1, user2, LTR(chc_from.r));
			chc_drawline(board, user1, user2, LTR(chc_to.r));
			endturn = 1;
		    } else {
			strlcpy(chc_warnmsg, "\033[1;33m���i�H������\033[m", sizeof(chc_warnmsg));
			bell();
			chc_drawline(board, user1, user2, WARN_ROW);
		    }
		}
	    } else if (board[chc_cursor.r][chc_cursor.c] &&
		     CHE_O(board[chc_cursor.r][chc_cursor.c]) == chc_turn) {
		chc_selected = 1;
		chc_select = chc_cursor;
		chc_drawline(board, user1, user2, LTR(chc_cursor.r));
	    }
	    break;
	}
    }
    return endgame;
}

static void
mainloop(int s, chcusr_t *user1, chcusr_t *user2, board_t board, play_func_t play_func[2])
{
    int             endgame;
    char	    buf[80];
    board_t         tmpbrd;

    if (!(chc_mode & CHC_WATCH))
	chc_turn = 1;
    for (endgame = 0; !endgame; chc_turn ^= 1) {
	chc_firststep = 0;
	chc_drawline(board, user1, user2, TURN_ROW);
	if (chc_ischeck(board, chc_turn)) {
	    strlcpy(chc_warnmsg, "\033[1;31m�N�x!\033[m", sizeof(chc_warnmsg));
	    bell();
	} else
	    chc_warnmsg[0] = 0;
	chc_drawline(board, user1, user2, WARN_ROW);
	endgame = play_func[chc_turn] (s, user1, user2, board, tmpbrd);
    }

    if (chc_mode & CHC_VERSUS) {
	if (endgame == 1) {
	    strlcpy(chc_warnmsg, "���{��F!", sizeof(chc_warnmsg));
	    user1->win++;
	    currutmp->chc_win++;
	} else if (endgame == 2) {
	    strlcpy(chc_warnmsg, "�A�{��F!", sizeof(chc_warnmsg));
	    user1->lose++;
	    currutmp->chc_lose++;
	} else {
	    strlcpy(chc_warnmsg, "�M��", sizeof(chc_warnmsg));
	    user1->tie++;
	    currutmp->chc_tie++;
	}
	user1->lose--;
	chcusr_put(&cuser, user1);
	passwd_update(usernum, &cuser);
    }
    else if (chc_mode & CHC_WATCH) {
	strlcpy(chc_warnmsg, "�����[��", sizeof(chc_warnmsg));
    }
    else {
	strlcpy(chc_warnmsg, "��������", sizeof(chc_warnmsg));
    }

    chc_log("=> ");
    if (endgame == 3)
	chc_log("�M��");
    else{
	sprintf(buf, "%s��\n", chc_my && endgame == 1 ? "��" : "��");
	chc_log(buf);
    }

    chc_drawline(board, user1, user2, WARN_ROW);
    bell();
    oflush();
}

static void
chc_init_play_func(chcusr_t *user1, chcusr_t *user2, play_func_t play_func[2])
{
    char	    userid[2][IDLEN + 1];

    if (chc_mode & CHC_PERSONAL) {
	strlcpy(userid[0], cuser.userid, sizeof(userid[0]));
	strlcpy(userid[1], cuser.userid, sizeof(userid[1]));
	play_func[0] = play_func[1] = myplay;
    }
    else if (chc_mode & CHC_WATCH) {
	userinfo_t *uinfo = search_ulist_userid(currutmp->mateid);
	strlcpy(userid[0], uinfo->userid, sizeof(userid[0]));
	strlcpy(userid[1], uinfo->mateid, sizeof(userid[1]));
	play_func[0] = play_func[1] = hisplay;
    }
    else {
	strlcpy(userid[0], cuser.userid, sizeof(userid[0]));
	strlcpy(userid[1], currutmp->mateid, sizeof(userid[1]));
	play_func[chc_my] = myplay;
	play_func[chc_my ^ 1] = hisplay;
    }

    getuser(userid[0]);
    chcusr_get(&xuser, user1);
    getuser(userid[1]);
    chcusr_get(&xuser, user2);
}

static void
chc_watch_request(int signo)
{
    if (!(currstat & CHC))
	return;
    chc_act_list *tmp;
    for(tmp = act_list; tmp->next != NULL; tmp = tmp->next);
    tmp->next = (chc_act_list *)malloc(sizeof(chc_act_list));
    tmp = tmp->next;
    tmp->sock = reply_connection_request(uip);
    if (tmp->sock < 0)
	return;
    tmp->next = NULL;
    write(tmp->sock, chc_bp, sizeof(board_t));
    write(tmp->sock, &chc_my, sizeof(chc_my));
    write(tmp->sock, &chc_turn, sizeof(chc_turn));
    write(tmp->sock, &currutmp->turn, sizeof(currutmp->turn));
    write(tmp->sock, &chc_firststep, sizeof(chc_firststep));
    write(tmp->sock, &chc_mode, sizeof(chc_mode));
}

static void
chc_init(int s, chcusr_t *user1, chcusr_t *user2, board_t board, play_func_t play_func[2])
{
    userinfo_t     *my = currutmp;

    setutmpmode(CHC);
    clear();
    chc_warnmsg[0] = 0;

    /* �q���P�ӷ���l�ƦU���ܼ� */
    if (!(chc_mode & CHC_WATCH)) {
	if (chc_mode & CHC_PERSONAL)
	    chc_my = 1;
	else
	    chc_my = my->turn;
	chc_firststep = 1;
	chc_init_board(board);
	chc_cursor.r = 9, chc_cursor.c = 0;
    }
    else {
	char mode;
	read(s, board, sizeof(board_t));
	read(s, &chc_my, sizeof(chc_my));
	read(s, &chc_turn, sizeof(chc_turn));
	read(s, &my->turn, sizeof(my->turn));
	read(s, &chc_firststep, sizeof(chc_firststep));
	read(s, &mode, sizeof(mode));
	if (mode & CHC_PERSONAL)
	    chc_mode |= CHC_WATCH_PERSONAL;
    }

    act_list = (chc_act_list *)malloc(sizeof(*act_list));
    act_list->sock = s;
    act_list->next = 0;

    chc_init_play_func(user1, user2, play_func);

    chc_redraw(user1, user2, board);
    add_io(s, 0);

    signal(SIGUSR1, chc_watch_request);

    if (my->turn && !(chc_mode & CHC_WATCH))
	chc_broadcast_recv(act_list, board);

    user1->lose++;

    if (chc_mode & CHC_VERSUS) {
	passwd_query(usernum, &xuser);
	chcusr_put(&xuser, user1);
	passwd_update(usernum, &xuser);
    }

    if (!my->turn) {
	if (!(chc_mode & CHC_WATCH))
	    chc_broadcast_send(act_list, board);
    	user2->lose++;
    }
    chc_redraw(user1, user2, board);
}

void
chc(int s, int mode)
{
    chcusr_t	    user1, user2;
    play_func_t     play_func[2];
    board_t	    board;
    char	    mode0 = currutmp->mode;
    char	    file[80];

    signal(SIGUSR1, SIG_IGN);

    chc_mode = mode;
    chc_bp = &board;

    chc_init(s, &user1, &user2, board, play_func);
    
    setuserfile(file, CHC_LOG);
    if (chc_log_open(&user1, &user2, file) < 0)
	vmsg("�L�k�����ѧ�");
    
    mainloop(s, &user1, &user2, board, play_func);

    /* close these fd */
    if (chc_mode & CHC_PERSONAL)
	act_list = act_list->next;
    while(act_list){
	close(act_list->sock);
	act_list = act_list->next;
    }

    add_io(0, 0);
    if (chc_my)
	pressanykey();

    currutmp->mode = mode0;

    if (getans("�O�_�N���бH�^�H�c�H[N/y]") == 'y') {
	char title[80];
	sprintf(title, "%s V.S. %s", user1.userid, user2.userid);
	chc_log("\n--\n\n");
	chc_log_poem();
	chc_log_close();
	mail_id(cuser.userid, title, file, "[���e�~��]");
    }
    else
	chc_log_close();
    signal(SIGUSR1, talk_request);
}

static userinfo_t *
chc_init_utmp(void)
{
    char            uident[16];
    userinfo_t	   *uin;

    stand_title("���e�~�ɤ���");
    generalnamecomplete(msg_uid, uident, sizeof(uident),
			SHM->UTMPnumber,
			completeutmp_compar,
			completeutmp_permission,
			completeutmp_getname);
    if (uident[0] == '\0')
	return NULL;

    if ((uin = search_ulist_userid(uident)) == NULL)
	return NULL;

    uin->sig = SIG_CHC;
    return uin;
}

int
chc_main(void)
{
    userinfo_t     *uin;
    
    if ((uin = chc_init_utmp()) == NULL)
	return -1;
    uin->turn = 1;
    currutmp->turn = 0;
    strlcpy(uin->mateid, currutmp->userid, sizeof(uin->mateid));
    strlcpy(currutmp->mateid, uin->userid, sizeof(currutmp->mateid));
    
    my_talk(uin, friend_stat(currutmp, uin), 'c');
    return 0;
}

int
chc_personal(void)
{
    chc(0, CHC_PERSONAL);
    return 0;
}

int
chc_watch(void)
{
    int 	    sock, msgsock;
    userinfo_t     *uin;

    if ((uin = chc_init_utmp()) == NULL)
	return -1;

    if (uin->uid == currutmp->uid || uin->mode != CHC)
	return -1;

    if ((sock = make_connection_to_somebody(uin, 10)) < 0) {
	vmsg("�L�k�إ߳s�u");
	return -1;
    }
    msgsock = accept(sock, (struct sockaddr *) 0, (socklen_t *) 0);
    close(sock);
    if (msgsock < 0)
	return -1;

    strlcpy(currutmp->mateid, uin->userid, sizeof(currutmp->mateid));
    chc(msgsock, CHC_WATCH);
    close(msgsock);
    return 0;
}