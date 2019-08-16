#include "mirc_bot.h"

void *
play_game(void *arg)
{
	char	*user = NULL;
	BOT_CTX	*bptr = NULL;
	int	randnum;

	user = (char *)arg;
	bptr = &bot;

	randnum = (rand()%6)+1;
	
}
