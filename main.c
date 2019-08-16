#include "mirc_bot.h"

int		host_max;
BOT_CTX		bot;
char		*PROG_NAME;
FILE		*lfp;
struct winsize	ws;
int		NUM_IN, MAX_IN;
char		**INCOMING;
pthread_mutex_t	SCREEN_MUTEX;

static void irc_bot_init(void) __attribute__ ((constructor));
static void irc_bot_fini(void) __attribute__ ((destructor));
static void init_bot(BOT_CTX *) __nonnull ((1));

int
main(int argc, char *argv[])
{
	strncpy(PROG_NAME, argv[0], strlen(argv[0]));
	PROG_NAME[strlen(argv[0])] = 0;

	/*draw_chat_window();
	sleep(2);
	exit(EXIT_SUCCESS);*/

	memset(&bot, 0, sizeof(bot));
	init_bot(&bot);
	if (parse_options(argc, argv, &bot) < 0)
	  { log_err("main > parse_options"); exit(EXIT_FAILURE); }

	do_connect(&bot);
	do_bot_stuff(&bot);
	exit(EXIT_SUCCESS);
}

void
init_bot(BOT_CTX *bot)
{
	if (!(bot->botname = calloc(host_max+1, 1)))
		exit(EXIT_FAILURE);
	if (!(bot->buffer = calloc(BOT_BUFFERSZ, 1)))
		exit(EXIT_FAILURE);
	if (!(bot->sbuffer = calloc(BOT_BUFFERSZ, 1)))
		exit(EXIT_FAILURE);
	if (!(bot->remote_server = calloc(host_max+1, 1)))
		exit(EXIT_FAILURE);
	bot->method = TLSv1_2_client_method();
	bot->ssl_ctx = NULL;
	bot->ssl = NULL;
	bot->num_chans = 0;
	bot->chans = NULL;

	bot->ops = malloc(sizeof(BOT_OPS));
	memset(bot->ops, 0, sizeof(BOT_OPS));

	bot->ops->registration = registration;
	bot->ops->send_pong = send_pong;
	bot->ops->get_uptime = get_uptime;
	bot->ops->show_all_info = show_all_info;
	bot->ops->show_chans = show_chans;
	bot->ops->join_chan = join_chan;
	bot->ops->get_remote_host_addr = get_remote_host_addr;
	bot->ops->shutdown_bot = shutdown_bot;
	bot->ops->reply = reply;
	bot->ops->get = get;
	bot->ops->check_privmsg = check_privmsg;
	bot->ops->check_for_command = check_for_command;
	bot->ops->get_uname = get_uname;
	bot->ops->already_in_list = already_in_list;
	bot->ops->send_err = send_err;

	bot->state = DISCONNECTED;
}

void
irc_bot_init(void)
{
	int		fd;
	int		i;

	srand(time(NULL));

	memset(&ws, 0, sizeof(ws));
	setvbuf(stdout, NULL, _IONBF, 0);
	if (pthread_mutex_init(&SCREEN_MUTEX, NULL) != 0)
	  { log_err("irc_bot_init > pthread_mutex_init"); exit(EXIT_FAILURE); }

	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) < 0)
	  { log_err("irc_bot_init > ioctl (TIOCGWINSZ)"); exit(EXIT_FAILURE); }

	MAX_IN = (ws.ws_row-5);
	NUM_IN &= ~NUM_IN;

	if (!(INCOMING = calloc(MAX_IN, sizeof(char *))))
	  { log_err("irc_bot_init > calloc(INCOMING)"); exit(EXIT_FAILURE); }

	for (i = 0; i < MAX_IN; ++i)
	  {
		INCOMING[i] = NULL;
		if (!(INCOMING[i] = calloc(MAXLINE, sizeof(char))))
		  { log_err("irc_bot_init > calloc(INCOMING)"); exit(EXIT_FAILURE); }
		memset(INCOMING[i], 0, MAXLINE);
	  }

	if (access(LOG_FILE, F_OK) != 0)
	  {
		if ((fd = open(LOG_FILE, O_RDWR|O_CREAT|O_TRUNC, S_IRWXU & ~S_IXUSR)) < 0)
		  { log_err("irc_bot_init > open"); exit(EXIT_FAILURE); }
		if (!(lfp = fdopen(fd, "r+")))
		  { log_err("irc_bot_init > fdopen"); exit(EXIT_FAILURE); }
	  }
	else
	  {
		if (!(lfp = fopen(LOG_FILE, "r+")))
		  { log_err("irc_bot_init > fopen"); exit(EXIT_FAILURE); }
	  }

	host_max &= ~host_max;
	if ((host_max = sysconf(_SC_HOST_NAME_MAX)) == 0)
		host_max = 64;
	OpenSSL_add_all_algorithms();
	ERR_load_crypto_strings();

	OPENSSL_config(NULL);
	// DEPRECATED! (v1.1.1a) OPENSSL_config(NULL);
	// New configuration code is in use in v1.1.1a:
	//if (!(conf_meth = NCONF_default())) { log_err("irc_bot_init > NCONF_default"); goto fail; }
	//if (!(conf = NCONF_new(conf_meth))) { log_err("irc_bot_init > NCONF_new"); goto fail; }

	if (!(PROG_NAME = calloc(host_max, 1))) { log_err("irc_bot_init > calloc"); goto fail; }
	memset(PROG_NAME, 0, host_max);

	return;

	fail:
	exit(EXIT_FAILURE);
}

void
irc_bot_fini(void)
{
	int		i;

	if (PROG_NAME != NULL) { free(PROG_NAME); PROG_NAME = NULL; }

	if (INCOMING != NULL)
	  {
		for (i = 0; i < MAX_IN; ++i)
			if (INCOMING[i] != NULL) { free(INCOMING[i]); INCOMING[i] = NULL; }
		free(INCOMING);
		INCOMING = NULL;
	  }
}

void
usage(int status)
{
	fprintf((status==EXIT_FAILURE?stderr:stdout),
		"mirc_bot <botname> <irc_server>\n");
	exit(status);
}
