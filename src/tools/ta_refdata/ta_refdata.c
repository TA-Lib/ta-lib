/* ta_refdata: the reference run behind every function page on ta-lib.org.
 * One CSV per registered function, default parameters, over the committed
 * corpus (src/tools/ta_refdata/corpus); the generator charts the CSV.
 *
 *   ta_refdata [--check] <corpus-dir> <out-dir>
 *   ta_refdata --replay <csv>...
 *
 * A written file is self-contained: its input rows are exactly what the
 * writer passed to TA_CallFunc with startIdx 0, so --replay (and any port)
 * reproduces the output columns from the rows alone. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>

#include "ta_libc.h"
#include "codegen_tol.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#include <unistd.h>
#endif

#include "ta_refdata_domains.h"

#define WINDOW_BARS 252
#define PRICE_COLS 6                    /* date,open,high,low,close,volume */
#define MAXEXTRA 8                      /* derived input columns after them */
#define MAXOUT 8
#define MAXCOL (PRICE_COLS + MAXEXTRA + MAXOUT)
#define TIGHT_TOL 1e-12
#define CORPUS_REF "ibm_1999.csv"

extern TA_Real TA_SREF_open_daily_ref_0_PRIV[];
extern TA_Real TA_SREF_high_daily_ref_0_PRIV[];
extern TA_Real TA_SREF_low_daily_ref_0_PRIV[];
extern TA_Real TA_SREF_close_daily_ref_0_PRIV[];
extern TA_Real TA_SREF_volume_daily_ref_0_PRIV[];

typedef struct {
    char name[256];
    char label[256];                    /* `# label:` line of the file, else its name */
    int synthetic;                      /* `# synthetic:` line present */
    int n;
    char (*date)[11];
    double *o, *h, *l, *c, *v;
} Series;

static Series *corpus;
static int nCorpus, refSeries = -1, maxRows;
static int checkMode;
static int findings;                    /* --check: anything that fails it */
static int nWritten, nUpToDate, nStale;
static char **funcNames; static int nFuncNames;

static void die(const char *fmt, ...)
#if defined(__GNUC__)
    __attribute__((format(printf, 1, 2), noreturn))
#endif
;
static void die(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fputs("ta_refdata: ", stderr);
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
    va_end(ap);
    exit(1);
}

static void *xmalloc(size_t n)
{
    void *p = malloc(n ? n : 1);
    if (!p) die("out of memory");
    return p;
}

static char *xstrdup(const char *s)
{
    char *p = xmalloc(strlen(s) + 1);
    strcpy(p, s);
    return p;
}

/* ---- text buffer ---- */
typedef struct { char *s; size_t len, cap; } Buf;

static void buf_add(Buf *b, const char *s)
{
    size_t n = strlen(s);
    if (b->len + n + 1 > b->cap) {
        b->cap = (b->len + n + 1) * 2 + 4096;
        b->s = realloc(b->s, b->cap);
        if (!b->s) die("out of memory");
    }
    memcpy(b->s + b->len, s, n + 1);
    b->len += n;
}

static void buf_addf(Buf *b, const char *fmt, ...)
{
    char tmp[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(tmp, sizeof tmp, fmt, ap);
    va_end(ap);
    buf_add(b, tmp);
}

/* Shortest decimal that strtod reads back to the same bits. */
static const char *fmt_real(char *out, size_t n, double v)
{
    int prec;
    for (prec = 15; prec <= 17; prec++) {
        double back;
        snprintf(out, n, "%.*g", prec, v);
        back = strtod(out, NULL);
        if (memcmp(&back, &v, sizeof v) == 0) return out;
    }
    return out;
}

static char *read_file(const char *path, size_t *len)
{
    FILE *f = fopen(path, "rb");
    char *s;
    long sz;
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    s = xmalloc((size_t)sz + 1);
    if (sz > 0 && fread(s, 1, (size_t)sz, f) != (size_t)sz) die("%s: read error", path);
    fclose(f);
    s[sz] = 0;
    if (len) *len = (size_t)sz;
    return s;
}

static int has_csv_suffix(const char *name)
{
    size_t n = strlen(name);
    return n > 4 && strcmp(name + n - 4, ".csv") == 0;
}

static int cmp_str(const void *a, const void *b)
{
    return strcmp(*(char *const *)a, *(char *const *)b);
}

/* The `*.csv` names of a directory, sorted. */
static char **list_csv(const char *dir, int *count)
{
    char **names = NULL;
    int n = 0, cap = 0;
#ifdef _WIN32
    char pat[1024];
    WIN32_FIND_DATAA fd;
    HANDLE hf;
    snprintf(pat, sizeof pat, "%s\\*.csv", dir);
    hf = FindFirstFileA(pat, &fd);
    if (hf == INVALID_HANDLE_VALUE) {
        DWORD e = GetLastError();
        if (e == ERROR_FILE_NOT_FOUND || e == ERROR_NO_MORE_FILES) { *count = 0; return NULL; }
        die("%s: cannot list directory (error %lu)", dir, (unsigned long)e);
    }
    do {
        if (!has_csv_suffix(fd.cFileName)) continue;
        if (n == cap) { cap = cap * 2 + 16; names = realloc(names, (size_t)cap * sizeof *names); if (!names) die("out of memory"); }
        names[n++] = xstrdup(fd.cFileName);
    } while (FindNextFileA(hf, &fd));
    FindClose(hf);
#else
    DIR *d = opendir(dir);
    struct dirent *e;
    if (!d) die("%s: %s", dir, strerror(errno));
    while ((e = readdir(d)) != NULL) {
        if (!has_csv_suffix(e->d_name)) continue;
        if (n == cap) { cap = cap * 2 + 16; names = realloc(names, (size_t)cap * sizeof *names); if (!names) die("out of memory"); }
        names[n++] = xstrdup(e->d_name);
    }
    closedir(d);
#endif
    if (n) qsort(names, (size_t)n, sizeof *names, cmp_str);
    *count = n;
    return names;
}

/* ---- a CSV in memory: `#` lines, the column header, then rows of cells ---- */
typedef struct {
    int nHeader; char **header;
    char *colHeader;
    int nCols; char *cols[MAXCOL];
    int nRows; char ***cells;           /* cells[row][col], "" for an empty cell */
} Table;

static int split_cells(char *line, char **cells, int max)
{
    int n = 0;
    char *p = line;
    for (;;) {
        char *comma = strchr(p, ',');
        if (n == max) return -1;
        cells[n++] = p;
        if (!comma) break;
        *comma = 0;
        p = comma + 1;
    }
    return n;
}

static int parse_table(const char *text, Table *t, char *err, size_t errn)
{
    char *copy = xstrdup(text), *line, *next;
    int cap = 0;
    memset(t, 0, sizeof *t);
    for (line = copy; line && *line; line = next) {
        size_t n;
        next = strchr(line, '\n');
        if (next) { *next = 0; next++; }
        n = strlen(line);
        if (n && line[n - 1] == '\r') line[--n] = 0;
        if (!t->colHeader) {
            if (line[0] == '#') {
                t->header = realloc(t->header, (size_t)(t->nHeader + 1) * sizeof *t->header);
                if (!t->header) die("out of memory");
                t->header[t->nHeader++] = line;
                continue;
            }
            t->colHeader = xstrdup(line);
            t->nCols = split_cells(t->colHeader, t->cols, MAXCOL);
            if (t->nCols < PRICE_COLS || strcmp(t->cols[0], "date") != 0) {
                snprintf(err, errn, "bad column header");
                return 0;
            }
            continue;
        }
        if (t->nRows == cap) {
            cap = cap * 2 + 64;
            t->cells = realloc(t->cells, (size_t)cap * sizeof *t->cells);
            if (!t->cells) die("out of memory");
        }
        t->cells[t->nRows] = xmalloc(sizeof(char *) * MAXCOL);
        if (split_cells(line, t->cells[t->nRows], MAXCOL) != t->nCols) {
            snprintf(err, errn, "row %d: wrong cell count", t->nRows + 1);
            return 0;
        }
        t->nRows++;
    }
    if (!t->colHeader) { snprintf(err, errn, "no column header"); return 0; }
    return 1;
}

static const char *table_meta(const Table *t, const char *key)
{
    size_t kl = strlen(key);
    int i;
    for (i = 0; i < t->nHeader; i++) {
        const char *h = t->header[i];
        if (strncmp(h, "# ", 2) == 0 && strncmp(h + 2, key, kl) == 0 && h[2 + kl] == ':')
            return h[3 + kl] == ' ' ? h + 4 + kl : h + 3 + kl;
    }
    return NULL;
}

/* ---- corpus ---- */
static Series series_from_rows(const char *name, int nRows, char ***cells)
{
    Series s;
    int i;
    memset(&s, 0, sizeof s);
    snprintf(s.name, sizeof s.name, "%s", name);
    snprintf(s.label, sizeof s.label, "%s", name);
    s.n = nRows;
    s.date = xmalloc(sizeof *s.date * (size_t)(nRows ? nRows : 1));
    s.o = xmalloc(sizeof(double) * (size_t)(nRows ? nRows : 1));
    s.h = xmalloc(sizeof(double) * (size_t)(nRows ? nRows : 1));
    s.l = xmalloc(sizeof(double) * (size_t)(nRows ? nRows : 1));
    s.c = xmalloc(sizeof(double) * (size_t)(nRows ? nRows : 1));
    s.v = xmalloc(sizeof(double) * (size_t)(nRows ? nRows : 1));
    for (i = 0; i < nRows; i++) {
        char *end;
        if (strlen(cells[i][0]) != 10 || cells[i][0][4] != '-' || cells[i][0][7] != '-')
            die("%s row %d: date `%s` is not YYYY-MM-DD", name, i + 1, cells[i][0]);
        if (i > 0 && strcmp(cells[i][0], cells[i - 1][0]) <= 0)
            die("%s row %d: dates must increase", name, i + 1);
        memcpy(s.date[i], cells[i][0], 11);
        s.o[i] = strtod(cells[i][1], &end); if (*end) die("%s row %d: bad open", name, i + 1);
        s.h[i] = strtod(cells[i][2], &end); if (*end) die("%s row %d: bad high", name, i + 1);
        s.l[i] = strtod(cells[i][3], &end); if (*end) die("%s row %d: bad low", name, i + 1);
        s.c[i] = strtod(cells[i][4], &end); if (*end) die("%s row %d: bad close", name, i + 1);
        s.v[i] = strtod(cells[i][5], &end); if (*end) die("%s row %d: bad volume", name, i + 1);
    }
    return s;
}

static void load_corpus(const char *dir)
{
    int count, i;
    char **names = list_csv(dir, &count);
    if (count == 0) die("%s: no *.csv corpus file", dir);
    corpus = xmalloc(sizeof *corpus * (size_t)count);
    for (i = 0; i < count; i++) {
        char path[1024], err[128];
        Table t;
        char *text;
        snprintf(path, sizeof path, "%s/%s", dir, names[i]);
        text = read_file(path, NULL);
        if (!text) die("%s: %s", path, strerror(errno));
        if (!parse_table(text, &t, err, sizeof err)) die("%s: %s", path, err);
        if (t.nCols != PRICE_COLS || strcmp(t.colHeader, "date") != 0 ||
            strcmp(t.cols[1], "open") || strcmp(t.cols[2], "high") || strcmp(t.cols[3], "low") ||
            strcmp(t.cols[4], "close") || strcmp(t.cols[5], "volume"))
            die("%s: columns must be date,open,high,low,close,volume", path);
        corpus[i] = series_from_rows(names[i], t.nRows, t.cells);
        snprintf(corpus[i].label, sizeof corpus[i].label, "%s",
                 table_meta(&t, "label") ? table_meta(&t, "label") : names[i]);
        corpus[i].synthetic = table_meta(&t, "synthetic") != NULL;
        if (corpus[i].n > maxRows) maxRows = corpus[i].n;
        if (strcmp(names[i], CORPUS_REF) == 0) refSeries = i;
    }
    nCorpus = count;
    if (refSeries < 0) die("%s: %s missing", dir, CORPUS_REF);
    {
        const Series *r = &corpus[refSeries];
        if (r->n != 252) die("%s: %d bars, test_data.c has 252", CORPUS_REF, r->n);
        for (i = 0; i < 252; i++)
            if (r->o[i] != TA_SREF_open_daily_ref_0_PRIV[i] || r->h[i] != TA_SREF_high_daily_ref_0_PRIV[i] ||
                r->l[i] != TA_SREF_low_daily_ref_0_PRIV[i] || r->c[i] != TA_SREF_close_daily_ref_0_PRIV[i] ||
                r->v[i] != TA_SREF_volume_daily_ref_0_PRIV[i])
                die("%s row %d (%s) differs from ta_regtest's TA_SREF_*_daily_ref_0_PRIV arrays",
                    CORPUS_REF, i + 1, r->date[i]);
    }
}

/* ---- one run: a function over `rows` bars of a series from `first` ---- */
typedef struct {
    const TA_FuncInfo *fi;
    TA_ParamHolder *h;
    const Series *s;
    int first, rows;
    int nExtra;                         /* derived inputs: one CSV column each, after the prices */
    const char *extraName[MAXEXTRA];
    double *extra[MAXEXTRA];
    double *rout[MAXOUT];
    int *iout[MAXOUT];
    int isInt[MAXOUT], isPattern[MAXOUT];
    int lookback, beg, nb;
    char inputs[512];
} Run;

/* The input's synthetic series ('r' ramp, 's' sine) over [lo, hi], or 0 for a price column. */
static char series_input(const TA_FuncInfo *fi, const char *input, double *lo, double *hi)
{
    size_t i;
    for (i = 0; REFDATA_DOMAIN[i].func; i++)
        if (strcmp(fi->name, REFDATA_DOMAIN[i].func) == 0 && strcmp(input, REFDATA_DOMAIN[i].input) == 0) {
            *lo = REFDATA_DOMAIN[i].lo; *hi = REFDATA_DOMAIN[i].hi;
            return REFDATA_DOMAIN[i].series;
        }
    return 0;
}

static void run_alloc(Run *r, const TA_FuncInfo *fi, int rowsCap)
{
    unsigned i;
    memset(r, 0, sizeof *r);
    r->fi = fi;
    if (fi->nbOutput > MAXOUT) die("%s: %u outputs, MAXOUT is %d", fi->name, fi->nbOutput, MAXOUT);
    if (TA_ParamHolderAlloc(fi->handle, &r->h) != TA_SUCCESS) die("%s: TA_ParamHolderAlloc", fi->name);
    for (i = 0; i < MAXEXTRA; i++) r->extra[i] = xmalloc(sizeof(double) * (size_t)rowsCap);
    for (i = 0; i < fi->nbOutput; i++) {
        const TA_OutputParameterInfo *oo;
        TA_GetOutputParameterInfo(fi->handle, i, &oo);
        r->isInt[i] = oo->type == TA_Output_Integer;
        r->isPattern[i] = (oo->flags & (TA_OUT_PATTERN_BOOL | TA_OUT_PATTERN_BULL_BEAR |
                                        TA_OUT_PATTERN_CONFIRM | TA_OUT_PATTERN_WEAK)) != 0;
        if (r->isInt[i]) {
            r->iout[i] = xmalloc(sizeof(int) * (size_t)rowsCap);
            TA_SetOutputParamIntegerPtr(r->h, i, r->iout[i]);
        } else {
            r->rout[i] = xmalloc(sizeof(double) * (size_t)rowsCap);
            TA_SetOutputParamRealPtr(r->h, i, r->rout[i]);
        }
    }
    if (TA_GetLookback(r->h, &r->lookback) != TA_SUCCESS || r->lookback < 0)
        die("%s: TA_GetLookback", fi->name);
}

static void run_free(Run *r)
{
    unsigned i;
    TA_ParamHolderFree(r->h);
    for (i = 0; i < MAXEXTRA; i++) free(r->extra[i]);
    for (i = 0; i < r->fi->nbOutput; i++) { free(r->rout[i]); free(r->iout[i]); }
}

/* A derived input's column of `t`, by the input's name, parsed into `dst`. */
static void column_from_table(const Table *t, const char *name, double *dst, const char *func)
{
    int j, i;
    for (j = PRICE_COLS; j < t->nCols; j++)
        if (strcmp(t->cols[j], name) == 0) break;
    if (j == t->nCols) die("%s: no `%s` column to replay from", func, name);
    for (i = 0; i < t->nRows; i++) {
        char *end;
        dst[i] = strtod(t->cells[i][j], &end);
        if (end == t->cells[i][j] || *end) die("%s row %d: bad %s", func, i + 1, name);
    }
}

/* Binds the inputs over s[first .. first+rows) and calls the function with
 * startIdx 0: what a reader of the written rows can reproduce. A derived input
 * (not a plain price column) is synthesised over the rows, with the formula the
 * generated examples use, or, under --replay, read from `fromFile`'s column of
 * its name. */
static TA_RetCode run_call(Run *r, const Series *s, int first, int rows, const Table *fromFile)
{
    const TA_FuncInfo *fi = r->fi;
    unsigned i;
    int realIdx = 0, b;
    double lo, hi;
    char kind;
    const double *o = s->o + first, *h = s->h + first, *l = s->l + first, *c = s->c + first, *v = s->v + first;
    r->s = s; r->first = first; r->rows = rows;
    r->inputs[0] = 0;
    r->nExtra = 0;
    for (i = 0; i < fi->nbInput; i++) {
        const TA_InputParameterInfo *ii;
        char item[128];
        TA_GetInputParameterInfo(fi->handle, i, &ii);
        if (ii->type == TA_Input_Price) {
            if (ii->flags & ~(TA_IN_PRICE_OPEN | TA_IN_PRICE_HIGH | TA_IN_PRICE_LOW | TA_IN_PRICE_CLOSE | TA_IN_PRICE_VOLUME))
                die("%s: %s needs a price component the corpus has no column for", fi->name, ii->paramName);
            TA_SetInputParamPricePtr(r->h, i,
                ii->flags & TA_IN_PRICE_OPEN ? o : NULL, ii->flags & TA_IN_PRICE_HIGH ? h : NULL,
                ii->flags & TA_IN_PRICE_LOW ? l : NULL, ii->flags & TA_IN_PRICE_CLOSE ? c : NULL,
                ii->flags & TA_IN_PRICE_VOLUME ? v : NULL, NULL);
            snprintf(item, sizeof item, "%s%s=%s%s%s%s%s", i ? " " : "", ii->paramName,
                     ii->flags & TA_IN_PRICE_OPEN ? "O" : "", ii->flags & TA_IN_PRICE_HIGH ? "H" : "",
                     ii->flags & TA_IN_PRICE_LOW ? "L" : "", ii->flags & TA_IN_PRICE_CLOSE ? "C" : "",
                     ii->flags & TA_IN_PRICE_VOLUME ? "V" : "");
        } else if (ii->type == TA_Input_Real) {
            const double *src;
            const char *desc;
            double *derived = NULL;
            if (strcmp(ii->paramName, "inPeriods") == 0) {
                derived = r->extra[r->nExtra];
                for (b = 0; b < rows; b++) derived[b] = (b < rows / 2) ? 5.0 : 20.0;
                src = derived; desc = "5(first-half)|20(second-half)";
            } else if ((kind = series_input(fi, ii->paramName, &lo, &hi)) != 0) {
                char lob[32], hib[32];
                static char descbuf[MAXEXTRA][96];
                derived = r->extra[r->nExtra];
                if (kind == 'r')
                    for (b = 0; b < rows; b++) derived[b] = rows > 1 ? lo + (hi - lo) * b / (rows - 1) : lo;
                else
                    for (b = 0; b < rows; b++) derived[b] = (lo + hi) / 2 + (hi - lo) / 2 * sin(0.1 * b);
                src = derived;
                snprintf(descbuf[r->nExtra], sizeof descbuf[0], "%s[%s,%s]", kind == 'r' ? "ramp" : "sine",
                         fmt_real(lob, sizeof lob, lo), fmt_real(hib, sizeof hib, hi));
                desc = descbuf[r->nExtra];
            } else if (realIdx == 0 && strcmp(ii->paramName, "inReal0") == 0) {
                src = h; desc = "high";
            } else if (realIdx == 0) {
                src = c; desc = "close";
            } else {
                src = l; desc = "low";
            }
            if (derived) {
                if (r->nExtra == MAXEXTRA) die("%s: more than %d derived inputs", fi->name, MAXEXTRA);
                if (fromFile) column_from_table(fromFile, ii->paramName, derived, fi->name);
                r->extraName[r->nExtra++] = ii->paramName;
            }
            realIdx++;
            TA_SetInputParamRealPtr(r->h, i, src);
            snprintf(item, sizeof item, "%s%s=%s", i ? " " : "", ii->paramName, desc);
        } else {
            die("%s: %s is an integer input, which the corpus cannot feed", fi->name, ii->paramName);
        }
        strcat(r->inputs, item);
    }
    r->beg = r->nb = 0;
    if (rows <= 0) return TA_SUCCESS;
    return TA_CallFunc(r->h, 0, rows - 1, &r->beg, &r->nb);
}

static int run_is_pattern(const Run *r)
{
    unsigned i;
    if (r->fi->flags & TA_FUNC_FLG_CANDLESTICK) return 1;
    for (i = 0; i < r->fi->nbOutput; i++) if (r->isPattern[i]) return 1;
    return 0;
}

/* Bar `b` (a row index) has a non-zero pattern output. */
static int run_flagged(const Run *r, int b)
{
    unsigned i;
    int k = b - r->beg;
    if (k < 0 || k >= r->nb) return 0;
    for (i = 0; i < r->fi->nbOutput; i++) {
        if (!r->isPattern[i] && !(r->fi->flags & TA_FUNC_FLG_CANDLESTICK)) continue;
        if (r->isInt[i] ? r->iout[i][k] != 0 : r->rout[i][k] != 0.0) return 1;
    }
    return 0;
}

static int run_is_transcendental(const Run *r)
{
    unsigned i;
    if (r->fi->flags & TA_FUNC_FLG_USES_TRANSCENDENTAL) return 1;
    for (i = 0; i < r->fi->nbOptInput; i++) {
        const TA_OptInputParameterInfo *oi;
        TA_GetOptInputParameterInfo(r->fi->handle, i, &oi);
        if (oi->paramName && strstr(oi->paramName, "MAType") &&
            ((int)oi->defaultValue == TA_MAType_MAMA || (int)oi->defaultValue == TA_MAType_ALMA))
            return 1;
    }
    return 0;
}

static void lower_name(char *out, size_t n, const char *name)
{
    size_t i;
    for (i = 0; i + 1 < n && name[i]; i++) out[i] = (char)tolower((unsigned char)name[i]);
    out[i] = 0;
}

/* ---- the written text ---- */
static void render(const Run *r, Buf *b, int flaggedBars)
{
    const TA_FuncInfo *fi = r->fi;
    unsigned i;
    int row, e;
    char num[32];
    const int lb = r->lookback;
    const int outRows = r->rows > lb ? r->rows - lb : 0;
    buf_addf(b, "# TA-Lib reference data: %s\n# series: %s\n", fi->name, r->s->label);
    buf_addf(b, "# input: %s; window %s..%s (%d bars)", r->s->name,
             outRows ? r->s->date[r->first + lb] : "-", outRows ? r->s->date[r->first + r->rows - 1] : "-", outRows);
    if (lb) buf_addf(b, " plus %d lookback bar%s before it", lb, lb == 1 ? "" : "s");
    if (run_is_pattern(r)) buf_addf(b, "; flagged bars: %d", flaggedBars);
    buf_add(b, "\n# parameters:");
    for (i = 0; i < fi->nbOptInput; i++) {
        const TA_OptInputParameterInfo *oi;
        TA_GetOptInputParameterInfo(fi->handle, i, &oi);
        if (oi->type == TA_OptInput_IntegerRange || oi->type == TA_OptInput_IntegerList)
            buf_addf(b, " %s=%d", oi->paramName, (int)oi->defaultValue);
        else
            buf_addf(b, " %s=%s", oi->paramName, fmt_real(num, sizeof num, oi->defaultValue));
    }
    buf_add(b, " (defaults)\n");
    buf_addf(b, "# inputs: %s\n", r->inputs);
    buf_add(b, "# outputs:");
    for (i = 0; i < fi->nbOutput; i++) {
        const TA_OutputParameterInfo *oo;
        int sh = 0;
        TA_GetOutputParameterInfo(fi->handle, i, &oo);
        TA_GetDisplayShift(r->h, i, &sh);
        buf_addf(b, " %s (shift %d)", oo->paramName, sh);
    }
    buf_addf(b, "\n# lookback: %d\n", lb);
    buf_add(b, "date,open,high,low,close,volume");
    for (e = 0; e < r->nExtra; e++) buf_addf(b, ",%s", r->extraName[e]);
    for (i = 0; i < fi->nbOutput; i++) {
        const TA_OutputParameterInfo *oo;
        TA_GetOutputParameterInfo(fi->handle, i, &oo);
        buf_addf(b, ",%s", oo->paramName);
    }
    buf_add(b, "\n");
    for (row = 0; row < r->rows; row++) {
        int a = r->first + row, k = row - r->beg;
        buf_addf(b, "%s,%s", r->s->date[a], fmt_real(num, sizeof num, r->s->o[a]));
        buf_addf(b, ",%s", fmt_real(num, sizeof num, r->s->h[a]));
        buf_addf(b, ",%s", fmt_real(num, sizeof num, r->s->l[a]));
        buf_addf(b, ",%s", fmt_real(num, sizeof num, r->s->c[a]));
        buf_addf(b, ",%s", fmt_real(num, sizeof num, r->s->v[a]));
        for (e = 0; e < r->nExtra; e++) buf_addf(b, ",%s", fmt_real(num, sizeof num, r->extra[e][row]));
        for (i = 0; i < fi->nbOutput; i++) {
            if (k < 0 || k >= r->nb) buf_add(b, ",");
            else if (r->isInt[i]) buf_addf(b, ",%d", r->iout[i][k]);
            else buf_addf(b, ",%s", fmt_real(num, sizeof num, r->rout[i][k]));
        }
        buf_add(b, "\n");
    }
}

/* ---- compare a committed file to the computed text ---- */
static int same_real(double a, double b, double tol)
{
    if (isnan(a) || isnan(b)) return isnan(a) && isnan(b);
    if (isinf(a) || isinf(b)) return a == b;
    return fabs(a - b) <= tol * fmax(1.0, fabs(a));
}

/* Findings in `committed` against `computed`: the header must match textually,
 * the input cells by value, the output cells (from column `firstOut`) by the
 * rule. Every finding is printed under --check; the plain run prints the first
 * and rewrites. */
static int compare_tables(const char *func, const Table *committed, const Table *computed,
                          int firstOut, const int *isInt, double tol, int reportAll)
{
    int i, j, n = 0;
    char err[256];
    if (committed->nHeader != computed->nHeader) {
        snprintf(err, sizeof err, "%d header lines, %d expected", committed->nHeader, computed->nHeader);
        printf("  %s: %s\n", func, err);
        return 1;
    }
    for (i = 0; i < committed->nHeader; i++)
        if (strcmp(committed->header[i], computed->header[i]) != 0) {
            printf("  %s: header line %d differs\n    committed: %s\n    computed:  %s\n", func, i + 1,
                   committed->header[i], computed->header[i]);
            if (!reportAll) return 1;
            n++;
        }
    if (strcmp(committed->colHeader, computed->colHeader) != 0 || committed->nCols != computed->nCols) {
        printf("  %s: column header differs\n", func);
        return n + 1;
    }
    if (committed->nRows != computed->nRows) {
        printf("  %s: %d rows, %d expected\n", func, committed->nRows, computed->nRows);
        return n + 1;
    }
    for (i = 0; i < committed->nRows; i++) {
        char **a = committed->cells[i], **b = computed->cells[i];
        for (j = 0; j < committed->nCols; j++) {
            int bad = 0;
            if (j == 0) bad = strcmp(a[0], b[0]) != 0;
            else if (!*a[j] || !*b[j]) bad = (*a[j] != 0) != (*b[j] != 0);
            else if (j < firstOut) bad = strtod(a[j], NULL) != strtod(b[j], NULL);
            else if (isInt[j - firstOut]) bad = strtol(a[j], NULL, 10) != strtol(b[j], NULL, 10);
            else
                bad = !same_real(strtod(a[j], NULL), strtod(b[j], NULL), tol);
            if (bad) {
                printf("  %s row %s %s: committed=%s computed=%s\n", func, a[0], committed->cols[j],
                       *a[j] ? a[j] : "(empty)", *b[j] ? b[j] : "(empty)");
                n++;
                if (!reportAll) return n;
            }
        }
    }
    return n;
}

static void free_table(Table *t)
{
    int i;
    free(t->header);
    free(t->colHeader);
    for (i = 0; i < t->nRows; i++) free(t->cells[i]);
    free(t->cells);
}

/* ---- per function ---- */
typedef struct { const char *outdir; } Ctx;

static void one_function(const TA_FuncInfo *fi, void *opaque)
{
    const Ctx *ctx = opaque;
    Run r;
    const Series *best = &corpus[refSeries];
    int bestFirst = 0, bestRows = best->n, bestCount = -1, k, flagged = 0;
    char lower[64], path[1024];
    Buf text = { NULL, 0, 0 };
    Table computed, committed;
    char err[128];
    char *old;
    TA_RetCode rc;

    funcNames = realloc(funcNames, (size_t)(nFuncNames + 1) * sizeof *funcNames);
    if (!funcNames) die("out of memory");
    lower_name(lower, sizeof lower, fi->name);
    funcNames[nFuncNames++] = xstrdup(lower);

    run_alloc(&r, fi, maxRows);
    if (run_is_pattern(&r)) {
        /* Every full window of lookback + WINDOW_BARS bars, in every file, scored
         * by its flagged bars; the default window is always a candidate so one
         * 252-bar file still has one. Ties: count desc, file asc, start asc.
         * A synthetic file is a candidate only when no market file fires. */
        const int lb = r.lookback;
        int f, pass;
        bestCount = 0;
        rc = run_call(&r, best, 0, best->n, NULL);
        if (rc != TA_SUCCESS) die("%s over %s: TA_CallFunc returned %d", fi->name, best->name, (int)rc);
        for (k = 0; k < best->n; k++) bestCount += run_flagged(&r, k);
        for (pass = 0; pass < 2 && (pass == 0 || bestCount == 0); pass++)
        for (f = 0; f < nCorpus; f++) {
            const Series *s = &corpus[f];
            int start, count = 0;
            if (s->synthetic != pass || s->n < lb + WINDOW_BARS) continue;
            rc = run_call(&r, s, 0, s->n, NULL);
            if (rc != TA_SUCCESS) die("%s over %s: TA_CallFunc returned %d", fi->name, s->name, (int)rc);
            for (k = lb; k < lb + WINDOW_BARS; k++) count += run_flagged(&r, k);
            for (start = lb; start + WINDOW_BARS <= s->n; start++) {
                int rows = lb + WINDOW_BARS, first = start - lb;
                int isDefault = s == &corpus[refSeries] && first == 0 && rows == s->n;
                if (start > lb) count += run_flagged(&r, start + WINDOW_BARS - 1) - run_flagged(&r, start - 1);
                if (isDefault) continue;
                if (count > bestCount ||
                    (count == bestCount && (strcmp(s->name, best->name) < 0 ||
                     (s == best && strcmp(s->date[start], best->date[bestFirst + lb]) < 0)))) {
                    bestCount = count; best = s; bestFirst = first; bestRows = rows;
                }
            }
        }
        if (bestCount == 0) { best = &corpus[refSeries]; bestFirst = 0; bestRows = best->n; }
    }

    rc = run_call(&r, best, bestFirst, bestRows, NULL);
    if (rc != TA_SUCCESS) die("%s over %s: TA_CallFunc returned %d", fi->name, best->name, (int)rc);
    if (r.nb <= 0)
        die("%s: lookback %d leaves no output bar over %d rows", fi->name, r.lookback, r.rows);
    if (r.beg != r.lookback || r.beg + r.nb != r.rows)
        die("%s: outBegIdx %d / outNBElement %d do not match lookback %d over %d rows",
            fi->name, r.beg, r.nb, r.lookback, r.rows);
    for (k = 0; k < r.rows; k++) flagged += run_flagged(&r, k);
    render(&r, &text, flagged);
    if (!parse_table(text.s, &computed, err, sizeof err)) die("%s: rendered text: %s", fi->name, err);

    snprintf(path, sizeof path, "%s/%s.csv", ctx->outdir, lower);
    old = read_file(path, NULL);
    if (!old) {
        printf("  %s: missing\n", fi->name);
        findings++;
    } else if (!parse_table(old, &committed, err, sizeof err)) {
        printf("  %s: unreadable (%s)\n", fi->name, err);
        findings++;
    } else {
        int bad = compare_tables(fi->name, &committed, &computed, PRICE_COLS + r.nExtra, r.isInt,
                                 run_is_transcendental(&r) ? CODEGEN_TRANSCENDENTAL_TOL : TIGHT_TOL, checkMode);
        free_table(&committed);
        if (!bad) { nUpToDate++; goto done; }
        findings += bad;
        if (!checkMode) printf("  %s: out of tolerance\n", fi->name);
    }
    if (!checkMode) {
        FILE *f = fopen(path, "wb");
        if (!f || fwrite(text.s, 1, text.len, f) != text.len || fclose(f)) die("%s: %s", path, strerror(errno));
        printf("  %s: written %s\n", fi->name, path);
        nWritten++;
    }
done:
    free(old);
    free_table(&computed);
    free(text.s);
    run_free(&r);
}

static void sweep_stale(const char *outdir)
{
    int count, i, j;
    char **names = list_csv(outdir, &count);
    for (i = 0; i < count; i++) {
        int known = 0;
        char base[256];
        snprintf(base, sizeof base, "%s", names[i]);
        base[strlen(base) - 4] = 0;
        for (j = 0; j < nFuncNames && !known; j++) known = strcmp(base, funcNames[j]) == 0;
        if (known) continue;
        nStale++;
        if (checkMode) {
            printf("  stale: %s/%s names no registered function\n", outdir, names[i]);
            findings++;
        } else {
            char path[1024];
            snprintf(path, sizeof path, "%s/%s", outdir, names[i]);
            if (remove(path) != 0) die("%s: %s", path, strerror(errno));
            printf("  removed stale %s\n", path);
        }
    }
}

/* --replay: recompute a written CSV from its own rows, bit for bit. */
static int replay(const char *path)
{
    size_t len;
    char *text = read_file(path, &len), err[128], name[64], lower[64];
    Table t;
    const char *meta;
    const TA_FuncHandle *handle;
    const TA_FuncInfo *fi;
    Series s;
    Run r;
    int i, bad = 0, firstOut;
    unsigned o;
    if (!text) die("%s: %s", path, strerror(errno));
    if (!parse_table(text, &t, err, sizeof err)) die("%s: %s", path, err);
    meta = table_meta(&t, "TA-Lib reference data");
    if (!meta || sscanf(meta, "%63s", name) != 1) die("%s: no `# TA-Lib reference data: <function>` line", path);
    if (TA_GetFuncHandle(name, &handle) != TA_SUCCESS || TA_GetFuncInfo(handle, &fi) != TA_SUCCESS)
        die("%s: %s is not a registered function", path, name);
    lower_name(lower, sizeof lower, name);
    s = series_from_rows(lower, t.nRows, t.cells);
    run_alloc(&r, fi, s.n ? s.n : 1);
    if (run_call(&r, &s, 0, s.n, &t) != TA_SUCCESS) die("%s: TA_CallFunc failed", path);
    firstOut = PRICE_COLS + r.nExtra;
    for (i = 0; i < r.nExtra; i++)
        if (PRICE_COLS + i >= t.nCols || strcmp(t.cols[PRICE_COLS + i], r.extraName[i]) != 0)
            die("%s: column %d must be `%s`", path, PRICE_COLS + i + 1, r.extraName[i]);
    if ((int)fi->nbOutput != t.nCols - firstOut)
        die("%s: %d output columns, %u expected", path, t.nCols - firstOut, fi->nbOutput);
    for (i = 0; i < t.nRows; i++) {
        int k = i - r.beg;
        for (o = 0; o < fi->nbOutput; o++) {
            const char *cell = t.cells[i][firstOut + o];
            int have = k >= 0 && k < r.nb, differ;
            if (!*cell || !have) differ = (*cell != 0) != have;
            else if (r.isInt[o]) differ = strtol(cell, NULL, 10) != r.iout[o][k];
            else { double x = strtod(cell, NULL); differ = memcmp(&x, &r.rout[o][k], sizeof x) != 0; }
            if (differ) {
                char num[32];
                const char *got = !have ? "(empty)" : r.isInt[o] ? (snprintf(num, sizeof num, "%d", r.iout[o][k]), num)
                                                                 : fmt_real(num, sizeof num, r.rout[o][k]);
                printf("  %s row %s %s: file=%s replay=%s\n", name, t.cells[i][0], t.cols[firstOut + o], *cell ? cell : "(empty)", got);
                bad++;
            }
        }
    }
    printf("%s: %s over %d rows, %d output cells, %d differ%s\n", path, name, t.nRows,
           t.nRows * (int)fi->nbOutput, bad, bad ? "" : " (bit-exact)");
    run_free(&r);
    free_table(&t);
    free(text);
    return bad;
}

int main(int argc, char **argv)
{
    int argi = 1;
    Ctx ctx;
    if (argc >= 3 && strcmp(argv[1], "--replay") == 0) {
        int bad = 0, i;
        TA_Initialize();
        for (i = 2; i < argc; i++) bad += replay(argv[i]);
        TA_Shutdown();
        return bad ? 1 : 0;
    }
    if (argi < argc && strcmp(argv[argi], "--check") == 0) { checkMode = 1; argi++; }
    if (argc - argi != 2) {
        fprintf(stderr, "usage: ta_refdata [--check] <corpus-dir> <out-dir>\n"
                        "       ta_refdata --replay <csv>...\n");
        return 2;
    }
    load_corpus(argv[argi]);
    ctx.outdir = argv[argi + 1];
    TA_Initialize();
    TA_ForEachFunc(one_function, &ctx);
    TA_Shutdown();
    sweep_stale(ctx.outdir);
    if (checkMode) {
        printf("ta_refdata --check: %d functions, %d up to date, %d finding%s\n",
               nFuncNames, nUpToDate, findings, findings == 1 ? "" : "s");
        return findings ? 1 : 0;
    }
    printf("ta_refdata: %d functions, %d up to date, %d written, %d stale removed\n",
           nFuncNames, nUpToDate, nWritten, nStale);
    return 0;
}
