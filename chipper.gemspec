Gem::Specification.new do |s|
  s.name              = "chipper"
  s.version           = "0.3.6"
  s.date              = "2011-12-13"
  s.authors           = ["Bharanee Rathna", "John Barratt"]
  s.email             = ["deepfryed@gmail.com", "djon00@gmail.com"]
  s.summary           = "twitter text extractor"
  s.description       = "twitter text extraction utilities"
  s.homepage          = "http://github.com/deepfryed/chipper"
  s.files             = Dir["ext/**/*.{cc,c,mak,h}"] + %w(README.rdoc ext/*.rb) + Dir["test/*.rb"]
  s.extra_rdoc_files  = %w(README.rdoc)
  s.extensions        = %w(ext/extconf.rb)
  s.require_paths     = %w(lib)
end
