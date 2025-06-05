#include "catch.hpp"

#include "StarPackets.h"
#include "Utils.h"

using namespace StarPackets;

std::string hexify(const std::vector<uint8_t> &data)
{
  std::string d;
  for(auto c: data) {
    if(!d.empty()) d += " ";
    d += Utils::hexify(c);
  }

  return d;
}

TEST_CASE("StarEmulatorPacketGenerate", "[star][emulator]")
{
  // Test pattern for each type

  SECTION ("PR") {
    std::vector<uint16_t> error_data
      {0x77f4, 0x0000, 0x0400, 0x0000};

    // Pass raw error words directly
    auto error_pr_data = buildPhysicsPacket(error_data, PacketTypes::PR,
                                            0x13, 0x8f);
    std::vector<uint8_t> exp_data
      {0x11, 0x3f,
       0x77, 0xf4, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
       0x6f, 0xed};
    CAPTURE(hexify(error_pr_data), hexify(exp_data));
    CHECK( error_pr_data == exp_data);
  }

  SECTION ("LP") {
    {
      std::array<std::vector<uint16_t>, HCC_INPUT_CHANNEL_COUNT> empty_data;
      std::vector<uint16_t> empty_chip;
      empty_data[0] = empty_chip;
      auto empty_lp_data = buildPhysicsPacket(empty_data, PacketTypes::PR, 0x7f, 0xff);

      // If there are no ICs enabled then the packet disappears
      std::vector<uint8_t> exp_data{};
      CAPTURE(hexify(empty_lp_data), hexify(exp_data));
      CHECK( empty_lp_data == exp_data);
    }

    {
      // IC 0
      std::vector<uint16_t> ic0{0x078f, 0x038f, 0x07af, 0x03af};
      // IC 1
      std::vector<uint16_t> ic1{0x0f8f, 0x0b8f, 0x0faf, 0x0baf};
      // IC 2
      std::vector<uint16_t> ic2{0x178f, 0x138f, 0x17af, 0x13af,
                                0x17cf, 0x13cf, 0x17ee, 0x13ee};
      std::array<std::vector<uint16_t>, HCC_INPUT_CHANNEL_COUNT> clusters;
      clusters[0] = ic0;
      clusters[1] = ic1;
      clusters[2] = ic2;

      // Long (LP) packet (30 hits on 10 chips)
      auto some_lp_data = buildPhysicsPacket
        (clusters, PacketTypes::LP, 0x7f, 0xff);

      std::vector<uint8_t> exp_data{
        0x27, 0xfe,
        // Channel 0...
        0x07, 0x8f, 0x03, 0x8f, 0x07, 0xaf, 0x03, 0xaf,
        // Channel 1...
        0x0f, 0x8f, 0x0b, 0x8f, 0x0f, 0xaf, 0x0b, 0xaf,
        // Channel 2
        0x17, 0x8f, 0x13, 0x8f, 0x17, 0xaf, 0x13, 0xaf,
        0x17, 0xcf, 0x13, 0xcf, 0x17, 0xee, 0x13, 0xee,
        // END
        0x6f, 0xed,
      };

      CAPTURE(hexify(some_lp_data), hexify(exp_data));
      CHECK( some_lp_data == exp_data);
    }
  }

  SECTION ("ABC_REG") {
    auto abc_reg_data = buildABCRegisterPacket
      (PacketTypes::ABCRegRd, 9, 0x81, 0x87654321, 0x8001);

    std::vector<uint8_t> exp_data{0x49, 0x81, 0x08, 0x76, 0x54, 0x32, 0x18, 0x00, 0x10};

    CAPTURE(hexify(abc_reg_data), hexify(exp_data));
    CHECK( abc_reg_data == exp_data);
  }

  SECTION ("HCC_REG") {
    auto hcc_reg_data = buildHCCRegisterPacket
      (PacketTypes::HCCRegRd, 0x7, 0x00007855);

    std::vector<uint8_t> exp_data{0x80, 0x70, 0x00, 0x07, 0x85, 0x50};
    CAPTURE(hexify(hcc_reg_data), hexify(exp_data));
    CHECK( hcc_reg_data == exp_data);
  }

  SECTION ("ABC_HPR") {
    auto abc_hpr_data = buildABCRegisterPacket
      (PacketTypes::ABCHPR, 0, 0x3f, 0x7855002b, 0);

    std::vector<uint8_t> exp_data{0xd0, 0x3f, 0x07, 0x85, 0x50, 0x02, 0xb0, 0x00, 0x00};

    CAPTURE(hexify(abc_hpr_data), hexify(exp_data));
    CHECK( abc_hpr_data == exp_data);
  }

  SECTION ("HCC_HPR") {
    auto hcc_hpr_data = buildHCCRegisterPacket
      (PacketTypes::HCCHPR, 0xf, 0x7855002b);

    std::vector<uint8_t> exp_data{0xe0, 0xf7, 0x85, 0x50, 0x02, 0xb0};

    CAPTURE(hexify(hcc_hpr_data), hexify(exp_data));
    CHECK( hcc_hpr_data == exp_data);
  }
}
