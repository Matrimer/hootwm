NAME= lswm

# OpenBSD:
#CFLAGS= -Wall -Wextra -pedantic
#CFLAGS+= -std=c99 -O2
#LIBPATH= -L/usr/X11R6/lib
#INCPATH= -I/usr/X11R6/include

# GNU+Linux
#CFLAGS= -Wall -Wextra -pedantic -isystem /usr/include/bsd -DLIBBSD_OVERLAY -lbsd
#CFLAGS+= -std=gnu99 -O2


LIBS= -lxcb
CC= cc
INSTALLPATH= /usr/local/bin/

FLAGS= $(INCPATH) $(LIBPATH) $(CFLAGS) $(LIBS)

all: main

install: main
	mv $(NAME) $(INSTALLPATH)

uninstall: clean
	rm -f $(INSTALLPATH)/$(NAME)

main:
	$(CC) $(FLAGS) $(NAME).c -o $(NAME)

clean:
	rm -f $(NAME) *.core
