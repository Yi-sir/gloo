#include <numeric>

#include "gloo/rendezvous/context.h"
#include "gloo/rendezvous/file_store.h"
#include "gloo/rendezvous/redis_store.h"
#include "gloo/sophgo_allreduce_ring.h"
#include "gloo/sophon.h"
#include "gloo/transport/tcp/device.h"


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

  auto store = gloo::rendezvous::RedisStore("172.26.13.171");
  //   auto store = gloo::rendezvous::FileStore("/tmp");

  auto device = gloo::transport::tcp::CreateDevice("localhost");
  auto context = std::make_shared<gloo::rendezvous::Context>(rank, size);
  context->connectFullMesh(store, device);

  // 申请设备内存，构造gloo::SophonAllreduceRing
  SophonDeviceMem dev(0); // device id
  dev.requestHandle();
  dev.allocMem(4, SophonMemType::INT);


  int input[4] = {0,1,2,3};
  dev.updateMem(input);

  auto algorithm = std::unique_ptr<gloo::Algorithm>(new gloo::SophonAllreducRing<int>(context, ))
}