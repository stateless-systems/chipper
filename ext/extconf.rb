#!/usr/bin/env ruby

require 'mkmf'
require 'fileutils'

Config::CONFIG['CC']  = 'g++'
Config::CONFIG['CPP'] = 'g++'

$CFLAGS  = '-fPIC -Ofast -pthread -Wno-sign-compare '
$CFLAGS += '-I/usr/include -I/opt/local/include -I/usr/local/include'

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

exit 1 unless library_installed? 'stemmer', apt_install_hint('libstemmer-dev')
create_makefile 'chipper', srcprefix

# NOTE mkmk.rb is shit, it does not allow for multiple source dirs - hence the following hack.
currdir  = File.dirname(__FILE__)
filename = File.join(currdir, 'Makefile')
makefile = File.read(filename)
sources  = Dir["#{srcprefix}/*.cc"]

makefile.gsub! %r{srcdir = [^\r\n]+}mi, "srcdir = #{srcprefix.gsub(/[{}]/, '').gsub(/[\s,]+/, ':')}"
makefile.gsub! %r{SRCS = [^\r\n]+}m,    "SRCS = #{sources.join(' ')}"

File.open(filename, 'w') {|fh| fh.write(makefile) }
