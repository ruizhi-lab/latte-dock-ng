#include <pthread.h>
void
posixReturnCheck(pthread_mutex_t *mutex)
{
    // POSIX APIs return zero or a positive error code, never a negative one.
    if (pthread_mutex_init(mutex, nullptr) < 0) {}
}
