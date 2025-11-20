/** @file
  EC ACPI Battery Status DXE Driver - Common Definitions

  Copyright (c) 2025
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef EC_ACPI_BATTERY_STATUS_DXE_H_
#define EC_ACPI_BATTERY_STATUS_DXE_H_

#include <Protocol/BatteryStatus.h>
#include <Uefi.h>

//
// ACPI EC Port Definitions
//
#define EC_SC    0x66  // EC Status/Command Register
#define EC_DATA  0x62  // EC Data Register

//
// EC Status Register Bits
//
#define EC_IBF  BIT1  // Input Buffer Full (1 = data ready for EC)
#define EC_OBF  BIT0  // Output Buffer Full (1 = data ready for host)

//
// EC Commands
//
#define EC_CMD_ACPI_READ  0x80  // Read from ACPI memmap (ChromeOS)
#define RD_EC             0x80  // Read Embedded Controller (Merlin/ThinkPad)
#define WR_EC             0x81  // Write Embedded Controller (ThinkPad)

//
// Timeout values (in microseconds)
//
#define EC_TIMEOUT_US  (100 * 1000)  // 100ms

//
// EC Type Definitions
//
typedef enum {
  EcTypeUnknown = 0,
  EcTypeChromeOs,
  EcTypeMerlin,
  EcTypeThinkPad
} EC_TYPE;

//
// EC Profile Structure
//
typedef struct {
  EC_TYPE  Type;
  CHAR8    *Name;
  //
  // EC Detection
  //
  EFI_STATUS  (*CheckEcPresent)(VOID);
  //
  // Battery Information Retrieval
  //
  EFI_STATUS  (*GetBatteryInfo)(
    OUT UINT8    *BatteryPercentage,
    OUT BOOLEAN  *BatteryPresent,
    OUT BOOLEAN  *BatteryCharging
    );
} EC_PROFILE;

//
// Driver Private Data
//
typedef struct {
  EFI_BATTERY_STATUS_PROTOCOL    Protocol;
  BOOLEAN                         EcInitialized;
  BOOLEAN                         EcPresent;
  EC_TYPE                         EcType;
  EC_PROFILE                      *Profile;
} EC_BATTERY_PRIVATE_DATA;

//
// Common EC Communication Functions (implemented in main driver)
//
EFI_STATUS
WaitForEcReadySend (
  VOID
  );

EFI_STATUS
WaitForEcReadyRecv (
  VOID
  );

//
// External references to private data
//
extern EC_BATTERY_PRIVATE_DATA  *mEcBatteryPrivate;

//
// EC Profile Functions (implemented in separate files)
//
EFI_STATUS
CheckChromeOsEcPresent (
  VOID
  );

EFI_STATUS
GetChromeOsBatteryInfo (
  OUT UINT8    *BatteryPercentage,
  OUT BOOLEAN  *BatteryPresent,
  OUT BOOLEAN  *BatteryCharging
  );

EFI_STATUS
CheckMerlinEcPresent (
  VOID
  );

EFI_STATUS
GetMerlinBatteryInfo (
  OUT UINT8    *BatteryPercentage,
  OUT BOOLEAN  *BatteryPresent,
  OUT BOOLEAN  *BatteryCharging
  );

EFI_STATUS
CheckThinkPadEcPresent (
  VOID
  );

EFI_STATUS
GetThinkPadBatteryInfo (
  OUT UINT8    *BatteryPercentage,
  OUT BOOLEAN  *BatteryPresent,
  OUT BOOLEAN  *BatteryCharging
  );

#endif // EC_ACPI_BATTERY_STATUS_DXE_H_

