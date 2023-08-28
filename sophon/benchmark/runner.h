/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>

#include "sophon/algorithm.h"
#include "sophon/barrier.h"
#include "sophon/benchmark/benchmark.h"
#include "sophon/benchmark/options.h"
#include "sophon/benchmark/timer.h"
#include "sophon/config.h"
#include "sophon/rendezvous/context.h"
#include "sophon/transport/device.h"

namespace sophon {
namespace benchmark {

// Forward declaration
class RunnerThread;

// RunnerJob holds the state associated with repetetive calls of an arbitrary
// function (which is typically equal to the benchmark function).
class RunnerJob {
 public:
  explicit RunnerJob(std::function<void()> fn, int i) :
    done_(false), fn_(fn), iterations_(i) {}

  const Samples& getSamples() const {
    return samples_;
  }

  void wait() {
    std::unique_lock<std::mutex> lock(mutex_);
    while (!done_) {
      cond_.wait(lock);
    }
  }

 protected:
  void done() {
    std::unique_lock<std::mutex> lock(mutex_);
    done_ = true;
    cond_.notify_all();
  }

  bool done_;
  std::mutex mutex_;
  std::condition_variable cond_;

  std::function<void()> fn_;
  int iterations_;
  Samples samples_;

  friend class RunnerThread;
};

// RunnerThread takes a RunnerJob and runs the function a number of times,
// keeping track of its runtime in the job's member variable `samples_`. Upon
// completion, it signals the job using the `done()` function.
class RunnerThread {
 public:
  RunnerThread();
  ~RunnerThread();

  void run(RunnerJob* job);

 protected:
  void spawn();

  bool stop_;
  RunnerJob* job_;
  std::mutex mutex_;
  std::condition_variable cond_;
  std::thread thread_;
};

class Runner {
 public:
  template <typename T>
  using BenchmarkFn = std::function<std::unique_ptr<Benchmark<T>>(
      std::shared_ptr<::sophon::Context>&)>;

  explicit Runner(const options& options);
  ~Runner();

  template <typename T>
  void run(BenchmarkFn<T>& fn);

  template <typename T>
  void run(BenchmarkFn<T>& fn, size_t n);

  template <typename T>
  Samples createAndRun(
    std::vector<std::unique_ptr<Benchmark<T>>> &benchmarks,
    int niters
  );

 protected:
#if SOPHON_USE_REDIS
  void rendezvousRedis();
#endif

#if SOPHON_USE_MPI
  void rendezvousMPI();
#endif

  void rendezvousFileSystem();

  long broadcast(long value);

  std::shared_ptr<Context> newContext();

  void printHeader();
  void printDistribution(
      size_t elements,
      size_t elementSize,
      const Distribution& samples);
  void printVerifyHeader();
  void printFooter();

  // Checks and prints errors, exits the program with
  // status 1 if any errors were found
  void checkErrors();

  options options_;
  std::vector<std::shared_ptr<transport::Device>> transportDevices_;
  std::shared_ptr<rendezvous::ContextFactory> contextFactory_;
  std::vector<std::string> keyFilePaths_;
  std::vector<std::unique_ptr<RunnerThread>> threads_;

  long broadcastValue_;
  std::unique_ptr<Algorithm> broadcast_;
  std::unique_ptr<Barrier> barrier_;

  std::vector<std::string> mismatchErrors_;
};

} // namespace benchmark
} // namespace sophon
