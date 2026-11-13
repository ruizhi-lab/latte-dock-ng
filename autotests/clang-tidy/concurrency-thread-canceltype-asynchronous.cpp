#include <pthread.h>
void
asyncCancellation()
{
    // Asynchronous cancellation can interrupt a thread while shared state is inconsistent.
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, nullptr);
}
