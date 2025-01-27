/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "sophon/rendezvous/store.h"

namespace sophon {
namespace rendezvous {

// constexpr std::chrono::milliseconds Store::kDefaultTimeout;

// Have to provide implementation for pure virtual destructor.
Store::~Store() {}

} // namespace rendezvous
} // namespace sophon
