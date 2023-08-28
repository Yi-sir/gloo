#include <numeric>

#include "sophon/cuda_allreduce_ring.h"
#include "sophon/cuda_private.h"
#include "sophon/rendezvous/context.h"
#include "sophon/rendezvous/file_store.h"
#include "sophon/transport/tcp/device.h"

int cudaNumDevices() {
  int n = 0;
  cudaGetDeviceCount(&n);
  return n;
}

using namespace sophon;

int main(int argc, char* argv[]) {
  if (argc != 3) {
    std::cout << argv[0] << " <rank> <size>" << std::endl;
    return 1;
  }

  int availableCudaDevices = cudaNumDevices();
  std::cout << "Cuda Device Num is " << availableCudaDevices << std::endl;
  if(availableCudaDevices == 0) {
    std::cout << "============== No Cuda Devices is Available! =============" << std::endl;
    return 1;
  }

  int rank = std::stoi(argv[1]);
  int size = std::stoi(argv[2]);

  auto store = sophon::rendezvous::FileStore("/tmp");

  auto device = ::sophon::transport::tcp::CreateDevice("localhost");
  auto context = std::make_shared<::sophon::rendezvous::Context>(rank, size);
  context->connectFullMesh(store, device);

  std::vector<CudaMemory<float>> cudaSrcs;
  std::vector<CudaDevicePointer<float>> cudaPtrs;
  std::vector<CudaStream> cudaStreams;
  std::vector<cudaStream_t> streams;
  std::vector<float*> ptrs;

  size_t ptr_num = 1, count = 4;
  for (int i = 0; i < ptr_num; ++i) {
    CudaDeviceScope scope(i % cudaNumDevices());
    // *cudaSrcs[i]，返回的是设备内存中的地址，float*
    cudaSrcs.push_back(CudaMemory<float>(count));
    // 调用CudaDevicePointer的构造函数，device_是输入的ptr_，即设备内存地址；count_为count；deviceId_是从ptr_获取的设备编号
    cudaPtrs.push_back(
        CudaDevicePointer<float>::create(*cudaSrcs.back(), count));
    // 调用CudaStream的构造，i是dev_id
    cudaStreams.push_back(CudaStream(i));

    streams.push_back(cudaStreams.back().getStream());
    ptrs.push_back(*cudaPtrs.back());
  }

  std::vector<float> input(count);
  // 0 1 2 3
  std::iota(input.begin(), input.end(), 0);
  for (auto i = 0; i < cudaSrcs.size(); i++) {
    CudaDeviceScope scope(cudaStreams[i].getDeviceID());
    // cudaMemcpyAsync: void* dst, const void* src, size_t count, cudaMemcpyKind kind
    // 其中第四个参数是memcpy的方向，这里是把系统内存cp到设备内存
    CUDA_CHECK(cudaMemcpyAsync(*cudaSrcs[i], input.data(), cudaSrcs[i].bytes,
                               cudaMemcpyHostToDevice, *cudaStreams[i]));
  }
  // Synchronize every stream to ensure the memory copies have completed.
  for (auto i = 0; i < cudaSrcs.size(); i++) {
    CudaDeviceScope scope(cudaStreams[i].getDeviceID());
    CUDA_CHECK(cudaStreamSynchronize(*cudaStreams[i]));
  }
  // 这里的三个参数，特殊的是ptrs和streams。ptrs是指向一个cuda地址的指针，streams是cudaStream_t的vector
  auto algorithm = std::unique_ptr<::sophon::Algorithm>(
      new ::sophon::CudaAllreduceRing<float>(context, ptrs, count, streams));
  algorithm->run();

  std::vector<float> output(count);
  for (auto i = 0; i < cudaSrcs.size(); i++) {
    CUDA_CHECK(cudaMemcpyAsync(output.data(), *cudaSrcs[i], cudaSrcs[i].bytes,
                               cudaMemcpyDeviceToHost, *cudaStreams[i]));
  }

  for (const auto& stream : cudaStreams) {
    CudaDeviceScope scope(stream.getDeviceID());
    CUDA_CHECK(cudaStreamSynchronize(stream.getStream()));
  }

  for (int i = 0; i < count; ++i) {
    std::cout << output[i] << std::endl;
  }

  return 0;
}
