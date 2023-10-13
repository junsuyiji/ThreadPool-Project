#include "threadpool.h"
#include <thread>
#include <iostream>
#include <chrono>

const int TASK_MAX_THRESHHOLD = 4;
const int THREAD_MAX_THRESHHOLD = 10;
const int THREAD_MAX_IDLE_TIME = 60;//单位秒


//线程池构造
ThreadPool::ThreadPool() 
  : initThreadSize_(4)
  , taskSize_(0)
  , idleThreadSize_(0)
  , curThreadSize_(0)
  , taskQueMaxThreshHold_(TASK_MAX_THRESHHOLD)
  , threadSizeThreshHold_(THREAD_MAX_THRESHHOLD)
  , poolMode_(PoolMode::MODE_FIXED)
  , isPoolRunning_(false)
{}

//线程池析构
ThreadPool::~ThreadPool()
{}

//设置线程池工作模式
void ThreadPool::setMode(PoolMode mode)
{
  if (checkRunningState())
  {
    return ;
  }
  poolMode_ = mode;
}

//设置初始线程数量
void ThreadPool::setInitThreadSize(int size)
{

  initThreadSize_ = size;
}

//设置任务队列最大阈值
void ThreadPool::setTaskQueMaxThreshHold(int threshhold)
{
  if (checkRunningState())
  {
    return ;
  }
  taskQueMaxThreshHold_ = threshhold;
}

void ThreadPool::setThreadSizeThreshHold(int threshhold)
{
  if (checkRunningState())
  {
    return ;
  }
  if (poolMode_ == PoolMode::MODE_CACHED)
    threadSizeThreshHold_ = threshhold;

}

//给线程池提交任务
Result ThreadPool::submitTask(std::shared_ptr<Task> sp)
{
  //获取锁
  std::unique_lock<std::mutex> lock(taskQueMtx_);
  
  //线程通信，判断是不是有空余,等待如果超过1s，默认返回失败
  if (!notFull_.wait_for(lock, 
                    std::chrono::seconds(1),
                    [&]()->bool {return taskQue_.size() < (size_t)taskQueMaxThreshHold_;}))
  {
    std::cerr<<" task queue is error" << std::endl;
    return Result(sp, false);
  }

  //如果有空，就把任务放在任务队列中
  taskQue_.emplace(sp);
  taskSize_++;

  //cached模式
  if (poolMode_ == PoolMode::MODE_CACHED 
      && taskSize_ > idleThreadSize_
      && curThreadSize_ < threadSizeThreshHold_)
  {
    auto ptr = std::make_unique<Thread> (std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1));

    int threadId_  = ptr->getId();
    threads_.emplace(threadId_, std::move(ptr));
  }

  //已经放入了任务，进行通知
  notEmpty_.notify_all();
  return Result(sp);
}

bool ThreadPool::checkRunningState() const
{
  return isPoolRunning_;
}

//开启线程池
void ThreadPool::start(int initThreadSize)
{
  //设置线程池运行状态
  isPoolRunning_ = true;

  //记录初始线程个数   
  initThreadSize_ = initThreadSize;

  curThreadSize_ = initThreadSize;

  //创建线程
  for (int i = 0; i < initThreadSize_; i++)
  {

    auto ptr = std::make_unique<Thread> (std::bind(&ThreadPool::threadFunc, this));

    int threadId_  = ptr->getId();
    threads_.emplace(threadId_, std::move(ptr));
    //threads_.emplace_back(std::move(ptr));
  }

  //启动线程
  for (int i = 0; i < initThreadSize_; i++)
  {
    threads_[i]->start();
    idleThreadSize_++;
  }

}


//定义线程函数
void ThreadPool::threadFunc(int threadid)
{
  auto lastTime = std::chrono::high_resolution_clock().now();

  for (;;)
  {
    

    std::shared_ptr<Task> task;
    {
    //获取锁
    std::unique_lock<std::mutex> lock(taskQueMtx_);
    
    //尝试获取任务
    std::cout<<"tid: "<<std::this_thread::get_id()<<" 尝试获取任务。。。"<<std::endl;
    
    //cached 模式回收线程
    if (poolMode_ == PoolMode::MODE_CACHED)
    {
      while(taskQue_.size() > 0)
      {
        if (std::cv_status::timeout == notEmpty_.wait_for(lock, std::chrono::seconds(1)))
        {
          auto now = std::chrono::high_resolution_clock().now();
          auto dur = std::chrono::duration_cast<std::chrono::seconds> (now - lastTime);
          if (dur.count() >= THREAD_MAX_IDLE_TIME && curThreadSize_ > initThreadSize_)
          {
            //记录线程数量的值修改
            //线程对象从线程列表容器中删除

          }
        }
      }
    }
    else
    {
      //等待notEmpty不为空的条件
      notEmpty_.wait(lock, [&]()->bool {return taskQue_.size() > 0;});
    }

    
    idleThreadSize_--;

    std::cout<<"tid: "<<std::this_thread::get_id()<<" 获取任务成功"<<std::endl;
    //从任务队列中取出来一个任务
    task =  taskQue_.front();
    taskQue_.pop();
    taskSize_--;

    //如果依然有剩余任务，通知其他线程
    if (taskQue_.size() > 0) 
    {
      notEmpty_.notify_all();
    }

    //取出一个任务进行通知
    notFull_.notify_all();
    //释放锁
    }

    if (task != nullptr) 
    {
      task->exec();
    }
    //当前线程负责执行这个任务
    
    idleThreadSize_++;

    lastTime = std::chrono::high_resolution_clock().now();

  }

}

//////////////////////////////////线程方法

int Thread::generateId_ = 0;

//线程构造
Thread::Thread(ThreadFunc func)
  : func_(func)
  , threadId_(generateId_++)
{}

//线程析构
Thread::~Thread()
{}

//启动线程
void Thread::start()
{
  std::thread t(func_, threadId_);
  t.detach();
}

int Thread::getId() const 
{ 
  return threadId_;
}

//////////////////////////////////result

Result::Result(std::shared_ptr<Task> task, bool isValid)
  : isValid_(isValid)
  , task_(task)
{
  task_->setResult(this);
}

Any Result::get()
{
  if (!isValid_)
  {
    return "";
  }

  sem_.wait();

  return std::move(any_);
}

void Result::setVal(Any any)
{

  this->any_ = std::move(any);
  sem_.post();
}



//////////////////////////////////task

Task::Task()
    : result_(nullptr)
{}

void Task::exec()
{
  if (result_ != nullptr)
  {
    result_->setVal(run());
  }
  
}

void Task::setResult(Result *res)
{
  result_ = res;
}