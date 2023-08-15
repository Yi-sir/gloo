#include "gloo/sophon_allreduce_ring.h"

namespace gloo {

template <typename T>
SophonAllreduceRing<T>::SophonAllreduceRing(
    const std::shared_ptr<Context>& context,
    const std::vector<SophonDeviceMem>& mems, const int count)
    : Algorithm(context),
      count_(count),
      bytes_(mems[0].bytes_),
      fn_(SophonReductionFunction<T>::sum),
      deviceScratch_(mems[0].dev_id_) {
  int n = mems.size();
  for (int i = 0; i < n; ++i) {
    deviceMems_.push_back(mems[i]);
  }
  init();
  if (this->contextSize_ == 1) return;

  auto& leftPair = this->getLeftPair();
  auto& rightPair = this->getRightPair();
  auto slot = this->context_->nextSlot();

  // Buffer to send to (rank+1).
  sendDataBuf_ = rightPair->createSendBuffer(slot, *outbox_, bytes_);

  // Buffer that (rank-1) writes to.
  recvDataBuf_ = leftPair->createRecvBuffer(slot, *inbox_, bytes_);

  auto notificationSlot = this->context_->nextSlot();
  sendNotificationBuf_ =
      leftPair->createSendBuffer(notificationSlot, &dummy_, sizeof(dummy_));
  recvNotificationBuf_ =
      rightPair->createRecvBuffer(notificationSlot, &dummy_, sizeof(dummy_));
}


template <typename T>
void SophonAllreduceRing<T>::init() {
    deviceScratch_.requestHandle();
    deviceScratch_.allocMem(count_, deviceMems_[0].type_);

    hostScratch_ = new T[count_];
    
}

}  // namespace gloo