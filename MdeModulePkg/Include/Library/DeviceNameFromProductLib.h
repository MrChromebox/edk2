/** @file
  Map SMBIOS product names to friendly device names.

  Copyright (c) 2020, Matt DeVillier. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef DEVICE_NAME_FROM_PRODUCT_LIB_H_
#define DEVICE_NAME_FROM_PRODUCT_LIB_H_

/**
  Fill DeviceName with a friendly name for the SMBIOS product string.

  The caller allocates *DeviceName (zeroed). On no match, leave it empty.

  @param[in]  Product     SMBIOS product name.
  @param[in]  BufferSize  Size in bytes of the *DeviceName buffer.
  @param[out] DeviceName  On input, pointer to a zeroed CHAR16 buffer;
                          on output, may hold a friendly device name.
**/
VOID
EFIAPI
GetDeviceNameFromProduct (
  IN  CHAR16  *Product,
  IN  UINTN   BufferSize,
  OUT CHAR16  **DeviceName
  );

#endif
