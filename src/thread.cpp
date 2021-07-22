#include "thread.h"

#include <errno.h>
#include <linux/unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

#include <atomic>
#include <thread>
#include <type_traits>

#include "logging.h"
#include "timestamp.h"

namespace {
pid_t gettid() { return static_cast<pid_t>(syscall(SYS_gettid)); }

void afterFork() {
  // Need to set t_cachedTid everytime a new thread is forked otherwise it is
  // undefined
  CurrentThread::t_cachedTid = 0;
  CurrentThread::t_threadName = "main";
  CurrentThread::tid();
}
}  // namespace

namespace CurrentThread {
thread_local int t_cachedTid = 0;
thread_local const char* t_threadName = "default";

pid_t tid() {
  if (t_cachedTid == 0) {
    t_cachedTid = gettid();
    return t_cachedTid;
  }
}

const char* name() { return t_threadName; }

bool isMainThread() { return tid() == getpid(); }
}  // namespace CurrentThread

class ThreadNameInitializer {
 public:
  ThreadNameInitializer() {
    // Only initialize t_cachedTid in this thread, so in afterFork() t_cachedTid
    // needs to be initialized
    CurrentThread::t_cachedTid = 0;
    CurrentThread::t_threadName = "main";
    // Register afterFork to explicitly fork()-ed process
    pthread_atfork(NULL, NULL, &afterFork);
  }
};

// ThreadNameInitializer init;

/* Useful data of each created thread */
struct ThreadData {
  using ThreadFunc = Thread::ThreadFunc;
  ThreadFunc func_;
  std::string name_;
  // Cache tid to prevent trap into kernel every time
  pid_t* tid_;
  CountDownLatch* latch_;

  ThreadData(ThreadFunc func, const std::string& name, pid_t* tid,
             CountDownLatch* latch)
      : func_(std::move(func)), name_(name), tid_(tid), latch_(latch) {}

  void runInThread() {
    *tid_ = CurrentThread::tid();
    tid_ = nullptr;
    latch_->countDown();
    latch_ = nullptr;

    CurrentThread::t_threadName = name_.empty() ? "Thread" : name_.c_str();
    prctl(PR_SET_NAME, CurrentThread::t_threadName);
    // Invoke IOThreadFunc()
    func_();
    CurrentThread::t_threadName = "finished";
    // try {
    //   func_();
    //   CurrentThread::t_threadName = "finished";
    // } catch (const Exception& ex) {
    //   CurrentThread::t_threadName = "crashed";
    //   fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
    //   fprintf(stderr, "reason: %s\n", ex.what());
    //   fprintf(stderr, "stack trace: %s\n", ex.stackTrace());
    //   abort();
    // } catch (const std::exception& ex) {
    //   CurrentThread::t_threadName = "crashed";
    //   fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
    //   fprintf(stderr, "reason: %s\n", ex.what());
    //   abort();
    // } catch (...) {
    //   fprintf(stderr, "unknown exception caught in Thread %s\n",
    //   name_.c_str()); throw;  // rethrow
    // }
  }
};

void* startThread(void* obj) {
  ThreadData* data = static_cast<ThreadData*>(obj);
  data->runInThread();
  // FIXME: use unique_ptr to manage ThreadData
  delete data;
  return nullptr;
}

std::atomic<int> Thread::num_created_{0};

Thread::Thread(const ThreadFunc& func, const std::string& n)
    : started_(false),
      joined_(false),
      pthread_id_(0),
      tid_(0),
      func_(std::move(func)),
      name_(n),
      latch_(1) {
  num_created_++;
}

Thread::~Thread() {
  if (started_ && !joined_) {
    pthread_detach(pthread_id_);
  }
}

void Thread::setDefaultName() {
  if (name_.empty()) {
    char buf[32];
    snprintf(buf, sizeof buf, "Thread%d", static_cast<int>(num_created_));
    name_ = buf;
  }
}

void Thread::start() {
  assert(!started_);
  started_ = true;
  // FIXME: move(func_)
  ThreadData* thread_data = new ThreadData(func_, name_, &tid_, &latch_);
  if (pthread_create(&pthread_id_, NULL, &startThread, thread_data)) {
    started_ = false;
    delete thread_data;
    LOG << "Failed in pthread_create";
  } else {
    // Wait until startThread starts
    latch_.wait();
    assert(tid_ > 0);
  }
}

int Thread::join() {
  assert(started_);
  assert(!joined_);
  joined_ = true;
  return pthread_join(pthread_id_, NULL);
}
