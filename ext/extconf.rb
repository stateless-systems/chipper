#!/usr/bin/env ruby

require 'mkmf'
require 'fileutils'

puts 'compiling libstemmer_c'
logs = IO.popen(File.join(File.dirname(__FILE__), 'stemmer.rb') + ' 2>&1') {|io| io.read }

File.open('mkmf.log', 'w') {|fh| fh.write(logs)}

raise "unable to compile libstemmer_c" if $?.exitstatus != 0

EXTCONFIG        = Object.const_get(defined?(RbConfig) ? :RbConfig : :Config)::CONFIG
EXTCONFIG['CC']  = 'g++'
EXTCONFIG['CPP'] = 'g++'

version  = %x{g++ --version | head -n1}.strip.sub(%r{^.* ([\d\.]+)$}, '\1')
maj, min = version.split('.').values_at(0, 1).map(&:to_i)

$CFLAGS  = '-pthread -Wno-sign-compare '
$CFLAGS += (maj >= 4 && min > 5 || maj > 4)  ? '-Ofast ' : '-O3 '
$CFLAGS += '-I/usr/include -I/opt/local/include -I/usr/local/include'

snowball  = File.join(File.dirname(__FILE__), 'libstemmer_c')
$CFLAGS  += " -I#{File.expand_path(File.join(snowball, 'include'))} "
$libs     = 'libstemmer_c/libstemmer.o'

def apt_install_hint pkg
  "sudo apt-get install #{pkg}"
end

def library_installed? name, hint
  if find_library(name, 'main', *%w(/usr/lib /usr/local/lib /opt/lib /opt/local/lib /sw/lib))
    true
  else
    $stderr.puts <<-ERROR

      Unable to find required library: #{name}.
      On debian systems, it can be installed as,

      #{hint}

    ERROR
    false
  end
end

# ugly mkmf hack: manually assign source and object directories.
$srcs = Dir["{re2/*.cc,src/*.cc}"]
$objs = $srcs.map {|name| File.join(File.dirname(name), File.basename(name, ".cc") + ".o")}

class File
  def self.basename name
    name
  end
end

create_makefile 'chipper'
