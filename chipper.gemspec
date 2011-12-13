Gem::Specification.new do |s|
  s.name              = "chipper"
  s.version           = "0.3.5"
  s.date              = "2011-12-13"
  s.authors           = "Bharanee Rathna"
  s.email             = "deepfryed@gmail.com"
  s.summary           = "twitter text extractor"
  s.description       = "twitter text extraction utilities"
  s.homepage          = "http://github.com/deepfryed/chipper"
  s.files             = Dir["ext/**/*.{cc,h}"] + %w(README.rdoc ext/extconf.rb) + Dir["test/*.rb"] + Dir["libstemmer_c/*/*.{c,h,txt}"] + 
    ["libstemmer_c/MANIFEST","libstemmer_c/Makefile","libstemmer_c/README","libstemmer_c/mkinc.mak","libstemmer_c/mkinc_utf8.mak"]   
  s.extra_rdoc_files  = %w(README.rdoc)
  s.extensions        = %w(ext/extconf.rb)
  s.require_paths     = %w(lib)
end
