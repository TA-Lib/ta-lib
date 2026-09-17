#include "codegen_pipe.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

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
void codegen_ride_reset(void)
{
    int i;
    g_rideMismatches = 0;
    g_rideVerdicts = 0;
    g_rideBars = 0;
    g_rideRejects = 0;
    for( i = 0; i < CODEGEN_RIDE_SKIP_N; i++ ) g_rideSkips[i] = 0;
}

#if defined(WIN32) || defined(_WIN32)

/* Subprocess JSON-RPC pipes are not implemented for Windows yet.
 * These stubs keep ta_regtest building; codegen verification reports
 * servers as unavailable and the C reference tests run unaffected.
 */
ErrorNumber codegen_pipe_open(CodegenPipe *cp, const char *const argv[])
{
    (void)argv;
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
    if( ok < 0 ) return;             /* ta_ref_serve and pre-feature builds */
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
