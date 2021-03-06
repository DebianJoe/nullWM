PREFIX?=/usr/X11R6
CFLAGS?=-Os -pedantic -Wall

all:
	$(CC) $(CFLAGS) -I$(PREFIX)/include nullwm.c -L$(PREFIX)/lib -lX11 -o nullwm

sauce:
	$(CC) $(CFLAGS) -I$(PREFIX)/include nullwmsauce.c -L$(PREFIX)/lib -lX11 -o nullwm

install:
	cp nullwm /usr/bin/nullwm

clean:
	rm -f nullwm
