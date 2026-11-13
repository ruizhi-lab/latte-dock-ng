#include <condition_variable>
#include <mutex>

void
waitWithoutRechecking(std::condition_variable &condition, std::mutex &mutex, bool ready)
{
    std::unique_lock<std::mutex> lock(mutex);
    if (ready) {
        // A wake can be spurious; the predicate needs a loop. Braces are required for this check to match the conditional wait.
        condition.wait(lock);
    }
}
