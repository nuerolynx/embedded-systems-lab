#pragma once
#include <stdint.h>
enum class OperatingMode : uint8_t { OSDP_PD = 0, WIEGAND_READER = 1, STANDALONE_DEMO = 2 };
#ifndef ODR_MODE
#define ODR_MODE 0
#endif
static_assert(ODR_MODE >= 0 && ODR_MODE <= 2, "Invalid exclusive operating mode");
constexpr OperatingMode MODE = static_cast<OperatingMode>(ODR_MODE);
constexpr uint32_t OSDP_BAUD = 38400;
constexpr uint32_t RELAY_MS = 3000;
constexpr uint32_t DUPLICATE_MS = 1200;
constexpr uint32_t CREDENTIAL_TTL_MS = 1500;
// Wiegand preserves raw bit length. Panel must explicitly support 32/40/56/80 bit UIDs.
// LF HID ASCII payload has unresolved bit packing and is never converted implicitly.
constexpr bool ALLOW_HF_RAW_WIEGAND = false;
constexpr bool ALLOW_EM_RAW_WIEGAND = false;
