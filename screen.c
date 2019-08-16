#include "mirc_bot.h"

static char *get_username(char *) __nonnull ((1)) __wur;
static char *get_room(char *) __nonnull ((1)) __wur;
static char *get_msg(char *) __nonnull ((1)) __wur;
static int is_user_message(char *) __nonnull ((1)) __wur;
static int is_private_msg(char *) __nonnull ((1)) __wur;
static void place_new_msg(char *) __nonnull ((1));
static int is_bot_message(char *) __nonnull ((1)) __wur;
static int deal_with_bot_message(char *) __nonnull ((1)) __wur;

void
clear_line(int x)
{
	int		i;

	putchar(0x0d);
	for (i = 0; i < (x==0?ws.ws_col:x); ++i)
		printf("%s%c\e[m", BLACK, 0x20);
	putchar(0x0d);
	return;
}

void
change_bg_colour(const char *colour)
{
	int		i;

	clear_screen();
	for (i = 0; i < ws.ws_row; ++i)
	  {
		draw_line_x(colour, ws.ws_col, 0);
		putchar(0x0d);
		if (i != (ws.ws_row-1))
			putchar(0x0a);
	  }

	return;
}

void
draw_line_x(const char *colour, int len, int dir)
{
	int		i;

	for (i = 0; i < len; ++i)
	  {
		if (dir)
			printf("%s%c%c%c\e[m", colour, 0x20, 0x08, 0x08);
		else
			printf("%s%c\e[m", colour, 0x20);
	  }

	return;
}

void
draw_line_y(const char *colour, int len, int dir)
{
	int		i;

	for (i = 0; i < len; ++i)
	  {
		if (dir)
			printf("%s%c%c\e[A\e[m", colour, 0x20, 0x08);
		else
			printf("%s%c%c\e[B\e[m", colour, 0x20, 0x08);
	  }

	return;
}

void
clear_screen(void)
{
	int		i;

	putchar(0x0d);
	for (i = 0; i < ws.ws_row; ++i)
		printf("\e[A");
	for (i = 0; i < ws.ws_row; ++i)
	  { clear_line(0); putchar(0x0a); }
	return;
}

void
center_x(int left, int right)
{
	int		i;

	for (i = 0; i < ((ws.ws_col/2)-left+right); ++i)
		printf("\e[C");
	return;
}

void
center_y(int _up, int down)
{
	int		i;

	for (i = 0; i < ((ws.ws_row/2)+_up-down); ++i)
		printf("\e[A");
	return;
}

void
up(int x)
{
	int		i;

	for (i = 0; i < x; ++i)
		printf("\e[A");
	return;
}

void
down(int x)
{
	int		i;

	for (i = 0; i < x; ++i)
		printf("\e[B");
	return;
}

void
left(int x)
{
	int		i;

	for (i = 0; i < x; ++i)
		printf("\e[D");
	return;
}

void
right(int x)
{
	int		i;

	for (i = 0; i < x; ++i)
		printf("\e[C");
	return;
}

void
draw_chat_window(void)
{
	int		u, r;

	u &= ~u;
	r &= ~r;

	//clear_screen();
	change_bg_colour(BG_COL);
	draw_line_y(BORDER_COL, ws.ws_row-1, 1); u += (ws.ws_row-1);
	right(1); ++r;
	draw_line_y(BORDER_COL, ws.ws_row-1, 0); u -= (ws.ws_row-1);
	draw_line_x(BORDER_COL, ws.ws_col-2, 0); r += (ws.ws_col-2);
	putchar(0x08); --r;
	draw_line_y(BORDER_COL, ws.ws_col-1, 1); u += (ws.ws_col-1);
	left(1); --r;
	draw_line_y(BORDER_COL, ws.ws_row-1, 0); u -= (ws.ws_row-1);
	putchar(0x0d); r &= r;
	up(ws.ws_row-1); u += (ws.ws_row-1);
	draw_line_x(BORDER_COL, ws.ws_col-1, 0); r += (ws.ws_col-1);
	putchar(0x0d); r &= ~r;
	down(1); --u;
	draw_line_x(BORDER_COL, ws.ws_col-1, 0); r += (ws.ws_col-1);
	putchar(0x0d); r &= ~r;
	down(1); --u;
	draw_line_x(BORDER_COL, ws.ws_col-1, 0); r += (ws.ws_col-1);
	putchar(0x0d); r &= ~r;
	up(1); ++u;
	center_x((strlen(bot.botname)/2), 0);
	printf("%s%s%s\e[m", BORDER_COL, TITLE_COL, bot.botname);
	putchar(0x0d);
	down(u); u &= ~u;

	if (r > 0) { left(r); r &= ~r; }
	if (u > 0) { down(u); u &= ~u; }
}

int
add_messages(char *buf)
{
	int		linelen;
	char		*p = NULL, *start = NULL, *end = NULL;
	size_t		len;

	char		*uname = NULL;
	char		*room = NULL;
	char		*msg = NULL;
	char		*tmp = NULL;
	char		*buffer = NULL;

	if (posix_memalign((void **)&tmp, 16, 4096) < 0)
	  { log_err("add_messages > posix_memalign"); goto fail; }
	if (posix_memalign((void **)&buffer, 16, MAXLINE) < 0)
	  { log_err("add_messages > posix_memalign"); goto fail; }

	memset(tmp, 0, 4096);
	memset(buffer, 0, MAXLINE);

	if (strlen(buf) <= 0 || buf[0] == 0)
		return(0);

	len = strlen(buf);
	linelen = (ws.ws_col-6);

	start = p = end = buf;

	while (p < (buf + len))
	  {
		while (*p != 0x0d && *p != 0x0a && p < (buf + len)) ++p;
		end = p;

		if ((end - start) == 0) break;
		strncpy(tmp, start, (end - start));
		tmp[(end - start)] = 0;
		strip_crnl(tmp);

		if (is_bot_message(tmp))
		  {
			if (deal_with_bot_message(tmp) != 0) goto fail;
			while ((*end == 0x0d || *end == 0x0a) && end < (buf + len)) ++end;
			start = p = end;
			continue;
		  }

		if (is_user_message(tmp))
		  {
			uname = get_username(tmp);
			room = get_room(tmp);
			msg = get_msg(tmp);

			if (is_private_msg(tmp))
			  {
				sprintf(tmp,
					"%s%s%s %*.*s> %s\e[m",
					BG_COL,
					PRIV_COL,
					uname,
					(int)(24 - strlen(uname) - 1),
					(int)(24 - strlen(uname) - 1),
					"-------------------------",
					bot.botname);

				place_new_msg(tmp);

				if (strlen(msg) >= (linelen-2))
				  {
					char		*a = NULL, *b = NULL;

					a = b = msg;
					for (;;)
					  {
						while ((b - a) < (linelen-2) && b < (msg + strlen(msg))) ++b;
						if ((b - a) == 0) break;
						memset(tmp, 0, 4096);
						sprintf(tmp, "%s  %s%.*s\e[m",
							BG_COL, TEXT_COL, (int)(b - a), a);
						place_new_msg(tmp);
						a = b;
						if (b == (msg + strlen(msg))) break;
					  }
				  }
				else
				  {
					memset(tmp, 0, 4096);
					sprintf(tmp, "%s  %s%s\e[m",
						BG_COL, TEXT_COL, msg);
					place_new_msg(tmp);
				  }
			  }

			if (strstr(tmp, "JOIN"))
			  {
				sprintf(tmp,
					"%s%s%s joined %s\e[m",
					BG_COL, JOIN_COL, uname, room);

				place_new_msg(tmp);
			  }
			else if (strstr(tmp, "QUIT"))
			  {
				size_t		lm;
				size_t		fl;
				size_t		lpcnt;

				lm = strlen(msg);
				if ((lm + strlen(uname) + 7) >= linelen)
				  {
					char		*a = NULL, *b = NULL;

					fl = (linelen - strlen(uname) - 7);
					a = b = msg;
					lpcnt &= ~lpcnt;
					for(;;)
					  {
						if (lpcnt == 0)
							while ((b - a) < fl && b < (msg + lm)) ++b;
						else
							while ((b - a) < linelen && b < (msg + lm)) ++b;
						if ((b - a) == 0) break;

						if (lpcnt == 0)
						  {
							sprintf(tmp,
								"%s%s%s quit (%*.*s\e[m",
								BG_COL, QUIT_COL, uname,
								(int)(b - a), (int)(b - a),
								a);
						  }
						else
						  {
							sprintf(tmp,
								"%s%s%*.*s\e[m",
								BG_COL, QUIT_COL,
								(int)(b - a), (int)(b - a),
								a);
						  }

						place_new_msg(tmp);
						++lpcnt;
						a = b;
						if (b == (msg + lm)) break;
					  }
				  }
				else
				  {
					sprintf(tmp,
						"%s%s%s quit (%s)\e[m",
						BG_COL, QUIT_COL, uname, msg);

					place_new_msg(tmp);
				  }
			  }
			else if (strstr(tmp, "PART"))
			  {
				size_t		lm;
				size_t		fl;
				size_t		lpcnt;

				if (!msg)
				  {
					sprintf(tmp, "%s%s%s left %s\e[m",
						BG_COL, PART_COL, uname, room);

					place_new_msg(tmp);
					goto bottom;
				  }

				lm = strlen(msg);
				if ((lm + strlen(uname) + strlen(room) + 8) >= linelen)
				  {
					char		*a = NULL, *b = NULL;

					fl = (linelen - strlen(uname) - strlen(room) - 8);
					a = b = msg;
					lpcnt &= ~lpcnt;
					for(;;)
					  {
						if (lpcnt == 0)
							while ((b - a) < fl && b < (msg + lm)) ++b;
						else
							while ((b - a) < linelen && b < (msg + lm)) ++b;
						if ((b - a) == 0) break;

						if (lpcnt == 0)
						  {
							sprintf(tmp,
								"%s%s%s left %s (%*.*s\e[m",
								BG_COL, QUIT_COL, uname, room,
								(int)(b - a), (int)(b - a),
								a);
						  }
						else
						  {
							sprintf(tmp,
								"%s%s%*.*s\e[m",
								BG_COL, QUIT_COL,
								(int)(b - a), (int)(b - a),
								a);
						  }

						place_new_msg(tmp);
						++lpcnt;
						a = b;
						if (b == (msg + lm)) break;
					  }
				  }
				else
				  {
					sprintf(tmp,
						"%s%s%s left %s (%s)\e[m",
						BG_COL, QUIT_COL, uname, room, msg);

					place_new_msg(tmp);
				  }
				sprintf(tmp,
					"%s%s%s left %s\e[m",
					BG_COL, PART_COL, uname, room);

				place_new_msg(tmp);
			  }
			else if (strstr(tmp, "PRIVMSG"))
			  {
				sprintf(tmp,
					"%s%s%s %*.*s> %s\e[m",
					BG_COL,
					TDARK_GREEN,
					uname,
					(int)(24 - strlen(uname) - 1),
					(int)(24 - strlen(uname) - 1),
					"-------------------------",
					room);

				place_new_msg(tmp);

				if (strlen(msg) >= (linelen-2))
				  {
					char	*a = NULL, *b = NULL;

					a = b = msg;

					for (;;)
					  {
						while ((b - a) < (linelen-2) && b < (msg + strlen(msg))) ++b;
						if ((b - a) == 0) break;
						memset(tmp, 0, 4096);
						sprintf(tmp, "%s  %s%.*s\e[m%c",
							BG_COL, TEXT_COL, (int)(b - a), a, 0x00);

						place_new_msg(tmp);
						a = b;
						if (b == (msg + strlen(msg))) break;
					  }
				  }
				else
				  {
					memset(tmp, 0, 4096);
					sprintf(tmp, "%s  %s%s\e[m%c",
						BG_COL, TEXT_COL, msg, 0x00);
					place_new_msg(tmp);
				  }
			  }
		  }
		else
		  {
			if (strlen(tmp) >= linelen)
			  {
				char		*a = NULL, *b = NULL;

				a = b = tmp;

				for(;;)
				  {
					while ((b - a) < (linelen-2) && b < (tmp + strlen(tmp))) ++b;
					if ((b - a) == 0) break;
					memset(buffer, 0, MAXLINE);
					sprintf(buffer, "%s%s%.*s\e[m%c",
						BG_COL, TEXT_COL, (int)(b - a), a, 0x00);

					place_new_msg(buffer);
					a = b;
					if (b == (tmp + strlen(tmp))) break;
				  }
			  }
			else
			  {
				memset(buffer, 0, MAXLINE);
				sprintf(buffer, "%s%s%s\e[m%c", BG_COL, TEXT_COL, tmp, 0x00);
				place_new_msg(buffer);
			  }
		  }

		bottom:
		while ((*end == 0x0a || *end == 0x0d) && end < (buf + len)) ++end;
		if (end == (buf + len)) break;

		start = p = end;
	  }

	if (buffer != NULL) { free(buffer); buffer = NULL; }
	if (tmp != NULL) { free(tmp); tmp = NULL; }
	return (0);

	fail:
	if (buffer != NULL) { free(buffer); buffer = NULL; }
	if (tmp != NULL) { free(tmp); tmp = NULL; }
	return(-1);
}

char *
get_username(char *buf)
{
	static char	un[64];
	char		*p = NULL, *q = NULL;
	size_t		l;
	int		saw_colon;

	l = strlen(buf);
	p = q = (buf+1); // go past initial 0x3a
	saw_colon &= ~saw_colon;
	while (*q != 0x21 && q < (buf + l))
	  { if (*q == 0x3a) saw_colon = 1; ++q; }

	if (*q != 0x21) return(NULL);
	if (saw_colon) return(NULL);

	strncpy(un, p, (q - p));
	un[(q - p)] = 0;

	return(un);
}

char *
get_room(char *buf)
{
	static char	rm[64];
	char		*p = NULL, *q = NULL;
	size_t		l;

	l = strlen(buf);
	p = q = buf;
	while (*p != 0x23 && p < (buf + l)) ++p;
	if (*p != 0x23) return(NULL);

	q = p;
	while (*q != 0x20 && q < (buf + l)) ++q;
	strncpy(rm, p, (q - p));
	rm[(q - p)] = 0;
	return(rm);
}

char *
get_msg(char *buf)
{
	static char	msg[4096];
	char		*p = NULL, *q = NULL;
	size_t		l;

	l = strlen(buf);
	if (l <= 0) return(NULL);

	if ((p = strstr(buf, "QUIT")))
	  {
		while (*p != 0x3a && p < (buf + l)) ++p;
		if (*p != 0x3a) return(NULL);
		++p;
		q = (buf + l);
		strncpy(msg, p, (q - p));
		msg[(q - p)] = 0;
		return(msg);
	  }
	else if ((p = strstr(buf, "PART")))
	  {
		while (*p != 0x3a && p < (buf + l)) ++p;
		if (*p != 0x3a) return(NULL);
		++p;
		q = (buf + l);
		strncpy(msg, p, (q - p));
		msg[(q - p)] = 0;
		return(msg);
	  }
	else if ((p = strstr(buf, "PRIVMSG")))
	  {
		while (*p != 0x3a && p < (buf + l)) ++p;
		if (*p != 0x3a) return(NULL);
		++p;
		q = (buf + l);
		strncpy(msg, p, (q - p));
		msg[(q - p)] = 0;
		return(msg);
	  }
	else
		return(NULL);
}

int
is_user_message(char *buf)
{
	int		excl, at;
	char		*p = NULL;
	size_t		l;

	l = strlen(buf);
	p = buf;
	excl &= ~excl;
	at &= ~at;

	while (p < (buf + l))
	  {
		if (*p == 0x21)
		  {
			excl = 1;
		  }
		if (*p == 0x40)
		  {
			at = 1;
		  }
		++p;
	  }

	return((excl & at));
}

int
is_private_msg(char *buf)
{
	int		excl, at, priv, hash;
	char		*p = NULL;
	size_t		l;

	l = strlen(buf);
	p = buf;
	excl &= ~excl;
	at &= ~at;
	priv &= ~priv;
	hash &= ~hash;

	while (p < (buf + l))
	  {
		if (*p == 0x21)
			excl = 1;
		if (*p == 0x40)
			at = 1;
		if (*p == 0x50 && strncmp("PRIVMSG", p, 7) == 0)
			priv = 1;
		if (*p == 0x23)
			hash = 1;
		++p;
	  }

	return((excl & at & priv & ~hash));
}

void
place_new_msg(char *msg)
{
	int		i, j;
	int		u;
	int		r;

	if (NUM_IN < MAX_IN)
	  {
		memset(INCOMING[NUM_IN], 0, MAXLINE);
		strcpy(INCOMING[NUM_IN], msg);
		++NUM_IN;
	  }
	else
	  {
		for (i = 0; i < (MAX_IN-1); ++i)
		  {
			memset(INCOMING[i], 0, MAXLINE);
			strcpy(INCOMING[i], INCOMING[i+1]);
		  }
		memset(INCOMING[i], 0, MAXLINE);
		strcpy(INCOMING[i], msg);
	  }

	r &= ~r;
	u &= ~u;

	pthread_mutex_lock(&SCREEN_MUTEX);
	if (NUM_IN < MAX_IN)
	  { up(NUM_IN+1); u += (NUM_IN+1); }
	else
	  { up(MAX_IN); u += MAX_IN; }

	for (i = 0; i < NUM_IN; ++i)
	  {
		right(2); r += 2;
		for (j = 0; j < (ws.ws_col-5); ++j)
			printf("%s%c\e[m", BG_COL, 0x20);
		putchar(0x0d); r &= ~r;
		right(2); r += 2;
		printf("%s", INCOMING[i]);
		putchar(0x0d); r &= ~r;
		down(1); --u;
	  }

	if (u > 0) { down(u); u &= ~u; }
	if (r > 0) { left(r); r &= ~r; }
	pthread_mutex_unlock(&SCREEN_MUTEX);

	return;
}

int
is_bot_message(char *buf)
{
	if (strncmp("JOIN", buf, 4) == 0) return(1);
	else if (strncmp("QUIT", buf, 4) == 0) return(1);
	else if (strncmp("PART", buf, 4) == 0) return(1);
	else if (strncmp("PRIVMSG", buf, 7) == 0) return(1);
	else return(0);
}

int
deal_with_bot_message(char *buf)
{
	char		*to = NULL, *m = NULL, *r = NULL;
	char		*a = NULL, *b = NULL;
	char		*out = NULL;
	size_t		l, lm;
	int		maxlen;

	maxlen = (ws.ws_col-6);
	l = strlen(buf);
	if (posix_memalign((void **)&to, 16, MAXLINE) < 0) goto fail;
	if (posix_memalign((void **)&m, 16, MAXLINE) < 0) goto fail;
	if (posix_memalign((void **)&r, 16, MAXLINE) < 0) goto fail;
	if (posix_memalign((void **)&out, 16, MAXLINE) < 0) goto fail;

	memset(to, 0, MAXLINE);
	memset(m, 0, MAXLINE);
	memset(r, 0, MAXLINE);
	memset(out, 0, MAXLINE);

	if (strncmp("PRIVMSG", buf, 7) == 0)
	  {
		a = b = buf;
		while (*a != 0x20 && a < (buf + l)) ++a;
		if (*a != 0x20) goto fail;
		++a;
		b = a;
		while (*b != 0x20 && b < (buf + l)) ++b;
		if (*b != 0x20) goto fail;
		strncpy(to, a, (b - a));
		to[(b - a)] = 0;

		a = (b+2); // a now points at first char of actual message
		b = (buf + l);
		strncpy(m, a, (b - a));
		m[(b - a)] = 0;
		lm = strlen(m);

		sprintf(out, "%s\e[38;5;25m%s %*.*s> %s\e[m",
			BG_COL,
			bot.botname,
			(int)(24 - strlen(bot.botname) -1),
			(int)(24 - strlen(bot.botname) -1),
			"-------------------------",
			to);
		place_new_msg(out);

		if (lm >= (maxlen-2))
		  {
			a = b = m;
			for(;;)
			  {
				while ((b - a) < (maxlen-2) && b < (m + strlen(m))) ++b;
				if ((b - a) == 0) break;
				sprintf(out, "%s  %s%.*s\e[m",
					BG_COL, TEXT_COL, (int)(b - a), a);
				place_new_msg(out);
				a = b;
				if (b == (m + strlen(m))) break;
			  }
		  }
		else
		  {
			sprintf(out, "%s  %s%s\e[m", BG_COL, TEXT_COL, m);
			place_new_msg(out);
		  }
	  }
	else if (strncmp("JOIN", buf, 4) == 0)
	  {
		a = b = buf;
		while (*a != 0x23 && a < (buf + l)) ++a;
		if (*a != 0x23) goto fail;
		b = a;
		while (*b != 0x20 && b < (buf + l)) ++b;
		if (*b != 0x20) goto fail;
		strncpy(r, a, (b - a));
		r[(b - a)] = 0;

		sprintf(out, "%s%s%s joined %s\e[m", BG_COL, TGREEN, bot.botname, r);
		place_new_msg(out);
	  }
	else if (strncmp("QUIT", buf, 4) == 0)
	  {
		sprintf(out, "%s%s%s quit! Such a quitter!\e[m", BG_COL, TPURPLE, bot.botname);
		place_new_msg(out);
	  }
	else if (strncmp("PART", buf, 4) == 0)
	  {
		a = b = buf;
		while (*a != 0x23 && a < (buf + l)) ++a;
		if (*a != 0x23) goto fail;
		b = a;
		while (*b != 0x20 && b < (buf + l)) ++b;
		if (*b != 0x20) goto fail;
		strncpy(r, a, (b - a));
		r[(b - a)] = 0;

		sprintf(out, "%s%s%s left %s\e[m", BG_COL, TYELLOW, bot.botname, r);
		place_new_msg(out);
	  }
	else // just print it to screen raw
	  {
		if (l >= maxlen)
		  {
			a = b = buf;
			for (;;)
			  {
				while ((b - a) < maxlen && b < (buf + l)) ++b;
				if ((b - a) == 0) break;
				sprintf(out, "%s%s%.*s\e[m",
					BG_COL, TEXT_COL, (int)(b - a), a);
				place_new_msg(out);
				a = b;
				if (b == (buf + l)) break;
			  }
		  }
		else
		  {
			sprintf(out, "%s%s%s\e[m", BG_COL, TEXT_COL, buf);
			place_new_msg(out);
		  }
	  }

	if (out != NULL) { free(out); out = NULL; }
	if (to != NULL) { free(to); to = NULL; }
	if (m != NULL) { free(m); m = NULL; }
	if (r != NULL) { free(r); r = NULL; }
	return(0);

	fail:
	if (out != NULL) { free(out); out = NULL; }
	if (to != NULL) { free(to); to = NULL; }
	if (m != NULL) { free(m); m = NULL; }
	if (r != NULL) { free(r); r = NULL; }
	return(-1);
}

/* thread that will update time on screen every second */
void *
update_time(void *arg)
{
	int		r, u;
	struct tm	TIME;
	time_t		now;
	static char	t_string[64];

	r &= ~r;
	u &= ~u;

	for(;;)
	  {
		memset(&TIME, 0, sizeof(TIME));
		time(&now);
		if (gmtime_r(&now, &TIME) < 0)
		  { log_err("time thread: gmtime_r"); continue; }
		strftime(t_string, 64, "%a, %b %d %Y %H:%M:%S", &TIME);

		pthread_mutex_lock(&SCREEN_MUTEX);
		up(ws.ws_row-3); u += (ws.ws_row-3);
		r += ((ws.ws_col) - strlen(t_string) - 3);
		right(r);
		printf("%s%s%s\e[m", BORDER_COL, TIME_COL, t_string);
		putchar(0x0d); r &= ~r;
		down(u); u &= ~u;
		pthread_mutex_unlock(&SCREEN_MUTEX);
		sleep(1);
	  }
}
