# test makefile

GCC = gcc

# SDL Mac OS X
CFLAGS = -Wall -D DARWIN -O -I /Developer/SDKs/MacOSX10.4u.sdk/System/Library/Frameworks/OpenGL.framework/Headers -I /Developer/SDKs/MacOSX10.4u.sdk/System/Library/Frameworks/GLUT.framework/Headers -I /Library/Frameworks/SDL.framework/Headers -I/usr/X11/include/freetype2 -I/usr/X11/include

LFLAGS = -framework Cocoa -framework OpenGL -framework GLUT -framework SDL -lstdc++ -Wl -framework CoreServices -framework ApplicationServices -Wl,-dylib_file,/System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGL.dylib:/System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGL.dylib

OBJ = test.o SDLMain.o bbq.o

DEPENDS = test.h bbq.h

# link
test: $(OBJ) Inconsolata.bin FreeSans.bin
	$(GCC) $(OBJ) -o test $(LFLAGS)

# compile
bbq.o: bbq.c bbq.h
	$(GCC) $(CFLAGS) -c $<

# compile
test.o: test.c font.h $(DEPENDS)
	$(GCC) $(CFLAGS) -c $<

# generate bbq header
font.h : font.dd
	bbq-burn font.dd font.h

# generate bbq binary
Inconsolata.bin : Inconsolata.di font.dd Inconsolata.yaml Inconsolata.raw
	bbq-cook Inconsolata.di Inconsolata.bin

# generate bbq binary
FreeSans.bin : FreeSans.di font.dd FreeSans.yaml FreeSans.raw
	bbq-cook FreeSans.di FreeSans.bin

# invoke swiftglyph to generate texture & yaml file
Inconsolata.yaml Inconsolata.raw : Inconsolata.otf
	../swiftglyph -width 256 Inconsolata.otf

FreeSans.yaml FreeSans.raw : FreeSans.otf
	../swiftglyph -width 512 FreeSans.otf

# Mac OS X system files
SDLMain.o : SDLMain.m $(DEPENDS)
	$(GCC) $(CFLAGS) -c $<

clean:
	rm $(OBJ) font.h test FreeSans.yaml FreeSans.raw FreeSans.bin Inconsolata.yaml Inconsolata.raw Inconsolata.bin

