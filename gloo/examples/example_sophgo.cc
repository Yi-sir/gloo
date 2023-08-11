/**
 * Copyright (c) Facebook, Inc. and its affiliates.
 */

#include <iostream>
#include <memory>

#include "gloo/allreduce_ring.h"
#include "gloo/sophgo/context.h"
#include "gloo/sophgo/file_store.h"
#include "gloo/sophgo/prefix_store.h"
#include "gloo/sophgo/redis_store.h"
#include "gloo/transport/sophgo/device.h"
/**
 * @brief 本例程用于测试gloo在装有sophon设备的机器上的通信
 * 使用方法：
 * A: SIZE=2 RANK=0 DEVID=0 example_sophgo
 * B: SIZE=2 RANK=1 DEVID=1 example_sophgo
 * Expected output
 * data[0] = 0
 * data[1] = 2
 * data[2] = 4
 * data[3] = 6
 */

int main(void) {
  if (getenv("SIZE") == nullptr || getenv("RANK") == nullptr) {
    std::cerr << "Please set environment variables PREFIX, SIZE, and RANK."
              << std::endl;
    return 1;
  }
  gloo::transport::sophgo::attr attr;
  attr.iface = "enp5s0";
  attr.hostname = "172.26.13.15";  // 好像没用
  attr.ai_family = AF_UNSPEC;

  // 构造Device，初始化loop_,listener_,interfaceName,interfaceSpeedMbps,picBusId,即网络相关信息
  auto dev = gloo::transport::sophgo::CreateDevice(attr);

  // store应该被理解为“储存介质”，每个通信对的信息被编码，通过储存介质在分布式系统中传递
  auto redisStore = gloo::sophgo::RedisStore("172.26.13.171");

  const int rank = atoi(getenv("RANK"));
  const int size = atoi(getenv("SIZE"));
  const int dev_id = atoi(getenv("DEVID"));
  printf("Current dev id is %d\n", dev_id);

  auto context = std::make_shared<gloo::sophgo::Context>(rank, size, dev_id);

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
      std::make_shared<gloo::AllreduceRing<int>>(context, ptrs, count);

  // Run the algorithm.
  allreduce->run();

  // Print the result.
  std::cout << "Output: " << std::endl;
  for (int i = 0; i < data.size(); i++) {
    std::cout << "data[" << i << "] = " << data[i] << std::endl;
  }

  return 0;
}
