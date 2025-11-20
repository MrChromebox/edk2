/** @file
  Battery Status Protocol

  This protocol provides an interface to retrieve battery information.

  Copyright (c) 2025
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef BATTERY_STATUS_PROTOCOL_H_
#define BATTERY_STATUS_PROTOCOL_H_

#include <Uefi.h>

#define EFI_BATTERY_STATUS_PROTOCOL_GUID \
  { \
    0x8A3B2C1D, 0x4E5F, 0x6A7B, { 0x8C, 0x9D, 0x0E, 0x1F, 0x2A, 0x3B, 0x4C, 0x5D } \
  }

typedef struct _EFI_BATTERY_STATUS_PROTOCOL EFI_BATTERY_STATUS_PROTOCOL;

/**
  Get battery information.

  @param[in]  This               Pointer to the EFI_BATTERY_STATUS_PROTOCOL instance.
  @param[out] BatteryPercentage  Battery charge percentage (0-100). 0xFF indicates error/unknown.
  @param[out] BatteryPresent     TRUE if battery is present, FALSE otherwise.
  @param[out] BatteryCharging    TRUE if battery is charging, FALSE otherwise.

  @retval EFI_SUCCESS            Battery information retrieved successfully.
  @retval EFI_DEVICE_ERROR       Communication error.
  @retval EFI_UNSUPPORTED        Battery not available or not supported.
  @retval EFI_INVALID_PARAMETER  One or more parameters are invalid.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_BATTERY_STATUS_GET_BATTERY_INFO)(
  IN  EFI_BATTERY_STATUS_PROTOCOL  *This,
  OUT UINT8                         *BatteryPercentage,
  OUT BOOLEAN                       *BatteryPresent,
  OUT BOOLEAN                       *BatteryCharging
  );

/**
  Structure for the Battery Status Protocol.
**/
struct _EFI_BATTERY_STATUS_PROTOCOL {
  EFI_BATTERY_STATUS_GET_BATTERY_INFO    GetBatteryInfo;
};

extern EFI_GUID  gEfiBatteryStatusProtocolGuid;

#endif // BATTERY_STATUS_PROTOCOL_H_

