#ifndef CODEGEN_PIPE_H
#define CODEGEN_PIPE_H

#include "ta_error_number.h"

/* Read buffer. Responses reach ~1.8MB, so a byte-at-a-time reader costs one
 * syscall per byte; a chunked one amortises that. Must persist across calls:
 * a read() can pull in bytes past the newline, and those belong to the next
 * response. Heap, not inline — these structs are main() locals (and an array of
 * SV_MAX_PIPES), which would not fit Windows' 1MB default stack. */
#define CODEGEN_PIPE_RBUF (256 * 1024)

/* 1 when this platform can actually spawn a JSON-RPC server subprocess.
 * Windows has stubs (codegen_pipe.c), so EVERY server is legitimately
 * unavailable there and a caller must not treat that as a failure. */
#if defined(WIN32) || defined(_WIN32)
#define CODEGEN_PIPE_SUPPORTED 0
#else
#define CODEGEN_PIPE_SUPPORTED 1
#endif

/* Opaque handle for the ta_codegen subprocess */
typedef struct {
    int to_child_fd;    /* fd to write JSON-RPC requests */
    int from_child_fd;  /* fd to read JSON-RPC responses */
    int child_pid;      /* pid of ta_codegen process */
    char *rbuf;         /* CODEGEN_PIPE_RBUF bytes, owned; NULL until open */
    int  rpos;          /* next unconsumed byte in rbuf */
    int  rlen;          /* bytes valid in rbuf */
} CodegenPipe;

/* Start a JSON-RPC subprocess.
 * argv: NULL-terminated array of strings (program + args).
 *   e.g. {"./ta_codegen", "serve", NULL}
 *   e.g. {"java", "-cp", "ta_codegen_java", "TaCodegenServe", NULL}
 * Returns TA_TEST_PASS on success, error code on failure.
 */
ErrorNumber codegen_pipe_open(CodegenPipe *cp, const char *const argv[]);

/* Send a JSON-RPC request line and read the response line.
 * request: null-terminated JSON string (newline will be appended)
 * response: buffer to receive response (caller allocates)
 * response_size: size of response buffer
 * Returns TA_TEST_PASS on success, error code on failure.
 */
ErrorNumber codegen_pipe_call(CodegenPipe *cp,
                              const char *request,
                              char *response,
                              int response_size);

/* Stop the ta_codegen subprocess and clean up.
 * Always safe to call (handles already-closed state).
 */
void codegen_pipe_close(CodegenPipe *cp);

/* Bounded append into a JSON request/response buffer.
 *
 * `pos += snprintf(buf + pos, buf_size - pos, ...)` lets `pos` run past
 * `buf_size` as soon as one call truncates; the next call then passes a
 * negative size that converts to a huge size_t and writes past the buffer
 * (CodeQL cpp/overflowing-snprintf). These saturate `pos` at `buf_size - 1`
 * instead, so the buffer stays NUL-terminated and in bounds.
 *
 * Both take and return an ABSOLUTE write position: use
 *   pos = codegen_appendf(buf, buf_size, pos, fmt, ...);
 * in place of the `pos +=` idiom.
 */
int codegen_appendf(char *buf, int buf_size, int pos, const char *fmt, ...);
int codegen_appendc(char *buf, int buf_size, int pos, char c);

#endif
