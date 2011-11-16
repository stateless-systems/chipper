#include <stdlib.h>
#include <iostream>
#include <vector>
#include "re2/re2.h"
#include "re2/stringpiece.h"
#include "libstemmer.h"

#if __GNUC__
#define STRSTR strcasestr
#else
#define STRSTR strstr
#endif

#include <ruby/ruby.h>
#include <ruby/io.h>

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
RE2 *SkipTokenRE;
RE2 *SkipTokenPatternRE;

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

void replace(char *string, const char *pattern, int c) {
    int width = strlen(pattern);
    char *ptr1, *ptr2 = string;

    while ((ptr1 = strstr(ptr2, pattern))) {
        memset(ptr1, c, width);
        ptr2 = ptr1 + width;
    }
}

void remove(char *string, const char *pattern) {
    int size = strlen(string), width = strlen(pattern);
    char *ptr1, *ptr2 = string;

    while ((ptr1 = strstr(ptr2, pattern))) {
        memcpy(ptr1, ptr1 + width, size - (ptr1 - string) - width);
        size        -= width;
        string[size] = 0;
    }
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

    VALUE urls            = rb_ary_new();
    rb_encoding *encoding = rb_enc_get(text);

    int size;
    string match;
    StringPiece input;
    input.set(RSTRING_PTR(text), RSTRING_LEN(text));

    while (RE2::FindAndConsume(&input, *UrlRE, &match)) {
        // TODO false positives ?
        // we don't want urls terminating with ! or .
        size = match.size();
        if (match.data()[size - 1] == '!') size--;
        if (match.data()[size - 1] == '.') size--;
        rb_ary_push(urls, rb_enc_str_new(match.data(), size, encoding));
    }

    return urls;
}

VALUE tokens(VALUE self, VALUE text) {
    static const char *phrase_delim = "\r\n:,;'\"{}()[]./\\%*|&!~`$+=<>?";
    static const char *word_delim   = "\t- ";

    if (NIL_P(text) || TYPE(text) != T_STRING)
        rb_raise(rb_eArgError, "requires tweet text");

    VALUE segment, result = rb_ary_new();
    rb_encoding *encoding = rb_enc_get(text);

    char *token, *ptr, *buffer = (char*)calloc(RSTRING_LEN(text) + 1, 1), *phrase_ptr, *word_ptr;

    if (!buffer)
        rb_raise(rb_eNoMemError, "ran out of memory copying tweet text");

    ptr = buffer;
    bzero(ptr, RSTRING_LEN(text) + 1);
    memcpy(ptr, RSTRING_PTR(text), RSTRING_LEN(text));

    // downcase input
    while (*ptr) *ptr++ = tolower(*ptr);
    ptr = buffer;

    // blank out urls
    char *ptr1, *ptr2 = ptr;
    while ((ptr1 = STRSTR(ptr2, "http://"))) {
        ptr2 = strtok_r(ptr1, "\r\n\t ", &phrase_ptr);
        ptr2 = phrase_ptr;
        memset(ptr1, '\n', ptr2 - ptr1);
    }

    ptr2 = ptr;
    while ((ptr1 = STRSTR(ptr2, "https://"))) {
        ptr2 = strtok_r(ptr1, "\r\n\t ", &phrase_ptr);
        ptr2 = phrase_ptr;
        memset(ptr1, '\n', ptr2 - ptr1);
    }

    // remove blank out single quotes
    remove(ptr, "'");
    remove(ptr, "\u2019");

    // segment at unicode quotes
    replace(ptr, "\u2018", '\t');
    replace(ptr, "\u201c", '\t');
    replace(ptr, "\u201d", '\t');

    // fullwidth AT => @
    replace(ptr, "\uff20", '@');

    // unicode spaces
    replace(ptr, "\u2000 ", ' ');
    replace(ptr, "\u2001 ", ' ');
    replace(ptr, "\u2002 ", ' ');
    replace(ptr, "\u2003 ", ' ');
    replace(ptr, "\u2004 ", ' ');
    replace(ptr, "\u2005 ", ' ');
    replace(ptr, "\u2006 ", ' ');
    replace(ptr, "\u2007 ", ' ');
    replace(ptr, "\u2008 ", ' ');
    replace(ptr, "\u2009 ", ' ');
    replace(ptr, "\u200A ", ' ');
    replace(ptr, "\u200B ", ' ');
    replace(ptr, "\u202F ", ' ');
    replace(ptr, "\u3000 ", ' ');

    // unicode dashes
    replace(ptr, "\u058A ", '-');
    replace(ptr, "\u1806 ", '-');
    replace(ptr, "\u2010 ", '-');
    replace(ptr, "\u2011 ", '-');
    replace(ptr, "\u2012 ", '-');
    replace(ptr, "\u2013 ", '-');
    replace(ptr, "\u2014 ", '-');
    replace(ptr, "\u2015 ", '-');
    replace(ptr, "\u207B ", '-');
    replace(ptr, "\u208B ", '-');
    replace(ptr, "\u2212 ", '-');
    replace(ptr, "\u301C ", '-');
    replace(ptr, "\u3030 ", '-');

    // corner brackets
    replace(ptr, "\u300C", '<');
    replace(ptr, "\u300E", '<');
    replace(ptr, "\u301D", '<');
    replace(ptr, "\u300D", '>');
    replace(ptr, "\u300F", '>');
    replace(ptr, "\u301F", '>');

    segment = rb_ary_new();
    while ((token = strtok_r(ptr, phrase_delim, &phrase_ptr))) {
        ptr = token;

        while ((token = strtok_r(ptr, word_delim, &word_ptr))) {
            ptr = NULL;

            if (strlen(token) < MIN_WORD_SIZE || *token == '@' || *token == '#') {
                if (RARRAY_LEN(segment) > 0) {
                    rb_ary_push(result, segment);
                    segment = rb_ary_new();
                }
                continue;
            }

            const sb_symbol *sbstem = sb_stemmer_stem(ENStemmer, (sb_symbol *)token, strlen(token));
            uint32_t sbstem_len     = sb_stemmer_length(ENStemmer);

            if (sbstem_len < MIN_WORD_SIZE) {
                if (RARRAY_LEN(segment) > 0) {
                    rb_ary_push(result, segment);
                    segment = rb_ary_new();
                }
                continue;
            }

            if (SkipTokenRE) {
                if (RE2::FullMatch(token, *SkipTokenRE)) {
                    if (RARRAY_LEN(segment) > 0) {
                        rb_ary_push(result, segment);
                        segment = rb_ary_new();
                    }
                    continue;
                }

                string stem((char*)sbstem, sbstem_len);
                if (RE2::FullMatch(stem,  *SkipTokenRE)) {
                    if (RARRAY_LEN(segment) > 0) {
                        rb_ary_push(result, segment);
                        segment = rb_ary_new();
                    }
                    continue;
                }
            }

            if (SkipTokenPatternRE && RE2::FullMatch(token, *SkipTokenPatternRE)) {
                if (RARRAY_LEN(segment) > 0) {
                    rb_ary_push(result, segment);
                    segment = rb_ary_new();
                }
                continue;
            }

            rb_ary_push(segment, rb_enc_str_new(token, strlen(token), encoding));
        }

        ptr = NULL;
        if (RARRAY_LEN(segment) > 0) {
            rb_ary_push(result, segment);
            segment = rb_ary_new();
        }
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
        rb_raise(rb_eArgError, "requires a list of screen names minus @");

    UserStopRE = new RE2("@" + build_alternating_expr(list), DefaultMatchOptions);
    if (!UserStopRE->ok())
        rb_raise(rb_eArgError, "%s", UserStopRE->error().c_str());

    return Qtrue;
}

VALUE skip_hashtags(VALUE self, VALUE list) {
    if (HashTagStopRE)
        delete HashTagStopRE;
    HashTagStopRE = NULL;

    if (NIL_P(list)) return Qtrue;

    if (TYPE(list) != T_ARRAY)
        rb_raise(rb_eArgError, "requires a list of hashtags minus #");

    HashTagStopRE = new RE2("#" + build_alternating_expr(list), DefaultMatchOptions);
    if (!HashTagStopRE->ok())
        rb_raise(rb_eArgError, "%s", HashTagStopRE->error().c_str());

    return Qtrue;
}

VALUE skip_tokens(VALUE self, VALUE list) {
    if (SkipTokenRE)
        delete SkipTokenRE;
    SkipTokenRE = NULL;

    if (NIL_P(list)) return Qtrue;

    if (TYPE(list) != T_ARRAY)
        rb_raise(rb_eArgError, "requires a list of words");

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
    SkipTokenRE = new RE2("^" + build_alternating_expr(list) + "$", DefaultMatchOptions);
    if (!SkipTokenRE->ok())
        rb_raise(rb_eArgError, "%s", SkipTokenRE->error().c_str());

    return Qtrue;
}

VALUE skip_token_pattern(VALUE self, VALUE re) {
    if (SkipTokenPatternRE)
        delete SkipTokenPatternRE;

    SkipTokenPatternRE = NULL;

    if (NIL_P(re)) return Qtrue;

    SkipTokenPatternRE = new RE2(CSTRING(re), DefaultMatchOptions);
    if (!SkipTokenPatternRE->ok())
        rb_raise(rb_eArgError, "%s", SkipTokenPatternRE->error().c_str());

    return Qtrue;
}


extern "C" {
    void Init_chipper(void) {
        ENStemmer          = sb_stemmer_new("english", "UTF_8");
        UserRE             = new RE2("(?:^|[^[:alnum:]])+([@＠][[:alnum:]_\\-]+)");
        HashTagRE          = new RE2("(?:^|[^[:alnum:]])+(#[[:alnum:]}_]+)");
        UrlRE              = new RE2("(https?://[[:alnum:]\\-_\\.:@]+\\.[[:alnum:]\\-_]+/?[^\\s\\r\\n]*)");
        UserStopRE         = NULL;
        HashTagStopRE      = NULL;
        SkipTokenRE        = NULL;
        SkipTokenPatternRE = NULL;

        DefaultMatchOptions.set_case_sensitive(false);
        DefaultMatchOptions.set_log_errors(false);

        id_users    = ID2SYM(rb_intern("users"));
        id_hashtags = ID2SYM(rb_intern("hashtags"));
        id_urls     = ID2SYM(rb_intern("urls"));

        rb_gc_mark(id_users);
        rb_gc_mark(id_hashtags);
        rb_gc_mark(id_urls);

        VALUE mTE = rb_define_module("Chipper");
        rb_define_module_function(mTE, "users",              RUBY_METHOD_FUNC(users), 1);
        rb_define_module_function(mTE, "hashtags",           RUBY_METHOD_FUNC(hashtags), 1);
        rb_define_module_function(mTE, "urls",               RUBY_METHOD_FUNC(urls), 1);
        rb_define_module_function(mTE, "tokens",             RUBY_METHOD_FUNC(tokens), 1);
        rb_define_module_function(mTE, "skip_users",         RUBY_METHOD_FUNC(skip_users), 1);
        rb_define_module_function(mTE, "skip_hashtags",      RUBY_METHOD_FUNC(skip_hashtags), 1);
        rb_define_module_function(mTE, "skip_tokens",        RUBY_METHOD_FUNC(skip_tokens), 1);
        rb_define_module_function(mTE, "skip_token_pattern", RUBY_METHOD_FUNC(skip_token_pattern), 1);
    }
}
