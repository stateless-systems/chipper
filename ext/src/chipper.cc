#include <ruby/ruby.h>
#include <ruby/io.h>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <vector>
#include "re2/re2.h"
#include "re2/stringpiece.h"
#include "libstemmer.h"

#define TO_S(v)       rb_funcall(v, rb_intern("to_s"), 0)
#define CSTRING(v)    RSTRING_PTR(TO_S(v))
#define MIN_TAG_SIZE  3
#define MIN_WORD_SIZE 3

using namespace std;
using namespace re2;

RE2 *UserRE;
RE2 *HashTagRE;
RE2 *UrlRE;
RE2 *UserStopRE;
RE2 *HashTagStopRE;
RE2 *StopWordRE;

RE2::Options DefaultMatchOptions;
VALUE id_users, id_hashtags, id_urls;
struct sb_stemmer *ENStemmer;

string build_alternating_expr(VALUE list) {
    VALUE v;
    string expr = "(?:";
    for (int i = 0; i < RARRAY_LEN(list) - 1; i++) {
        v     = rb_ary_entry(list, i);
        expr += string(RSTRING_PTR(v), RSTRING_LEN(v)) + "|";
    }
    v     = rb_ary_entry(list, RARRAY_LEN(list)-1);
    expr += string(RSTRING_PTR(v), RSTRING_LEN(v)) + ")";
    return expr;
}

// API

VALUE users(VALUE self, VALUE text) {
    if (NIL_P(text) || TYPE(text) != T_STRING)
        rb_raise(rb_eArgError, "Chipper#users requires tweet text");

    VALUE users = rb_ary_new();
    rb_encoding *encoding = rb_enc_get(text);

    string match;
    StringPiece input;
    input.set(RSTRING_PTR(text), RSTRING_LEN(text));
    while (RE2::FindAndConsume(&input, *UserRE, &match)) {
        if (UserStopRE && RE2::FullMatch(match, *UserStopRE)) continue;
        rb_ary_push(users, rb_enc_str_new(match.data(), match.size(), encoding));
    }

    return users;
}

VALUE hashtags(VALUE self, VALUE text) {
    if (NIL_P(text) || TYPE(text) != T_STRING)
        rb_raise(rb_eArgError, "Chipper#hashtags requires tweet text");

    VALUE hashtags = rb_ary_new();
    rb_encoding *encoding = rb_enc_get(text);
    string match;
    StringPiece input;
    input.set(RSTRING_PTR(text), RSTRING_LEN(text));
    while (RE2::FindAndConsume(&input, *HashTagRE, &match)) {
        if (match.size() < MIN_TAG_SIZE) continue;
        if (HashTagStopRE && RE2::FullMatch(match, *HashTagStopRE)) continue;
        rb_ary_push(hashtags, rb_enc_str_new(match.data(), match.size(), encoding));
    }

    return hashtags;
}

VALUE urls(VALUE self, VALUE text) {
    if (NIL_P(text) || TYPE(text) != T_STRING)
        rb_raise(rb_eArgError, "Chipper#urls requires tweet text");

    int size;
    VALUE urls = rb_ary_new();
    rb_encoding *encoding = rb_enc_get(text);
    string match;
    StringPiece input;
    input.set(RSTRING_PTR(text), RSTRING_LEN(text));
    while (RE2::FindAndConsume(&input, *UrlRE, &match)) {
        // we don't want urls terminating with !, it could just be an exclamation mark.
        size = match.data()[match.size() - 1] == '!' ? match.size() - 1 : match.size();
        rb_ary_push(urls, rb_enc_str_new(match.data(), size, encoding));
    }

    return urls;
}

VALUE tokens(VALUE self, VALUE text) {
    static const char *phrase_delim = "\r\n:,;'\"{}()[]./\\%*|&!~`$+=<>?";
    static const char *word_delim   = "\t- ";

    if (NIL_P(text) || TYPE(text) != T_STRING)
        rb_raise(rb_eArgError, "Chipper#tokens requires tweet text");

    VALUE segment, result = rb_ary_new();
    rb_encoding *encoding = rb_enc_get(text);

    char *token, *ptr, *buffer = (char*)malloc(RSTRING_LEN(text) + 1), *phrase_ptr, *word_ptr;

    ptr = buffer;
    bzero(ptr, RSTRING_LEN(text) + 1);
    memcpy(ptr, RSTRING_PTR(text), RSTRING_LEN(text));

    while ((token = strtok_r(ptr, phrase_delim, &phrase_ptr))) {
        ptr     = token;
        segment = rb_ary_new();
        while ((token = strtok_r(ptr, word_delim, &word_ptr))) {
            ptr = NULL;
            if (strlen(token) < MIN_WORD_SIZE || *token == '@' || *token == '#') continue;
            if (StopWordRE) {
                if (RE2::FullMatch(token, *StopWordRE)) continue;

                const sb_symbol *sbstem = sb_stemmer_stem(ENStemmer, (sb_symbol *)token, strlen(token));
                uint32_t sbstem_len     = sb_stemmer_length(ENStemmer);

                string stem((char*)sbstem, sbstem_len);
                if (RE2::FullMatch(stem,  *StopWordRE)) continue;
            }
            rb_ary_push(segment, rb_enc_str_new(token, strlen(token), encoding));
        }
        ptr = NULL;
        if (RARRAY_LEN(segment) > 0)
            rb_ary_push(result, segment);
    }

    free(buffer);
    return result;
}

VALUE skip_users(VALUE self, VALUE list) {
    if (UserStopRE)
        delete UserStopRE;
    UserStopRE = NULL;

    if (NIL_P(list)) return Qtrue;

    if (TYPE(list) != T_ARRAY)
        rb_raise(rb_eArgError, "Chipper#skip_users requires a list");

    UserStopRE = new RE2("@" + build_alternating_expr(list), DefaultMatchOptions);
    return Qtrue;
}

VALUE skip_hashtags(VALUE self, VALUE list) {
    if (HashTagStopRE)
        delete HashTagStopRE;
    HashTagStopRE = NULL;

    if (NIL_P(list)) return Qtrue;

    if (TYPE(list) != T_ARRAY)
        rb_raise(rb_eArgError, "Chipper#skip_hashtags requires a list");

    HashTagStopRE = new RE2("#" + build_alternating_expr(list), DefaultMatchOptions);
    return Qtrue;
}

VALUE skip_tokens(VALUE self, VALUE list) {
    if (StopWordRE)
        delete StopWordRE;
    StopWordRE = NULL;

    if (NIL_P(list)) return Qtrue;

    if (TYPE(list) != T_ARRAY)
        rb_raise(rb_eArgError, "Chipper#skip_tokens requires a list");

    // add stems as well
    int i, max = RARRAY_LEN(list);
    for (int i = 0; i < max; i++) {
        VALUE word              = rb_ary_entry(list, i);
        rb_encoding *encoding   = rb_enc_get(word);
        const sb_symbol *sbstem = sb_stemmer_stem(ENStemmer, (sb_symbol *)RSTRING_PTR(word), RSTRING_LEN(word));
        uint32_t sbstem_len     = sb_stemmer_length(ENStemmer);
        rb_ary_push(list, rb_enc_str_new((char*)sbstem, sbstem_len, encoding));
    }

    // too bad, no uniq c api
    rb_funcall(list, rb_intern("uniq!"), 0);
    StopWordRE = new RE2("^" + build_alternating_expr(list) + "$", DefaultMatchOptions);
    return Qtrue;
}

extern "C" {
    void Init_chipper(void) {
        UserRE        = new RE2("(?:^|[^[:alnum:]])+([@＠][[:alnum:]_\\-]+)");
        HashTagRE     = new RE2("(?:^|[^[:alnum:]])+(#[[:alnum:]}_]+)");
        UrlRE         = new RE2("(https?://[[:alnum:]\\-_\\.:@]+\\.[[:alnum:]\\-_]+/?[^\\s\\r\\n]*)");
        UserStopRE    = NULL;
        HashTagStopRE = NULL;
        StopWordRE    = NULL;
        ENStemmer     = sb_stemmer_new("english", "UTF_8");

        DefaultMatchOptions.set_case_sensitive(false);

        id_users    = ID2SYM(rb_intern("users"));
        id_hashtags = ID2SYM(rb_intern("hashtags"));
        id_urls     = ID2SYM(rb_intern("urls"));

        rb_gc_mark(id_users);
        rb_gc_mark(id_hashtags);
        rb_gc_mark(id_urls);

        VALUE mTE = rb_define_module("Chipper");
        rb_define_module_function(mTE, "users",         RUBY_METHOD_FUNC(users), 1);
        rb_define_module_function(mTE, "hashtags",      RUBY_METHOD_FUNC(hashtags), 1);
        rb_define_module_function(mTE, "urls",          RUBY_METHOD_FUNC(urls), 1);
        rb_define_module_function(mTE, "tokens",        RUBY_METHOD_FUNC(tokens), 1);
        rb_define_module_function(mTE, "skip_users",    RUBY_METHOD_FUNC(skip_users), 1);
        rb_define_module_function(mTE, "skip_hashtags", RUBY_METHOD_FUNC(skip_hashtags), 1);
        rb_define_module_function(mTE, "skip_tokens",   RUBY_METHOD_FUNC(skip_tokens), 1);
    }
}
