# C++ rakefile

require 'compile'

# figure out the build platform
case `uname`.chomp
when /mingw32.*/i
  $PLATFORM = :windows
when "Darwin"
  $PLATFORM = :darwin
when "Linux"
  $PLATFORM = :linux
else
  $PLATFORM = :unknown
end

$verbose = true

CC = 'g++'

# compiler flags
freetype_config_command = $PLATFORM == :windows ? "sh freetype-config" : "freetype-config"

c_flags = ['-Wall', `#{freetype_config_command} --cflags`.chomp]

# linker flags
l_flags = [`#{freetype_config_command} --libs`.chomp]
l_flags << "-Lc:/MinGW/lib" if $PLATFORM == :windows

OBJECTS = ['swiftglyph.o', 'tga.o']
EXE = ['swiftglyph']

Compile.setup(:cc => CC,
              :c_flags => c_flags,
              :l_flags => l_flags,
              :objects => OBJECTS,
              :exe => EXE)

####################################

task :default do
  # build exe
  Compile.build
end

