#!/usr/bin/env ruby

require 'fileutils'

################################################################################
################################################################################
## Derived from ruby-stemmer https://github.com/aurelian/ruby-stemmer

# FreeBSD make is gmake
make= (RUBY_PLATFORM =~ /freebsd/)? 'gmake' : 'make'

LIBSTEMMER = File.expand_path(File.join(File.dirname(__FILE__), 'libstemmer_c'))

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
  Dir.chdir(LIBSTEMMER) {
    system(make) || exit(false)
  }
end

################################################################################
################################################################################
