/*++

Copyright (c)  1999  - 2014, Intel Corporation. All rights reserved
                                                                                   

  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   


--*/

/** @file
**/

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include "PlatformGopPolicy.h"

#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

PLATFORM_GOP_POLICY_PROTOCOL  mPlatformGOPPolicy;
VOID                          *mVbt;
UINTN                         mVbtSize;

//
// Function implementations
//

/**
  The function will execute with as the platform policy, and gives
  the Platform Lid Status. IBV/OEM can customize this code for their specific
  policy action.

  @param CurrentLidStatus  Gives the current LID Status

  @retval EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
GetPlatformLidStatus (
   OUT LID_STATUS *CurrentLidStatus
)
{
  *CurrentLidStatus = LidOpen;

  return EFI_SUCCESS;
}

/**
  The function will execute and gives the Video Bios Table Size and Address.

  @param VbtAddress  Gives the Physical Address of Video BIOS Table

  @param VbtSize     Gives the Size of Video BIOS Table

  @retval EFI_STATUS.

**/

EFI_STATUS
EFIAPI
GetVbtData (
   OUT EFI_PHYSICAL_ADDRESS *VbtAddress,
   OUT UINT32 *VbtSize
)
{
  if (VbtAddress == NULL || VbtSize == NULL){
    return EFI_INVALID_PARAMETER;
  }

  if (mVbt == NULL) {
    return EFI_NOT_FOUND;
  }

  *VbtAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)mVbt;
  *VbtSize    = (UINT32)mVbtSize;

  return EFI_SUCCESS;
}

/**
  Entry point for the Platform GOP Policy Driver.

  @param ImageHandle       Image handle of this driver.
  @param SystemTable       Global system service table.

  @retval EFI_SUCCESS           Initialization complete.
  @retval EFI_OUT_OF_RESOURCES  Do not have enough resources to initialize the driver.

**/

EFI_STATUS
EFIAPI
PlatformGOPPolicyEntryPoint (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )

{
  EFI_STATUS  Status = EFI_SUCCESS;

  gBS = SystemTable->BootServices;

  gBS->SetMem (
         &mPlatformGOPPolicy,
         sizeof (PLATFORM_GOP_POLICY_PROTOCOL),
         0
         );

  mPlatformGOPPolicy.Revision                = PLATFORM_GOP_POLICY_PROTOCOL_REVISION_01;
  mPlatformGOPPolicy.GetPlatformLidStatus    = GetPlatformLidStatus;
  mPlatformGOPPolicy.GetVbtData              = GetVbtData;

  //
  // Install protocol to allow access to this Policy.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ImageHandle,
                  &gPlatformGOPPolicyGuid,
                  &mPlatformGOPPolicy,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  LoadGopFromCbfs (&mVbt, &mVbtSize);

  return EFI_SUCCESS;
}
