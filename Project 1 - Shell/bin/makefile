CC=gcc
CFLAGS=-Wall -g
AR=ar
AFLAGS=rcs

all : libmsh.a msh_v1 msh_v2 msh_v3 msh_v4 msh_v5 msh_v6

msh_v1 : ../V1/msh_v1.c
	$(CC) $(CFLAGS) $< -o $@ -static -L. -lmsh

msh_v2 : ../V2/msh_v2.c
	$(CC) $(CFLAGS) $< -o $@ -static -L. -lmsh

msh_v3 : ../V3/msh_v3.c
	$(CC) $(CFLAGS) $< -o $@ -static -L. -lmsh

msh_v4 : ../V4/msh_v4.c
	$(CC) $(CFLAGS) $< -o $@ -static -L. -lmsh

msh_v5 : ../V5/msh_v5.c
	$(CC) $(CFLAGS) $< -o $@ -static -L. -lmsh

msh_v6 : ../V6/msh_v6.c
	$(CC) $(CFLAGS) $< -o $@ -static -L. -lmsh


libmsh.a : libmsh_v1.o libmsh_v2.o libmsh_v3.o libmsh_v4.o libmsh_v5.o libmsh_v6.o
	$(AR) $(AFLAGS) $@ libmsh_v1.o libmsh_v2.o libmsh_v3.o libmsh_v4.o libmsh_v5.o libmsh_v6.o

libmsh_v1.o : ../V1/libmsh_v1.c
	$(CC) $(CFLAGS) -c $< -o $@

libmsh_v2.o : ../V2/libmsh_v2.c
	$(CC) $(CFLAGS) -c $< -o $@

libmsh_v3.o : ../V3/libmsh_v3.c
	$(CC) $(CFLAGS) -c $< -o $@

libmsh_v4.o : ../V4/libmsh_v4.c
	$(CC) $(CFLAGS) -c $< -o $@

libmsh_v5.o : ../V5/libmsh_v5.c
	$(CC) $(CFLAGS) -c $< -o $@

libmsh_v6.o : ../V6/libmsh_v6.c
	$(CC) $(CFLAGS) -c $< -o $@


clean:
	rm -v *msh*

