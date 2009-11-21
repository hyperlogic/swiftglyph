# C++ rakefile

require 'compile'

$verbose = false

CC = 'g++'

# compiler flags
C_FLAGS = ['-Wall', `freetype-config --cflags`.chomp]

# linker flags
L_FLAGS = [`freetype-config --libs`.chomp]
OBJECTS = ['swiftglyph.o', 'tga.o']
EXE = ['swiftglyph']

Compile.setup(:cc => CC,
              :c_flags => C_FLAGS,
              :l_flags => L_FLAGS,
              :objects => OBJECTS,
              :exe => EXE)

####################################

task :default do
  # build exe
  Compile.build
end

