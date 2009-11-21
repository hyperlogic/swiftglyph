require 'rake/clean'

#$verbose = false

module Compile

  class << self
    attr_accessor :cc, :c_flags, :l_flags, :objects, :exe
  end

  def Compile.setup hash

    hash.each {|key, value| self.send(key.to_s + '=', value)}

    # make sure all parameters are present
    [:cc, :c_flags, :l_flags, :objects, :exe].each do |param|
      raise "Missing value for hash key #{param}"  unless self.send(param)
    end

    # check objects
    @objects.each do |obj| 
      raise "Bad object file #{obj}, must end in .o" unless obj.split('.')[1] == 'o'
    end
    @object_bases = @objects.map {|obj| File.basename(obj).split('.')[0]}

    puts "objects = #{@objects.inspect}" if $verbose
    puts "object_bases = #{@object_bases.inspect}" if $verbose

    # build @srcs array
    @srcs = @object_bases.map do |f|
      if File.exists?(f + '.cpp')
        f + '.cpp'
      elsif File.exists?(f + '.c')
        f + '.c'
      elsif File.exists?(f + '.m')
        f + '.m'
      elsif File.exists?(f + '.mm')
        f + '.mm'
      else
        nil
      end
    end

    # generate tasks to build each source file
    puts "    Adding task #{:make_deps} => #{@srcs.inspect}" if $verbose
    @deps = @object_bases.map {|base| base + '.d'}
    task :make_deps => @srcs + @deps do
      @object_bases.each {|base| add_deps base}
    end

    # generate link task
    puts "    Adding file #{@exe} => #{@objects.inspect}" if $verbose
    file @exe => @objects do |t|
      link t.name, t.prerequisites
    end

    # file tasks for every .d file
    @object_bases.zip(@srcs).each do |base, src|
      # generate file .d => .cpp file tasks
      puts "    Adding file #{base + '.d'} => #{src}" if $verbose
      file base + '.d' => src do
        make_deps base + '.d', src
      end
    end

    CLEAN.include @objects, @deps
    CLOBBER.include @exe

  end

  def Compile.build
    # builds .d files, and also creates file tasks for every .o file
    Rake::Task[:make_deps].invoke

    # builds .o files and links them together to form the executable
    Rake::Task[@exe].invoke
  end

  def Compile.shell cmd
    puts cmd if $verbose
    result = `#{cmd} 2>&1`
    result.each {|line| print line}
  end

  def Compile.compile obj, src
    puts "    Compiling #{obj}"
    cmd = "#{CC} #{@c_flags.join " "} -c #{src} -o #{obj}"
    shell cmd
  end

  def Compile.link exe, objects
    puts "    Linking #{exe}"
    cmd = "#{CC} #{@objects.join " "} -o #{exe} #{@l_flags.join " "}"
    shell cmd
  end

  def Compile.add_deps base
    puts "    Adding depenencies from #{base + '.d'}" if $verbose
    # open up the .d file which is a GNU Makefile rule
    files = []
    File.open(base + '.d', 'r') {|f| f.each {|line| files |= line.split}}
    files.reject! {|x| x == '\\'}  # remove '\\' entries
    puts "    Adding file #{base + '.o'} => #{files[1,files.size].inspect}" if $verbose
    file base + '.o' => files[1,files.size] do |t|
      compile t.name, t.prerequisites[0]
    end
  end

  def Compile.make_deps deps, src
    puts "    Makedeps #{deps}"
    cmd = "#{CC} -MM -MF #{deps} #{@c_flags.join " "} -c #{src}"
    shell cmd
  end


end
