#include "gloo/sophon.h"

#include <cstring>

#include "bmlib_runtime.h"
#include "bmruntime_interface.h"

namespace gloo {

const int kInvalidDeviceId = -1;

void SophonDeviceMem::requestHandle() { bm_dev_request(&handle_, dev_id_); }

SophonDeviceMem::~SophonDeviceMem() {
  bm_free_device(handle_, mem_);
  bm_dev_free(handle_);
}

void SophonDeviceMem::allocMem(int count, SophonMemType type) {
  count_ = count;
  type_ = type;
  if (handle_ != nullptr) {
    int sz = 0;
    switch (type_) {
      case SophonMemType::INT8:
        sz = count_ * sizeof(uint8_t);
        break;
      case SophonMemType::INT:
        sz = count_ * sizeof(int);
        break;
      case SophonMemType::FP32:
        sz = count_ * sizeof(_Float32);
        break;
      default:
        sz = 1;
    }
    bytes_ = sz;
    bm_malloc_device_byte(handle_, &mem_, sz);
  }
}

void SophonDeviceMem::updateMem(void* input) {
  bm_memcpy_s2d(handle_, mem_, input);
}

void SophonDeviceMem::updateHost(void* output) {
  bm_memcpy_d2s(handle_, output, mem_);
}

SophonStream::SophonStream(int deviceId, bm_handle_t handle) {
  deviceId_ = deviceId;
  handle_ = handle;
}

SophonStream::SophonStream(const SophonStream& other) {
  deviceId_ = other.deviceId_;
  handle_ = other.handle_;
}

SophonStream::SophonStream(SophonStream&& other) noexcept {
  deviceId_ = other.deviceId_;
  handle_ = other.handle_;

  other.deviceId_ = kInvalidDeviceId;
  other.handle_ = nullptr;
}

SophonStream::~SophonStream() noexcept(false) {
  deviceId_ = kInvalidDeviceId;
  handle_ = nullptr;
}

template <typename T>
void SophonStream::copySync(T* dst, std::shared_ptr<SophonDeviceMem>& src,
                            size_t count) {
  bm_memcpy_d2s(src->handle_, (void*)dst, src->mem_);
}

template <typename T>
void SophonStream::copySync(std::shared_ptr<SophonDeviceMem>& dst, T* src,
                            size_t count) {
  bm_memcpy_s2d(dst->handle_, dst->mem_, (void*)src);
}

// template <typename T>
// void memcpy_T(T* dst, T* src, size_t count) {

//   memcpy((void*)dst, (void*)src, )
// }

template <typename T>
void SophonStream::copySync(T* dst, T* src, size_t count) {
  int bytes = count * sizeof(T);
  memcpy((void*)dst, (void*)src, bytes);
}

void SophonStream::copySync(std::shared_ptr<SophonDeviceMem>& dst,
                            std::shared_ptr<SophonDeviceMem>& src,
                            size_t count) {}

#define INSTANTIATE_COPY_SYNC(T)                                           \
  template void SophonStream::copySync<T>(T * dst, T * src, size_t count); \
  template void SophonStream::copySync<T>(                                 \
      T * dst, std::shared_ptr<SophonDeviceMem> & src, size_t count);      \
  template void SophonStream::copySync<T>(                                 \
      std::shared_ptr<SophonDeviceMem> & dst, T * src, size_t count);

INSTANTIATE_COPY_SYNC(int)
INSTANTIATE_COPY_SYNC(float)

// template <typename T>
// void copy(SophonDevicePointer<T>& dst, SophonDevicePointer<T>& src) {
//   bm_memcpy_c2c(src.getHandle(), dst.getHandle(), src.getMem(), dst.getMem(),
//                 true);
// }

// template <typename T>
// void copy(SophonHostPointer<T>& dst, SophonHostPointer<T>& src) {
//   memcpy(dst.getMem(), src.getMem(), src.getCount());
// }

// template <typename T>
// void copy(SophonHostPointer<T>& dst, SophonDevicePointer<T>& src) {
//   bm_memcpy_d2s(src.getHandle(), dst.getMem(), dst.getMem());
// }

// template <typename T>
// void copy(SophonDevicePointer<T>& dst, SophonHostPointer<T>& src) {
//   bm_memcpy_s2d(dst.getHandle(), dst.getMem(), src.getMem());
// }

// template <typename T>
// SophonDevicePointer<T> SophonDevicePointer<T>::alloc(int devid, size_t count)
// {
//   bm_handle_t handle_;
//   bm_device_mem_t mem_;
//   bm_dev_request(&handle_, devid);
//   bm_malloc_device_byte(handle_, &mem_, count);

//   return create(handle_, mem_, devid);
// }

// template <typename T>
// SophonDevicePointer<T> SophonDevicePointer<T>::create(bm_handle_t handle,
//                                                       bm_device_mem_t mem,
//                                                       int devid) {
//   SophonDevicePointer p(handle, mem, devid);
//   return p;
// }

// template <typename T>
// SophonDevicePointer<T>::SophonDevicePointer(bm_handle_t handle,
//                                             bm_device_mem_t mem, int devid)
//     : handle_(handle), device_(mem), deviceId_(devid) {}

// template <typename T>
// SophonDevicePointer<T>::SophonDevicePointer(
//     SophonDevicePointer<T>&& other) noexcept
//     : handle_(other.handle_),
//       device(other.device_),
//       deviceId_(other.deviceId_) {
//   //   other.handle_ = nullptr;
//   other.deviceId = kInvalidDeviceId;
// }

// template <typename T>
// SophonDevicePointer<T>& SophonDevicePointer<T>::operator=(
//     SophonDevicePointer<T>&& other) {
//   deviceId_ = other.deviceId_;
//   handle_ = other.handle_;
//   device_ = other.device_;

//   other.deviceId = kInvalidDeviceId;
//   return create(handle_, device_, deviceId_);
// }

// template <typename T>
// SophonDevicePointer<T>::~SophonDevicePointer() noexcept(false) {
//   if (deviceId_ == kInvalidDeviceId) return;
//   bm_free_device(handle_, device_);
//   bm_dev_free(handle_);
// }

// template <typename T>
// SophonHostPointer<T> SophonHostPointer<T>::alloc(size_t count) {
//   // size_t bytes = count * sizeof(T);
//   T* ptr = new T[count];
//   return SophonHostPointer<T>(ptr, count, true);
// }

// template <typename T>
// SophonHostPointer<T>::SophonHostPointer(T* ptr, size_t count, bool owner)
//     : host_(ptr), count_(count), owner_(owner) {}

// template <typename T>
// SophonHostPointer<T>::SophonHostPointer(SophonHostPointer&& other)
//     : host_(other.host_), count_(other.count_), owner_(owner_) {
//   other.host_ = nullptr;
//   other.count_ = 0;
//   other.owner_ = false;
// }

// template <typename T>
// SophonHostPointer<T>& SophonHostPointer<T>::operator=(
//     SophonHostPointer&& other) {
//   host_ = other.host_;
//   count_ = other.count_;
//   owner_ = other.owner_;
//   other.host_ = nullptr;
//   other.count_ = 0;
//   other.owner_ = false;
//   return SophonHostPointer<T>(host_, count_, owner_);
// }

// template <typename T>
// SophonHostPointer<T>::~SophonHostPointer() noexcept(false) {
//   if (owner_) {
//     delete[] host_;
//   }
// }

// Instantiate templates
// #define INSTANTIATE_COPY_ASYNC(T)                                         \
//   template class SophonDevicePointer<T>;                                  \
//   template class SophonHostPointer<T>;                                    \
//                                                                           \
//   template void SophonStream::copyAsync<T>(SophonHostPointer<T> & dst,    \
//                                            SophonDevicePointer<T> & src); \
//                                                                           \
//   template void SophonStream::copyAsync<T>(SophonHostPointer<T> & dst,    \
//                                            SophonHostPointer<T> & src);   \
//                                                                           \
//   template void SophonStream::copyAsync<T>(SophonDevicePointer<T> & dst,  \
//                                            SophonDevicePointer<T> & src); \
//                                                                           \
//   template void SophonStream::copyAsync<T>(SophonDevicePointer<T> & dst,  \
//                                            SophonHostPointer<T> & src);

// INSTANTIATE_COPY_ASYNC(int8_t);
// INSTANTIATE_COPY_ASYNC(uint8_t);
// INSTANTIATE_COPY_ASYNC(int32_t);
// INSTANTIATE_COPY_ASYNC(int64_t);
// INSTANTIATE_COPY_ASYNC(uint64_t);
// INSTANTIATE_COPY_ASYNC(float16);
// INSTANTIATE_COPY_ASYNC(float);
// INSTANTIATE_COPY_ASYNC(double);

}  // namespace gloo