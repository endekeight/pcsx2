// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#include "IopImportTable.h"
#include "IopImportTableData.h"

namespace R3000A {

namespace {

template <u32 N>
constexpr u64 PackLit(const char (&s)[N]) {
  static_assert(N >= 2 && N <= 9, "name must be 1 to 8 bytes plus NUL");
  u64 key = 0;
  for (u32 i = 0; i < N - 1; i++)
    key |= static_cast<u64>(static_cast<u8>(s[i])) << (8 * i);
  return key;
}

u64 PackName(const char* name, u32 len) {
  u64 key = 0;
  for (u32 i = 0; i < len; i++)
    key |= static_cast<u64>(static_cast<u8>(name[i])) << (8 * i);
  return key;
}

const IopImportModule* FindModuleByKey(u64 key) {
  const IopImportModule* first = kIopImportModulesByKey;
  const IopImportModule* last = kIopImportModulesByKey + kIopImportModuleCount;
  while (first < last) {
    const IopImportModule* mid = first + (last - first) / 2;
    if (mid->packed_name < key)
      first = mid + 1;
    else if (mid->packed_name > key)
      last = mid;
    else
      return mid;
  }
  return nullptr;
}

const char* FindExportName(const IopImportExport* exports, u16 count, u16 index) {
  const IopImportExport* first = exports;
  const IopImportExport* last = exports + count;
  while (first < last) {
    const IopImportExport* mid = first + (last - first) / 2;
    if (mid->index < index)
      first = mid + 1;
    else if (mid->index > index)
      last = mid;
    else
      return mid->name;
  }
  return nullptr;
}

const IopImportModule* ModuleById(IopModuleId module) {
  const u16 id = static_cast<u16>(module);
  if (id >= kIopImportModuleCount)
    return nullptr;
  return &kIopImportModulesById[id];
}

IopHleSlot IomanFamilyHle(u16 index, bool iomanx) {
  switch (index) {
  case 4:
    return IopHleSlot::open;
  case 5:
    return IopHleSlot::close;
  case 6:
    return IopHleSlot::read;
  case 7:
    return IopHleSlot::write;
  case 8:
    return IopHleSlot::lseek;
  case 10:
    return IopHleSlot::remove;
  case 11:
    return IopHleSlot::mkdir;
  case 12:
    return IopHleSlot::rmdir;
  case 13:
    return IopHleSlot::dopen;
  case 14:
    return IopHleSlot::dclose;
  case 15:
    return iomanx ? IopHleSlot::dreadx : IopHleSlot::dread;
  case 16:
    return iomanx ? IopHleSlot::getStatx : IopHleSlot::getStat;
  default:
    return IopHleSlot::None;
  }
}

} // namespace

IopModuleId IopLookupModule(const char* name, u32 len) {
  if (len > 8)
    return IopModuleId::Unknown;

  const u64 key = PackName(name, len);
  if (const IopImportModule* module = FindModuleByKey(key))
    return static_cast<IopModuleId>(module->id);
  if (key == PackLit("iomanx"))
    return IopModuleId::IomanX;
  return IopModuleId::Unknown;
}

const char* IopImportFuncname(IopModuleId module, u16 index) {
  if (const IopImportModule* desc = ModuleById(module)) {
    if (const char* name = FindExportName(desc->exports, desc->export_count, index))
      return name;
  }

  switch (index) {
  case 0:
    return "start";
  case 2:
    return "shutdown";
  default:
    return "";
  }
}

IopHleSlot IopImportHleSlot(IopModuleId module, u16 index) {
  if (module == IopModuleId::IomanX)
    return IomanFamilyHle(index, true);

  const IopImportModule* desc = ModuleById(module);
  if (!desc)
    return IopHleSlot::None;

  if (desc->packed_name == PackLit("loadcore")) {
    if (index == 6)
      return IopHleSlot::RegisterLibraryEntries;
    if (index == 7)
      return IopHleSlot::ReleaseLibraryEntries;
    return IopHleSlot::None;
  }
  if (desc->packed_name == PackLit("sysmem")) {
    if (index == 14)
      return IopHleSlot::Kprintf;
    return IopHleSlot::None;
  }
  if (desc->packed_name == PackLit("ioman"))
    return IomanFamilyHle(index, false);
  // Unreachable until upstream adds MODULE(iomanx). Keeps iomanx HLE on
  // IomanFamilyHle(..., true) if a real table row replaces the IomanX sentinel.
  if (desc->packed_name == PackLit("iomanx"))
    return IomanFamilyHle(index, true);

  return IopHleSlot::None;
}

IopDebugSlot IopImportDebugSlot(IopModuleId module, u16 index) {
  const IopImportModule* desc = ModuleById(module);
  if (!desc)
    return IopDebugSlot::None;

  if (desc->packed_name == PackLit("intrman") && index == 4)
    return IopDebugSlot::IntrmanRegisterIntrHandler;
  if (desc->packed_name == PackLit("sifcmd") && index == 17)
    return IopDebugSlot::SifcmdSceSifRegisterRpc;
  return IopDebugSlot::None;
}

} // namespace R3000A
