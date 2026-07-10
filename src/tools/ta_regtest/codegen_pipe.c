#include "codegen_pipe.h"

#include <stdio.h>
#include <stdlib.h>
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

    /* Read response line (one byte at a time until newline) */
    int idx = 0;
    while( idx < response_size - 1 )
    {
        ssize_t n = read(cp->from_child_fd, &response[idx], 1);
        if( n <= 0 )
            return TA_CODEGEN_PIPE_READ_FAILED;
        if( response[idx] == '\n' )
        {
            response[idx] = '\0';
            return TA_TEST_PASS;
        }
        idx++;
    }

    /* Buffer full without seeing newline */
    response[response_size - 1] = '\0';
    return TA_TEST_PASS;
}

void codegen_pipe_close(CodegenPipe *cp)
{
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
