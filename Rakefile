require 'rake/testtask'

Rake::TestTask.new(:test) do |test|
  test.libs   << 'ext' << 'test'
  test.pattern = 'test/**/test_*.rb'
  test.verbose = true
end

# i don't use jeweler.
task :gemspec do 
  files   = Dir["ext/**/*.{cc,c,mak,h}"] + Dir["{ext,test}/*.rb"] + %w(ext/libstemmer_c/Makefile README.rdoc) 
  gemspec =<<-RUBY
    Gem::Specification.new do |s|
      s.name              = "chipper"
      s.version           = "__VERSION__"
      s.date              = "__DATE__"
      s.authors           = ["Bharanee Rathna", "John Barratt"]
      s.email             = ["deepfryed@gmail.com", "djon00@gmail.com"]
      s.summary           = "twitter text extractor"
      s.description       = "twitter text extraction utilities"
      s.homepage          = "http://github.com/deepfryed/chipper"
      s.files             = __FILES__
      s.extra_rdoc_files  = %w(README.rdoc)
      s.extensions        = %w(ext/extconf.rb)
      s.require_paths     = %w(lib)
    end
  RUBY

  gemspec.sub! /__VERSION__/, File.read(File.join(File.dirname(__FILE__), 'ext/src/version.h')).scan(/[\d.]+/).first
  gemspec.sub! /__DATE__/,    Time.now.strftime("%F")
  gemspec.sub! /__FILES__/,   ('[%s]' % files.map(&:inspect).join(', '))
  gemspec.gsub! /^\s{4}/m,    ''

  File.open('chipper.gemspec', 'w') {|fh| fh.write(gemspec)}
end

task :default => :test
