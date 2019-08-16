#include "mirc_bot.h"

void
log_info(char *fmt, ...)
{
	va_list		args;
	char		*tmp = NULL;
	char		*tmp2 = NULL;

	posix_memalign((void **)&tmp, 16, MAXLINE);
	posix_memalign((void **)&tmp2, 16, MAXLINE);

	va_start(args, fmt);
	vsprintf(tmp, fmt, args);
	va_end(args);

	sprintf(tmp2, "%s%s    ~~%s~~\e[m",
		BG_COL, TPURPLE, tmp);
	add_messages(tmp2);
	//fprintf(stdout, "[+] %s\n", tmp);
	if (tmp != NULL) { free(tmp); tmp = NULL; }
	if (tmp2 != NULL) { free(tmp2); tmp2 = NULL; }

	return;
}

void
log_err(const char *fmt, ...)
{
	va_list		args;
	char		*tmp = NULL;
	char		*tmp2 = NULL;
	int		_errno;

	_errno = errno;

	tmp = calloc(MAXLINE, 1);
	tmp2 = calloc(MAXLINE, 1);
	memset(tmp, 0, MAXLINE);
	memset(tmp2, 0, MAXLINE);

	va_start(args, fmt);
	vsprintf(tmp, fmt, args);
	va_end(args);

	sprintf(tmp2, "[-] %s (%s)\n", tmp, strerror(_errno));
	add_messages(tmp2);
	//fprintf(stderr, "[-] %s (%s)\n", tmp, strerror(_errno));
	if (tmp != NULL) { free(tmp); tmp = NULL; }
	if (tmp2 != NULL) { free(tmp2); tmp2 = NULL; }
	errno = _errno;
	return;
}

void
debug(char *fmt, ...)
{
	va_list		args;
	char		*tmp = NULL;

	if (!DEBUG)
		return;

	posix_memalign((void **)&tmp, 16, MAXLINE);

	va_start(args, fmt);
	vsprintf(tmp, fmt, args);
	va_end(args);
	fprintf(stdout, "\e[48;5;0m\e[38;5;9m[debug] %s\e[m\n", tmp);
	if (tmp != NULL) { free(tmp); tmp = NULL; }
	return;
}

void
log_fp(FILE *fp, const char *fmt, ...)
{
	char		*tmp = NULL;
	va_list		args;
	struct tm	TIME;
	time_t		now;
	static char	tstring[64];

	if (!(tmp = calloc(MAXLINE, 1)))
		exit(EXIT_FAILURE);
	
	memset(&TIME, 0, sizeof(TIME));
	memset(tmp, 0, MAXLINE);
	va_start(args, fmt);
	vsprintf(tmp, fmt, args);
	va_end(args);
	time(&now);
	gmtime_r(&now, &TIME);
	strftime(tstring, 64, "%H:%M:%S", &TIME);
	fprintf(fp, "[%s] %s\n", tstring, tmp);

	if (tmp != NULL) { free(tmp); tmp = NULL; }
	return;
}
