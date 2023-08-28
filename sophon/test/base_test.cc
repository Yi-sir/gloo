/**
 * Copyright (c) 2020-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "sophon/test/base_test.h"
#include "sophon/test/openssl_utils.h"

namespace sophon {
namespace test {

const char *kDefaultDevice = "localhost";

std::shared_ptr<::sophon::transport::Device> createDevice(Transport transport) {
#if SOPHON_HAVE_TRANSPORT_TCP
  if (transport == Transport::TCP) {
    return ::sophon::transport::tcp::CreateDevice(kDefaultDevice);
  }
#endif
#if SOPHON_HAVE_TRANSPORT_TCP_TLS
  if (transport == Transport::TCP_TLS) {
    return ::sophon::transport::tcp::tls::CreateDevice(
        kDefaultDevice, pkey_file, cert_file, ca_cert_file, "");
  }
#endif
#if SOPHON_HAVE_TRANSPORT_UV
  if (transport == Transport::UV) {
#ifdef _WIN32
    sophon::transport::uv::attr attr;
    attr.ai_family = AF_UNSPEC;
    return ::sophon::transport::uv::CreateDevice(attr);
#else
    return ::sophon::transport::uv::CreateDevice(kDefaultDevice);
#endif
  }
#endif
  return nullptr;
}

} // namespace test
} // namespace sophon
