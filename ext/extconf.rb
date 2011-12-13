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
maj, min = version.split(/\./).values_at(0, 1).map(&:to_i)

$CFLAGS  = '-pthread -Wno-sign-compare '
$CFLAGS += maj > 3 && min > 5 ? '-Ofast ' : '-O3 '
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

srcprefix = '{re2,src}'

create_makefile 'chipper', srcprefix

# NOTE mkmk.rb is shit, it does not allow for multiple source dirs - hence the following hack.
currdir  = File.dirname(__FILE__)
filename = File.join(currdir, 'Makefile')
makefile = File.read(filename)
sources  = Dir["#{srcprefix}/*.cc"]

makefile.gsub! %r{srcdir = [^\r\n]+}mi, "srcdir = #{srcprefix.gsub(/[{}]/, '').gsub(/[\s,]+/, ':')}"
makefile.gsub! %r{SRCS = [^\r\n]+}m,    "SRCS = #{sources.join(' ')}"

File.open(filename, 'w') {|fh| fh.write(makefile) }
