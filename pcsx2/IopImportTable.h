// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include "common/Pcsx2Types.h"

namespace R3000A {

// Opaque module identity. Not a table index. 0xFFFE and 0xFFFF are reserved and are
// not table indices. IomanX is a successful lookup with no MODULE row of its own.
// Unknown means no module matched. Callers must pass the id straight back into
// IopImportFuncname, IopImportHleSlot, or IopImportDebugSlot rather than indexing
// anything with it.
enum class IopModuleId : u16 { IomanX = 0xFFFE, Unknown = 0xFFFF };

// `name` is up to `len` bytes copied straight out of IOP memory; `len` is the number of
// bytes before the first NUL, capped at 8. Any len > 8 yields Unknown.
IopModuleId IopLookupModule(const char *name, u32 len);

// Same return value as irxImportFuncname, including the "start"/"shutdown"/"" fallthrough.
const char *IopImportFuncname(IopModuleId module, u16 index);

// The HLE and Debug interception points, as identifiers rather than function pointers.
// Stage 1 maps these to the real pcsx2 function pointers.
enum class IopHleSlot : u8 {
  None = 0,
  RegisterLibraryEntries,
  ReleaseLibraryEntries,
  Kprintf,
  open,
  close,
  read,
  write,
  lseek,
  remove,
  mkdir,
  rmdir,
  dopen,
  dclose,
  dread,
  dreadx,
  getStat,
  getStatx,
};

enum class IopDebugSlot : u8 {
  None = 0,
  IntrmanRegisterIntrHandler,
  SifcmdSceSifRegisterRpc,
};

IopHleSlot IopImportHleSlot(IopModuleId module, u16 index);
IopDebugSlot IopImportDebugSlot(IopModuleId module, u16 index);

} // namespace R3000A
