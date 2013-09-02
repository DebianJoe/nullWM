PREFIX?=/usr/X11R6
CFLAGS?=-Os -pedantic -Wall

all:
	$(CC) $(CFLAGS) -I$(PREFIX)/include nullwm.c -L$(PREFIX)/lib -lX11 -o nullwm

clean:
	rm -f nullwm
