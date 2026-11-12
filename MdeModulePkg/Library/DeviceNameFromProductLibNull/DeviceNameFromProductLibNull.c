/** @file
  NULL DeviceNameFromProductLib instance.

  Copyright (c) 2020, Matt DeVillier. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/DeviceNameFromProductLib.h>

VOID
EFIAPI
GetDeviceNameFromProduct (
  IN  CHAR16  *Product,
  IN  UINTN   BufferSize,
  OUT CHAR16  **DeviceName
  )
{
  //
  // No product mapping; leave the caller buffer empty.
  //
  return;
}
