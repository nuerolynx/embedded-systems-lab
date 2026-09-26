#pragma once
#include "credential.h"
// Deliberately empty. Add exact kind + length + bytes only after enrollment.
// UID possession is not cryptographic authentication. Demonstration use only.
constexpr Credential DEMO_WHITELIST[] = {{}};
constexpr size_t DEMO_WHITELIST_COUNT = 0;
