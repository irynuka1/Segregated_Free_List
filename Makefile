# Coitu Sebastian-Teodor 314CAa 2023-2024

CC=gcc
CFLAGS=-Wall -Wextra -std=c99 -g -ggdb3
LIBS=-lm

TARGETS = sfl

build: $(TARGETS)

image_editor: sfl.c
	$(CC) $(CFLAGS) sfl.c -o sfl util.c mat_util.c $(LIBS)

pack:
	zip -FSr 314CA_CoituSebastian-Teodor_Tema1.zip README Makefile *.c *.h

clean:
	rm -f $(TARGETS)

.PHONY: pack clean
