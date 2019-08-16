#include "mirc_bot.h"

pthread_t	time_tid;
pthread_attr_t	time_attr;

int
do_connect(BOT_CTX *bot)
{
	char			*inet4_string = NULL;
	struct sockaddr_in	server;

	bot->fd = -1;
	if (USING_SSL)
		SSL_library_init();

	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons((USING_SSL?IRC_SPORT:IRC_PORT));

	draw_chat_window();
	if (pthread_attr_setdetachstate(&time_attr, PTHREAD_CREATE_DETACHED) < 0) goto fail;
	if (pthread_create(&time_tid, &time_attr, update_time, NULL) < 0) goto fail;

	log_info("Getting IP4 of %s", bot->remote_server);

	if (!(inet4_string = get_inet4_record(bot->remote_server)))
		goto fail;
	inet_pton(AF_INET, inet4_string, &server.sin_addr.s_addr);
	log_info("Got IP4: %s", inet4_string);

	if ((bot->fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	  { log_err("do_connect: failed to open socket"); goto fail; }
	log_info("Opened socket on fd %d", bot->fd);

	log_info("Connecting to %s:%hu ...",
		inet4_string,
		(USING_SSL?IRC_SPORT:IRC_PORT));

	if (connect(bot->fd, (struct sockaddr *)&server, (socklen_t)sizeof(server)) != 0)
	  { log_err("do_connect: failed to connect to server"); goto fail; }

	log_info("Connected to %s:%hu",
		inet4_string,
		(USING_SSL?IRC_SPORT:IRC_PORT));

	if (USING_SSL)
	  {
		if (!(bot->ssl_ctx = SSL_CTX_new(bot->method)))
			goto fail;

		if (!(bot->ssl = SSL_new(bot->ssl_ctx)))
			goto fail;

		SSL_set_fd(bot->ssl, bot->fd);
		SSL_set_connect_state(bot->ssl);
	  }

	log_info("%sonnected to %s:%hu%s",
		(USING_SSL?"Securely c":"C"),
		inet4_string,
		(USING_SSL?IRC_SPORT:IRC_PORT),
		(USING_SSL?" using TLS v1.2!":""));

	return(0);

	fail:
	ERR_print_errors_fp(stderr);
	bot->ops->shutdown_bot(bot);
	return(-1);
}
