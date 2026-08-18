#include "common/HostSys.h"
#include "common/MemoryInterface.h"
#include "pcsx2/Memory.h"
#include "pcsx2/IopMem.h"
#include "pcsx2/Host.h"
#include "pcsx2/GameDatabase.h"
#include "pcsx2/Config.h"
#include "pcsx2/Achievements.h"
#include "pcsx2/vtlb.h"
#include "pcsx2/R5900.h"

PageFaultHandler::HandlerResult PageFaultHandler::HandlePageFault(void* exception_pc, void* fault_address, bool is_write)
{
    return PageFaultHandler::HandlerResult::ExecuteNextHandler;
}

// EEMemoryInterface implementations
u8 EEMemoryInterface::Read8(u32 address, bool* valid) { return 0; }
u16 EEMemoryInterface::Read16(u32 address, bool* valid) { return 0; }
u32 EEMemoryInterface::Read32(u32 address, bool* valid) { return 0; }
u64 EEMemoryInterface::Read64(u32 address, bool* valid) { return 0; }
u128 EEMemoryInterface::Read128(u32 address, bool* valid) { return {}; }
bool EEMemoryInterface::ReadBytes(u32 address, void* dest, u32 size) { return false; }
bool EEMemoryInterface::Write8(u32 address, u8 value) { return false; }
bool EEMemoryInterface::Write16(u32 address, u16 value) { return false; }
bool EEMemoryInterface::Write32(u32 address, u32 value) { return false; }
bool EEMemoryInterface::Write64(u32 address, u64 value) { return false; }
bool EEMemoryInterface::Write128(u32 address, u128 value) { return false; }
bool EEMemoryInterface::WriteBytes(u32 address, const void* src, u32 size) { return false; }
bool EEMemoryInterface::CompareBytes(u32 address, const void* src, u32 size) { return false; }

// IOPMemoryInterface implementations
u8 IOPMemoryInterface::Read8(u32 address, bool* valid) { return 0; }
u16 IOPMemoryInterface::Read16(u32 address, bool* valid) { return 0; }
u32 IOPMemoryInterface::Read32(u32 address, bool* valid) { return 0; }
u64 IOPMemoryInterface::Read64(u32 address, bool* valid) { return 0; }
u128 IOPMemoryInterface::Read128(u32 address, bool* valid) { return {}; }
bool IOPMemoryInterface::ReadBytes(u32 address, void* dest, u32 size) { return false; }
bool IOPMemoryInterface::Write8(u32 address, u8 value) { return false; }
bool IOPMemoryInterface::Write16(u32 address, u16 value) { return false; }
bool IOPMemoryInterface::Write32(u32 address, u32 value) { return false; }
bool IOPMemoryInterface::Write64(u32 address, u64 value) { return false; }
bool IOPMemoryInterface::Write128(u32 address, u128 value) { return false; }
bool IOPMemoryInterface::WriteBytes(u32 address, const void* src, u32 size) { return false; }
bool IOPMemoryInterface::CompareBytes(u32 address, const void* src, u32 size) { return false; }

void Host::AddIconOSDMessage(std::string message, const char* icon, std::string_view duration_str, float duration) {}
int Host::GetIntSettingValue(const char* section, const char* key, int default_value) { return default_value; }
const char* Host::TranslateToCString(std::string_view context, std::string_view msg) { return msg.data(); }
std::vector<std::string> Host::GetStringListSetting(const char* section, const char* key) { return {}; }
std::string Host::GetStringSettingValue(const char* section, const char* key, const char* default_value) { return default_value ? default_value : ""; }
std::string_view Host::TranslateToStringView(std::string_view context, std::string_view msg) { return msg; }

const std::string* GameDatabaseSchema::GameEntry::findPatch(u32 crc) const { return nullptr; }
const GameDatabaseSchema::GameEntry* GameDatabase::findGame(std::string_view serial) { return nullptr; }

bool Achievements::IsHardcoreModeActive() { return false; }

namespace EmuFolders {
std::string Cheats = "";
std::string Patches = "";
std::string Resources = "";
std::string UserResources = "";
}

TraceLogsEE::TraceLogsEE() {}
TraceLogsIOP::TraceLogsIOP() {}
TraceLogsMISC::TraceLogsMISC() {}
const char* Pcsx2Config::GSOptions::DEFAULT_CAPTURE_CONTAINER = "mp4";
const char* Pcsx2Config::GSOptions::AspectRatioNames[] = { "4:3", "16:9", nullptr };

Pcsx2Config::CpuOptions::CpuOptions() {}
Pcsx2Config::GSOptions::GSOptions() {}
Pcsx2Config::SPU2Options::SPU2Options() {}
Pcsx2Config::PadOptions::PadOptions() {}
Pcsx2Config::AchievementsOptions::AchievementsOptions() {}
Pcsx2Config::SavestateOptions::SavestateOptions() {}
Pcsx2Config::USBOptions::USBOptions() {}
Pcsx2Config::DEV9Options::DEV9Options() {}
Pcsx2Config::GamefixOptions::GamefixOptions() {}
Pcsx2Config::FilenameOptions::FilenameOptions() {}
Pcsx2Config::ProfilerOptions::ProfilerOptions() {}
Pcsx2Config::SpeedhackOptions::SpeedhackOptions() {}
Pcsx2Config::RecompilerOptions::RecompilerOptions() {}
Pcsx2Config::EmulationSpeedOptions::EmulationSpeedOptions() {}
TraceLogFilters::TraceLogFilters() {}
Pcsx2Config::Pcsx2Config() {}
Pcsx2Config EmuConfig;

R5900cpu *Cpu = nullptr;

template<> void vtlb_memWrite<u32>(u32 mem, u32 value) {}
void* vtlb_GetPhyPtr(u32 pAddr) { return nullptr; }

namespace vtlb_private {
alignas(64) MapData vtlbdata;
}

mem8_t vtlb_MissRead8(u32 addr) { return 0; }
mem16_t vtlb_MissRead16(u32 addr) { return 0; }
mem32_t vtlb_MissRead32(u32 addr) { return 0; }
mem64_t vtlb_MissRead64(u32 addr) { return 0; }
void vtlb_MissWrite8(u32 addr, mem8_t data) {}
void vtlb_MissWrite16(u32 addr, mem16_t data) {}
void vtlb_MissWrite32(u32 addr, mem32_t data) {}
void vtlb_MissWrite64(u32 addr, mem64_t data) {}

// Zip stubs
extern "C" {
struct zip {};
struct zip_source {};
struct zip_stat {};
struct zip_file {};

zip* zip_open_from_source(zip_source*, int, void*) { return nullptr; }
int zip_close(zip*) { return 0; }
long long zip_name_locate(zip*, const char*, int) { return -1; }
int zip_stat_index(zip*, unsigned long long, int, zip_stat*) { return -1; }
zip_file* zip_fopen_index(zip*, unsigned long long, int) { return nullptr; }
long long zip_fread(zip_file*, void*, unsigned long long) { return -1; }
const char* zip_error_strerror(void*) { return "zip error"; }
zip_source* zip_source_file_create(const char*, unsigned long long, long long, void*) { return nullptr; }
void zip_source_free(zip_source*) {}
}
