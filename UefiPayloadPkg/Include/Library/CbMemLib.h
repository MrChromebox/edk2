/** @file
  Coreboot CBMEM access and payload timestamp support.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Base.h>

#define CBMEM_TS_UPL_ENTRY          1300
#define CBMEM_TS_UPL_HOB_READY      1301
#define CBMEM_TS_UPL_DXE_LOAD_START 1302
#define CBMEM_TS_UPL_DXE_LOAD_END   1303
#define CBMEM_TS_UPL_DXE_HANDOFF    1304
#define CBMEM_TS_UPL_DXE_DRIVER     1305
#define CBMEM_TS_UPL_READY_TO_BOOT  1306
#define CBMEM_TS_UPL_EXIT_BOOT      1307

/**
  Find a CBMEM entry exported by the coreboot table.

  @param[in]  Id          CBMEM entry ID.
  @param[out] Address     Entry address.
  @param[out] Size        Entry size in bytes.

  @retval RETURN_SUCCESS  Entry found and validated.
  @retval RETURN_NOT_FOUND Entry is not exported by the bootloader.
  @retval RETURN_COMPROMISED_DATA  The coreboot table is malformed.
**/
RETURN_STATUS
EFIAPI
CbMemFind (
  IN  UINT32  Id,
  OUT VOID    **Address,
  OUT UINT32  *Size
  );

/**
  Publish the validated coreboot table address for later payload modules.

  The entry module's bootloader-parameter PCD is patchable per module, so
  later DXE modules use this HOB to find the same coreboot table.
**/
RETURN_STATUS
EFIAPI
CbMemPublishTableHob (
  VOID
  );

/**
  Add one payload timestamp to coreboot's existing timestamp table.

  The timestamp uses the same x86 TSC and base-time convention as coreboot,
  allowing the normal cbmem utility to display it with firmware timestamps.
**/
RETURN_STATUS
EFIAPI
CbMemTimestampAdd (
  IN UINT32  Id
  );

/**
  Log a bounded summary of the timestamp CBMEM entry.
**/
RETURN_STATUS
EFIAPI
CbMemLogSummary (
  VOID
  );
