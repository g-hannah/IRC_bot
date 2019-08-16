#include "mirc_bot.h"

struct HEADER
{
	us ident;
#if __BYTE_ORDER == __LITTLE_ENDIAN
	us rd:1; /* recursion desired */
	us tc:1; /* response truncated */
	us aa:1; /* authoritative answers */
	us opcode:4;
	us qr:1; /* query / response */
	us rcode:4;
	us cd:1;  /* checking disabled */
	us ad:1; /* authentic data */
	us z:1; /* reserved; zero */
	us ra:1; /* recursion available */
#elif __BYTE_ORDER == __BIG_ENDIAN
	us qr:1;
	us opcode:4;
	us aa:1;
	us tc:1;
	us rd:1;
	us ra:1;
	us z:1;
	us ad:1;
	us cd:1;
	us rcode:4;
#else
# error "please adjust <bits/endian.h>"
#endif
	us qdcnt;
	us ancnt;
	us nscnt;
	us arcnt;
};

typedef struct HEADER DNS_HEADER;

struct QUESTION
{
	us qtype;
	us qclass;
};

typedef struct QUESTION DNS_QUESTION;
