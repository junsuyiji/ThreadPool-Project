# ThreadPool-Project
The thread pool manager is a component used to manage multiple threads, which can receive tasks and assign them to idle threads for execution. This project aims to provide a reliable and efficient implementation of a thread pool, enabling better management of task execution and resource utilization in a multi-threaded environment.

## example

Print brackets (), but the brackets are required to be matched. You can choose a bracket. () like this is correct (()) but not. You can choose two brackets, like this (()), (()()), they are all correct. Anyway, as long as they are paired, they are correct. It is very simple.

```cpp
#include <iostream>
#include <chrono>
#include <thread>


#include "threadpool.h"

int n = 0; 
int count_ = 0;
std::mutex lk;
std::condition_variable cv;
#define CAN_PRODUCE (count_ < n)
#define CAN_CONSUME (count_ > 0)


class Producer : public Task
{
    public:
        Producer() = default;
        ~Producer() = default;
        Any run()
        {
            while (1)
            {
                std::unique_lock<std::mutex> lock(lk);
                while (!CAN_PRODUCE)
                {
                    cv.wait(lock);
                }
                std::cout<<"(";
                count_++;
                cv.notify_all();
                lock.unlock();
            }
        }

    private:


};

class Consumer : public Task
{
    public:
        Consumer() = default;
        ~Consumer() = default;
        Any run()
        {
            while (1)
            {
                std::unique_lock<std::mutex> lock(lk);
                while (!CAN_CONSUME)
                {
                    cv.wait(lock);
                }
                std::cout<<")";
                count_--;
                cv.notify_all();
                lock.unlock();
            }
        }

    
};


int main()
{
    n = 3;
    ThreadPool pool;
    pool.setMode(PoolMode::MODE_CACHED);
    pool.start(2);
    pool.submitTask(std::make_shared<Producer>());
    pool.submitTask(std::make_shared<Consumer>());
};
```
