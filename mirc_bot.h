#include <dnslib.h>
#include <openssl/bio.h>
#include <openssl/conf.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/ssl.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <hashlib.h>
#include <encodelib.h>
#include <misclib.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#define IRC_PORT		6667
#define IRC_SPORT		6697	// secure
#define MAXLINE			1024
#define BOT_BUFFERSZ		8192

#define MAX_PASSWORD		100
#define LOG_FILE		"./pm_log.txt"

extern int			host_max;
extern char			**CHANNELS;
extern char			**authorised_users;
extern int			AUTH_USERS;
extern char			**messages;
extern int			NUM_MSGS;
extern char			*PASSWORD;
extern FILE			*lfp;
extern struct winsize		ws;
extern int			NUM_IN;
extern int			MAX_IN;
extern char			**INCOMING;
extern pthread_mutex_t		SCREEN_MUTEX;
extern pthread_t		time_tid;
extern pthread_attr_t		time_attr;
extern pthread_t		game_tid;
extern pthread_attr_t		game_attr;

struct Bot_ctx_st;
struct Bot_ops_st;

enum Con_state
{
	DISCONNECTED = 0,
	REGISTERING,
	CONNECTED
};

typedef enum Con_state		CON_STATE;

struct Bot_ctx_st
{
	CON_STATE	state;
	const SSL_METHOD	*method;
	SSL_CTX		*ssl_ctx;
	SSL		*ssl;
	int		fd;
	char		*buffer;
	char		*sbuffer;
	time_t		joined;

	/* function pointers */
	char		*botname;
	char		*remote_server;
	/* array of channels we are currently in */
	int		num_chans;
	char		**chans;
	struct Bot_ops_st		*ops;
};

typedef struct Bot_ctx_st	BOT_CTX;

int parse_options(int, char *[], BOT_CTX *) __nonnull ((2,3)) __wur;

/* BOT ops */
void registration(BOT_CTX *) __nonnull ((1));
void send_pong(BOT_CTX *) __nonnull ((1)) __wur;
time_t get_uptime(time_t) __wur;
void show_chans(BOT_CTX *) __nonnull ((1)) __wur;
int join_chan(char *, BOT_CTX *) __nonnull ((1,2)) __wur;
char *get_remote_host_addr(char *) __nonnull ((1)) __wur;
void shutdown_bot(BOT_CTX *) __nonnull ((1));
void show_all_info(BOT_CTX *) __nonnull ((1));
ssize_t reply(BOT_CTX *) __nonnull ((1)) __wur;
ssize_t get(BOT_CTX *) __nonnull ((1)) __wur;
int check_privmsg(BOT_CTX *) __nonnull ((1)) __wur;
int check_for_command(BOT_CTX *) __nonnull ((1)) __wur;
int already_in_list(char *, char **) __nonnull ((1)) __wur;
int get_uname(BOT_CTX *, char *) __nonnull ((1,2)) __wur;
void send_err(char *, const char *, BOT_CTX *) __nonnull ((1,2,3));

struct Bot_ops_st
{
	void (*registration)(BOT_CTX *);
	void (*send_pong)(BOT_CTX *);
	time_t (*get_uptime)(time_t);
	void (*show_all_info)(BOT_CTX *);
	void (*show_chans)(BOT_CTX *);
	int (*join_chan)(char *, BOT_CTX *);
	char *(*get_remote_host_addr)(char *);
	void (*shutdown_bot)(BOT_CTX *);
	ssize_t (*reply)(BOT_CTX *);
	ssize_t (*get)(BOT_CTX *);
	int (*check_privmsg)(BOT_CTX *);
	int (*check_for_command)(BOT_CTX *);
	int (*already_in_list)(char *, char **);
	int (*get_uname)(BOT_CTX *, char *);
	void (*send_err)(char *, const char *, BOT_CTX *);
};

typedef struct Bot_ops_st	BOT_OPS;

extern BOT_CTX			bot;
extern char			*PROG_NAME;
extern int			USING_SSL;
extern int			DEBUG;

int do_connect(BOT_CTX *) __nonnull ((1)) __wur;
void do_bot_stuff(BOT_CTX *) __nonnull ((1));

// defined in logging.c
void log_info(char *, ...) __nonnull ((1));
void log_err(const char *, ...) __nonnull ((1));
void debug(char *, ...) __nonnull ((1));
void log_fp(FILE *, const char *, ...) __nonnull ((1,2));

// defined in screen.c
void clear_screen(void);
void change_bg_colour(const char *) __nonnull ((1));
void clear_line(int);
void center_x(int, int);
void center_y(int, int);
void up(int);
void down(int);
void left(int);
void right(int);
void draw_line_x(const char *, int, int) __nonnull ((1));
void draw_line_y(const char *, int, int) __nonnull ((1));
void draw_chat_window(void);
int add_messages(char *) __nonnull ((1)) __wur;

/* thread functions */
void *update_time(void *);

/* background colours */
#define WHITE		"\e[48;5;15m"
#define RED		"\e[48;5;9m"
#define BLUE		"\e[48;5;12m"
#define DARK_BLUE	"\e[48;5;20m"
#define LIGHT_BLUE	"\e[48;5;33m"
#define SKY_BLUE	"\e[48;5;153m"
#define GREEN		"\e[48;5;10m"
#define DARK_GREEN	"\e[48;5;22m"
#define PINK		"\e[48;5;13m"
#define CYAN		"\e[48;5;14m"
#define YELLOW		"\e[48;5;11m"
#define PURPLE		"\e[48;5;5m"
#define GREY		"\e[48;5;8m"
#define BLACK		"\e[48;5;0m"
#define LEMON		"\e[48;5;229m"
#define AQUA		"\e[48;5;45m"
#define DARK_RED	"\e[48;5;88m"
#define CREAM		"\e[48;5;230m"
#define VIOLET		"\e[48;5;183m"
#define TEAL		"\e[48;5;30m"
#define ORANGE		"\e[48;5;208m"
#define PEACH		"\e[48;5;215m"
#define DARK_GREY	"\e[48;5;240m"
#define LIGHT_GREY	"\e[48;5;252m"
#define SALMON		"\e[48;5;217m"
#define OLIVE		"\e[48;5;100m"

/* text colours */
#define TWHITE		"\e[38;5;15m"
#define TRED		"\e[38;5;9m"
#define TBLUE		"\e[38;5;12m"
#define TDARK_BLUE	"\e[38;5;20m"
#define TLIGHT_BLUE	"\e[38;5;33m"
#define TSKY_BLUE	"\e[38;5;153m"
#define TGREEN		"\e[38;5;10m"
#define TDARK_GREEN	"\e[38;5;22m"
#define TPINK		"\e[38;5;13m"
#define TCYAN		"\e[38;5;14m"
#define TYELLOW		"\e[38;5;11m"
#define TPURPLE		"\e[38;5;5m"
#define TGREY		"\e[38;5;8m"
#define TBLACK		"\e[38;5;0m"
#define TLEMON		"\e[38;5;229m"
#define TAQUA		"\e[38;5;45m"
#define TDARK_RED	"\e[38;5;88m"
#define TCREAM		"\e[38;5;230m"
#define TVIOLET		"\e[38;5;183m"
#define TTEAL		"\e[38;5;30m"
#define TORANGE		"\e[38;5;208m"
#define TPEACH		"\e[38;5;215m"
#define TDARK_GREY	"\e[38;5;240m"
#define TLIGHT_GREY	"\e[38;5;252m"
#define TSALMON		"\e[38;5;217m"
#define TOLIVE		"\e[38;5;100m"

#define BG_COL		WHITE
#define BORDER_COL	WHITE
#define TEXT_COL	TBLACK
#define JOIN_COL	TBLUE
#define QUIT_COL	TRED
#define PART_COL	TPINK
#define PRIV_COL	TDARK_GREEN
#define TIME_COL	TBLACK
#define TITLE_COL	TDARK_GREY
