CC       = gcc
CFLAGS   = -std=c99 -Wall -Wextra -pedantic -O2
INCLUDES = -Iinclude
AR       = ar
LDLIBS   = -lm

LIB      = libvptree.a

.PHONY: all teste executar clean

all: $(LIB) exemplo

$(LIB): src/vp_tree.c include/vp_tree.h
	$(CC) $(CFLAGS) $(INCLUDES) -c src/vp_tree.c -o vp_tree.o
	$(AR) rcs $@ vp_tree.o

exemplo: examples/exemplo.c $(LIB)
	$(CC) $(CFLAGS) $(INCLUDES) examples/exemplo.c -L. -lvptree $(LDLIBS) -o $@

teste_vp: tests/teste.c $(LIB)
	$(CC) $(CFLAGS) $(INCLUDES) tests/teste.c -L. -lvptree $(LDLIBS) -o $@

teste: teste_vp
	./teste_vp

executar: exemplo
	./exemplo

clean:
	$(RM) vp_tree.o $(LIB) exemplo exemplo.exe teste_vp teste_vp.exe
