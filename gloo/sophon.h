#pragma once

#include <algorithm>
#include <atomic>
#include <mutex>

#include "bmlib_runtime.h"
#include "bmruntime_interface.h"
#include "gloo/algorithm.h"
#include "gloo/common/logging.h"
#include "gloo/config.h"

namespace gloo {

extern const int kInvalidDeviceId;

template <typename T>
class SophonDevicePointer;
template <typename T>
class SophonReductionFunction;

class SophonStream {
 public:
  explicit SophonStream(int deviceId, bm_handle_t handle);

  SophonStream(SophonStream&& other) noexcept;

  ~SophonStream() noexcept(false);

  int getDeviceID() const { return deviceId_; }
  bm_handle_t getHandle() const { return handle_; }

  template typename T >
      void copy(SophonDevicePointer<T>& dst, SophonDevicePointer<T>& src);

  template <typename T>
  void copy(SophonHostPointer<T>& dst, SophonHostPointer<T>& src);

  template <typename T>
  void copy(SophonHostPointer<T>& dst, SophonDevicePointer<T>& src);

  template <typename T>
  void copy(SophonDevicePointer<T>& dst, SophonHostPointer<T>& src);

 protected:
  SophonStream(const SophonStream&) = delete;
  SophonStream& operator=(const SophonStream&) = delete;

  int deviceId_;
  bm_handle_t handle_;
};

template <typename T>
class SophonDevicePointer {
 public:
  static SophonDevicePointer<T> alloc(int devid, size_t count);

//   static SophonDevicePointer<T> create(T* ptr, size_t count);

//   static SophonDevicePointer<T> create(const SophonDevicePointer<T>& ptr) {
//     return SophonDevicePointer<T>::create(*ptr, ptr.getCount());
//   }

  static SophonDevicePointer<T> create(bm_handle_t handle, bm_device_mem_t mem,
                                       int devid);

  int getDeviceID() const { return deviceId_; }
  bm_device_mem_t getMem() const { return device_; }
  bm_handle_t getHandle() const { return handle_; }

  SophonDevicePointer(SophonDevicePointer&&) noexcept;
  ~SophonDevicePointer();

  SophonDevicePointer()
      : device_(nullptr), count_(0), deviceId(kInvalidDeviceId) {}

  SophonDevicePointer& operator=(SophonDevicePointer&&);

  bool operator==(const SophonDevicePointer<T>& other) const {
    return device_.u.device.device_addr == other.device_u.device.device_addr &&
           count_ == other.count_;
  }

  bm_device_mem_t operator*() const { return device_; }

//   T& operator[](size_t index) const { return device_[index]; }

  int getCount() const { return count_; }

  int getDeviceID() const { return deviceId_; }

 protected:
  //   SophonDevicePointer(T* ptr, size_t count, bool owner);

  SophonDevicePointer(bm_handle_t handle, bm_device_mem_t mem, int devid);

  SophonDevicePointer(const SophonDevicePointer&) = delete;
  SophonDevicePointer& operator=(const SophonDevicePointer&) = delete;

  //   T* device_;

  bm_device_mem_t device_;
  bm_handle_t handle_;

  size_t count_;

  //   bool owner_ = false;

  int deviceId_;
};

template <typename T>
class SophonHostPointer {
 public:
  static SophonHostPointer<T> alloc(size_t count);

  static SophonHostPointer<T> create(T* ptr, size_t count);

  static SophonHostPointer<T> create(const SophonHostPointer<T>& ptr) {
    return SophonHostPointer<T>::create(*ptr, ptr.getCount());
  }

  int getDeviceID() const { return deviceId_; }
  T* getMem() const { return device_; }
  bm_handle_t getHandle() const { return handle_; }

  SophonHostPointer(SophonHostPointer&&) noexcept;
  ~SophonHostPointer();

  SophonHostPointer()
      : device_(nullptr), count_(0), deviceId(kInvalidDeviceId) {}

  SophonHostPointer& operator=(SophonHostPointer&&);

  bool operator==(const SophonHostPointer<T>& other) const {
    return device_ == other.device_ && count_ == other.count_;
  }

  T* operator*() const { return device_; }

  T& operator[](size_t index) const { return device_[index]; }

  int getCount() const { return count_; }

  int getDeviceID() const { return deviceId_; }

 protected:
  SophonHostPointer(T* ptr, size_t count, bool owner);

  SophonHostPointer(const SophonHostPointer&) = delete;
  SophonHostPointer& operator=(const SophonHostPointer&) = delete;

  T* host_;

  //   bm_device_mem_t device_;
  bm_handle_t handle_;

  size_t count_;

  bool owner_ = false;

  int deviceId_;
};

template <typename T>
void sophonSum(T* x, const T* y, size_t n);

template <typename T>
void sophonProduct(T* x, const T* y, size_t n);

template <typename T>
void sophonMax(T* x, const T* y, size_t n);

template <typename T>
void sophonMin(T* x, const T* y, size_t n);

template <typename T>
class SophonReductionFunction {
  using DeviceFunction = void(T*, const T*, size_t n);  // 可能需要改
  using HostFunction = void(T*, const T*, size_t n);

 public:
  static const SophonReductionFunction<T>* sum;
  static const SophonReductionFunction<T>* product;
  static const SophonReductionFunction<T>* min;
  static const SophonReductionFunction<T>* max;

  SophonReductionFunction(ReductionType type, DeviceFunction* deviceFn,
                          HostFunction* hostFn)
      : type_(type), deviceFn_(deviceFn), hostFn_(hostFn) {}
  ReductionType type() const { return type_; }

  void call(T* x, const T* y, size_t n) const { return; }

  void call(SophonDevicePointer<T>& dst, const SophonDevicePointer<T>& src,
            size_t n) const {
    deviceFn_(*dst, *src, n);
  }

 protected:
  const ReductionType type_;
  DeviceFunction* deviceFn_;
  HostFunction* hostFn_;

  friend class SophonDevicePointer<T>;
};

template <typename T>
const SophonReductionFunction<T>* SophonReductionFunction::sum =
    new SophonReductionFunction<T>(SUM, &::gloo::sophonSum<T>, &::gloo::sum<T>);

template <typename T>
const SophonReductionFunction<T>* SophonReductionFunction::product =
    new SophonReductionFunction<T>(PRODUCT, &::gloo::sophonProduct<T>,
                                   &::gloo::product<T>);

const SophonReductionFunction<T>* SophonReductionFunction::min =
    new SophonReductionFunction<T>(Min, &::gloo::sophonMin<T>, &::gloo::min<T>);

const SophonReductionFunction<T>* SophonReductionFunction::max =
    new SophonReductionFunction<T>(MAX, &::gloo::sophonMax<T>, &::gloo::max<T>);

}  // namespace gloo