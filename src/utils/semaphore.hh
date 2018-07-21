#include <condition_variable>
#include <mutex>
#include <thread>

class Semaphore {
    std::mutex              mtx;
    std::condition_variable cv;
    int                     count;

public:
    Semaphore(int count_ = 0)
        : count(count_) {}

    inline void notify() {
        std::unique_lock<std::mutex> lock(mtx);
        count++;
        cv.notify_one();
    }

    inline void wait() {
        std::unique_lock<std::mutex> lock(mtx);

        while (count == 0) {
            std::this_thread::yield();
            cv.wait(lock);
        }
        count--;
    }
};
