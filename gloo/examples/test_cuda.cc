#include <numeric>
#include "gloo/cuda_allreduce_ring.h"
#include "gloo/transport/tcp/device.h"
#include "gloo/rendezvous/context.h"
#include "gloo/rendezvous/file_store.h"
#include "gloo/cuda_private.h"

int cudaNumDevices() {
  int n = 0;
  cudaGetDeviceCount(&n);
  return n;
}

using namespace gloo;

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        std::cout << argv[0] << " <rank> <size>" << std::endl;
        return 1;
    }

    int rank = std::stoi(argv[1]);
    int size = std::stoi(argv[2]);

    auto store = gloo::rendezvous::FileStore("/tmp");

    auto device = ::gloo::transport::tcp::CreateDevice("localhost");
    auto context =
        std::make_shared<::gloo::rendezvous::Context>(rank, size);
    context->connectFullMesh(store, device);

    std::vector<CudaMemory<float>> cudaSrcs;
    std::vector<CudaDevicePointer<float>> cudaPtrs;
    std::vector<CudaStream> cudaStreams;
    std::vector<cudaStream_t> streams;
    std::vector<float *> ptrs;

    size_t ptr_num = 1, count = 4;
    for (int i = 0; i < ptr_num; ++i)
    {
        CudaDeviceScope scope(i % cudaNumDevices());
        cudaSrcs.push_back(CudaMemory<float>(count));
        cudaPtrs.push_back(CudaDevicePointer<float>::create(*cudaSrcs.back(), count));
        cudaStreams.push_back(CudaStream(i));

        streams.push_back(cudaStreams.back().getStream());
        ptrs.push_back(*cudaPtrs.back());
    }

    std::vector<float> input(count);
    std::iota(input.begin(), input.end(), 0);
    for (auto i = 0; i < cudaSrcs.size(); i++) {
      CudaDeviceScope scope(cudaStreams[i].getDeviceID());
      CUDA_CHECK(cudaMemcpyAsync(
          *cudaSrcs[i],
          input.data(),
          cudaSrcs[i].bytes,
          cudaMemcpyHostToDevice,
          *cudaStreams[i]));
    }
    // Synchronize every stream to ensure the memory copies have completed.
    for (auto i = 0; i < cudaSrcs.size(); i++) {
      CudaDeviceScope scope(cudaStreams[i].getDeviceID());
      CUDA_CHECK(cudaStreamSynchronize(*cudaStreams[i]));
    }

    auto algorithm = std::unique_ptr<::gloo::Algorithm>(
        new ::gloo::CudaAllreduceRing<float>(context, ptrs, count, streams));
    algorithm->run();

    std::vector<float> output(count);
    for (auto i = 0; i < cudaSrcs.size(); i++) {
        CUDA_CHECK(cudaMemcpyAsync(
            output.data(),
            *cudaSrcs[i],
            cudaSrcs[i].bytes,
            cudaMemcpyDeviceToHost,
            *cudaStreams[i]));
    }

    for (const auto& stream : cudaStreams) {
        CudaDeviceScope scope(stream.getDeviceID());
        CUDA_CHECK(cudaStreamSynchronize(stream.getStream()));
    }

    for (int i = 0; i < count; ++i)
    {
        std::cout << output[i] << std::endl;
    }

    return 0;
}
