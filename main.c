#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#define INITIAL_CAP 128

static char *read_file_to_string(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc(sz + 1);
    if (!buf) { fclose(f); return NULL; }
    fread(buf, 1, sz, f);
    buf[sz] = '\0';
    fclose(f);
    return buf;
}

static char *trim(char *s) {
    if (!s) return s;
    while (*s && isspace((char)*s)) s++;
    if (*s == 0) return s;
    char *end = s + strlen(s) - 1;
    while (end > s && isspace((char)*end)) *end-- = '\0';
    return s;
}

typedef struct {
    char **arr;
    int len;
    int cap;
} StrVec;

static void vec_init(StrVec *v) {
    v->len = 0;
    v->cap = INITIAL_CAP;
    v->arr = malloc(v->cap * sizeof(char *));
}

static void vec_push(StrVec *v, char *s) {
    if (v->len == v->cap) {
        v->cap *= 2;
        v->arr = realloc(v->arr, v->cap * sizeof(char *));
    }
    v->arr[v->len++] = s;
}

static void vec_free(StrVec *v) {
    for (int i = 0; i < v->len; i++) free(v->arr[i]);
    free(v->arr);
}

static void load_wordlist(const char *path, StrVec *out, int *maxlen) {
    char *txt = read_file_to_string(path);
    if (!txt) { fprintf(stderr, "Failed to open %s\n", path); exit(1); }
    char *p = txt;
    *maxlen = 0;
    while (*p) {
        char *line = p;
        while (*p && *p != '\n' && *p != '\r') p++;
        char saved = *p;
        *p = '\0';
        char *t = trim(line);
        if (*t) {
            int L = (int)strlen(t);
            char *dup = malloc(L + 1);
            for (int i = 0; i <= L; i++) dup[i] = (char)tolower((char)t[i]);
            vec_push(out, dup);
            if (L > *maxlen) *maxlen = L;
        }
        if (saved == '\0') break;
        *p = saved;
        while (*p == '\r' || *p == '\n') p++;
    }
    free(txt);
}

static void write_freq_file(const char *path, StrVec *words, long *counts, int words_len) {
    FILE *f = fopen(path, "w");
    if (!f) { perror("fopen freq"); return; }
    for (int i = 0; i < words_len; i++) {
        if (counts[i] > 0) {
            fprintf(f, "%s : %ld\n", words->arr[i], counts[i]);
        }
    }
    fclose(f);
}

static int suffix_matches(const char *buf, int buflen, const char *phrase, int phlen) {
    if (phlen == 0) return 0;
    if (buflen < phlen) return 0;
    const char *start = buf + (buflen - phlen);
    for (int i = 0; i < phlen; i++) {
        if ((char)tolower((char)start[i]) != phrase[i]) return 0;
    }
    return 1;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("usage: %s <n>  (reads wordlist.txt in cwd, writes freq.txt)\n", argv[0]);
        return 1;
    }
    long n = atol(argv[1]);
    if (n <= 0) { fprintf(stderr, "n must be positive\n"); return 1; }

    StrVec words; vec_init(&words);
    int maxlen = 0;
    load_wordlist("wordlist.txt", &words, &maxlen);
    if (words.len == 0) {
        fprintf(stderr, "No words loaded from wordlist.txt\n");
        vec_free(&words);
        return 1;
    }

    long *counts = calloc(words.len, sizeof(long));
    if (!counts) { fprintf(stderr, "counts allocation failed\n"); return 1; }

    char *charset = NULL;
    int charset_len = 0;
    for (int c = 'A'; c <= 'Z'; c++) {
        charset = realloc(charset, charset_len + 2);
        charset[charset_len++] = (char)c;
    }
    for (int c = 'a'; c <= 'z'; c++) {
        charset = realloc(charset, charset_len + 2);
        charset[charset_len++] = (char)c;
    }
    int ranges[][2] = { {33,47}, {58,64}, {91,96}, {123,126} };
    for (int r = 0; r < 4; r++) {
        for (int c = ranges[r][0]; c <= ranges[r][1]; c++) {
            charset = realloc(charset, charset_len + 2);
            charset[charset_len++] = (char)c;
        }
    }
    if (!charset) { fprintf(stderr, "charset allocation failed\n"); free(counts); return 1; }
    charset[charset_len] = '\0';

    char *buf = malloc(maxlen + 1);
    if (!buf) { fprintf(stderr, "buf allocation failed\n"); free(charset); free(counts); return 1; }
    int buflen = 0;
    buf[0] = '\0';

    srand((unsigned)time(NULL));

    for (long i = 0; i < n; i++) {
        char ch = charset[rand() % charset_len];
        if (buflen < maxlen) {
            buf[buflen++] = (char)tolower((char)ch);
            buf[buflen] = '\0';
        } else {
            memmove(buf, buf + 1, maxlen - 1);
            buf[maxlen - 1] = (char)tolower((char)ch);
            buflen = maxlen;
            buf[buflen] = '\0';
        }
        for (int wi = 0; wi < words.len; wi++) {
            int phlen = (int)strlen(words.arr[wi]);
            if (suffix_matches(buf, buflen, words.arr[wi], phlen)) {
                counts[wi] += 1;
            }
        }
    }

    write_freq_file("freq.txt", &words, counts, words.len);

    long total_matches = 0;
    for (int i = 0; i < words.len; i++) total_matches += counts[i];

    printf("done. freq.txt updated. Generated %ld chars (letters+punctuation only).\n", n);
    printf("Total word matches: %ld\n", total_matches);

    free(buf);
    free(charset);
    free(counts);
    vec_free(&words);
    return 0;
}


