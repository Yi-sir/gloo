#include <numeric>

#include "sophon/rendezvous/context.h"
#include "sophon/rendezvous/file_store.h"
#include "sophon/rendezvous/redis_store.h"
#include "sophon/sophon_allreduce_ring.h"
#include "sophon/sophon.h"
#include "sophon/transport/tcp/device.h"

int sophonNumDevices() {
  int n = 0;
  bm_dev_getcount(&n);
  return n;
}

int main(int argc, char* argv[]) {
  if (argc != 3) {
    std::cout << argv[0] << " <rank> <size>" << std::endl;
    return 1;
  }

  int availableSophonDevices = sophonNumDevices();
  std::cout << "Sophon Device Num is " << availableSophonDevices << std::endl;
  if (availableSophonDevices == 0) {
    std::cout << "============== No Sophon Devices is Available! ============="
              << std::endl;
    return 1;
  }

  int rank = std::stoi(argv[1]);
  int size = std::stoi(argv[2]);

  // 多机通信，需要开启redis服务，ip需要参考当前网络环境进行修改
  auto store = sophon::rendezvous::RedisStore("172.26.13.171");

  // 单机多进程通信，可以使用redis服务，也可以使用本地文件系统
  //   auto store = sophon::rendezvous::FileStore("/tmp");

  // 多机通信，attr需要按照本机ip进行配置
  sophon::transport::tcp::attr attr;
  attr.iface = "eno1";
  attr.hostname = "172.26.13.180";

  // 多机通信，device构造参数需要使用attr；单机通信可以使用"localhost"，该情况下无法多机通信
  auto device = sophon::transport::tcp::CreateDevice(attr);
  auto context = std::make_shared<sophon::rendezvous::Context>(rank, size);
  context->connectFullMesh(store, device);

  // 申请设备内存，构造sophon::SophonAllreduceRing
  // sophon::SophonDeviceMem dev(0);  // device id

  std::shared_ptr<sophon::SophonDeviceMem> dev(new sophon::SophonDeviceMem(0));
  int count = 4;

  // 初始化handle和设备内存
  dev->requestHandle();
  dev->allocMem(count, sophon::SophonMemType::INT);
  int* input = new int[count];
  std::iota(input, input + 4, 0);
  dev->updateMem(input);

  std::vector<std::shared_ptr<sophon::SophonDeviceMem>> mems;
  mems.push_back(dev);

  std::vector<sophon::SophonStream> streams;
  streams.push_back(sophon::SophonStream(0, dev->handle_));

  auto algorithm = std::unique_ptr<sophon::Algorithm>(
      new sophon::SophonAllreduceRing<int>(context, mems, count, streams));
  algorithm->run();

  int* output = new int [count];
  dev->updateHost((void*)output);

  for(int i = 0; i < count; ++i) {
    std::cout << output[i] << " ";
  }
  std::cout << std::endl;

  return 0;
}