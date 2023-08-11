/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "gloo/sophgo/store.h"

namespace gloo {
namespace sophgo {

constexpr std::chrono::milliseconds Store::kDefaultTimeout;

// Have to provide implementation for pure virtual destructor.
Store::~Store() {}

} // namespace sophgo
} // namespace gloo
