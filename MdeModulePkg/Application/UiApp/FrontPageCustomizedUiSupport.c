/** @file

  This library class defines a set of interfaces to customize Ui module

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Uefi.h>

#include <Guid/MdeModuleHii.h>
#include <Guid/GlobalVariable.h>

#include <Protocol/HiiConfigAccess.h>
#include <Protocol/HiiString.h>

#include <Library/HiiLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PcdLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiHiiServicesLib.h>
#include <Library/DevicePathLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include "FrontPageCustomizedUiSupport.h"

//
// This is the VFR compiler generated header file which defines the
// string identifiers.
//

#define UI_HII_DRIVER_LIST_SIZE  0x8

#define FRONT_PAGE_KEY_CONTINUE  0x1000
#define FRONT_PAGE_KEY_RESET     0x1001
#define FRONT_PAGE_KEY_DRIVER    0x2000

typedef struct {
  EFI_STRING_ID    PromptId;
  EFI_STRING_ID    HelpId;
  EFI_STRING_ID    DevicePathId;
  EFI_GUID         FormSetGuid;
  BOOLEAN          EmptyLineAfter;
} UI_HII_DRIVER_INSTANCE;

UI_HII_DRIVER_INSTANCE  *gHiiDriverList;
extern EFI_HII_HANDLE   gStringPackHandle;


/**
  This function processes the results of changes in configuration.


  @param HiiHandle       Points to the hii handle for this formset.
  @param Action          Specifies the type of action taken by the browser.
  @param QuestionId      A unique value which is sent to the original exporting driver
                         so that it can identify the type of data to expect.
  @param Type            The type of value for the question.
  @param Value           A pointer to the data being sent to the original exporting driver.
  @param ActionRequest   On return, points to the action requested by the callback function.
  @param Status          Return the handle status.

  @retval  TRUE          The callback successfully handled the action.
  @retval  FALSE         The callback not supported in this handler.

**/
BOOLEAN
UiSupportLibCallbackHandler (
  IN  EFI_HII_HANDLE              HiiHandle,
  IN  EFI_BROWSER_ACTION          Action,
  IN  EFI_QUESTION_ID             QuestionId,
  IN  UINT8                       Type,
  IN  EFI_IFR_TYPE_VALUE          *Value,
  OUT EFI_BROWSER_ACTION_REQUEST  *ActionRequest,
  OUT EFI_STATUS                  *Status
  )
{
  if ((QuestionId != FRONT_PAGE_KEY_CONTINUE) &&
      (QuestionId != FRONT_PAGE_KEY_RESET))
  {
    return FALSE;
  }

  if (Action == EFI_BROWSER_ACTION_RETRIEVE) {
    *Status = EFI_UNSUPPORTED;
    return TRUE;
  }

  if (Action != EFI_BROWSER_ACTION_CHANGED) {
    //
    // Do nothing for other UEFI Action. Only do call back when data is changed.
    //
    *Status = EFI_UNSUPPORTED;
    return TRUE;
  }

  if (Action == EFI_BROWSER_ACTION_CHANGED) {
    if ((Value == NULL) || (ActionRequest == NULL)) {
      *Status = EFI_INVALID_PARAMETER;
      return TRUE;
    }

    *Status = EFI_SUCCESS;
    switch (QuestionId) {
      case FRONT_PAGE_KEY_CONTINUE:
        //
        // This is the continue - clear the screen and return an error to get out of FrontPage loop
        //
        *ActionRequest = EFI_BROWSER_ACTION_REQUEST_EXIT;
        break;

      case FRONT_PAGE_KEY_RESET:
        //
        // Reset
        //
        gRT->ResetSystem (EfiResetCold, EFI_SUCCESS, 0, NULL);
        *Status = EFI_UNSUPPORTED;

      default:
        break;
    }
  }

  return TRUE;
}

/**
  Create continue menu in the front page.

  @param[in]    HiiHandle           The hii handle for the Uiapp driver.
  @param[in]    StartOpCodeHandle   The opcode handle to save the new opcode.

**/
VOID
UiCreateContinueMenu (
  IN EFI_HII_HANDLE  HiiHandle,
  IN VOID            *StartOpCodeHandle
  )
{
  HiiCreateActionOpCode (
    StartOpCodeHandle,
    FRONT_PAGE_KEY_CONTINUE,
    STRING_TOKEN (STR_BOOT_DEFAULT_PROMPT),
    STRING_TOKEN (STR_BOOT_DEFAULT_HELP),
    EFI_IFR_FLAG_CALLBACK,
    0
    );
}

/**
  Create empty line menu in the front page.

  @param    HiiHandle           The hii handle for the Uiapp driver.
  @param    StartOpCodeHandle   The opcode handle to save the new opcode.

**/
VOID
UiCreateEmptyLine (
  IN EFI_HII_HANDLE  HiiHandle,
  IN VOID            *StartOpCodeHandle
  )
{
  HiiCreateSubTitleOpCode (StartOpCodeHandle, STRING_TOKEN (STR_NULL_STRING), 0, 0, 0);
}

/**
  Create Reset menu in the front page.

  @param[in]    HiiHandle           The hii handle for the Uiapp driver.
  @param[in]    StartOpCodeHandle   The opcode handle to save the new opcode.

**/
VOID
UiCreateResetMenu (
  IN EFI_HII_HANDLE  HiiHandle,
  IN VOID            *StartOpCodeHandle
  )
{
  HiiCreateActionOpCode (
    StartOpCodeHandle,
    FRONT_PAGE_KEY_RESET,
    STRING_TOKEN (STR_RESET_STRING),
    STRING_TOKEN (STR_RESET_STRING_HELP),
    EFI_IFR_FLAG_CALLBACK,
    0
    );
}

/**
  Extract device path for given HII handle and class guid.

  @param Handle          The HII handle.

  @retval  NULL          Fail to get the device path string.
  @return  PathString    Get the device path string.

**/
CHAR16 *
ExtractDevicePathFromHiiHandle (
  IN      EFI_HII_HANDLE  Handle
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  DriverHandle;

  ASSERT (Handle != NULL);

  if (Handle == NULL) {
    return NULL;
  }

  Status = gHiiDatabase->GetPackageListHandle (gHiiDatabase, Handle, &DriverHandle);
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  return ConvertDevicePathToText (DevicePathFromHandle (DriverHandle), FALSE, FALSE);
}

/**
  Check whether this driver need to be shown in the front page.

  @param    HiiHandle           The hii handle for the driver.
  @param    Guid                The special guid for the driver which is the target.
  @param    PromptId            Return the prompt string id.
  @param    HelpId              Return the help string id.
  @param    FormsetGuid         Return the formset guid info.

  @retval   EFI_SUCCESS         Search the driver success

**/
BOOLEAN
RequiredDriver (
  IN  EFI_HII_HANDLE  HiiHandle,
  IN  EFI_GUID        *Guid,
  OUT EFI_STRING_ID   *PromptId,
  OUT EFI_STRING_ID   *HelpId,
  OUT VOID            *FormsetGuid
  )
{
  EFI_STATUS        Status;
  UINT8             ClassGuidNum;
  EFI_GUID          *ClassGuid;
  EFI_IFR_FORM_SET  *Buffer;
  UINTN             BufferSize;
  UINT8             *Ptr;
  UINTN             TempSize;
  BOOLEAN           RetVal;

  Status = HiiGetFormSetFromHiiHandle (HiiHandle, &Buffer, &BufferSize);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  RetVal   = FALSE;
  TempSize = 0;
  Ptr      = (UINT8 *)Buffer;
  while (TempSize < BufferSize) {
    TempSize += ((EFI_IFR_OP_HEADER *)Ptr)->Length;

    if (((EFI_IFR_OP_HEADER *)Ptr)->Length <= OFFSET_OF (EFI_IFR_FORM_SET, Flags)) {
      Ptr += ((EFI_IFR_OP_HEADER *)Ptr)->Length;
      continue;
    }

    ClassGuidNum = (UINT8)(((EFI_IFR_FORM_SET *)Ptr)->Flags & 0x3);
    ClassGuid    = (EFI_GUID *)(VOID *)(Ptr + sizeof (EFI_IFR_FORM_SET));
    while (ClassGuidNum-- > 0) {
      if (!CompareGuid (Guid, ClassGuid)) {
        ClassGuid++;
        continue;
      }

      *PromptId = ((EFI_IFR_FORM_SET *)Ptr)->FormSetTitle;
      *HelpId   = ((EFI_IFR_FORM_SET *)Ptr)->Help;
      CopyMem (FormsetGuid, &((EFI_IFR_FORM_SET *)Ptr)->Guid, sizeof (EFI_GUID));
      RetVal = TRUE;
    }
  }

  FreePool (Buffer);

  return RetVal;
}

/**
  Search the drivers in the system which need to show in the front page
  and insert the menu to the front page.

  @param    HiiHandle           The hii handle for the Uiapp driver.
  @param    ClassGuid           The class guid for the driver which is the target.
  @param    SpecialHandlerFn    The pointer to the special handler function, if any.
  @param    StartOpCodeHandle   The opcode handle to save the new opcode.

  @retval   EFI_SUCCESS         Search the driver success

**/
EFI_STATUS
UiListThirdPartyDrivers (
  IN EFI_HII_HANDLE          HiiHandle,
  IN EFI_GUID                *ClassGuid,
  IN DRIVER_SPECIAL_HANDLER  SpecialHandlerFn,
  IN VOID                    *StartOpCodeHandle
  )
{
  UINTN                   Index;
  EFI_STRING              String;
  EFI_STRING_ID           Token;
  EFI_STRING_ID           TokenHelp;
  EFI_HII_HANDLE          *HiiHandles;
  CHAR16                  *DevicePathStr;
  UINTN                   Count;
  UINTN                   CurrentSize;
  UI_HII_DRIVER_INSTANCE  *DriverListPtr;
  EFI_STRING              NewName;
  BOOLEAN                 EmptyLineAfter;

  if (gHiiDriverList != NULL) {
    FreePool (gHiiDriverList);
  }

  HiiHandles = HiiGetHiiHandles (NULL);
  ASSERT (HiiHandles != NULL);

  gHiiDriverList = AllocateZeroPool (UI_HII_DRIVER_LIST_SIZE * sizeof (UI_HII_DRIVER_INSTANCE));
  ASSERT (gHiiDriverList != NULL);
  DriverListPtr = gHiiDriverList;
  CurrentSize   = UI_HII_DRIVER_LIST_SIZE;

  for (Index = 0, Count = 0; HiiHandles[Index] != NULL; Index++) {
    if (!RequiredDriver (HiiHandles[Index], ClassGuid, &Token, &TokenHelp, &gHiiDriverList[Count].FormSetGuid)) {
      continue;
    }

    String = HiiGetString (HiiHandles[Index], Token, NULL);
    if (String == NULL) {
      String = HiiGetString (gStringPackHandle, STRING_TOKEN (STR_MISSING_STRING), NULL);
      ASSERT (String != NULL);
    } else if (SpecialHandlerFn != NULL) {
      //
      // Check whether need to rename the driver name.
      //
      EmptyLineAfter = FALSE;
      if (SpecialHandlerFn (String, &NewName, &EmptyLineAfter)) {
        FreePool (String);
        String                              = NewName;
        DriverListPtr[Count].EmptyLineAfter = EmptyLineAfter;
      }
    }

    DriverListPtr[Count].PromptId = HiiSetString (HiiHandle, 0, String, NULL);
    FreePool (String);

    String = HiiGetString (HiiHandles[Index], TokenHelp, NULL);
    if (String == NULL) {
      String = HiiGetString (gStringPackHandle, STRING_TOKEN (STR_MISSING_STRING), NULL);
      ASSERT (String != NULL);
    }

    DriverListPtr[Count].HelpId = HiiSetString (HiiHandle, 0, String, NULL);
    FreePool (String);

    DevicePathStr = ExtractDevicePathFromHiiHandle (HiiHandles[Index]);
    if (DevicePathStr != NULL) {
      DriverListPtr[Count].DevicePathId = HiiSetString (HiiHandle, 0, DevicePathStr, NULL);
      FreePool (DevicePathStr);
    } else {
      DriverListPtr[Count].DevicePathId = 0;
    }

    Count++;
    if (Count >= CurrentSize) {
      DriverListPtr = ReallocatePool (
                        CurrentSize * sizeof (UI_HII_DRIVER_INSTANCE),
                        (Count + UI_HII_DRIVER_LIST_SIZE)
                        * sizeof (UI_HII_DRIVER_INSTANCE),
                        gHiiDriverList
                        );
      ASSERT (DriverListPtr != NULL);
      gHiiDriverList = DriverListPtr;
      CurrentSize   += UI_HII_DRIVER_LIST_SIZE;
    }
  }

  FreePool (HiiHandles);

  Index = 0;
  while (gHiiDriverList[Index].PromptId != 0) {
    HiiCreateGotoExOpCode (
      StartOpCodeHandle,
      0,
      gHiiDriverList[Index].PromptId,
      gHiiDriverList[Index].HelpId,
      0,
      (EFI_QUESTION_ID)(Index + FRONT_PAGE_KEY_DRIVER),
      0,
      &gHiiDriverList[Index].FormSetGuid,
      gHiiDriverList[Index].DevicePathId
      );

    // Always add an empty line after each entry
    UiCreateEmptyLine (HiiHandle, StartOpCodeHandle);

    Index++;
  }

  return EFI_SUCCESS;
}
