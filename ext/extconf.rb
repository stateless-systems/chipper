#!/usr/bin/env ruby

require 'mkmf'
require 'fileutils'

################################################################################
################################################################################
## Derived from ruby-stemmer https://github.com/aurelian/ruby-stemmer

# FreeBSD make is gmake
make= (RUBY_PLATFORM =~ /freebsd/)? 'gmake' : 'make'

ROOT = File.expand_path(File.join(File.dirname(__FILE__), '..'))
LIBSTEMMER = File.join(ROOT, 'libstemmer_c')

# MacOS architecture mess up
if RUBY_PLATFORM =~ /darwin/
  # see: #issue/3, #issue/5
  begin
    ENV['ARCHFLAGS']= "-arch " + %x[file #{File.expand_path(File.join(Config::CONFIG['bindir'], Config::CONFIG['RUBY_INSTALL_NAME']))}].strip!.match(/executable (.+)$/)[1] unless ENV['ARCHFLAGS'].nil?
  rescue
    $stderr << "Failed to get your ruby executable architecture.\n"
    $stderr << "Please specify one using $ARCHFLAGS environment variable.\n"
    exit
  end
  # see: #issue/9, #issue/6
  # see: man compat
  if ENV['COMMAND_MODE'] == 'legacy'
    $stdout << "Setting compat mode to unix2003\n."
    ENV['COMMAND_MODE']= 'unix2003'
  end
end

# make libstemmer_c. unless we're cross-compiling.
unless RUBY_PLATFORM =~ /i386-mingw32/
  system "cd #{LIBSTEMMER}; #{make} libstemmer.o; cd #{ROOT};"
  exit unless $? == 0
end

################################################################################
################################################################################



EXTCONFIG        = Object.const_get(defined?(RbConfig) ? :RbConfig : :Config)::CONFIG
EXTCONFIG['CC']  = 'g++'
EXTCONFIG['CPP'] = 'g++'

version  = %x{g++ --version | head -n1}.strip.sub(%r{^.* ([\d\.]+)$}, '\1')
maj, min = version.split(/\./).values_at(0, 1).map(&:to_i)

$CFLAGS  = '-pthread -Wno-sign-compare '
$CFLAGS += maj > 3 && min > 5 ? '-Ofast ' : '-O3 '
$CFLAGS += '-I/usr/include -I/opt/local/include -I/usr/local/include'

$CFLAGS  += " -I#{File.expand_path(File.join(LIBSTEMMER, 'include'))} "
$libs    += " -L#{LIBSTEMMER} #{File.expand_path(File.join(LIBSTEMMER, 'libstemmer.o'))} "

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
