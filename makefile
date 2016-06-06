#No nonsense makefile
#Make sure you have libgd and libgd-devel installed!

all:
	PKG_CONFIG_PATH=$(PKG_CONFIG_PATH):/usr/local/lib/pkgconfig;
	gcc `pkg-config --cflags --libs sndfile` -Wall -o enscribe enscribe.c -lgd -lpng -lz -ljpeg -lfreetype -lm

install:
	install enscribe /usr/local/bin/enscribe

