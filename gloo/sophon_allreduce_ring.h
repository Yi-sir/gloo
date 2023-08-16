#pragma once

#include "gloo/algorithm.h"
#include "gloo/sophon.h"

namespace gloo {

template <typename T>
class SophonAllreduceRing : public Algorithm {
 public:
  SophonAllreduceRing(const std::shared_ptr<Context>& context,
                        const std::vector<SophonDeviceMem>& mems, const int count,
                        const std::vector<SophonStream>& streams);

  virtual ~SophonAllreduceRing() = default;

  virtual void run() override;

 protected:
  void init();
  // std::vector<SophonDevicePointer<T>> devicePtrs_;
  // SophonDevicePointer<T> scratch_;
  std::vector<SophonDeviceMem> deviceMems_;
  std::vector<SophonStream> streams_
  SophonDeviceMem deviceScratch_;
  SophonStream* scratchStream_;
  T* hostScratch_;

  const int count_;
  const int bytes_;
  const SophonReductionFunction<T>* fn_;

  std::unique_ptr<LocalOp<T>> localReduceOp_;
  std::unique_ptr<LocalOp<T>> localBroadcastOp_;

  T* inbox_;
  T* outbox_;

  // SophonDevicePointer<T> inbox_;
  // SophonDevicePointer<T> outbox_;
  std::unique_ptr<transport::Buffer> sendDataBuf_;
  std::unique_ptr<transport::Buffer> recvDataBuf_;

  int dummy_;
  std::unique_ptr<transport::Buffer> sendNotificationBuf_;
  std::unique_ptr<transport::Buffer> recvNotificationBuf_;
};

}  // namespace gloo