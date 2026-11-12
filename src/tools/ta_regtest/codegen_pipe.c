#include "codegen_pipe.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "ta_abstract.h"

/* The ride-along verdict rides on EVERY response, but the codegen driver reads it
 * at one of its ~31 call sites, so a divergence found by the parameter sweep or
 * the large-period pass was computed by the server and then overwritten by the
 * next request. This is the one point every response passes through, which is
 * what makes those sites count. */
static int       g_rideMismatches;
static long      g_rideVerdicts;
static long long g_rideBars;
static long      g_rideSkips[CODEGEN_RIDE_SKIP_N];
static long      g_rideRejects;
static int       g_rideMismatchesEver;
static long      g_rideVerdictsEver;
static long      g_rideRejectsEver;

int       codegen_ride_mismatches(void) { return g_rideMismatches; }
long      codegen_ride_verdicts(void)   { return g_rideVerdicts; }
long long codegen_ride_bars(void)       { return g_rideBars; }
long codegen_ride_skips(int reason)
{
    if( reason < 0 || reason >= CODEGEN_RIDE_SKIP_N ) return 0;
    return g_rideSkips[reason];
}
long codegen_ride_rejects(void)         { return g_rideRejects; }
int  codegen_ride_mismatches_ever(void) { return g_rideMismatchesEver; }
long codegen_ride_verdicts_ever(void)   { return g_rideVerdictsEver; }
long codegen_ride_rejects_ever(void)    { return g_rideRejectsEver; }

static long g_tailDirtyEver;
static long g_tailReportsEver;
long codegen_tail_dirty_ever(void)   { return g_tailDirtyEver; }
long codegen_tail_reports_ever(void) { return g_tailReportsEver; }
void codegen_ride_reset(void)
{
    int i;
    g_rideMismatches = 0;
    g_rideVerdicts = 0;
    g_rideBars = 0;
    g_rideRejects = 0;
    for( i = 0; i < CODEGEN_RIDE_SKIP_N; i++ ) g_rideSkips[i] = 0;
}

static int json_int_field(const char *s, const char *key, int dflt)
{
    const char *p = strstr(s, key);
    return p ? atoi(p + strlen(key)) : dflt;
}

/* "key":"value" -> value, "" when absent. */
static void json_str_field(const char *s, const char *key, char *out, int outSize)
{
    const char *p = strstr(s, key);
    int i = 0;
    out[0] = '\0';
    if( !p ) return;
    p += strlen(key);
    if( *p != '"' ) return;
    p++;
    while( i < outSize - 1 && p[i] && p[i] != '"' ) { out[i] = p[i]; i++; }
    out[i] = '\0';
}

#define ENGINE_MAX_FUNCS 512
typedef struct {
    char name[48];
    int  exported;
    long ok;        /* successful batch replies */
    long wasm;      /* calls that ran in WASM */
    char why[24];   /* the first expected fall-through label */
} EngineFunc;
static EngineFunc g_engineFunc[ENGINE_MAX_FUNCS];
static int        g_engineFuncCount;
static long       g_engineFuncDropped;
static int        g_enginePipes;
static long       g_engineReplies;
static long       g_engineWasm;
static long       g_engineDefects;
static long       g_engineExitFailures;

/* The labels the server's expectedFallThrough answers with. */
static const char *const ENGINE_WHY[] = { "no-export", "ma-type", "core", "lengths", "budget", "js-rejects" };
#define ENGINE_NWHY ((int)(sizeof(ENGINE_WHY) / sizeof(ENGINE_WHY[0])))
static long g_engineExpected[ENGINE_NWHY];

static void engine_defect(const char *fn, const char *what, const char *detail)
{
    if( g_engineDefects++ < 20 )
        printf("  ENGINE DEFECT [TA_%s]: %s%s%s\n", fn, what, detail[0] ? ": " : "", detail);
}

static EngineFunc *engine_func(const char *fn, int exported)
{
    int i;
    for( i = 0; i < g_engineFuncCount; i++ )
        if( strcmp(g_engineFunc[i].name, fn) == 0 ) return &g_engineFunc[i];
    if( g_engineFuncCount == ENGINE_MAX_FUNCS ) { g_engineFuncDropped++; return NULL; }
    memset(&g_engineFunc[i], 0, sizeof(g_engineFunc[i]));
    snprintf(g_engineFunc[i].name, sizeof(g_engineFunc[i].name), "%s", fn);
    g_engineFunc[i].exported = exported;
    g_engineFuncCount++;
    return &g_engineFunc[i];
}

/* Every reply to a batch call of a WASM-engine server, whichever pass made it.
 * `timed` and the stream warm-up arms (`bench_mode`) are not batch calls. */
static void engine_scan(const CodegenPipe *cp, const char *request, const char *response)
{
    const TA_FuncHandle *handle;
    const TA_FuncInfo *info;
    const char *m;
    char fn[48], why[24], defectWhy[96];
    int i = 0, exported, calls, wasm, expected, defects, w;
    EngineFunc *f;

    if( !cp->engineWasm ) return;
    m = strstr(request, "\"method\":\"TA_");
    if( !m ) return;
    m += 13;
    while( i < (int)sizeof(fn) - 1 && m[i] && m[i] != '"' ) { fn[i] = m[i]; i++; }
    fn[i] = '\0';
    if( TA_GetFuncHandle(fn, &handle) != TA_SUCCESS || TA_GetFuncInfo(handle, &info) != TA_SUCCESS )
        return;
    if( json_int_field(request, "\"timed\":", 0) != 0 || json_int_field(request, "\"bench_mode\":", 0) != 0 )
        return;

    exported = (info->flags & TA_FUNC_FLG_USES_TRANSCENDENTAL) == 0;
    f = engine_func(fn, exported);
    g_engineReplies++;
    if( strncmp(response, "{\"error\"", 8) == 0 )
    {
        engine_defect(fn, "the server answered an error", response);
        return;
    }
    calls    = json_int_field(response, "\"engine_calls\":", -1);
    wasm     = json_int_field(response, "\"engine_wasm\":", 0);
    expected = json_int_field(response, "\"engine_expected\":", 0);
    defects  = json_int_field(response, "\"engine_defects\":", 0);
    if( calls < 1 )
    {
        engine_defect(fn, "a batch reply without engine evidence", "");
        return;
    }
    if( json_int_field(response, "\"engine_exported\":", -1) != exported )
        engine_defect(fn, exported ? "the server calls it not exported, but the library flags no transcendental"
                                   : "the server calls it exported, but the library flags it transcendental", "");
    if( defects > 0 )
    {
        json_str_field(response, "\"engine_defect_why\":", defectWhy, (int)sizeof(defectWhy));
        engine_defect(fn, "fell through for a reason that is not the engine's contract", defectWhy);
    }
    json_str_field(response, "\"engine_why\":", why, (int)sizeof(why));
    if( expected > 0 )
    {
        for( w = 0; w < ENGINE_NWHY; w++ )
            if( strcmp(why, ENGINE_WHY[w]) == 0 ) break;
        if( w == ENGINE_NWHY )
            engine_defect(fn, "an expected fall-through under an unknown label", why);
        else
            g_engineExpected[w] += expected;
    }
    g_engineWasm += wasm;
    if( f && json_int_field(response, "\"retCode\":", -1) == 0 )
    {
        f->ok++;
        f->wasm += wasm;
        if( !f->why[0] && expected > 0 )
            snprintf(f->why, sizeof(f->why), "%s", why);
    }
}

int codegen_engine_report(void)
{
    int failures = (int)(g_engineDefects + g_engineExitFailures + (g_engineFuncDropped > 0));
    int covered = 0, i, w;
    long never[ENGINE_NWHY + 1];

    if( g_enginePipes == 0 ) return 0;
    memset(never, 0, sizeof(never));
    for( i = 0; i < g_engineFuncCount; i++ )
    {
        const EngineFunc *f = &g_engineFunc[i];
        if( f->wasm > 0 ) { covered++; continue; }
        if( f->ok == 0 ) continue;
        if( f->exported )
        {
            printf("  ENGINE VACUOUS [TA_%s]: exported, %ld successful batch call(s), none ran in "
                   "WASM (first fall-through: %s)\n", f->name, f->ok, f->why[0] ? f->why : "none");
            failures++;
            continue;
        }
        for( w = 0; w < ENGINE_NWHY; w++ )
            if( strcmp(f->why, ENGINE_WHY[w]) == 0 ) break;
        never[w]++;
    }
    if( g_engineReplies == 0 )
    {
        printf("  ENGINE VACUOUS: %d WASM-engine server(s) ran and answered no batch call\n",
               g_enginePipes);
        failures++;
    }
    if( g_engineFuncDropped > 0 )
        printf("  ENGINE TABLE FULL: %ld reply(ies) of functions past %d went unrecorded\n",
               g_engineFuncDropped, ENGINE_MAX_FUNCS);
    if( g_engineDefects > 20 )
        printf("  (%ld engine defect(s) in all; the first 20 are above)\n", g_engineDefects);

    printf("  wasm engine: %ld call(s) ran in WASM, over %d function(s); expected fall-throughs:",
           g_engineWasm, covered);
    for( w = 0; w < ENGINE_NWHY; w++ )
        printf(" %s=%ld", ENGINE_WHY[w], g_engineExpected[w]);
    printf("\n  wasm engine: functions that never ran in WASM, by first fall-through:");
    for( w = 0; w < ENGINE_NWHY; w++ )
        if( never[w] ) printf(" %s=%ld", ENGINE_WHY[w], never[w]);
    if( never[ENGINE_NWHY] ) printf(" other=%ld", never[ENGINE_NWHY]);
    printf("\n");
    return failures;
}

#if defined(WIN32) || defined(_WIN32)

/* Subprocess JSON-RPC pipes are not implemented for Windows yet.
 * These stubs keep ta_regtest building; codegen verification reports
 * servers as unavailable and the C reference tests run unaffected.
 */
ErrorNumber codegen_pipe_open(CodegenPipe *cp, const char *const argv[])
{
    (void)argv;
    cp->engineWasm = 0;
    cp->to_child_fd = -1;
    cp->from_child_fd = -1;
    cp->child_pid = -1;
    cp->rbuf = NULL;
    cp->rpos = cp->rlen = 0;
    return TA_CODEGEN_PIPE_OPEN_FAILED;
}

ErrorNumber codegen_pipe_call(CodegenPipe *cp,
                              const char *request,
                              char *response,
                              int response_size)
{
    (void)cp;
    (void)request;
    if( response_size > 0 )
        response[0] = '\0';
    return TA_CODEGEN_PIPE_WRITE_FAILED;
}

void codegen_pipe_close(CodegenPipe *cp)
{
    cp->to_child_fd = -1;
    cp->from_child_fd = -1;
    cp->child_pid = -1;
    cp->rbuf = NULL;
    cp->rpos = cp->rlen = 0;
}

#else /* POSIX implementation */

#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>

ErrorNumber codegen_pipe_open(CodegenPipe *cp, const char *const argv[])
{
    int parent_to_child[2]; /* parent writes, child reads (child's stdin) */
    int child_to_parent[2]; /* child writes, parent reads (child's stdout) */

    /* Suppress SIGPIPE so a crashed server returns EPIPE from write()
     * instead of killing the entire ta_regtest process. */
    signal(SIGPIPE, SIG_IGN);

    cp->rbuf = NULL;    /* before any early return: close() frees this */
    cp->rpos = cp->rlen = 0;
    cp->engineWasm = 0;
    {
        int a;
        for( a = 1; argv[a]; a++ )
            if( strcmp(argv[a], "--engine=wasm") == 0 ) cp->engineWasm = 1;
    }
    cp->to_child_fd = -1;
    cp->from_child_fd = -1;
    cp->child_pid = -1;

    /* Pre-flight: the C-family servers are launched by explicit path
     * (e.g. "./ta_codegen_serve_c"), which resolves against the current
     * working directory. If ta_regtest is not run from the directory that
     * holds the servers (bin/), fork+execvp would still "succeed" here and the
     * failure would only surface later as a misleading wall of per-call errors.
     * Detect the missing server up front and say so clearly (#106). Commands
     * resolved via PATH (java, dotnet) have no '/', so they are left to execvp. */
    if( strchr(argv[0], '/') != NULL && access(argv[0], X_OK) != 0 )
    {
        printf("  server not found: %s\n"
               "  (run ta_regtest from the directory holding the servers, e.g. cd bin)\n",
               argv[0]);
        return TA_CODEGEN_PIPE_SERVER_NOT_FOUND;
    }

    if( pipe(parent_to_child) != 0 )
        return TA_CODEGEN_PIPE_OPEN_FAILED;

    if( pipe(child_to_parent) != 0 )
    {
        close(parent_to_child[0]);
        close(parent_to_child[1]);
        return TA_CODEGEN_PIPE_OPEN_FAILED;
    }

    pid_t pid = fork();
    if( pid < 0 )
    {
        close(parent_to_child[0]);
        close(parent_to_child[1]);
        close(child_to_parent[0]);
        close(child_to_parent[1]);
        return TA_CODEGEN_PIPE_FORK_FAILED;
    }

    if( pid == 0 )
    {
        /* Child process */
        close(parent_to_child[1]); /* close write end of parent->child */
        close(child_to_parent[0]); /* close read end of child->parent */

        dup2(parent_to_child[0], STDIN_FILENO);
        dup2(child_to_parent[1], STDOUT_FILENO);

        close(parent_to_child[0]);
        close(child_to_parent[1]);

        execvp(argv[0], (char *const *)argv);

        /* If execvp returns, it failed */
        _exit(127);
    }

    /* Parent process */
    close(parent_to_child[0]); /* close read end of parent->child */
    close(child_to_parent[1]); /* close write end of child->parent */

    cp->to_child_fd = parent_to_child[1];
    cp->from_child_fd = child_to_parent[0];
    cp->child_pid = (int)pid;
    if( cp->engineWasm ) g_enginePipes++;
    cp->rpos = cp->rlen = 0;
    cp->rbuf = (char *)malloc(CODEGEN_PIPE_RBUF);
    if( !cp->rbuf )
    {
        codegen_pipe_close(cp);
        return TA_CODEGEN_PIPE_OPEN_FAILED;
    }

    return TA_TEST_PASS;
}

static int ride_int_field(const char *s, const char *key)
{
    const char *p = strstr(s, key);
    if( !p ) return -1;
    return atoi(p + strlen(key));
}

/* "ride_batch":"3ff0000000000000" -> the 16 hex characters, "?" when absent. */
static void ride_hex_field(const char *s, const char *key, char out[17])
{
    const char *p = strstr(s, key);
    out[0] = '\0';
    if( !p ) return;
    p += strlen(key);
    while( *p == ' ' ) p++;
    if( *p != '"' ) return;
    p++;
    {
        int i = 0;
        while( i < 16 && p[i] && p[i] != '"' ) { out[i] = p[i]; i++; }
        out[i] = '\0';
    }
}

static void ride_scan(const char *request, const char *response)
{
    int ok = ride_int_field(response, "\"ride_ok\":");
    if( ok < 0 ) return;             /* a ta_ref serve and pre-feature builds */
    g_rideVerdicts++;
    g_rideVerdictsEver++;
    /* `ride_rej` is how many streaming entry points AGREED with the batch
     * tier's rejection, computed by the comparison itself -- so this total
     * cannot outlive the comparison that feeds it. */
    {
        int rej = ride_int_field(response, "\"ride_rej\":");
        if( rej > 0 ) { g_rideRejects += rej; g_rideRejectsEver += rej; }
    }
    /* A dedup hit re-reports the cached counts, so counting it would credit bars
     * nobody compared on this call. Only a replay that actually ran counts. */
    if( ok != 0 )
    {
        int reason = ride_int_field(response, "\"ride_skip\":");
        if( reason >= 0 && reason < CODEGEN_RIDE_SKIP_N ) g_rideSkips[reason]++;
        if( ride_int_field(response, "\"ride_dedup\":") <= 0 )
        {
            int ob = ride_int_field(response, "\"ride_open_bars\":");
            int fb = ride_int_field(response, "\"ride_fill_bars\":");
            if( ob > 0 ) g_rideBars += ob;
            if( fb > 0 ) g_rideBars += fb;
        }
        return;
    }

    g_rideMismatches++;
    g_rideMismatchesEver++;
    {
        static const char *const LEG[] = {
            "?", "Open+Update", "OpenAndFill", "rejection code", "exception class"
        };
        char b[17], st[17], fn[64];
        const char *m = strstr(request, "\"method\":\"");
        int leg = ride_int_field(response, "\"ride_leg\":");
        int i = 0;
        fn[0] = '\0';
        if( m )
        {
            m += 10;
            while( i < (int)sizeof(fn) - 1 && m[i] && m[i] != '"' ) { fn[i] = m[i]; i++; }
            fn[i] = '\0';
        }
        if( leg < 0 || leg > 4 ) leg = 0;
        if( leg >= 3 )
        {
            printf("  RIDE MISMATCH [%s]: leg %d (%s) batch=%d Open=%d "
                   "OpenAndFill=%d  (m=%d lookback=%d)\n",
                   fn[0] ? fn : "?", leg, LEG[leg],
                   ride_int_field(response, "\"ride_rc_batch\":"),
                   ride_int_field(response, "\"ride_rc_open\":"),
                   ride_int_field(response, "\"ride_rc_fill\":"),
                   ride_int_field(response, "\"ride_m\":"),
                   ride_int_field(response, "\"ride_lb\":"));
        }
        if( leg < 3 )
        {
            ride_hex_field(response, "\"ride_batch\":", b);
            ride_hex_field(response, "\"ride_stream\":", st);
            printf("  RIDE MISMATCH [%s]: leg %d (%s) bar %d output %d  "
                   "batch=%s stream=%s  (m=%d lookback=%d)\n",
                   fn[0] ? fn : "?", leg, LEG[leg],
                   ride_int_field(response, "\"ride_bar\":"),
                   ride_int_field(response, "\"ride_out\":"),
                   b[0] ? b : "?", st[0] ? st : "?",
                   ride_int_field(response, "\"ride_m\":"),
                   ride_int_field(response, "\"ride_lb\":"));
        }
    }
}

static void tail_scan(const char *request, const char *response)
{
    int dirty = ride_int_field(response, "\"out_tail_dirty\":");
    if( dirty < 0 ) return;
    g_tailReportsEver++;
    if( dirty == 0 ) return;
    if( g_tailDirtyEver++ < 10 )
    {
        const char *m = strstr(request, "\"method\":\"");
        int n = 0;
        if( m ) { m += 10; while( m[n] && m[n] != '"' && n < 48 ) n++; }
        printf("  TAIL WRITE [%.*s]: %d output slot(s) at or past outNBElement "
               "changed by a successful call\n", n, m ? m : "?", dirty);
    }
}

ErrorNumber codegen_pipe_call(CodegenPipe *cp,
                              const char *request,
                              char *response,
                              int response_size)
{
    /* Write request + newline (loop for large requests that exceed pipe buffer) */
    int req_len = (int)strlen(request);
    int total_written = 0;
    while( total_written < req_len )
    {
        ssize_t n = write(cp->to_child_fd, request + total_written, req_len - total_written);
        if( n <= 0 )
            return TA_CODEGEN_PIPE_WRITE_FAILED;
        total_written += (int)n;
    }

    /* Write newline if request doesn't end with one */
    if( req_len == 0 || request[req_len - 1] != '\n' )
    {
        ssize_t n = write(cp->to_child_fd, "\n", 1);
        if( n != 1 )
            return TA_CODEGEN_PIPE_WRITE_FAILED;
    }

    /* Read the response line out of the chunk buffer, refilling as needed. */
    int idx = 0;
    for( ;; )
    {
        if( cp->rpos >= cp->rlen )
        {
            ssize_t n = read(cp->from_child_fd, cp->rbuf, CODEGEN_PIPE_RBUF);
            if( n <= 0 )
                return TA_CODEGEN_PIPE_READ_FAILED;
            cp->rlen = (int)n;
            cp->rpos = 0;
        }

        /* Consume up to the newline, or the whole chunk if there isn't one. */
        {
            char *chunk = cp->rbuf + cp->rpos;
            int avail = cp->rlen - cp->rpos;
            char *nl = (char *)memchr(chunk, '\n', (size_t)avail);
            int take = nl ? (int)(nl - chunk) : avail;
            int room = response_size - 1 - idx;
            int copy = (take < room) ? take : room;

            if( copy > 0 )
            {
                memcpy(response + idx, chunk, (size_t)copy);
                idx += copy;
            }
            cp->rpos += take + (nl ? 1 : 0);

            /* Overlong response: keep draining to the newline so the stream
               stays aligned for the next call, but return what fits. */
            if( nl || room <= take )
            {
                if( !nl )
                {
                    response[response_size - 1] = '\0';
                    continue;
                }
                response[idx] = '\0';
                ride_scan(request, response);
                tail_scan(request, response);
                engine_scan(cp, request, response);
                return TA_TEST_PASS;
            }
        }
    }
}

void codegen_pipe_close(CodegenPipe *cp)
{
    free(cp->rbuf);
    cp->rbuf = NULL;
    cp->rpos = cp->rlen = 0;

    if( cp->to_child_fd >= 0 )
    {
        close(cp->to_child_fd);
        cp->to_child_fd = -1;
    }

    if( cp->from_child_fd >= 0 )
    {
        close(cp->from_child_fd);
        cp->from_child_fd = -1;
    }

    if( cp->child_pid > 0 )
    {
        /* Closing stdin should cause ta_codegen to exit.
         * Wait briefly, then kill if still running.
         */
        int status;
        pid_t result = waitpid(cp->child_pid, &status, WNOHANG);
        if( result == 0 )
        {
            /* Still running — give it a moment */
            usleep(100000); /* 100ms */
            result = waitpid(cp->child_pid, &status, WNOHANG);
            if( result == 0 )
            {
                kill(cp->child_pid, SIGTERM);
                waitpid(cp->child_pid, &status, 0);
            }
        }
        /* A WASM-engine server that could not start the engine exits non-zero. */
        if( cp->engineWasm && result == (pid_t)cp->child_pid &&
            WIFEXITED(status) && WEXITSTATUS(status) != 0 )
        {
            printf("  ENGINE DEFECT: the WASM-engine server (pid %d) exited with status %d\n",
                   cp->child_pid, WEXITSTATUS(status));
            g_engineExitFailures++;
        }
        cp->child_pid = -1;
    }
}

#endif /* WIN32 / POSIX */

/* ---- Bounded JSON buffer append (platform independent) ---- */

int codegen_appendf( char *buf, int buf_size, int pos, const char *fmt, ... )
{
    va_list ap;
    int avail, n;

    if( buf_size <= 0 ) return 0;
    if( pos < 0 ) pos = 0;
    if( pos >= buf_size - 1 ) return buf_size - 1;

    avail = buf_size - pos;
    va_start( ap, fmt );
    n = vsnprintf( buf + pos, (size_t)avail, fmt, ap );
    va_end( ap );

    /* C11 7.21.6.12: only a non-negative return guarantees what was written,
       so on an encoding error re-terminate rather than trust the buffer. */
    if( n < 0 ) { buf[pos] = '\0'; return pos; }
    if( n >= avail ) return buf_size - 1; /* truncated: saturate */
    return pos + n;
}

int codegen_appendc( char *buf, int buf_size, int pos, char c )
{
    if( buf_size <= 0 ) return 0;
    if( pos < 0 ) pos = 0;
    if( pos >= buf_size - 1 ) return buf_size - 1;
    buf[pos++] = c;
    buf[pos] = '\0';
    return pos;
}
