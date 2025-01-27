/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <sophon/transport/tcp/loop.h>

#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include <array>

#include <sophon/common/error.h>
#include <sophon/common/logging.h>

#if defined(__SANITIZE_THREAD__)
#define TSAN_ENABLED
#elif defined(__has_feature)
#if __has_feature(thread_sanitizer)
#define TSAN_ENABLED
#endif
#endif

#ifdef TSAN_ENABLED
#define TSAN_ANNOTATE_HAPPENS_BEFORE(addr) \
    AnnotateHappensBefore(__FILE__, __LINE__, (void*)(addr))
#define TSAN_ANNOTATE_HAPPENS_AFTER(addr) \
    AnnotateHappensAfter(__FILE__, __LINE__, (void*)(addr))
extern "C" void AnnotateHappensBefore(const char* f, int l, void* addr);
extern "C" void AnnotateHappensAfter(const char* f, int l, void* addr);
#else
#define TSAN_ANNOTATE_HAPPENS_BEFORE(addr)
#define TSAN_ANNOTATE_HAPPENS_AFTER(addr)
#endif

namespace sophon {
namespace transport {
namespace tcp {

Deferrables::Deferrables() {
  std::array<int, 2> fds;
  auto rv = pipe2(fds.data(), O_NONBLOCK);
  SOPHON_ENFORCE_NE(rv, -1, "pipe: ", strerror(errno));
  rfd_ = fds[0];
  wfd_ = fds[1];
}

Deferrables::~Deferrables() {
  close(rfd_);
  close(wfd_);
}

void Deferrables::defer(function_t fn) {
  std::lock_guard<std::mutex> guard(mutex_);
  functions_.push_back(std::move(fn));

  // Write byte to pipe to make epoll(2) wake up.
  if (!triggered_) {
    for (;;) {
      char byte = 0;
      auto rv = write(wfd_, &byte, sizeof(byte));
      if (rv == -1 && errno == EINTR) {
        continue;
      }
      SOPHON_ENFORCE_NE(rv, -1, "write: ", strerror(errno));
      break;
    }
    triggered_ = true;
  }
}

void Deferrables::handleEvents(int /* unused */) {
  decltype(functions_) localFunctions;

  {
    std::lock_guard<std::mutex> guard(mutex_);
    std::swap(localFunctions, functions_);

    // Read byte from pipe to drain it.
    for (;;) {
      char byte = 0;
      auto rv = read(rfd_, &byte, sizeof(byte));
      if (rv == -1 && errno == EINTR) {
        continue;
      }
      SOPHON_ENFORCE_NE(rv, -1, "read: ", strerror(errno));
      break;
    }
    triggered_ = false;
  }

  // Execute deferred functions.
  for (auto fn : localFunctions) {
    fn();
  }
}

Loop::Loop() {
  fd_ = epoll_create(1);
  SOPHON_ENFORCE_NE(fd_, -1, "epoll_create: ", strerror(errno));
  loop_.reset(new std::thread(&Loop::run, this));
  registerDescriptor(deferrables_.rfd_, EPOLLIN, &deferrables_);
}

Loop::~Loop() {
  if (loop_) {
    done_ = true;
    loop_->join();
  }
  if (fd_ >= 0) {
    close(fd_);
  }
}
// 向epoll实例注册一个文件描述符，监视特定的事件
void Loop::registerDescriptor(int fd, int events, Handler* h) {
  struct epoll_event ev;
  ev.events = events;
  ev.data.ptr = h;

  auto rv = epoll_ctl(fd_, EPOLL_CTL_ADD, fd, &ev); // 添加一个fd，关注的事件是event，关联的处理方法为handle
  if (rv == -1 && errno == EEXIST) {
    rv = epoll_ctl(fd_, EPOLL_CTL_MOD, fd, &ev); // 返回值是-1而且errno是EEXIST，说明fd已经存在，所以修改现有的fd 
  }
  SOPHON_ENFORCE_NE(rv, -1, "epoll_ctl: ", strerror(errno));
}

void Loop::unregisterDescriptor(int fd, Handler* h) {
  auto rv = epoll_ctl(fd_, EPOLL_CTL_DEL, fd, nullptr);
  SOPHON_ENFORCE_NE(rv, -1, "epoll_ctl: ", strerror(errno));

  // Wait for loop to tick before returning, to make sure the handler
  // for this fd is not called once this function returns.
  if (std::this_thread::get_id() != loop_->get_id()) {
    std::unique_lock<std::mutex> lock(m_);
    cv_.wait(lock);
    TSAN_ANNOTATE_HAPPENS_AFTER(h);
  }
}

void Loop::defer(std::function<void()> fn) {
  deferrables_.defer(std::move(fn));
}

void Loop::run() {
  std::array<struct epoll_event, capacity_> events;
  int nfds;

  while (!done_) {
    // Wakeup everyone waiting for a loop tick to finish.
    cv_.notify_all();

    // Wait for something to happen
    nfds = epoll_wait(fd_, events.data(), events.size(), 10); // 返回值是发生事件的文件描述符个数
    if (nfds == 0) {
      continue; // 超时，没有事件发生
    }
    if (nfds == -1 && errno == EINTR) {
      continue; // wait被中断
    }

    SOPHON_ENFORCE_NE(nfds, -1);

    for (int i = 0; i < nfds; i++) {
      Handler* h = reinterpret_cast<Handler*>(events[i].data.ptr);
      h->handleEvents(events[i].events);
      TSAN_ANNOTATE_HAPPENS_BEFORE(h);
    }
  }
}

} // namespace tcp
} // namespace transport
} // namespace sophon
