/** @file
  UFS Platform Driver - Header File

  This driver provides platform-specific UFS host controller configuration
  for Intel platforms. It implements the EDKII_UFS_HC_PLATFORM_PROTOCOL to
  configure platform-specific settings required for proper UFS operation.

  Key functionality:
  - Disables PA_LOCAL_TX_LCC_ENABLE for Intel UFS controllers
  - Activates all connected UFS lanes for maximum throughput
  - Programs High-Speed (HS) recipe for stable HS mode operation
  - Switches link to High-Speed mode for maximum performance
  - Validates lane configuration for optimal performance
  - Sets reference clock frequency to 19.2 MHz

  Copyright (c) 2025, Matt DeVillier. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _UFS_PLATFORM_DXE_H_
#define _UFS_PLATFORM_DXE_H_

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Protocol/UfsHostControllerPlatform.h>

//
// UniPro PHY Adapter Layer Attributes
// Reference: MIPI UniPro v1.6 and UFS Specification
//
#define PA_LOCAL_TX_LCC_ENABLE       0x155E
#define PA_CONNECTED_TX_DATA_LANES   0x1561
#define PA_CONNECTED_RX_DATA_LANES   0x1581
#define PA_ACTIVE_TX_DATA_LANES      0x1560
#define PA_ACTIVE_RX_DATA_LANES      0x1580
#define PA_HS_SERIES                 0x156A
#define PA_RX_TERMINATION            0x1584
#define PA_TX_TERMINATION            0x1569
#define PA_RX_GEAR                   0x1583
#define PA_TX_GEAR                   0x1568
#define PA_MAX_RX_HS_GEAR            0x1587
#define PA_PWR_MODE                  0x1571

//
// Data Link Layer Attributes
// Reference: MIPI UniPro v1.6 and UFS Specification
//
#define DL_FC0_PROTECTION_TIMEOUT_VAL   0x15B0
#define DL_TC0_REPLAY_TIMEOUT_VAL       0x15B1
#define DL_AFC0_REQ_TIMEOUT_VAL         0x15B2

//
// HS Series Mode Values
//
#define UFS_PA_HS_MODE_A  0x01
#define UFS_PA_HS_MODE_B  0x02

//
// Power Mode Values
//
#define UFS_PWR_MODE_FAST      0x01
#define UFS_PWR_MODE_SLOW      0x02
#define UFS_PWR_MODE_FASTAUTO  0x04
#define UFS_PWR_MODE_SLOWAUTO  0x05

//
// HS Gear Values
//
#define UFS_HS_GEAR_1  0x01
#define UFS_HS_GEAR_2  0x02
#define UFS_HS_GEAR_3  0x03
#define UFS_HS_GEAR_4  0x04

//
// UFS Host Controller MMIO Register Offsets
//
#define UFS_HC_IS_OFFSET   0x0020  // Interrupt Status register
#define UFS_HC_IS_UPMS     0x0010  // UIC Power Mode Status bit

//
// UIC Command Opcodes
//
#define UIC_CMD_DME_GET       0x01
#define UIC_CMD_DME_SET       0x02
#define UIC_CMD_DME_PEER_GET  0x03
#define UIC_CMD_DME_PEER_SET  0x04

//
// DME Attribute Set Types
//
#define DME_ATTR_SET_TYPE_NORMAL  0x00
#define DME_ATTR_SET_TYPE_STATIC  0x01

/**
  Platform-specific callback function for UFS host controller initialization.

  This function is called at various phases during UFS host controller
  initialization to allow platform-specific configuration.

  @param[in]      ControllerHandle  Handle of the UFS controller.
  @param[in]      CallbackPhase     Specifies when the platform protocol is called.
  @param[in, out] CallbackData      Data specific to the callback phase.
                                    EDKII_UFS_HC_DRIVER_INTERFACE structure.

  @retval EFI_SUCCESS            Platform configuration completed successfully.
  @retval EFI_INVALID_PARAMETER  CallbackData is NULL or CallbackPhase is invalid.
  @retval EFI_DEVICE_ERROR       Failed to configure UFS controller.

**/
EFI_STATUS
EFIAPI
UfsPlatformCallback (
  IN     EFI_HANDLE                            ControllerHandle,
  IN     EDKII_UFS_HC_PLATFORM_CALLBACK_PHASE  CallbackPhase,
  IN OUT VOID                                  *CallbackData
  );

#endif // _UFS_PLATFORM_DXE_H_

