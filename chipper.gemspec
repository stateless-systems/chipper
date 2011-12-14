Gem::Specification.new do |s|
  s.name              = "chipper"
  s.version           = "0.4.2"
  s.date              = "2011-12-14"
  s.authors           = ["Bharanee Rathna", "John Barratt"]
  s.email             = ["deepfryed@gmail.com", "djon00@gmail.com"]
  s.summary           = "twitter text extractor"
  s.description       = "twitter text extraction utilities"
  s.homepage          = "http://github.com/deepfryed/chipper"
  s.files             = ["ext/re2/stringpiece.cc", "ext/re2/onepass.cc", "ext/re2/hash.cc", "ext/re2/prefilter.cc", "ext/re2/dfa.cc", "ext/re2/prog.cc", "ext/re2/perl_groups.cc", "ext/re2/rune.cc", "ext/re2/nfa.cc", "ext/re2/filtered_re2.cc", "ext/re2/compile.cc", "ext/re2/valgrind.cc", "ext/re2/re2.cc", "ext/re2/parse.cc", "ext/re2/set.cc", "ext/re2/unicode_casefold.cc", "ext/re2/prefilter_tree.cc", "ext/re2/simplify.cc", "ext/re2/tostring.cc", "ext/re2/unicode_groups.cc", "ext/re2/bitstate.cc", "ext/re2/mimics_pcre.cc", "ext/re2/regexp.cc", "ext/src/chipper.cc", "ext/libstemmer_c/examples/stemwords.c", "ext/libstemmer_c/libstemmer/libstemmer.c", "ext/libstemmer_c/libstemmer/libstemmer_utf8.c", "ext/libstemmer_c/runtime/utilities.c", "ext/libstemmer_c/runtime/api.c", "ext/libstemmer_c/src_c/stem_UTF_8_russian.c", "ext/libstemmer_c/src_c/stem_ISO_8859_1_french.c", "ext/libstemmer_c/src_c/stem_KOI8_R_russian.c", "ext/libstemmer_c/src_c/stem_UTF_8_swedish.c", "ext/libstemmer_c/src_c/stem_UTF_8_romanian.c", "ext/libstemmer_c/src_c/stem_UTF_8_italian.c", "ext/libstemmer_c/src_c/stem_UTF_8_porter.c", "ext/libstemmer_c/src_c/stem_UTF_8_finnish.c", "ext/libstemmer_c/src_c/stem_UTF_8_hungarian.c", "ext/libstemmer_c/src_c/stem_UTF_8_german.c", "ext/libstemmer_c/src_c/stem_UTF_8_dutch.c", "ext/libstemmer_c/src_c/stem_ISO_8859_1_english.c", "ext/libstemmer_c/src_c/stem_ISO_8859_2_romanian.c", "ext/libstemmer_c/src_c/stem_UTF_8_english.c", "ext/libstemmer_c/src_c/stem_UTF_8_danish.c", "ext/libstemmer_c/src_c/stem_ISO_8859_1_german.c", "ext/libstemmer_c/src_c/stem_UTF_8_portuguese.c", "ext/libstemmer_c/src_c/stem_ISO_8859_1_dutch.c", "ext/libstemmer_c/src_c/stem_UTF_8_spanish.c", "ext/libstemmer_c/src_c/stem_UTF_8_turkish.c", "ext/libstemmer_c/src_c/stem_UTF_8_french.c", "ext/libstemmer_c/src_c/stem_ISO_8859_1_italian.c", "ext/libstemmer_c/src_c/stem_ISO_8859_1_danish.c", "ext/libstemmer_c/src_c/stem_UTF_8_norwegian.c", "ext/libstemmer_c/src_c/stem_ISO_8859_1_finnish.c", "ext/libstemmer_c/src_c/stem_ISO_8859_1_spanish.c", "ext/libstemmer_c/src_c/stem_ISO_8859_1_hungarian.c", "ext/libstemmer_c/src_c/stem_ISO_8859_1_porter.c", "ext/libstemmer_c/src_c/stem_ISO_8859_1_portuguese.c", "ext/libstemmer_c/src_c/stem_ISO_8859_1_norwegian.c", "ext/libstemmer_c/src_c/stem_ISO_8859_1_swedish.c", "ext/libstemmer_c/mkinc_utf8.mak", "ext/libstemmer_c/mkinc.mak", "ext/re2/set.h", "ext/re2/prefilter_tree.h", "ext/re2/stringpiece.h", "ext/re2/regexp.h", "ext/re2/unicode_casefold.h", "ext/re2/variadic_function.h", "ext/re2/prog.h", "ext/re2/filtered_re2.h", "ext/re2/unicode_groups.h", "ext/re2/re2.h", "ext/re2/prefilter.h", "ext/re2/walker-inl.h", "ext/util/utf.h", "ext/util/mutex.h", "ext/util/random.h", "ext/util/util.h", "ext/util/logging.h", "ext/util/pcre.h", "ext/util/sparse_set.h", "ext/util/atomicops.h", "ext/util/test.h", "ext/util/benchmark.h", "ext/util/sparse_array.h", "ext/util/thread.h", "ext/util/flags.h", "ext/util/valgrind.h", "ext/util/arena.h", "ext/libstemmer_c/libstemmer/modules_utf8.h", "ext/libstemmer_c/libstemmer/modules.h", "ext/libstemmer_c/include/libstemmer.h", "ext/libstemmer_c/runtime/api.h", "ext/libstemmer_c/runtime/header.h", "ext/libstemmer_c/src_c/stem_ISO_8859_1_norwegian.h", "ext/libstemmer_c/src_c/stem_UTF_8_danish.h", "ext/libstemmer_c/src_c/stem_UTF_8_turkish.h", "ext/libstemmer_c/src_c/stem_ISO_8859_1_hungarian.h", "ext/libstemmer_c/src_c/stem_UTF_8_english.h", "ext/libstemmer_c/src_c/stem_ISO_8859_1_dutch.h", "ext/libstemmer_c/src_c/stem_UTF_8_italian.h", "ext/libstemmer_c/src_c/stem_ISO_8859_1_danish.h", "ext/libstemmer_c/src_c/stem_ISO_8859_1_english.h", "ext/libstemmer_c/src_c/stem_UTF_8_finnish.h", "ext/libstemmer_c/src_c/stem_UTF_8_spanish.h", "ext/libstemmer_c/src_c/stem_UTF_8_hungarian.h", "ext/libstemmer_c/src_c/stem_ISO_8859_1_swedish.h", "ext/libstemmer_c/src_c/stem_UTF_8_norwegian.h", "ext/libstemmer_c/src_c/stem_UTF_8_russian.h", "ext/libstemmer_c/src_c/stem_UTF_8_french.h", "ext/libstemmer_c/src_c/stem_ISO_8859_2_romanian.h", "ext/libstemmer_c/src_c/stem_UTF_8_portuguese.h", "ext/libstemmer_c/src_c/stem_UTF_8_dutch.h", "ext/libstemmer_c/src_c/stem_ISO_8859_1_portuguese.h", "ext/libstemmer_c/src_c/stem_ISO_8859_1_italian.h", "ext/libstemmer_c/src_c/stem_UTF_8_german.h", "ext/libstemmer_c/src_c/stem_ISO_8859_1_finnish.h", "ext/libstemmer_c/src_c/stem_KOI8_R_russian.h", "ext/libstemmer_c/src_c/stem_UTF_8_romanian.h", "ext/libstemmer_c/src_c/stem_UTF_8_swedish.h", "ext/libstemmer_c/src_c/stem_ISO_8859_1_german.h", "ext/libstemmer_c/src_c/stem_ISO_8859_1_porter.h", "ext/libstemmer_c/src_c/stem_UTF_8_porter.h", "ext/libstemmer_c/src_c/stem_ISO_8859_1_spanish.h", "ext/libstemmer_c/src_c/stem_ISO_8859_1_french.h", "ext/src/version.h", "ext/stemmer.rb", "ext/extconf.rb", "test/helper.rb", "test/test_tokens.rb", "test/test_entities.rb", "ext/libstemmer_c/Makefile", "README.rdoc"]
  s.extra_rdoc_files  = %w(README.rdoc)
  s.extensions        = %w(ext/extconf.rb)
  s.require_paths     = %w(lib)
end
