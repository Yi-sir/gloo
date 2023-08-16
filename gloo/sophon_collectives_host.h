#pragma once

#include "gloo/common/common.h"
#include "gloo/common/logging.h"
#include "gloo/sophon.h"

namespace gloo {

// template <typename T>
// class SophonLocalHostReduce

template <typename T>
std::unique_ptr<LocalOp<T>> SophonHostReduce(
    std::vector<SophonStream>& streams,
    std::vector<SophonDeviceMem>& deviceMems, T* dst,
    const SophonReductionFunction<T>* fn, size_t offset, size_t count) {
  if (deviceMems.size() == 1) {
    return make_unique<SophonLocalMemcpy<T>>(streams[0], deviceMems[0], dst,
                                             offset, count);
  }
  // else {
  //     return make_unique
  // }
}

template <typename T>
std::unique_ptr<LocalOp<T>> SophonHostBroadcast(
    std::vector<SophonStream>& streams,
    std::vector<SophonDeviceMem>& deviceMems, T* src, size_t offset,
    size_t count) {
  if (deviceMems.size() == 1) {
    return make_unique<SophonLocalMemcpy<T>>(streams[0], src, deviceMems[0],
                                             offset, count);
  }
}

}  // namespace gloo