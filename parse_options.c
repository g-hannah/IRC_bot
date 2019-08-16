#include "mirc_bot.h"

char		**CHANNELS;
char		*PASSWORD;
char		**messages;
int		NUM_MSGS;
int		USING_SSL;
int		DEBUG;

static void usage(int status) __attribute__ ((__noreturn__));

int
parse_options(int argc, char *argv[], BOT_CTX *bot)
{
	int		i, j, k, fd;
	char		*tmp = NULL, *p = NULL;
	struct stat	statb;

	i = 1;
	CHANNELS = NULL;
	PASSWORD = NULL;
	USING_SSL &= ~USING_SSL;
	DEBUG &= ~DEBUG;

	for (;;)
	  {
		while (i < argc &&
			(strncmp("--", argv[i], 2) != 0) &&
			(strncmp("-", argv[i], 1) != 0))
			++i;

		if (i >= argc)
			break;

		if ((strcmp("--help", argv[i]) == 0) ||
			(strcmp("-h", argv[i]) == 0))
		  {
			usage(EXIT_SUCCESS);
		  }
		else if ((strcmp("--name", argv[i]) == 0) ||
			(strcmp("-n", argv[i]) == 0))
		  {
			++i;
			if (strlen(argv[i]) >= host_max)
			  {
				fprintf(stderr, "parse_options: botname is too long! (%d chars max)\n", host_max-1);
				goto fail;
			  }
			strncpy(bot->botname, argv[i], strlen(argv[i]));
			bot->botname[strlen(argv[i])] = 0;
		  }
		else if ((strcmp("--chan", argv[i]) == 0) ||
			(strcmp("-c", argv[i])) == 0)
		  {
			int	k;

			++i;
			j = i;
			bot->num_chans = 0;

			while ((j < argc) &&
				(strncmp("--", argv[j], 2) != 0) &&
				(strncmp("-", argv[j], 1) != 0))
			  { bot->num_chans++; ++j; }

			j = i;

			if (!(CHANNELS = calloc(bot->num_chans, sizeof(char *))))
			  { log_err("parse_options > calloc"); goto fail; }

			for (k = 0; k < bot->num_chans; ++k)
				CHANNELS[k] = NULL;

			for (k = 0; k < bot->num_chans; ++k)
			  {
				if (strlen(argv[j]) >= host_max)
				  {
					fprintf(stderr,
						"Channel name \"%s\" is too large!\n",
						argv[j]);
					goto fail;
				  }
				if (!(CHANNELS[k] = calloc(host_max, sizeof(char))))
				  { log_err("parse_options > calloc"); goto fail; }
				memset(CHANNELS[k], 0, host_max);
				strncpy(CHANNELS[k], argv[j], strlen(argv[j]));
				CHANNELS[k][strlen(argv[j])] = 0;
				++j;
			  }
		  }
		else if ((strcmp("--server", argv[i]) == 0) ||
			(strcmp("-s", argv[i]) == 0))
		  {
			++i;
			if (strlen(argv[i]) >= host_max)
			  { fprintf(stderr, "Server name too large! (%d chars max)\n", host_max-1); goto fail; }
			strncpy(bot->remote_server, argv[i], strlen(argv[i]));
			bot->remote_server[strlen(argv[i])] = 0;
		  }
		else if ((strcmp("--TLS", argv[i]) == 0) ||
			(strcmp("-T", argv[i]) == 0))
		  {
			USING_SSL = 1;
			++i;
		  }
		else if ((strcmp("--mfile", argv[i]) == 0) ||
			(strcmp("-M", argv[i]) == 0))
		  {
			++i;
			if (access(argv[i], F_OK) != 0)
	  		  { fprintf(stderr, "%s does not exist!\n", argv[i]); goto fail; }
			else if (access(argv[i], R_OK) != 0)
	  		  { fprintf(stderr, "cannot read %s (insufficient privileges)\n", argv[i]); goto fail; }

			memset(&statb, 0, sizeof(statb));
			lstat(argv[i], &statb);

			if ((fd = open(argv[i], O_RDONLY)) < 0)
		  	  { log_err("parse_options > calloc"); goto fail; }

			NUM_MSGS &= ~NUM_MSGS;
			if (!(tmp = calloc(statb.st_size+1, 1)))
		  	  { log_err("parse_options > calloc"); goto fail; }

			memset(tmp, 0, statb.st_size+1);
			read_n(fd, tmp, statb.st_size);
			tmp[statb.st_size] = 0;

			p = tmp;
			while (p < (tmp + statb.st_size))
	  		  {
				if (*p == 0x0a)
				++NUM_MSGS;
				++p;
	  		  }

			if (!(messages = calloc(NUM_MSGS, sizeof(char *))))
		  	  { log_err("parse_options > calloc"); goto fail; }

			p = tmp;
			k &= ~k;
			for (i = 0; i < NUM_MSGS; ++i)
	  		  {
				messages[i] = NULL;
				if (!(messages[i] = calloc(MAXLINE, 1)))
		  		  { log_err("parse_options > calloc"); goto fail; }
				memset(messages[i], 0, MAXLINE);
				while (*p != 0x0a && p < (tmp + statb.st_size) && k < MAXLINE)
					messages[i][k++] = *p++;
				if (k == MAXLINE) --k;
				messages[i][k] = 0;
				k &= ~k;
				++p;
	  		  }

			if (tmp != NULL) { free(tmp); tmp = NULL; }
			close(fd);
		  }
		else if ((strcmp("--pass", argv[i]) == 0) ||
			(strcmp("-p", argv[i]) == 0))
		  {
			++i;
			if (strlen(argv[i]) >= MAX_PASSWORD)
			  {
				fprintf(stderr, "parse_options: password too large (max %d chars)\n",
					MAX_PASSWORD);
				goto fail;
			  }

			if (!(PASSWORD = calloc(MAX_PASSWORD+1, 1)))
			  { log_err("parse_options > calloc (line %d)", __LINE__); goto fail; }

			memset(PASSWORD, 0, MAX_PASSWORD+1);
			strncpy(PASSWORD, argv[i], strlen(argv[i]));
		  }
		else if ((strcmp("--debug", argv[i]) == 0) ||
			(strcmp("-d", argv[i]) == 0))
		  {
			DEBUG = 1;
			++i;
		  }
		else
		  {
			fprintf(stderr, "Invalid option \"%s\"\n", argv[i]);
			goto fail;
		  }
	  }

	if (CHANNELS == NULL)
	  { fprintf(stderr, "parse_options: you must specify a channel to join!\n"); return(-1); }
	if (tmp != NULL) { free(tmp); tmp = NULL; }

	return(0);

	fail:
	if (CHANNELS != NULL)
	  {
		for (i = 0; i < bot->num_chans; ++i)
		  {
			if (CHANNELS[i] != NULL)
			  { free(CHANNELS[i]); CHANNELS[i] = NULL; }
		  }
		free(CHANNELS);
		CHANNELS = NULL;
	  }
	if (tmp != NULL) { free(tmp); tmp = NULL; }
	return(-1);
}

void
usage(int status)
{
	fprintf((status==EXIT_FAILURE?stderr:stdout),
		"%s [OPTIONS]\n\n"
		"-s, --server	Name of IRC server to connect to\n"
		"-T, --TLS	Try to connect securely (not all IRC servers\n"
		"-c, --chan	Channel to join once connected (e.g., ##C)\n"
		"		 + have a secure port open for IRC)\n"
		"-n, --name	The name of the bot\n"
		"-M, --mfile	File with the messages to use for bot\n"
		"-p, --pass	Set password that users need to know in order\n"
		"		 + to communicate with the bot\n"
		"-d, --debug	Run in debug mode\n"
		"-h, --help	Display this information menu\n",
		PROG_NAME);
	exit(status);
}
