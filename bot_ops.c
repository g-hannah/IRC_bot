#include "mirc_bot.h"

static char		*tbuf = NULL;
static char		*uname = NULL;

time_t
get_uptime(time_t joined)
{
	time_t		now;
	time_t		uptime;

	time(&now);
	uptime = (now - joined);
	return(uptime);
}

int
join_chan(char *chan, BOT_CTX *bot)
{
	memset(bot->sbuffer, 0, BOT_BUFFERSZ);
	sprintf(bot->sbuffer, "JOIN %s\r\n%c", chan, 0x00);
	if (bot->ops->reply(bot) < 0)
		return(-1);
	else
		return(0);
}

void
show_chans(BOT_CTX *bot)
{
	int		i;
	char		*tmp = NULL;

	if (posix_memalign((void **)&tmp, 16, MAXLINE) < 0)
	  { log_err("show_chans > posix_memalign"); return; }

	memset(tmp, 0, MAXLINE);

	if (bot->num_chans <= 0) return;
	else
	  {
		sprintf(tmp, "  channels ->");
		log_info(tmp);
		memset(tmp, 0, MAXLINE);

		for (i = 0; i < bot->num_chans; ++i)
		  {
			sprintf(tmp, "    %s", bot->chans[i]);
			log_info(tmp);
			memset(tmp, 0, MAXLINE);
		  }
	  }

	if (tmp != NULL) { free(tmp); tmp = NULL; }
}

void
show_all_info(BOT_CTX *bot)
{
	char		*tmp = NULL;

	if (posix_memalign((void **)&tmp, 16, MAXLINE) < 0)
	  { log_err("show_all_info > posix_memalign"); return; }

	memset(tmp, 0, MAXLINE);

	sprintf(tmp,
			"%s stats:\n"
			"  uptime -> %ld seconds\n"
			"  joined -> %d channels\e[m",
		bot->botname,
		bot->ops->get_uptime(bot->joined),
		bot->num_chans);

	log_info(tmp);
	bot->ops->show_chans(bot);

	if (tmp != NULL) { free(tmp); tmp = NULL; }
}

void
send_pong(BOT_CTX *bot)
{
	char		*p = NULL, *q = NULL;
	char		*tmp = NULL;

	if (posix_memalign((void **)&tmp, 16, MAXLINE) < 0)
	  { log_info("send_pong > posix_memalign"); return; }

	memset(tmp, 0, MAXLINE);

	memset(bot->sbuffer, 0, BOT_BUFFERSZ);
	p = bot->buffer;
	q = bot->buffer;
	while (*q != 0x3a && q < (bot->buffer + strlen(bot->buffer)))
		++q;
	p = q++;
	while (*q != 0x0a && q < (bot->buffer + strlen(bot->buffer)))
		++q;
	if (*q == 0x0a) ++q;
	sprintf(bot->sbuffer, "PONG %*.*s",
		(int)(q - p),
		(int)(q - p),
		p);

	if (USING_SSL)
		SSL_write(bot->ssl, bot->sbuffer, strlen(bot->sbuffer));
	else
		send(bot->fd, bot->sbuffer, strlen(bot->sbuffer), 0);

	sprintf(tmp, "%s just played a round of ping pong with the server",
			bot->botname);
	log_info(tmp);

	if (tmp != NULL) { free(tmp); tmp = NULL; }
	return;
}

void
registration(BOT_CTX *bot)
{
	sprintf(bot->sbuffer,
		"NICK %s\r\n"
		"USER %s * * :Nasalis proboscium\r\n%c",
		bot->botname,
		bot->botname,
		0x00);

	if (USING_SSL)
		SSL_write(bot->ssl, bot->sbuffer, strlen(bot->sbuffer));
	else
		send(bot->fd, bot->sbuffer, strlen(bot->sbuffer), 0);

	return;
}

char *
get_remote_host_addr(char *remote_host)
{
	return (NULL);
}

void
shutdown_bot(BOT_CTX *bot)
{
	int		i;

	debug("Freeing bot->chans");
	if (bot->chans != NULL)
	  {
		for (i = 0; i < bot->num_chans; ++i)
			if (bot->chans[i] != NULL) { free(bot->chans[i]); bot->chans[i] = NULL; }
		free(bot->chans);
		bot->chans = NULL;
	  }

	bot->state = DISCONNECTED;
	debug("Freeing bot->ssl_ctx");
	if (bot->ssl_ctx != NULL) { SSL_CTX_free(bot->ssl_ctx); bot->ssl_ctx = NULL; }
	debug("Freeing bot->ssl");
	if (bot->ssl != NULL) { SSL_free(bot->ssl); bot->ssl = NULL; }
	debug("Freeing bot->buffer");
	if (bot->buffer != NULL) { free(bot->buffer); bot->buffer = NULL; }
	debug("Freeing bot->sbuffer");
	if (bot->sbuffer != NULL) { free(bot->sbuffer); bot->sbuffer = NULL; }

	return;
}

ssize_t
reply(BOT_CTX *bot)
{
	ssize_t		nbytes;
	size_t		tosend, total;
	char		*p = NULL;

	nbytes &= ~nbytes;
	tosend = strlen(bot->sbuffer);
	p = bot->sbuffer;
	total &= ~total;

	if (USING_SSL)
	  {
		while (tosend > 0 && (nbytes = SSL_write(bot->ssl, p, tosend)) > 0)
		  {
			if (nbytes <= 0)
			  {
				if (errno == EINTR)
					continue;
				else
				  { ERR_print_errors_fp(stderr); return(-1); }
			  }

			p += nbytes;
			total += nbytes;
			tosend -= nbytes;
		  }
	  }
	else
	  {
		while (tosend > 0 && (nbytes = send(bot->fd, p, tosend, 0)) > 0)
		  {
			if (nbytes <= 0)
			  {
				if (errno == EINTR)
					continue;
				else
				  { log_err("reply"); return(-1); }
			  }

			p += nbytes;
			total += nbytes;
			tosend -= nbytes;
		  }
	  }

	add_messages(bot->sbuffer);
	return(total);
}

ssize_t
get(BOT_CTX *bot)
{
	ssize_t		nbytes = 0;
	size_t		total = 0;

	if (USING_SSL)
	{
		while ((nbytes = SSL_read(bot->ssl, bot->buffer, BOT_BUFFERSZ-1)) > 0)
		{
			if (nbytes < 0)
			{
				if (errno == EINTR)
					continue;
				else
			  {
					ERR_print_errors_fp(stderr);
					return -1;
				}
			}

			bot->buffer[nbytes] = 0;
			if (strstr(bot->buffer, "PING"))
			{
				bot->ops->send_pong(bot); 
				continue;
			}
			else
			if (strstr(bot->buffer, "353"))
				continue;
			else
			if (strstr(bot->buffer, "366"))
				continue;
			else
			if (strstr(bot->buffer, "372"))
				continue;

			if (add_messages(bot->buffer) < 0)
				goto fail;

			bot->ops->check_privmsg(bot);
			total += nbytes;
		}
	}
	else
	{
		while ((nbytes = recv(bot->fd, bot->buffer, BOT_BUFFERSZ-1, 0)) > 0)
		{
			if (nbytes < 0)
			{
				if (errno == EINTR)
					continue;
				else
			  	  { log_err("get"); return(-1); }
				
			}
			else

			bot->buffer[nbytes] = 0;
			if (strstr(bot->buffer, "PING"))
			{
				bot->ops->send_pong(bot);
				continue;
			}

			if (add_messages(bot->buffer) < 0)
				goto fail;
			bot->ops->check_privmsg(bot);
			//fprintf(stdout, "%s", bot->buffer);
			total += nbytes;
		}
	}

	return 0;

	fail:
	return -1;
}

int
check_privmsg(BOT_CTX *bot)
{
	int		randnum;

	if (!(tbuf = calloc(MAXLINE, 1)))
	  { log_err("check_privmsg > calloc"); goto fail; }

	memset(tbuf, 0, MAXLINE);

	if (!(uname = calloc(MAXLINE, 1)))
	  { log_err("check_privmsg > calloc"); goto fail; }

	memset(uname, 0, MAXLINE);

	sprintf(tbuf, "PRIVMSG %s :%c", bot->botname, 0x00);

	if (strstr(bot->buffer, tbuf))
	{
		log_fp(lfp, "Received PM \"%s\"", bot->buffer);

		if (bot->ops->get_uname(bot, uname) < 0)
	 	  { log_err("check_privmsg > get_uname"); goto fail; }

		if (PASSWORD != NULL)
	  {
			if (!(strstr(bot->buffer, PASSWORD)))
		  {
				if (!bot->ops->already_in_list(uname, authorised_users))
				{
					sprintf(bot->sbuffer,
						"PRIVMSG %s :You need the right password to talk to me... (401) \r\n", uname);
				}
				else
				{
					int		ret;
					if ((ret = bot->ops->check_for_command(bot)) < 0)
					  { log_err("check_privmsg > check_for_command (line %d)", __LINE__); goto fail; }
					else
					if (ret == 0) // we never got a command
					{
						randnum = (rand() % NUM_MSGS);
						sprintf(bot->sbuffer,
							"PRIVMSG %s :%s\r\n", uname, messages[randnum]);
			 		}
					else
					{
						goto end;
					}
				}
			}
			else // the password was in the received private message
	  	{
				if (!authorised_users)
	  		{
					if (!(authorised_users = calloc(1, sizeof(char *))))
					  { log_err("check_privmsg > calloc"); goto fail; }
					authorised_users[0] = NULL;
					if (!(authorised_users[0] = calloc(host_max, 1)))
					  { log_err("check_privmsg > calloc"); goto fail; }
					memset(authorised_users[0], 0, host_max);
					strncpy(authorised_users[0], uname, strlen(uname));
					++AUTH_USERS;
				}
				else
				{
					if (!bot->ops->already_in_list(uname, authorised_users))
					{
						++AUTH_USERS;
						if (!(authorised_users = realloc(authorised_users, (AUTH_USERS * (sizeof(char *))))))
		  				  { log_err("do_bot_stuff > realloc (line %d)", __LINE__); goto fail; }
						authorised_users[AUTH_USERS-1] = NULL;
						if (!(authorised_users[AUTH_USERS-1] = calloc(host_max, 1)))
						  { log_err("do_bot_stuff > calloc (line %d)", __LINE__); goto fail; }
						memset(authorised_users[AUTH_USERS-1], 0, host_max);
						strncpy(authorised_users[AUTH_USERS-1], uname, strlen(uname));
			  	}
				}
				sprintf(bot->sbuffer,
					"PRIVMSG %s :Correct (200)! \r\n", uname);
			}
		} // if password != NULL
		else
		{
			int		ret;
			if ((ret = bot->ops->check_for_command(bot)) < 0)
			  { log_err("check_privmsg > check_for_command (line %d)", __LINE__); goto fail; }
			else
			if (ret == 0) // we never got a command
		  {
				randnum = (rand() % NUM_MSGS);
				sprintf(bot->sbuffer,
		  			"PRIVMSG %s :%s\r\n", uname, messages[randnum]);
		  }
			else // got a command and we answered it within the check_for_command() function
			{
				goto end;
			}
		}

		if (bot->ops->reply(bot) < 0)
		  { log_err("check_privmsg > reply"); goto fail; }

		memset(bot->sbuffer, 0, BOT_BUFFERSZ);
	}

	end:
	if (tbuf != NULL) { free(tbuf); tbuf = NULL; }
	if (uname != NULL) { free(uname); uname = NULL; }
	return (0);

	fail:
	if (tbuf != NULL) { free(tbuf); tbuf = NULL; }
	if (uname != NULL) { free(uname); uname = NULL; }
	return(-1);
}

int
get_uname(BOT_CTX *bot, char *un)
{
	char		*p = NULL, *q = NULL;

	p = bot->buffer;
	while (*p != 0x3a && p < (bot->buffer + strlen(bot->buffer)))
		++p;
	if (*p != 0x3a)
	  { errno = EPROTO; return(-1); }
	++p;

	q = p;
	while (*q != 0x21 && q < (bot->buffer + strlen(bot->buffer)))
		++q;
	if (*q != 0x21)
	  { errno = EPROTO; return(-1); }

	strncpy(un, p, (q - p));
	un[(q - p)] = 0;

	return(0);
}

int
check_for_command(BOT_CTX *bot)
{
	char		*htype = NULL, *p = NULL, *q = NULL;
	char		*string = NULL;
	char		*cmd = NULL;
	char		*hex = NULL;
	unsigned char	*digest = NULL;
	size_t		dlen;
	struct tm	TIME;
	int		GOT_COMMAND;

	GOT_COMMAND &= ~GOT_COMMAND;
	memset(&TIME, 0, sizeof(TIME));
	if (!(htype = calloc(32, 1)))
	  { log_err("check_for_command > calloc"); goto fail; }
	if (!(string = calloc(MAXLINE, 1)))
	  { log_err("check_for_command > calloc"); goto fail; }
	if (!(cmd = calloc(64, 1)))
	  { log_err("check_for_command > calloc"); goto fail; }

	memset(htype, 0, 32);
	memset(string, 0, MAXLINE);
	memset(cmd, 0, 64);
	memset(bot->sbuffer, 0, BOT_BUFFERSZ);

	p = q = bot->buffer;

	for (;;)
	  {
		if (!(p = strchr(bot->buffer, 0x3f)))
			goto end;

		if (strncmp("?hash?", p, 6) == 0)
		  {
			GOT_COMMAND = 1;
			q = p;

			while (*p != 0x20 && p < (bot->buffer + strlen(bot->buffer)))
				++p;

			if (*p != 0x20) { errno = EPROTO; goto fail; }

			++p;

			q = p;

			while (*q != 0x20 && q < (bot->buffer + strlen(bot->buffer)))
				++q;
			if (*q != 0x20) { errno = EPROTO; goto fail; }

			strncpy(htype, p, (q - p));
			htype[(q - p)] = 0;

			++q;

			p = q;

			while (*q != 0x0d && *q != 0x0a && q < (bot->buffer + strlen(bot->buffer)))
				++q;

			if (*q != 0x0d && *q != 0x0a) { errno = EPROTO; goto fail; }

			strncpy(string, p, (q - p));
			string[(q - p)] = 0;

			if (strcmp("md5", htype) == 0)
			  { digest = get_md5(string, (q - p)); dlen = EVP_MD_size(EVP_md5()); }
			else if (strcmp("sha1", htype) == 0)
			  { digest = get_sha1(string, (q - p)); dlen = EVP_MD_size(EVP_sha1()); }
			else if (strcmp("sha224", htype) == 0)
			  { digest = get_sha224(string, (q - p)); dlen = EVP_MD_size(EVP_sha224()); }
			else if (strcmp("sha256", htype) == 0)
			  { digest = get_sha256(string, (q - p)); dlen = EVP_MD_size(EVP_sha256()); }
			else if (strcmp("sha384", htype) == 0)
			  { digest = get_sha384(string, (q - p)); dlen = EVP_MD_size(EVP_sha384()); }
			else if (strcmp("sha512", htype) == 0)
			  { digest = get_sha512(string, (q - p)); dlen = EVP_MD_size(EVP_sha512()); }
			else if (strcmp("ripemd160", htype) == 0)
			  { digest = get_ripemd160(string, (q - p)); dlen = EVP_MD_size(EVP_ripemd160()); }
			else if (strcmp("whirlpool", htype) == 0)
			  { digest = get_whirlpool(string, (q - p)); dlen = EVP_MD_size(EVP_whirlpool()); }
			else
			  {
				sprintf(bot->sbuffer, "PRIVMSG %s :Digest \"%s\" not supported...\r\n%c",
					uname, htype, 0x00);
	
				goto end;
			  }

			hex = hexlify((char *)digest, dlen);
			sprintf(bot->sbuffer, "PRIVMSG %s :%s\r\n%c", uname, hex, 0x00);
			goto end;
		  }
		else if (strncmp("?time?", p, 6) == 0)
		  {
			time_t		now;

			GOT_COMMAND = 1;
			time(&now);
			if (!gmtime_r(&now, &TIME))
			  { log_err("check_for_command > gmtime_r (line %d)", __LINE__); goto fail; }
			memset(string, 0, 64);
			if (strftime(string, 64, "%a, %d %b %Y %H:%M:%S %Z %z", &TIME) < 0)
			  { log_err("check_for_command > strftime (line %d)", __LINE__); goto fail; }

			sprintf(bot->sbuffer, "PRIVMSG %s :Current time: %s\r\n%c", uname, string, 0x00);

			goto end;
		  }
		else if (strncmp("?ibot?", p, 6) == 0)
		  {
			time_t		utime, now;

			GOT_COMMAND = 1;
			time(&now);
			utime = (now - bot->joined);
			sprintf(bot->sbuffer, "PRIVMSG %s :%s uptime: %ld seconds\r\n%c",
				uname, bot->botname, utime, 0x00);

			goto end;
		  }
		else if (strncmp("?epoch?", p, 7) == 0)
		  {
			time_t		now;

			GOT_COMMAND = 1;

			time(&now);
			sprintf(bot->sbuffer,
				"PRIVMSG %s :%ld seconds elapsed since 1st Jan 1970, 00:00 GMT\r\n%c",
				uname, now, 0x00);
			goto end;
		  }
		else if (strncmp("?encode?", p, 8) == 0)
		  {
			char		*encode_type = NULL;
			char		*encoding = NULL;

			GOT_COMMAND = 1;

			// go past the command to the encode type
			while (*p != 0x20 && p < (bot->buffer + strlen(bot->buffer)))
				++p;
			if (p == (bot->buffer + strlen(bot->buffer)))
			  { bot->ops->send_err(uname, "need encoding type and string to encode", bot); goto end; }

			++p;

			q = p;
			
			while (*q != 0x20 && q < (bot->buffer + strlen(bot->buffer)))
				++q;
			if (*q != 0x20)
			  { bot->ops->send_err(uname, "need string to encode", bot); goto end; }

			if (!(encode_type = calloc(32, 1)))
			  { log_err("check_for_command > calloc (line %d)", __LINE__); goto fail; }

			memset(encode_type, 0, 32);
			if ((q - p) >= 32)
			  { bot->ops->send_err(uname, "encode type too large", bot); goto end; }

			strncpy(encode_type, p, (q - p));

			++q;
			p = q;

			// go to end of line
			while (*q != 0x0a && *q != 0x0d && q < (bot->buffer + strlen(bot->buffer)))
				++q;

			if (*q != 0x0a && *q != 0x0d)
			  { bot->ops->send_err(uname, "internal bot error (500)", bot); goto fail; }

			strncpy(string, p, (q - p));
			string[(q - p)] = 0;

			if (strcmp("base64", encode_type) == 0)
				encoding = b64encode(string, (q - p));
			else if (strcmp("url", encode_type) == 0)
				encoding = url_encode(string, (q - p));
			else
			  { bot->ops->send_err(uname, "unrecognised encoding type", bot); goto end; }

			sprintf(bot->sbuffer, "PRIVMSG %s :%s\r\n%c",
				uname, encoding, 0x00);

			if (encode_type != NULL) { free(encode_type); encode_type = NULL; }
			goto end;
		  }
		else if (strncmp("?echo?", p, 6) == 0)
		  {
			GOT_COMMAND = 1;
			while (*p != 0x20 && p < (bot->buffer + strlen(bot->buffer)))
				++p;
			if (*p != 0x20)
			  { bot->ops->send_err(uname, "?echo? requires a string", bot); goto end; }

			++p;

			q = p;

			while (*q != 0x0a && *q != 0x0d && q < (bot->buffer + strlen(bot->buffer)))
				++q;

			if (*q != 0x0a && *q != 0x0d)
			  { bot->ops->send_err(uname, "failed to find end of line to echo", bot); goto end; }

			strncpy(string, p, (q - p));
			string[(q - p)] = 0;

			sprintf(bot->sbuffer, "PRIVMSG %s :%s\r\n%c",
				uname, string, 0x00);

			goto end;
		  }
		else
		  {
			goto end;
		  }
	  }

	end:
	if (bot->ops->reply(bot) < 0) { log_err("check_for_command > reply"); goto fail; }
	memset(bot->sbuffer, 0, BOT_BUFFERSZ);

	if (string != NULL) { free(string); string = NULL; }
	if (htype != NULL) { free(htype); htype = NULL; }
	if (cmd != NULL) { free(cmd); cmd = NULL; }
	return(GOT_COMMAND);

	fail:
	if (string != NULL) { free(string); string = NULL; }
	if (htype != NULL) { free(htype); htype = NULL; }
	if (cmd != NULL) { free(cmd); cmd = NULL; }
	return(-1);
}

int
already_in_list(char *un, char **au)
{
	int		i;
	int		INLIST;

	INLIST &= ~INLIST;
	for (i = 0; i < AUTH_USERS; ++i)
	  {
		if (strcmp(un, au[i]) == 0)
		  { INLIST = 1; break; }
	  }

	return(INLIST);
}

void
send_err(char *un, const char *msg, BOT_CTX *bot)
{
	memset(bot->sbuffer, 0, BOT_BUFFERSZ);
	sprintf(bot->sbuffer, "PRIVMSG %s: %s\r\n%c", un, msg, 0x00);
	bot->ops->reply(bot);
	return;
}
