/*++

Copyright (c)  1999  - 2014, Intel Corporation. All rights reserved
                                                                                   

  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   


--*/

/** @file
**/

#ifndef _PLATFORM_GOP_POLICY_PROTOCOL_H_
#define _PLATFORM_GOP_POLICY_PROTOCOL_H_

#define EFI_PLATFORM_GOP_POLICY_PROTOCOL_GUID \
  { 0xec2e931b, 0x3281, 0x48a5, 0x81, 0x7, 0xdf, 0x8a, 0x8b, 0xed, 0x3c, 0x5d }

#define PLATFORM_GOP_POLICY_PROTOCOL_REVISION_01 0x01
#define PLATFORM_GOP_POLICY_PROTOCOL_REVISION_02 x0222

#pragma pack(1)

typedef enum {
  LidClosed,
  LidOpen,
  LidStatusMax
} LID_STATUS;

typedef enum {
  Docked,
  UnDocked,
  DockStatusMax
} DOCK_STATUS;

typedef
EFI_STATUS
(EFIAPI *GET_PLATFORM_LID_STATUS) (
   OUT LID_STATUS *CurrentLidStatus
);

typedef
EFI_STATUS
(EFIAPI *GET_VBT_DATA) (
   OUT EFI_PHYSICAL_ADDRESS *VbtAddress,
   OUT UINT32 *VbtSize
);

#pragma pack()

typedef struct _PLATFORM_GOP_POLICY_PROTOCOL {
  UINT32                             Revision;
  GET_PLATFORM_LID_STATUS            GetPlatformLidStatus;
  GET_VBT_DATA                       GetVbtData;
} PLATFORM_GOP_POLICY_PROTOCOL;

//
// Extern the GUID for protocol users.
//
extern EFI_GUID  gPlatformGOPPolicyGuid;

/**
  Load the GOP driver and VBT from CBFS, and hand the GOP driver to the DXE
  dispatcher.

  @param[out] Vbt      On success, the VBT. The caller owns the buffer.
  @param[out] VbtSize  On success, the size of Vbt in bytes.

  @retval EFI_SUCCESS  The GOP driver was installed and the VBT returned.
  @retval Others       The GOP driver or VBT could not be loaded.
**/
EFI_STATUS
LoadGopFromCbfs (
  OUT VOID   **Vbt,
  OUT UINTN  *VbtSize
  );

#endif
