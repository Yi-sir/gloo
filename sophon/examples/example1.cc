/**
 * Copyright (c) Facebook, Inc. and its affiliates.
 */

#include <iostream>
#include <memory>

#include "sophon/allreduce_ring.h"
#include "sophon/rendezvous/context.h"
#include "sophon/rendezvous/file_store.h"
#include "sophon/rendezvous/prefix_store.h"
#include "sophon/rendezvous/redis_store.h"
#include "sophon/transport/tcp/device.h"

// Usage:
//
// Open two machines. Run the same program in both machines, using
// a different RANK in each. For example:
//
// A: SIZE=2 RANK=0 example1
// B: SIZE=2 RANK=1 example1
//
// Expected output:
//
//   data[0] = 0
//   data[1] = 2
//   data[2] = 4
//   data[3] = 6
//

int main(void) {
  if (getenv("SIZE") == nullptr || getenv("RANK") == nullptr) {
    std::cerr << "Please set environment variables PREFIX, SIZE, and RANK."
              << std::endl;
    return 1;
  }
  sophon::transport::tcp::attr attr;
  attr.iface = "enp5s0";
  attr.hostname = "172.26.13.15";  // 好像没用

  // attr.ai_family = AF_INET; // Force IPv4
  // attr.ai_family = AF_INET6; // Force IPv6
  attr.ai_family = AF_UNSPEC;  // Use either (default)

  // 构造Device，初始化loop_,listener_,interfaceName,interfaceSpeedMbps,picBusId,即网络相关信息
  auto dev = sophon::transport::tcp::CreateDevice(attr);

  // store应该被理解为“储存介质”，每个通信对的信息被编码，通过储存介质在分布式系统中传递
  auto redisStore = sophon::rendezvous::RedisStore("172.26.13.15");

  const int rank = atoi(getenv("RANK"));
  const int size = atoi(getenv("SIZE"));
  auto context = std::make_shared<sophon::rendezvous::Context>(rank, size);

  context->connectFullMesh(redisStore, dev);  // 建立连接

  std::array<int, 4> data;
  std::cout << "Input: " << std::endl;
  for (int i = 0; i < data.size(); i++) {
    data[i] = i;
    std::cout << "data[" << i << "] = " << data[i] << std::endl;
  }

  // 注意这里的int是AllReduceRing中ptrs的模板，用于根据sizeof(T)获取占用的大小
  std::vector<int*> ptrs;
  ptrs.push_back(&data[0]);

  // The number of elements at the specified pointer.
  int count = data.size();

  // AllReduceRing的第四个参数默认是sum
  auto allreduce =
      std::make_shared<sophon::AllreduceRing<int>>(context, ptrs, count);

  // Run the algorithm.
  allreduce->run();

  // Print the result.
  std::cout << "Output: " << std::endl;
  for (int i = 0; i < data.size(); i++) {
    std::cout << "data[" << i << "] = " << data[i] << std::endl;
  }

  return 0;
}
