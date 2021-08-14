CC = clang
OBJDIRDEV = bin/obj/dev
OBJDIRREL = bin/obj/rel
VERSION = 1

SOURCES = \
	$(wildcard framework/core/*.c) \
	$(wildcard framework/tools/*.c) \
	$(wildcard framework/ext/*.c) \
	$(wildcard sources/actor/*.c) \
	$(wildcard sources/ui/*.c) \
	$(wildcard sources/*.c)

CFLAGS = \
	-I/usr/local/include \
	-I/usr/local/include/GL \
	-I/usr/local/include/SDL2 \
	-I. \
	-Iframework \
	-Iframework/core \
	-Iframework/tools \
	-Iframework/ext \
	-Isources

LDFLAGS = \
	-L/usr/local/lib \
	-lm \
	-lGL \
	-lGLEW \
	-lSDL2 \
	-lSDL2_mixer \
	-lpthread

OBJECTSDEV := $(addprefix $(OBJDIRDEV)/,$(SOURCES:.c=.o))
OBJECTSREL := $(addprefix $(OBJDIRREL)/,$(SOURCES:.c=.o))

rel: $(OBJECTSREL)
	$(CC) $^ -o bin/termite $(LDFLAGS)

dev: $(OBJECTSDEV)
	$(CC) $^ -o bin/termitedev $(LDFLAGS)

$(OBJECTSDEV): $(OBJDIRDEV)/%.o: %.c
	mkdir -p $(@D)
	$(CC) -c $< -o $@ $(CFLAGS) -g -DDEBUG -DVERSION=0 -DBUILD=0

$(OBJECTSREL): $(OBJDIRREL)/%.o: %.c
	mkdir -p $(@D)
	$(CC) -c $< -o $@ $(CFLAGS) -O3 -DVERSION=$(VERSION) -DBUILD=$(shell cat version.num)

clean:
	rm -f $(OBJECTSDEV) termite
	rm -f $(OBJECTSREL) termite

deps:
	@sudo pkg install ffmpeg sdl2 glew

vjump: 
	$(shell ./version.sh "$$(cat version.num)" > version.num)

rectest:
	tst/test_rec.sh 0

runtest:
	tst/test_run.sh

install: rel
	/usr/bin/install -c -s -m 755 bin/zenmusic /usr/local/bin
	/usr/bin/install -d -m 755 /usr/local/share/zenmusic
	cp res/* /usr/local/share/zenmusic/

remove:
	rm /usr/local/bin/zenmusic
	rm -r /usr/local/share/zenmusic
