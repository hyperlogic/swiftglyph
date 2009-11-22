# simple makefile

GCC = g++

# SDL Mac OS X
CFLAGS = -Wall -D DARWIN -O -I /Developer/SDKs/MacOSX10.4u.sdk/System/Library/Frameworks/OpenGL.framework/Headers -I /Developer/SDKs/MacOSX10.4u.sdk/System/Library/Frameworks/GLUT.framework/Headers -I /Library/Frameworks/SDL.framework/Headers -I/usr/X11/include/freetype2 -I/usr/X11/include

LFLAGS = -framework Cocoa -framework OpenGL -framework GLUT -framework SDL -lstdc++ -Wl -framework CoreServices -framework ApplicationServices -Wl,-dylib_file,/System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGL.dylib:/System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGL.dylib

OBJ = test.o SDLMain.o bbq.o

# TODO: fix dependencies
DEPENDS = test.h bbq.h

test: $(OBJ) Inconsolata.bin FreeSans.bin
	$(GCC) $(OBJ) -o test $(LFLAGS)

bbq.o: bbq.c bbq.h
	$(GCC) $(CFLAGS) -c $<

test.o: test.c font.h $(DEPENDS)
	$(GCC) $(CFLAGS) -c $<

# burn header
font.h : font.dd
	bbq-burn font.dd font.h

# cook font binaries
Inconsolata.bin : Inconsolata.di font.dd
	bbq-cook Inconsolata.di Inconsolata.bin

FreeSans.bin : FreeSans.di font.dd
	bbq-cook FreeSans.di FreeSans.bin

# Mac OS X system files
SDLMain.o : SDLMain.m $(DEPENDS)
	$(GCC) $(CFLAGS) -c $<

clean:
	rm $(OBJ) font.h test
