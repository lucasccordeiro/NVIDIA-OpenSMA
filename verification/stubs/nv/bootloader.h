// ESBMC verification stub for nv/bootloader.h
#pragma once
#include <cstdint>

namespace nv::bootloader {

enum class Status : uint32_t { Ok = 0, Error };
enum class Slot    : uint8_t  { Slot0, Slot1 };

inline Status get_active_slot(Slot&) { return Status::Ok; }
inline Status switch_slot()          { return Status::Ok; }

}  // namespace nv::bootloader
