#include "mirc_bot.h"
#ifndef _SIGNAL_H
# include <signal.h>
#endif

char		**authorised_users;
int		AUTH_USERS;

static void
handle_sigint(int signo)
{
	int		i;

	if (signo != SIGINT)
		return;

	for (i = 0; i < 2; ++i)
		printf("%c%c%c", 0x08, 0x20, 0x08);
	sprintf(bot.sbuffer, "QUIT :My creator pressed ctrl+c :( Shutting down...\r\n%c", 0x00);
	bot.ops->reply(&bot);

	if (USING_SSL) SSL_shutdown(bot.ssl);
	else shutdown(bot.fd, SHUT_RDWR);
	bot.ops->shutdown_bot(&bot);
	exit(SIGINT);
}

void
do_bot_stuff(BOT_CTX *bot)
{
	time_t		now;
	int		i;
	struct sigaction	sigact_n, sigact_o;

	authorised_users = NULL;
	AUTH_USERS &= ~AUTH_USERS;

	memset(&sigact_n, 0, sizeof(sigact_n));
	memset(&sigact_o, 0, sizeof(sigact_o));

	sigact_n.sa_handler = handle_sigint;
	sigact_n.sa_flags = 0;
	sigemptyset(&sigact_n.sa_mask);
	if (sigaction(SIGINT, &sigact_n, &sigact_o) < 0)
	  { log_err("do_bot_stuff > sigaction"); goto fail; }

	bot->ops->registration(bot);
	time(&bot->joined);

	if (!(bot->chans = calloc(bot->num_chans, sizeof (char *))))
	  { log_err("do_bot_stuff > calloc"); goto fail; }

	draw_chat_window();

	log_info("Requesting to join channels");
	for (i = 0; i < bot->num_chans; ++i)
	  {
		if (!(bot->chans[i] = calloc(host_max, 1)))
		  { log_err("do_bot_stuff > calloc"); goto fail; }

		memset(bot->chans[i], 0, host_max);
		strncpy(bot->chans[i], CHANNELS[i], strlen(CHANNELS[i]));
		bot->chans[i][strlen(CHANNELS[i])] = 0;
		log_info(bot->chans[i]);
		//printf(" %s\n", bot->chans[i]);
		if (bot->ops->join_chan(bot->chans[i], bot) < 0)
		  { log_err("do_bot_stuff > join_chan (line %d)", __LINE__); goto fail; }
	  }

	if (bot->ops->get(bot) < 0)
	  { log_err("do_bot_stuff > get (line %d)", __LINE__); goto fail; }

	sleep(2);
	for (i = 0; i < bot->num_chans; ++i)
		if (bot->ops->join_chan(bot->chans[i], bot) < 0)
		  { log_err("do_bot_stuff > join_chan (line %d)", __LINE__); goto fail; }

	for (;;)
	  {
		time(&now);
		if (((now - bot->joined) % 600) >= 0 &&
		   ((now - bot->joined) % 600) <= 2)
			bot->ops->show_all_info(bot);

		if (bot->ops->get(bot) < 0)
		  { log_err("do_bot_stuff > get (line %d)", __LINE__); goto fail; }
	  }

	bot->ops->shutdown_bot(bot);
	exit(EXIT_SUCCESS);

	fail:
	debug("Shutting down bot");
	bot->ops->shutdown_bot(bot);
	exit(EXIT_FAILURE);
}
