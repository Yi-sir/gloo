#pragma once

#include "gloo/algorithm.h"
#include "gloo/sophon.h"

namespace gloo {

template <typename T>
class SophonAllreducingRing : public Algorithm {
 public:
  SophonAllreducingRing(const std::shared_ptr<Context>& context,
                        const std::vector<T*>& ptrs, const int count)
      : Algorithm(context),
        count_(count),
        bytes_(count * sizeof(T)),
        fn_(SophonReductionFunction<T>::sum) {}

  virtual ~SophonAllreducingRing() = default;

  virtual void run() override;

 protected:
  void init();
  std::vector<SophonDevicePointer<T>> devicePtrs_;
  SophonDevicePointer<T> scratch_;

  const int count_;
  const int bytes_;
  const SophonReductionFunction<T>* fn_;

  std::unique_ptr<LocalOp<T>> localReduceOp_;
  std::unique_ptr<LocalOp<T>> localBroadcastOp_;

  SophonDevicePointer<T> inbox_;
  SophonDevicePointer<T> outbox_;
  std::unique_ptr<transport::Buffer> sendDataBuf_;
  std::unique_ptr<transport::Buffer> recvDataBuf_;

  int dummy_;
  std::unique_ptr<transport::Buffer> sendNotificationBuf_;
  std::unique_ptr<transport::Buffer> recvNotificationBuf_;
};

}  // namespace gloo