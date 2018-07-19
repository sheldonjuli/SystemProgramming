CC = gcc
CFLAGS = -Wall -g -std=c99

all: ext2_mkdir ext2_cp ext2_ln ext2_rm ext2_rm_bonus ext2_restore ext2_checker

library.o: library.c library.h
	$(CC) $(CFLAGS) -o $@ -c $<

ext2_mkdir: ext2_mkdir.c library.o
	$(CC) $(CFLAGS) -o $@ $^

ext2_cp: ext2_cp.c library.o
	$(CC) $(CFLAGS) -o $@ $^

ext2_ln: ext2_ln.c library.o
	$(CC) $(CFLAGS) -o $@ $^

ext2_rm: ext2_rm.c library.o
	$(CC) $(CFLAGS) -o $@ $^

ext2_rm_bonus: ext2_rm_bonus.c library.o
	$(CC) $(CFLAGS) -o $@ $^

ext2_restore: ext2_restore.c library.o
	$(CC) $(CFLAGS) -o $@ $^

ext2_checker: ext2_checker.c library.o
	$(CC) $(CFLAGS) -o $@ $^