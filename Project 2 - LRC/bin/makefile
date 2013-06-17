CC=gcc
CFLAGS=-Wall -g
AR=ar
AFLAGS=rcs

all : lrcsrv lrccli

lrcsrv : ../server/lrcsrv.c
	$(CC) $(CFLAGS) $< -o $@ -pthread -D_REENTRANT

lrccli : ../client/lrccli.c
	$(CC) $(CFLAGS) $< -o $@ -pthread -D_REENTRANT -I/usr/include/ncurses -lncurses

clean :
	rm -v *lrc* log*

runcli :
	./lrccli -u nick -n name -e email

runcli2 :
	./lrccli -u nick2 -n name2 -e email2

runcli3 :
	./lrccli -u nick3 -n name3 -e email3

runsrv :
	./lrcsrv -l log.txt

kill :
	killall lrcsrv lrccli

cleanfifo :
	rm -v /tmp/lrc*
