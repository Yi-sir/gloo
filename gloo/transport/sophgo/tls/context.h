/**
 * Copyright (c) 2020-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include "gloo/transport/sophgo/context.h"
#include "gloo/transport/sophgo/tls/openssl.h"

namespace gloo {
namespace transport {
namespace sophgo {
namespace tls {

std::string getSSLErrorMessage();

// Forward declaration
class Device;
class Pair;

class Context : public ::gloo::transport::sophgo::Context {
public:
  Context(std::shared_ptr<Device> device, int rank, int size);

  ~Context() override;

  SSL_CTX *create_ssl_ctx(const char *cert, const char *key,
                          const char *ca_file, const char *ca_path);

  std::unique_ptr<transport::Pair> &createPair(int rank) override;

protected:
  std::unique_ptr<SSL_CTX, void (*)(SSL_CTX *)> ssl_ctx_;

  friend class Pair;
};

} // namespace tls
} // namespace sophgo
} // namespace transport
} // namespace gloo
