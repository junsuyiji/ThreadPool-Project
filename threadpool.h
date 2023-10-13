#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <queue>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <unordered_map>


//线程池支持的模式
enum class PoolMode {
  
  MODE_FIXED,     //固定线程数量
  MODE_CACHED,    //线程数量可以动态增长
};

//接受任意类型的基类
class Any {
  public:
    Any() = default;
    ~Any() = default;
    Any(const Any&) = delete;
    Any& operator = (const  Any&) = delete;
    Any(Any&&) = default;
    Any& operator = (Any&&) = default;

    template<typename T>
    Any(T data) : base_(std::make_unique<Derive<T>>(data))
    {}

    template<typename T> 
    T cast_()
    {
      Derive<T>* pd = dynamic_cast<Derive<T>*>(base_.get());

      if (pd == nullptr)
      {
        throw "type is unmatch";
      }

      return pd->data_;
    }
  private:
    //基类类型
    class Base 
    {
      public:
        virtual ~Base() = default;
    };

    //派生类型
    template<typename T>
    class Derive : public Base
    {
      public:
        Derive(T data) : data_(data)
        {}
      T data_;
      private:
        
    };

  private:
    std::unique_ptr<Base> base_;

};

//实现一个信号量的类
class Semaphore
{
  public:
    Semaphore(int limit = 0)
      : resLimit_(limit)
    {}
    ~Semaphore() = default;

    //获取一个信号量资源
    void wait()
    {
      std::unique_lock<std::mutex> lock(mtx_);
      cond_.wait(lock, [&]()->bool {return resLimit_ > 0;});
      resLimit_--;
    }

    //增加一个信号量资源
    void post()
    {
      std::unique_lock<std::mutex> lock(mtx_);
      resLimit_++;
      cond_.notify_all();
    }


  private:
    int resLimit_;
    std::mutex mtx_;
    std::condition_variable cond_;


};

class Task;

//实现接受线程池task任务完成之后的返回值雷兴国result
class Result
{
  public:
    Result(std::shared_ptr<Task> task, bool isValid = true);
    ~Result() = default;

    void setVal(Any any);
    Any get();
  private:
    Any any_;
    Semaphore sem_;
    std::shared_ptr<Task> task_;
    std::atomic_bool isValid_;
};


//任务抽象基类
class Task {
  public:

    Task();
    ~Task() = default;

    void exec();
    void setResult(Result *res);
    //用户可以自定义任务类型，从Task继承，重写run方法，实现自定义任务处理
    virtual Any run() = 0;

  private:
    Result* result_;
};

//线程类型
class Thread {

  public:
    //线程函数对象类型
    using ThreadFunc = std::function<void(int)>;
    //线程构造
    Thread(ThreadFunc func);
    //线程析构
    ~Thread();

    void start();

    //获取线程id
    int getId() const;
  private:
    ThreadFunc func_;
    static int generateId_;
    int threadId_; //保存线程id
};

/*
example 
ThreadPool pool;
pool.start(4);

class MyTask : public Task
{
  public:
    void run() {code};

};

pool.submitTask(std::make_shared<MyTask>());

*/

//线程池类型
class ThreadPool {

  public:
    ThreadPool();
    ~ThreadPool();

    //设置工作模式
    void setMode(PoolMode mode);

    //开启线程池
    void start(int initThreadSize = 4);

    //给任务队列设置上限阈值
    void setTaskQueMaxThreshHold(int threshhold);

    //设置初始线程数量
    void setInitThreadSize(int size); 
    
    //给线程池提交任务
    Result submitTask(std::shared_ptr<Task> sp);

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator = (const ThreadPool&) = delete;

    //设置cached模式下线程的阈值
    void setThreadSizeThreshHold(int threshhold);

  private:
  //定义线程执行函数
  void threadFunc(int threadid);

  //检查pool的运行装填
  bool checkRunningState() const;

  private:
    //std::vector<std::unique_ptr<Thread>> threads_; //线程列表
    std::unordered_map<int, std::unique_ptr<Thread>> threads_;
    int initThreadSize_;           //初始线程数量
    std::atomic_int curThreadSize_;  //记录当前线程池里面的总数量
    int threadSizeThreshHold_;   //cached模式下，线程上限的阈值
    std::queue<std::shared_ptr<Task>> taskQue_; //任务队列
    std::atomic_int taskSize_; //任务数量
    int taskQueMaxThreshHold_; //任务队列数量上限

    std::mutex taskQueMtx_;        //保证任务队列线程安全
    std::condition_variable notFull_;  //保证任务队列不满
    std::condition_variable notEmpty_; //保证任务队列不空

    PoolMode poolMode_;  //当前线程池的工作模式

    //当前线程池的启动状态
    std::atomic_bool isPoolRunning_;

    //记录空闲线程数量
    std::atomic_int idleThreadSize_;
};


#endif
