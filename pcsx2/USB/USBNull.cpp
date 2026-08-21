// SPDX-FileCopyrightText: 2002-2024 PCSX2 Dev Team
// SPDX-License-Identifier: LGPL-3.0+

#include "PrecompiledHeader.h"

#include "USB.h"

u8* ram = nullptr;

void USBconfigure() {}

void DestroyDevices() {}
void CreateDevices() {}

void USBinit() {}
void USBasync(u32 cycles) {}
void USBshutdown() {}
void USBclose() {}
void USBreset() {}
bool USBopen() { return true; }


u8 USBread8(u32 addr) { return 0; }
u16 USBread16(u32 addr) { return 0; }
u32 USBread32(u32 addr) { return 0; }
void USBwrite8(u32 addr, u8 value) {}
void USBwrite16(u32 addr, u16 value) {}
void USBwrite32(u32 addr, u32 value) {}

void USBsetRAM(void* mem) { ram = static_cast<u8*>(mem); }

FILE* usbLog = nullptr;

s64 get_clock() { return 0; };
const char* USB::DeviceTypeIndexToName(s32 device) { return ""; }
std::string USB::GetConfigSection(int port) { return ""; }
s32 USB::DeviceTypeNameToIndex(const std::string_view device) { return 0; }

// There is no USB emulation in this build, so there is nothing to serialise and
// writing nothing is the correct result. Returning false made every save state
// fail outright: SaveState_DownloadState() treats any FreezeOut() failure as
// fatal ("FreezeOut() failed for USB.bin.") even though SavestateEntry_USB
// reports IsRequired() == false.
bool USB::DoState(StateWrapper& sw) { return true; }
void USB::SetDefaultConfiguration(SettingsInterface* si) {}
void USB::CheckForConfigChanges(const Pcsx2Config& old_config) {}
void USB::ClearPortBindings(SettingsInterface& si, u32 port) {}
std::span<const InputBindingInfo> USB::GetDeviceBindings(const std::string_view device, u32 subtype) { return std::span<const InputBindingInfo>(); }
std::span<const SettingInfo> USB::GetDeviceSettings(const std::string_view device, u32 subtype) { return std::span<const SettingInfo>(); }
std::span<const InputBindingInfo> USB::GetDeviceBindings(u32 port) { return std::span<const InputBindingInfo>(); }

std::vector<std::pair<const char*, const char*>> USB::GetDeviceTypes() { return std::vector<std::pair<const char*, const char*>>(); }
const char* USB::GetDeviceName(const std::string_view device) { return ""; };
const char* USB::GetDeviceSubtypeName(const std::string_view device, u32 subtype) { return ""; };
std::span<const char*> USB::GetDeviceSubtypes(const std::string_view device) { return std::span<const char*>(); };

float USB::GetDeviceBindValue(u32 port, u32 bind_index) { return 0; };
void USB::SetDeviceBindValue(u32 port, u32 bind_index, float value) {};

void USB::InputDeviceConnected(const std::string_view identifier) {};

void USB::InputDeviceDisconnected(const std::string_view identifier) {};
std::string USB::GetConfigDevice(const SettingsInterface& si, u32 port) { return ""; };
void USB::SetConfigDevice(SettingsInterface& si, u32 port, const char* devname) {};
u32 USB::GetConfigSubType(const SettingsInterface& si, u32 port, const std::string_view devname) { return 0; };
void USB::SetConfigSubType(SettingsInterface& si, u32 port, const std::string_view devname, u32 subtype) {};
std::string USB::GetConfigSubKey(const std::string_view device, const std::string_view bind_name) { return ""; };
bool USB::MapDevice(SettingsInterface& si, u32 port, const std::vector<std::pair<GenericInputBinding, std::string>>& mapping) { return false; };
void USB::CopyConfiguration(SettingsInterface* dest_si, const SettingsInterface& src_si, bool copy_devices, bool copy_bindings) {};
bool USB::ConfigKeyExists(SettingsInterface& si, u32 port, const char* devname, const char* key) { return false; };
bool USB::GetConfigBool(SettingsInterface& si, u32 port, const char* devname, const char* key, bool default_value) { return false; };
s32 USB::GetConfigInt(SettingsInterface& si, u32 port, const char* devname, const char* key, s32 default_value) { return 0; };
float USB::GetConfigFloat(SettingsInterface& si, u32 port, const char* devname, const char* key, float default_value) { return 0; };
std::string USB::GetConfigString(SettingsInterface& si, u32 port, const char* devname, const char* key, const char* default_value) { return ""; };
const char* USB::GetDeviceIconName(u32 port) { return ""; }