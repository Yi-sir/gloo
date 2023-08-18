#include "gloo/sophon_allreduce_ring.h"

#include <cstring>

#include "gloo/algorithm.h"
#include "gloo/sophon_collectives_host.h"

namespace gloo {

template <typename T>
SophonAllreduceRing<T>::SophonAllreduceRing(
    const std::shared_ptr<Context>& context,
    const std::vector<std::shared_ptr<SophonDeviceMem>>& mems, const int count,
    const std::vector<SophonStream>& streams)
    : Algorithm(context),
      count_(count),
      bytes_(mems[0]->bytes_),
      fn_(SophonReductionFunction<T>::sum),
      deviceScratch_(mems[0]->dev_id_) {
  int n = mems.size();
  for (int i = 0; i < n; ++i) {
    deviceMems_.push_back(mems[i]);
    streams_.push_back(streams[i]);
  }
  init();
  if (this->contextSize_ == 1) return;

  auto& leftPair = this->getLeftPair();
  auto& rightPair = this->getRightPair();
  auto slot = this->context_->nextSlot();

  // Buffer to send to (rank+1).
  sendDataBuf_ = rightPair->createSendBuffer(slot, outbox_, bytes_);

  // Buffer that (rank-1) writes to.
  recvDataBuf_ = leftPair->createRecvBuffer(slot, inbox_, bytes_);

  auto notificationSlot = this->context_->nextSlot();
  sendNotificationBuf_ =
      leftPair->createSendBuffer(notificationSlot, &dummy_, sizeof(dummy_));
  recvNotificationBuf_ =
      rightPair->createRecvBuffer(notificationSlot, &dummy_, sizeof(dummy_));
}

template <typename T>
void SophonAllreduceRing<T>::run() {
  if (localReduceOp_) {
    localReduceOp_->run();  // SophonLocalMemcpy->copyAsync->wait
  }

  SophonStream& stream = *scratchStream_;

  stream.copySync(outbox_, hostScratch_, count_);
  stream.wait();

  int numRounds = this->contextSize_ - 1;
  for (int round = 0; round < numRounds; ++round) {
    // outbox_发出去
    sendDataBuf_->send();
    // inbox_收到
    recvDataBuf_->waitRecv();
    // inbox_更新到hostScratch，做加法
    fn_->call(hostScratch_, inbox_, count_);

    sendDataBuf_->waitSend();
    // inbox_更新到outbox_，做下一次传递
    if (round < (numRounds - 1)) {
      memcpy(outbox_, inbox_, bytes_);
    }
    sendNotificationBuf_->send();
    recvNotificationBuf_->waitRecv();
  }

  if (localBroadcastOp_) {
    // 更新，把hostScratch_放到deviceMem
    localBroadcastOp_->run();
  }
}

template <typename T>
void SophonAllreduceRing<T>::init() {
  deviceScratch_.requestHandle();
  deviceScratch_.allocMem(count_, deviceMems_[0]->type_);

  scratchStream_ = &streams_[0];

  hostScratch_ = new T[count_];

  localReduceOp_ =
      SophonHostReduce(streams_, deviceMems_, hostScratch_, fn_, 0, count_);
  localBroadcastOp_ =
      SophonHostBroadcast(streams_, deviceMems_, hostScratch_, 0, count_);

  inbox_ = new T[count_];
  outbox_ = new T[count_];
}

#define INSTANTIATE_TEMPLATE(T) template class SophonAllreduceRing<T>;

// INSTANTIATE_TEMPLATE(int8_t);
INSTANTIATE_TEMPLATE(int);
// INSTANTIATE_TEMPLATE(float);

}  // namespace gloo