/*
 * r8j_trim.c: allocator probe of the R8j study (scratch tool, not production code).
 *
 * The nested dock is started with LD_PRELOAD of this library. It is loaded in
 * latte-dock-ng only (the program's short name) and only when R8J_LOG names a log
 * file; any other program, such as the preview helper latte-dock-ng-preview, is not
 * touched by the constructor.
 *
 * SIGUSR2 appends glibc's mallinfo2() counters to R8J_LOG. SIGUSR1 appends the
 * counters, calls malloc_trim(0) and appends the counters and the return value again.
 * The counters are: uordblks (bytes in use by malloc), fordblks (free bytes inside the
 * arenas), hblkhd (bytes in mmapped chunks), arena (non-mmapped space) and keepcost.
 *
 * Why a helper thread and not a signal handler: malloc_trim() and mallinfo2() take
 * arena locks and call malloc internally, so they are not async-signal-safe. Both
 * signals are blocked in every thread, the thread created here waits in sigwait(), and
 * the work runs in that thread. Because of this the probe adds one thread to the dock
 * (its Threads column is one higher than in R8d).
 *
 * Why LD_PRELOAD and not gdb: kernel.yama.ptrace_scope is 1 on the host, so gdb can
 * attach only to its own children. The dock must keep running across the cycles.
 *
 * Build (see the record): gcc -std=gnu11 -O2 -Wall -Wextra -Werror -fPIC -shared \
 *     -o r8j_trim.so r8j_trim.c -lpthread
 */

#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <malloc.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

static int g_log_fd = -1;
/* Only the worker thread touches the sequence number. */
static unsigned long g_seq = 0;

static void write_text(const char *text, size_t length)
{
    /* A short write to a regular file is not expected; the harness checks the log. */
    ssize_t written = write(g_log_fd, text, length);
    if (written < 0) {
        return;
    }
}

static void write_counters(const char *kind, int rc)
{
    struct mallinfo2 info = mallinfo2();
    struct timespec now;
    char line[384];
    clock_gettime(CLOCK_REALTIME, &now);
    int length = snprintf(line, sizeof line,
                          "%ld.%03ld %s seq=%lu rc=%d uordblks=%zu fordblks=%zu hblkhd=%zu "
                          "arena=%zu keepcost=%zu ordblks=%zu\n",
                          (long)now.tv_sec, now.tv_nsec / 1000000L, kind, g_seq, rc,
                          info.uordblks, info.fordblks, info.hblkhd, info.arena,
                          info.keepcost, info.ordblks);
    if (length > 0 && (size_t)length < sizeof line) {
        write_text(line, (size_t)length);
    }
}

static void *worker(void *argument)
{
    sigset_t wanted;
    (void)argument;
    sigemptyset(&wanted);
    sigaddset(&wanted, SIGUSR1);
    sigaddset(&wanted, SIGUSR2);
    for (;;) {
        int signal_number = 0;
        if (sigwait(&wanted, &signal_number) != 0) {
            continue;
        }
        ++g_seq;
        if (signal_number == SIGUSR1) {
            write_counters("trim-before", -1);
            int released = malloc_trim(0);
            write_counters("trim-after", released);
        } else if (signal_number == SIGUSR2) {
            write_counters("dump", -1);
        }
    }
    return NULL;
}

__attribute__((constructor)) static void r8j_probe_start(void)
{
    const char *log_path = getenv("R8J_LOG");
    sigset_t wanted;
    pthread_t thread;

    if (program_invocation_short_name == NULL ||
        strcmp(program_invocation_short_name, "latte-dock-ng") != 0) {
        return;
    }
    if (log_path == NULL || *log_path == '\0') {
        return;
    }
    g_log_fd = open(log_path, O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC, 0644);
    if (g_log_fd < 0) {
        return;
    }
    sigemptyset(&wanted);
    sigaddset(&wanted, SIGUSR1);
    sigaddset(&wanted, SIGUSR2);
    /* Threads created after this inherit the blocked mask, so only sigwait() takes the signals. */
    if (pthread_sigmask(SIG_BLOCK, &wanted, NULL) != 0) {
        close(g_log_fd);
        g_log_fd = -1;
        return;
    }
    if (pthread_create(&thread, NULL, worker, NULL) != 0) {
        close(g_log_fd);
        g_log_fd = -1;
        return;
    }
    pthread_detach(thread);
}
