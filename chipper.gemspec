Gem::Specification.new do |s|
  s.name              = "chipper"
  s.version           = "0.2.5"
  s.date              = "2011-11-11"
  s.authors           = "Bharanee Rathna"
  s.email             = "deepfryed@gmail.com"
  s.summary           = "twitter text extractor"
  s.description       = "twitter text extraction utilities"
  s.homepage          = "http://github.com/deepfryed/chipper"
  s.files             = Dir["ext/**/*.{cc,h}"] + %w(README.rdoc ext/extconf.rb) + Dir["test/*.rb"]
  s.extra_rdoc_files  = %w(README.rdoc)
  s.extensions        = %w(ext/extconf.rb)
  s.require_paths     = %w(lib)
end
