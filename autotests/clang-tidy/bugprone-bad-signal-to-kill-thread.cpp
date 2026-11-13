#include <pthread.h>
#include <signal.h>

void
terminateThread(pthread_t thread)
{
    // SIGTERM is unsafe for pthread_kill because its default action terminates the process.
    pthread_kill(thread, SIGTERM);
}
