CC=gcc
WFLAGS=-Wall -Werror
CFILES=main.c do_connect.c do_bot_stuff.c bot_ops.c logging.c parse_options.c screen.c
OFILES=main.o do_connect.o do_bot_stuff.o bot_ops.o logging.o parse_options.o screen.o
LIBS=-lcrypto -ldnslib -lencodelib -lhashlib -lmisclib -lssl -lpthread

.PHONY: clean

mirc_bot: $(OFILES)
	$(CC) -g $(WFLAGS) -o IRC_Bot $(OFILES) $(LIBS)

$(OFILES): $(CFILES)
	$(CC) -g $(WFLAGS) -c $(CFILES) $(LIBS)

clean:
	rm *.o
