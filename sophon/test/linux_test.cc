/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <functional>
#include <thread>
#include <vector>

#include "sophon/common/linux.h"
#include "sophon/common/linux_devices.h"
#include "sophon/test/base_test.h"

namespace sophon {
namespace test {
namespace {

// Test fixture.
class LinuxTest : public BaseTest {};

TEST_F(LinuxTest, NetworkInterfaceToBusID) {
  // Commented because not every machine has an eth0.
  //
  // auto nic = networkInterfaceToBusID("eth0");
  // ASSERT_NE("", nic);
}

TEST_F(LinuxTest, NetworkInterfaceSpeed) {
  // Commented because not every machine has an eth0.
  //
  // const std::string ifname("eth0");
  // int speed = getInterfaceSpeedByName(ifname);
  // ASSERT_GE(speed, 0) << "Uknown interface speed, ifname: " << ifname;
}

TEST_F(LinuxTest, PCIDistance) {
  auto nics = pciDevices(kPCIClassNetwork);
  auto gpus = pciDevices(kPCIClass3D);
  for (const auto& gpu : gpus) {
    auto distance = pciDistance(nics[0], gpu);
    ASSERT_GE(distance, 0);
  }
}

} // namespace
} // namespace test
} // namespace sophon
