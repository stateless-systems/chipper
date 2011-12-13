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
VALUE id_users, id_hashtags, id_urls, id_tokens;

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

typedef struct List {
    char *text;
    struct List *next;
} List;

typedef struct DList {
    List *list;
    struct DList *next;
} DList;

void list_free(List *list) {
    List *curr = list;
    while (list) {
        list = curr->next;
        if (curr->text)
            free(curr->text);
        free(curr);
        curr = list;
    }
}

List* list_push(List *root, List *curr, const char *text, int size) {
    List *node = (List *)malloc(sizeof(List));
    if (!node) {
        list_free(root);
        return 0;
    }

    node->text = (char *)malloc(size + 1);
    if (!node->text) {
        free(node);
        list_free(root);
        return 0;
    }

    memcpy(node->text, text, size);

    node->next       = 0;
    node->text[size] = 0;

    if (curr)
        curr->next = node;

    return node;
}

VALUE list_to_array(List *node, rb_encoding *encoding) {
    List *next;
    VALUE array = rb_ary_new();

    while (node) {
        rb_ary_push(array, rb_enc_str_new(node->text, strlen(node->text), encoding));
        next = node->next;
        free(node->text);
        free(node);
        node = next;
    }

    return array;
}

void dlist_free(DList *dlist) {
    DList *curr = dlist;
    while (dlist) {
        dlist = curr->next;
        if (curr->list)
            list_free(curr->list);
        free(curr);
        curr = dlist;
    }
}

DList* dlist_push(DList *root, DList *curr, List *list) {
    DList *node = (DList *)malloc(sizeof(DList));
    if (!node) {
        dlist_free(root);
        list_free(list);
        return 0;
    }

    node->list = list;
    node->next = 0;

    if (curr)
        curr->next = node;

    return node;
}

VALUE dlist_to_array(DList *node, rb_encoding *encoding) {
    DList *next;
    VALUE array = rb_ary_new();

    while (node) {
        rb_ary_push(array, list_to_array(node->list, encoding));
        next = node->next;
        free(node);
        node = next;
    }

    return array;
}

List* tbr_users(VALUE text) {
    List *lroot = 0, *lcurr = 0, *lnode;

    string match;
    StringPiece input;
    input.set(RSTRING_PTR(text), RSTRING_LEN(text));
    while (RE2::FindAndConsume(&input, *UserRE, &match)) {
        if (UserStopRE && RE2::FullMatch(match, *UserStopRE)) continue;

        if (!(lnode = list_push(lroot, lcurr, match.data(), match.size())))
            rb_raise(rb_eNoMemError, "ran out of memory while storing result");

        if (lcurr)
            lcurr = lnode;
        else
            lroot = lcurr = lnode;
    }

    return lroot;
}


List* tbr_hashtags(VALUE text) {
    List *lroot = 0, *lcurr = 0, *lnode;

    string match;
    StringPiece input;
    input.set(RSTRING_PTR(text), RSTRING_LEN(text));
    while (RE2::FindAndConsume(&input, *HashTagRE, &match)) {
        if (match.size() < MIN_TAG_SIZE) continue;
        if (HashTagStopRE && RE2::FullMatch(match, *HashTagStopRE)) continue;

        if (!(lnode = list_push(lroot, lcurr, match.data(), match.size())))
            rb_raise(rb_eNoMemError, "ran out of memory while storing result");

        if (lcurr)
            lcurr = lnode;
        else
            lroot = lcurr = lnode;
    }

    return lroot;
}

List* tbr_urls(VALUE text) {
    List *lroot = 0, *lcurr = 0, *lnode;

    int size;
    string match;
    StringPiece input;
    input.set(RSTRING_PTR(text), RSTRING_LEN(text));
    while (RE2::FindAndConsume(&input, *UrlRE, &match)) {
        size = match.size();
        if (match.data()[size - 1] == '!') size--;
        if (match.data()[size - 1] == '.') size--;

        if (!(lnode = list_push(lroot, lcurr, match.data(), size)))
            rb_raise(rb_eNoMemError, "ran out of memory while storing result");

        if (lcurr)
            lcurr = lnode;
        else
            lroot = lcurr = lnode;
    }

    return lroot;
}

void inline dlist_add_segment(DList **dlroot, DList **dlcurr, List **lroot, List **lcurr, sb_stemmer *stemmer) {
    DList *dlnode = dlist_push(*dlroot, *dlcurr, *lroot);

    if (!dlnode) {
        sb_stemmer_delete(stemmer);
        rb_raise(rb_eNoMemError, "ran out of memory while storing result");
    }

    if (*dlcurr)
        *dlcurr = dlnode;
    else
        *dlroot = *dlcurr = dlnode;

    *lroot = *lcurr = 0;
}

DList* tbr_tokens(VALUE text) {
    static const char *phrase_delim = "\r\n:,;'\"{}()[]./\\%*|&!~`$+=<>?^";
    static const char *word_delim   = "\t- ";
    static const char *token_delim  = "_\t- ";

    DList *dlroot = 0, *dlcurr = 0;
    List *lroot   = 0, *lcurr  = 0, *lnode;

    char *token, *ptr, *buffer = (char*)calloc(RSTRING_LEN(text) + 1, 1), *phrase_ptr, *word_ptr, *token_ptr;

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
        ptr2 = phrase_ptr ? phrase_ptr : buffer + RSTRING_LEN(text);
        memset(ptr1, '\n', ptr2 - ptr1);
    }

    ptr2 = ptr;
    while ((ptr1 = STRSTR(ptr2, "https://"))) {
        ptr2 = strtok_r(ptr1, "\r\n\t ", &phrase_ptr);
        ptr2 = phrase_ptr ? phrase_ptr : buffer + RSTRING_LEN(text);
        memset(ptr1, '\n', ptr2 - ptr1);
    }

    // remove blank out single quotes, prime
    remove(ptr, "'");
    remove(ptr, "\u2019");
    remove(ptr, "\u2032");

    // segment at unicode quotes
    replace(ptr, "\u2018", '\t');
    replace(ptr, "\u201c", '\t');
    replace(ptr, "\u201d", '\t');
    replace(ptr, "\u201e", '\t');
    replace(ptr, "\u201f", '\t');
    replace(ptr, "\u2033", '\t');
    replace(ptr, "\u2034", '\t');
    replace(ptr, "\u2035", '\t');
    replace(ptr, "\u2036", '\t');
    replace(ptr, "\u2037", '\t');

    // angle quote
    replace(ptr, "\u2039", '<');
    replace(ptr, "\u203A", '>');

    // slash
    replace(ptr, "\u2044", '/');

    // fullwidth AT => @
    replace(ptr, "\uff20", '@');

    // unicode spaces
    replace(ptr, "\u2000", ' ');
    replace(ptr, "\u2001", ' ');
    replace(ptr, "\u2002", ' ');
    replace(ptr, "\u2003", ' ');
    replace(ptr, "\u2004", ' ');
    replace(ptr, "\u2005", ' ');
    replace(ptr, "\u2006", ' ');
    replace(ptr, "\u2007", ' ');
    replace(ptr, "\u2008", ' ');
    replace(ptr, "\u2009", ' ');
    replace(ptr, "\u200A", ' ');
    replace(ptr, "\u200B", ' ');
    replace(ptr, "\u202F", ' ');
    replace(ptr, "\u3000", ' ');

    // unicode dashes
    replace(ptr, "\u058A", '-');
    replace(ptr, "\u1806", '-');
    replace(ptr, "\u2010", '-');
    replace(ptr, "\u2011", '-');
    replace(ptr, "\u2012", '-');
    replace(ptr, "\u2013", '-');
    replace(ptr, "\u2014", '-');
    replace(ptr, "\u2015", '-');
    replace(ptr, "\u207B", '-');
    replace(ptr, "\u208B", '-');
    replace(ptr, "\u2212", '-');
    replace(ptr, "\u301C", '-');
    replace(ptr, "\u3030", '-');

    // corner brackets
    replace(ptr, "\u300C", '<');
    replace(ptr, "\u300E", '<');
    replace(ptr, "\u301D", '<');
    replace(ptr, "\u300D", '>');
    replace(ptr, "\u300F", '>');
    replace(ptr, "\u301F", '>');

    struct sb_stemmer *en_stemmer = sb_stemmer_new("english", "UTF_8");
    while ((token = strtok_r(ptr, phrase_delim, &phrase_ptr))) {
        ptr = token;

        while ((token = strtok_r(ptr, word_delim, &word_ptr))) {
            ptr = NULL;

            if (strlen(token) < MIN_WORD_SIZE || *token == '@' || *token == '#') {
                if (lroot)
                    dlist_add_segment(&dlroot, &dlcurr, &lroot, &lcurr, en_stemmer);
                continue;
            }

            ptr = token;
            while ((token = strtok_r(ptr, token_delim, &token_ptr))) {
                ptr = NULL;

                const sb_symbol *sbstem = sb_stemmer_stem(en_stemmer, (sb_symbol *)token, strlen(token));
                uint32_t sbstem_len     = sb_stemmer_length(en_stemmer);

                if (sbstem_len < MIN_WORD_SIZE) {
                    if (lroot)
                        dlist_add_segment(&dlroot, &dlcurr, &lroot, &lcurr, en_stemmer);
                    continue;
                }

                if (SkipTokenRE) {
                    if (RE2::FullMatch(token, *SkipTokenRE)) {
                        if (lroot)
                            dlist_add_segment(&dlroot, &dlcurr, &lroot, &lcurr, en_stemmer);
                        continue;
                    }

                    string stem((char*)sbstem, sbstem_len);
                    if (RE2::FullMatch(stem,  *SkipTokenRE)) {
                        if (lroot)
                            dlist_add_segment(&dlroot, &dlcurr, &lroot, &lcurr, en_stemmer);
                        continue;
                    }
                }

                if (SkipTokenPatternRE && RE2::FullMatch(token, *SkipTokenPatternRE)) {
                    if (lroot)
                        dlist_add_segment(&dlroot, &dlcurr, &lroot, &lcurr, en_stemmer);
                    continue;
                }

                if (!(lnode = list_push(lroot, lcurr, token, strlen(token)))) {
                    dlist_free(dlroot);
                    sb_stemmer_delete(en_stemmer);
                    rb_raise(rb_eNoMemError, "ran out of memory while storing result");
                }

                if (lcurr)
                    lcurr = lnode;
                else
                    lroot = lcurr = lnode;
            }

            ptr = NULL;
        }

        ptr = NULL;
        if (lroot)
            dlist_add_segment(&dlroot, &dlcurr, &lroot, &lcurr, en_stemmer);
    }

    sb_stemmer_delete(en_stemmer);
    free(buffer);
    return dlroot;
}

#define TBR_FUNC(a)         (VALUE (*)(void*))(a)
#define TBR_CALL(a, text)   rb_thread_blocking_region(TBR_FUNC(a), (void *)text, RUBY_UBF_PROCESS, 0)

// API

VALUE users(VALUE self, VALUE text, bool validated = false) {
    if (!validated && (NIL_P(text) || TYPE(text) != T_STRING))
        rb_raise(rb_eArgError, "requires tweet text");
    return list_to_array((List*)TBR_CALL(tbr_users, text), rb_enc_get(text));
}

VALUE hashtags(VALUE self, VALUE text, bool validated = false) {
    if (!validated && (NIL_P(text) || TYPE(text) != T_STRING))
        rb_raise(rb_eArgError, "requires tweet text");
    return list_to_array((List*)TBR_CALL(tbr_hashtags, text), rb_enc_get(text));
}

VALUE urls(VALUE self, VALUE text, bool validated = false) {
    if (!validated && (NIL_P(text) || TYPE(text) != T_STRING))
        rb_raise(rb_eArgError, "requires tweet text");
    return list_to_array((List*)TBR_CALL(tbr_urls, text), rb_enc_get(text));
}

VALUE tokens(VALUE self, VALUE text, bool validated = false) {
    if (!validated && (NIL_P(text) || TYPE(text) != T_STRING))
        rb_raise(rb_eArgError, "requires tweet text");
    return dlist_to_array((DList*)TBR_CALL(tbr_tokens, text), rb_enc_get(text));
}

VALUE entities(VALUE self, VALUE text) {
    if (NIL_P(text) || TYPE(text) != T_STRING)
        rb_raise(rb_eArgError, "requires tweet text");

    VALUE result = rb_hash_new();
    rb_hash_aset(result, id_users,    users(self, text, true));
    rb_hash_aset(result, id_hashtags, hashtags(self, text, true));
    rb_hash_aset(result, id_urls,     urls(self, text, true));
    rb_hash_aset(result, id_tokens,   tokens(self, text, true));
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

    struct sb_stemmer *en_stemmer = sb_stemmer_new("english", "UTF_8");

    // add stems as well
    int i, max = RARRAY_LEN(list);
    for (int i = 0; i < max; i++) {
        VALUE word              = rb_ary_entry(list, i);
        rb_encoding *encoding   = rb_enc_get(word);
        const sb_symbol *sbstem = sb_stemmer_stem(en_stemmer, (sb_symbol *)RSTRING_PTR(word), RSTRING_LEN(word));
        uint32_t sbstem_len     = sb_stemmer_length(en_stemmer);
        rb_ary_push(list, rb_enc_str_new((char*)sbstem, sbstem_len, encoding));
    }

    sb_stemmer_delete(en_stemmer);

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
        UserRE             = new RE2("(?:^|[^[:alnum:]])+([@＠][[:alnum:]_\\-]+)");
        HashTagRE          = new RE2("(?:^|[^[:alnum:]])+(#[[:alnum:]}_]+)");
     // UrlRE              = new RE2("(https?://[[:alnum:]\\-_\\.:@]+\\.[[:alnum:]\\-_]+/?[^\\s\\r\\n]*)");

     // TODO using the following t.co shortcut instead
        UrlRE              = new RE2("(https?://t.co/[A-Za-z0-9]+)");

        UserStopRE         = NULL;
        HashTagStopRE      = NULL;
        SkipTokenRE        = NULL;
        SkipTokenPatternRE = NULL;

        DefaultMatchOptions.set_case_sensitive(false);
        DefaultMatchOptions.set_log_errors(false);

        id_users    = ID2SYM(rb_intern("users"));
        id_hashtags = ID2SYM(rb_intern("hashtags"));
        id_urls     = ID2SYM(rb_intern("urls"));
        id_tokens   = ID2SYM(rb_intern("tokens"));

        rb_gc_mark(id_users);
        rb_gc_mark(id_hashtags);
        rb_gc_mark(id_urls);
        rb_gc_mark(id_tokens);

        VALUE mTE = rb_define_module("Chipper");
        rb_define_module_function(mTE, "users",              RUBY_METHOD_FUNC(users), 1);
        rb_define_module_function(mTE, "hashtags",           RUBY_METHOD_FUNC(hashtags), 1);
        rb_define_module_function(mTE, "urls",               RUBY_METHOD_FUNC(urls), 1);
        rb_define_module_function(mTE, "tokens",             RUBY_METHOD_FUNC(tokens), 1);
        rb_define_module_function(mTE, "entities",           RUBY_METHOD_FUNC(entities), 1);
        rb_define_module_function(mTE, "skip_users",         RUBY_METHOD_FUNC(skip_users), 1);
        rb_define_module_function(mTE, "skip_hashtags",      RUBY_METHOD_FUNC(skip_hashtags), 1);
        rb_define_module_function(mTE, "skip_tokens",        RUBY_METHOD_FUNC(skip_tokens), 1);
        rb_define_module_function(mTE, "skip_token_pattern", RUBY_METHOD_FUNC(skip_token_pattern), 1);
    }
}
