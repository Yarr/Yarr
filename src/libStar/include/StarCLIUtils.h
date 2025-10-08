#pragma once

#include "HwController.h"
#include "LCBUtils.h"
#include "StarChipPacket.h"

#include <memory>
#include <string>

namespace StarCLIUtils {

/// @brief Set default logging configuration for CLI utilities.
/// @param programName Name of the program to use in the logger.
void setLoggingDefaults(const std::string &programName);

/// @brief Create a hardware controller from a configuration file.
/// @param configPath Path to the configuration file.
/// @return A unique pointer to the created controller.
std::unique_ptr<HwController> createHwController(const std::string &configPath);

/// @brief Prepare the hardware controller to send commands on a specific Tx
/// channel.
/// @param hwCtrl Reference to the hardware controller.
/// @param txChannel The Tx channel to prepare.
void prepareTx(HwController &hwCtrl, uint32_t txChannel);

/// @brief Prepare the hardware controller to receive commands on a specific Rx
/// channel.
/// @param hwCtrl Reference to the hardware controller.
/// @param rxChannel The Rx channel to prepare.
void prepareRx(HwController &hwCtrl, uint32_t rxChannel);

/// @brief Send a L0a command using a hardware controller.
/// @param hwCtrl Reference to the hardware controller.
/// @param mask L0a mask. Reduced to 4 bits. Can be 0 for a "lonely BCR".
/// @param tag L0a tag. Reduced to 7 bits.
/// @param bcr If true, sets the BCR bit.
void sendL0a(HwController &hwCtrl, unsigned mask, unsigned tag, bool bcr);

/// @brief Send a fast command using a hardware controller.
/// @param hwCtrl Reference to the hardware controller.
/// @param type The type of fast command to send.
/// @param delay The delay for the fast command.
void sendFastCommand(HwController &hwCtrl, LCB::FastCmdType type,
                     uint8_t delay);

/// @brief Send a register command (read or write) using a hardware controller.
/// @param hwCtrl Reference to the hardware controller.
/// @param hccId The HCC ID of the target chip.
/// @param abcId The ABC ID of the target chip. Ignored if isHcc is true.
/// @param address The register address to read from or write to.
/// @param isRead If true, performs a read operation; otherwise, performs a
/// write operation.
/// @param value The value to write. Ignored if isRead is true.
/// @param isHcc If true, the command is formatted for an HCC; otherwise, for an
/// ABC.
void sendRegisterCommand(HwController &hwCtrl, int hccId, int abcId,
                         int address, bool isRead, uint32_t value, bool isHcc);

/// @brief Extract a star packet from raw data.
/// @param packet Reference to the StarChipPacket to fill.
/// @param data Reference to the raw data object.
/// @return 0 on success, or an error code on failure.
int packetFromRawData(StarChipPacket &packet, RawData &data);

/// @brief Check if a raw data object was received on a specific Rx channel.
/// @param data Reference to the raw data object.
/// @param chn The Rx channel to check.
/// @return True if the data was received on the specified channel; otherwise,
/// false.
bool isFromChannel(RawData &data, uint32_t chn);

/// @brief Check if a raw data object contains a specific type of packet.
/// @param data Reference to the raw data object.
/// @param packet_type The type of packet to check for.
/// @return True if the data contains the specified type of packet; otherwise,
/// false.
bool isPacketType(RawData &data, PacketType packet_type);

/// @brief Read and filter data using a hardware controller.
/// @param hwCtrl Reference to the hardware controller.
/// @param filter_cb Raw data filter function. Should return True for any raw
/// data objects that should be kept.
/// @param timeout Timeout in milliseconds to wait for the expected data.
/// @return A vector of raw data pointers.
std::vector<RawDataPtr> readData(HwController &hwCtrl,
                                 std::function<bool(RawData &)> filter_cb,
                                 uint32_t timeout = 1000);

} // namespace StarCLIUtils
