#include "codegen_pipe.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

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
