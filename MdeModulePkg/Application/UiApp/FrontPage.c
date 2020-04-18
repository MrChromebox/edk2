/** @file
  FrontPage routines to handle the callbacks and browser calls

Copyright (c) 2004 - 2017, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2018 Hewlett Packard Enterprise Development LP<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "FrontPage.h"
#include "FrontPageCustomizedUi.h"

#define MAX_STRING_LEN            200

EFI_GUID   mFrontPageGuid      = FRONT_PAGE_FORMSET_GUID;

BOOLEAN   mResetRequired  = FALSE;

EFI_FORM_BROWSER2_PROTOCOL      *gFormBrowser2;
CHAR8     *mLanguageString;
BOOLEAN   mModeInitialized = FALSE;

FRONT_PAGE_CALLBACK_DATA  gFrontPagePrivate = {
  FRONT_PAGE_CALLBACK_DATA_SIGNATURE,
  NULL,
  NULL,
  NULL,
  {
    FakeExtractConfig,
    FakeRouteConfig,
    FrontPageCallback
  }
};

HII_VENDOR_DEVICE_PATH  mFrontPageHiiVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8) (sizeof (VENDOR_DEVICE_PATH)),
        (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    //
    // {8E6D99EE-7531-48f8-8745-7F6144468FF2}
    //
    { 0x8e6d99ee, 0x7531, 0x48f8, { 0x87, 0x45, 0x7f, 0x61, 0x44, 0x46, 0x8f, 0xf2 } }
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      (UINT8) (END_DEVICE_PATH_LENGTH),
      (UINT8) ((END_DEVICE_PATH_LENGTH) >> 8)
    }
  }
};

/**
  Update the banner information for the Front Page based on Smbios information.

**/
VOID
UpdateFrontPageBannerStrings (
  VOID
  );

/**
  This function allows a caller to extract the current configuration for one
  or more named elements from the target driver.


  @param This            Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param Request         A null-terminated Unicode string in <ConfigRequest> format.
  @param Progress        On return, points to a character in the Request string.
                         Points to the string's null terminator if request was successful.
                         Points to the most recent '&' before the first failing name/value
                         pair (or the beginning of the string if the failure is in the
                         first name/value pair) if the request was not successful.
  @param Results         A null-terminated Unicode string in <ConfigAltResp> format which
                         has all values filled in for the names in the Request string.
                         String to be allocated by the called function.

  @retval  EFI_SUCCESS            The Results is filled with the requested values.
  @retval  EFI_OUT_OF_RESOURCES   Not enough memory to store the results.
  @retval  EFI_INVALID_PARAMETER  Request is illegal syntax, or unknown name.
  @retval  EFI_NOT_FOUND          Routing data doesn't match any storage in this driver.

**/
EFI_STATUS
EFIAPI
FakeExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Request,
  OUT EFI_STRING                             *Progress,
  OUT EFI_STRING                             *Results
  )
{
  if (Progress == NULL || Results == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  *Progress = Request;
  return EFI_NOT_FOUND;
}

/**
  This function processes the results of changes in configuration.


  @param This            Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param Configuration   A null-terminated Unicode string in <ConfigResp> format.
  @param Progress        A pointer to a string filled in with the offset of the most
                         recent '&' before the first failing name/value pair (or the
                         beginning of the string if the failure is in the first
                         name/value pair) or the terminating NULL if all was successful.

  @retval  EFI_SUCCESS            The Results is processed successfully.
  @retval  EFI_INVALID_PARAMETER  Configuration is NULL.
  @retval  EFI_NOT_FOUND          Routing data doesn't match any storage in this driver.

**/
EFI_STATUS
EFIAPI
FakeRouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Configuration,
  OUT EFI_STRING                             *Progress
  )
{
  if (Configuration == NULL || Progress == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Progress = Configuration;

  return EFI_NOT_FOUND;
}

/**
  This function processes the results of changes in configuration.


  @param This            Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param Action          Specifies the type of action taken by the browser.
  @param QuestionId      A unique value which is sent to the original exporting driver
                         so that it can identify the type of data to expect.
  @param Type            The type of value for the question.
  @param Value           A pointer to the data being sent to the original exporting driver.
  @param ActionRequest   On return, points to the action requested by the callback function.

  @retval  EFI_SUCCESS           The callback successfully handled the action.
  @retval  EFI_OUT_OF_RESOURCES  Not enough storage is available to hold the variable and its data.
  @retval  EFI_DEVICE_ERROR      The variable could not be saved.
  @retval  EFI_UNSUPPORTED       The specified Action is not supported by the callback.

**/
EFI_STATUS
EFIAPI
FrontPageCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  EFI_BROWSER_ACTION                     Action,
  IN  EFI_QUESTION_ID                        QuestionId,
  IN  UINT8                                  Type,
  IN  EFI_IFR_TYPE_VALUE                     *Value,
  OUT EFI_BROWSER_ACTION_REQUEST             *ActionRequest
  )
{
  return UiFrontPageCallbackHandler (gFrontPagePrivate.HiiHandle, Action, QuestionId, Type, Value, ActionRequest);
}

/**

  Update the menus in the front page.

**/
VOID
UpdateFrontPageForm (
  VOID
  )
{
  VOID                        *StartOpCodeHandle;
  VOID                        *EndOpCodeHandle;
  EFI_IFR_GUID_LABEL          *StartGuidLabel;
  EFI_IFR_GUID_LABEL          *EndGuidLabel;

  //
  // Allocate space for creation of UpdateData Buffer
  //
  StartOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (StartOpCodeHandle != NULL);

  EndOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (EndOpCodeHandle != NULL);
  //
  // Create Hii Extend Label OpCode as the start opcode
  //
  StartGuidLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (StartOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  StartGuidLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  StartGuidLabel->Number       = LABEL_FRONTPAGE_INFORMATION;
  //
  // Create Hii Extend Label OpCode as the end opcode
  //
  EndGuidLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (EndOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  EndGuidLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  EndGuidLabel->Number       = LABEL_END;

  //
  //Updata Front Page form
  //
  UiCustomizeFrontPage (
    gFrontPagePrivate.HiiHandle,
    StartOpCodeHandle
    );

  HiiUpdateForm (
    gFrontPagePrivate.HiiHandle,
    &mFrontPageGuid,
    FRONT_PAGE_FORM_ID,
    StartOpCodeHandle,
    EndOpCodeHandle
    );

  HiiFreeOpCodeHandle (StartOpCodeHandle);
  HiiFreeOpCodeHandle (EndOpCodeHandle);
}

/**
  Initialize HII information for the FrontPage


  @retval  EFI_SUCCESS        The operation is successful.
  @retval  EFI_DEVICE_ERROR   If the dynamic opcode creation failed.

**/
EFI_STATUS
InitializeFrontPage (
  VOID
  )
{
  EFI_STATUS                  Status;
  //
  // Locate Hii relative protocols
  //
  Status = gBS->LocateProtocol (&gEfiFormBrowser2ProtocolGuid, NULL, (VOID **) &gFormBrowser2);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Install Device Path Protocol and Config Access protocol to driver handle
  //
  gFrontPagePrivate.DriverHandle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &gFrontPagePrivate.DriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mFrontPageHiiVendorDevicePath,
                  &gEfiHiiConfigAccessProtocolGuid,
                  &gFrontPagePrivate.ConfigAccess,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Publish our HII data
  //
  gFrontPagePrivate.HiiHandle = HiiAddPackages (
                                  &mFrontPageGuid,
                                  gFrontPagePrivate.DriverHandle,
                                  FrontPageVfrBin,
                                  UiAppStrings,
                                  NULL
                                  );
  ASSERT (gFrontPagePrivate.HiiHandle != NULL);

  //
  //Updata Front Page banner strings
  //
  UpdateFrontPageBannerStrings ();

  //
  // Update front page menus.
  //
  UpdateFrontPageForm();

  return Status;
}

/**
  Call the browser and display the front page

  @return   Status code that will be returned by
            EFI_FORM_BROWSER2_PROTOCOL.SendForm ().

**/
EFI_STATUS
CallFrontPage (
  VOID
  )
{
  EFI_STATUS                  Status;
  EFI_BROWSER_ACTION_REQUEST  ActionRequest;

  //
  // Begin waiting for USER INPUT
  //
  REPORT_STATUS_CODE (
    EFI_PROGRESS_CODE,
    (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_PC_INPUT_WAIT)
    );

  ActionRequest = EFI_BROWSER_ACTION_REQUEST_NONE;
  Status = gFormBrowser2->SendForm (
                            gFormBrowser2,
                            &gFrontPagePrivate.HiiHandle,
                            1,
                            &mFrontPageGuid,
                            0,
                            NULL,
                            &ActionRequest
                            );
  //
  // Check whether user change any option setting which needs a reset to be effective
  //
  if (ActionRequest == EFI_BROWSER_ACTION_REQUEST_RESET) {
    EnableResetRequired ();
  }

  return Status;
}

/**
  Remove the installed packages from the HiiDatabase.

**/
VOID
FreeFrontPage(
  VOID
  )
{
  EFI_STATUS                  Status;
  Status = gBS->UninstallMultipleProtocolInterfaces (
                  gFrontPagePrivate.DriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mFrontPageHiiVendorDevicePath,
                  &gEfiHiiConfigAccessProtocolGuid,
                  &gFrontPagePrivate.ConfigAccess,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Publish our HII data
  //
  HiiRemovePackages (gFrontPagePrivate.HiiHandle);
  if (gFrontPagePrivate.LanguageToken != NULL) {
    FreePool (gFrontPagePrivate.LanguageToken);
    gFrontPagePrivate.LanguageToken = NULL;
  }
}

/**
  Convert Processor Frequency Data to a string.

  @param ProcessorFrequency The frequency data to process
  @param Base10Exponent     The exponent based on 10
  @param String             The string that is created

**/
VOID
ConvertProcessorToString (
  IN  UINT16                               ProcessorFrequency,
  IN  UINT16                               Base10Exponent,
  OUT CHAR16                               **String
  )
{
  CHAR16  *StringBuffer;
  UINTN   Index;
  UINTN   DestMax;
  UINT32  FreqMhz;

  if (Base10Exponent >= 6) {
    FreqMhz = ProcessorFrequency;
    for (Index = 0; Index < (UINT32) Base10Exponent - 6; Index++) {
      FreqMhz *= 10;
    }
  } else {
    FreqMhz = 0;
  }
  DestMax = 0x20 / sizeof (CHAR16);
  StringBuffer = AllocateZeroPool (0x20);
  ASSERT (StringBuffer != NULL);
  UnicodeValueToStringS (StringBuffer, sizeof (CHAR16) * DestMax, LEFT_JUSTIFY, FreqMhz / 1000, 3);
  Index = StrnLenS (StringBuffer, DestMax);
  StrCatS (StringBuffer, DestMax, L".");
  UnicodeValueToStringS (
    StringBuffer + Index + 1,
    sizeof (CHAR16) * (DestMax - (Index + 1)),
    PREFIX_ZERO,
    (FreqMhz % 1000) / 10,
    2
    );
  StrCatS (StringBuffer, DestMax, L" GHz");
  *String = (CHAR16 *) StringBuffer;
  return ;
}


/**
  Convert Memory Size to a string.

  @param MemorySize      The size of the memory to process
  @param String          The string that is created

**/
VOID
ConvertMemorySizeToString (
  IN  UINT32          MemorySize,
  OUT CHAR16          **String
  )
{
  CHAR16  *StringBuffer;

  StringBuffer = AllocateZeroPool (0x24);
  ASSERT (StringBuffer != NULL);
  UnicodeValueToStringS (StringBuffer, 0x24, LEFT_JUSTIFY, MemorySize, 10);
  StrCatS (StringBuffer, 0x24 / sizeof (CHAR16), L" MB RAM");

  *String = (CHAR16 *) StringBuffer;

  return ;
}

/**

  Acquire the string associated with the Index from smbios structure and return it.
  The caller is responsible for free the string buffer.

  @param    OptionalStrStart  The start position to search the string
  @param    Index             The index of the string to extract
  @param    String            The string that is extracted

  @retval   EFI_SUCCESS       The function returns EFI_SUCCESS always.

**/
EFI_STATUS
GetOptionalStringByIndex (
  IN      CHAR8                   *OptionalStrStart,
  IN      UINT8                   Index,
  OUT     CHAR16                  **String
  )
{
  UINTN          StrSize;

  if (Index == 0) {
    *String = AllocateZeroPool (sizeof (CHAR16));
    return EFI_SUCCESS;
  }

  StrSize = 0;
  do {
    Index--;
    OptionalStrStart += StrSize;
    StrSize           = AsciiStrSize (OptionalStrStart);
  } while (OptionalStrStart[StrSize] != 0 && Index != 0);

  if ((Index != 0) || (StrSize == 1)) {
    //
    // Meet the end of strings set but Index is non-zero, or
    // Find an empty string
    //
    *String = GetStringById (STRING_TOKEN (STR_MISSING_STRING));
  } else {
    *String = AllocatePool (StrSize * sizeof (CHAR16));
    AsciiStrToUnicodeStrS (OptionalStrStart, *String, StrSize);
  }

  return EFI_SUCCESS;
}

UINT16 SmbiosTableLength (SMBIOS_STRUCTURE_POINTER SmbiosTableN)
{
  CHAR8  *AChar;
  UINT16  Length;

  AChar = (CHAR8 *)(SmbiosTableN.Raw + SmbiosTableN.Hdr->Length);
  while ((*AChar != 0) || (*(AChar + 1) != 0)) {
    AChar ++; //stop at 00 - first 0
  }
  Length = (UINT16)((UINTN)AChar - (UINTN)SmbiosTableN.Raw + 2); //length includes 00
  return Length;
}

SMBIOS_STRUCTURE_POINTER GetSmbiosTableFromType (
  SMBIOS_TABLE_ENTRY_POINT *SmbiosPoint, UINT8 SmbiosType, UINTN IndexTable)
{
  SMBIOS_STRUCTURE_POINTER SmbiosTableN;
  UINTN                    SmbiosTypeIndex;

  SmbiosTypeIndex = 0;
  SmbiosTableN.Raw = (UINT8 *)((UINTN)SmbiosPoint->TableAddress);
  if (SmbiosTableN.Raw == NULL) {
    return SmbiosTableN;
  }
  while ((SmbiosTypeIndex != IndexTable) || (SmbiosTableN.Hdr->Type != SmbiosType)) {
    if (SmbiosTableN.Hdr->Type == SMBIOS_TYPE_END_OF_TABLE) {
      SmbiosTableN.Raw = NULL;
      return SmbiosTableN;
    }
    if (SmbiosTableN.Hdr->Type == SmbiosType) {
      SmbiosTypeIndex++;
    }
    SmbiosTableN.Raw = (UINT8 *)(SmbiosTableN.Raw + SmbiosTableLength (SmbiosTableN));
  }
  return SmbiosTableN;
}

/**

  Update the banner information for the Front Page based on Smbios information.

**/
VOID
UpdateFrontPageBannerStrings (
  VOID
  )
{
  UINT8                             StrIndex;
  UINT8                             Str2Index;
  CHAR16                            *NewString;
  CHAR16                            *NewString2;
  CHAR16                            *NewString3;
  EFI_STATUS                        Status;
  EFI_STRING_ID                     TokenToUpdate;
  EFI_PHYSICAL_ADDRESS              *Table;
  SMBIOS_TABLE_ENTRY_POINT          *EntryPoint;
  SMBIOS_STRUCTURE_POINTER          SmbiosTable;
  UINT64                            InstalledMemory;

  InstalledMemory = 0;

  //
  // Update Front Page strings
  //
  Status = EfiGetSystemConfigurationTable (&gEfiSmbiosTableGuid, (VOID **)  &Table);
  if (EFI_ERROR (Status) || Table == NULL) {
  } else {
    EntryPoint = (SMBIOS_TABLE_ENTRY_POINT*)Table;

    SmbiosTable = GetSmbiosTableFromType (EntryPoint, EFI_SMBIOS_TYPE_BIOS_INFORMATION , 0);
    if (SmbiosTable.Raw != NULL) {
      NewString3 = AllocateZeroPool (0x60);

      StrIndex = SmbiosTable.Type0->BiosVersion;
      Str2Index = SmbiosTable.Type0->BiosReleaseDate;
      GetOptionalStringByIndex ((CHAR8*)((UINT8*)SmbiosTable.Raw + SmbiosTable.Hdr->Length), StrIndex, &NewString);
      GetOptionalStringByIndex ((CHAR8*)((UINT8*)SmbiosTable.Raw + SmbiosTable.Hdr->Length), Str2Index, &NewString2);
      StrCatS (NewString3, 0x60 / sizeof (CHAR16), L"FW: ");
      StrCatS (NewString3, 0x60 / sizeof (CHAR16), NewString);
      StrCatS (NewString3, 0x60 / sizeof (CHAR16), L" ");
      StrCatS (NewString3, 0x60 / sizeof (CHAR16), NewString2);
      TokenToUpdate = STRING_TOKEN (STR_FRONT_PAGE_BIOS_VERSION);
      HiiSetString (gFrontPagePrivate.HiiHandle, TokenToUpdate, NewString3, NULL);
      FreePool (NewString);
    }

    SmbiosTable = GetSmbiosTableFromType (EntryPoint, SMBIOS_TYPE_SYSTEM_INFORMATION , 0);
    if (SmbiosTable.Raw != NULL) {
      NewString3 = AllocateZeroPool (0x60);

      StrIndex = SmbiosTable.Type1->ProductName;
      Str2Index = SmbiosTable.Type1->Manufacturer;
      GetOptionalStringByIndex ((CHAR8*)((UINT8*)SmbiosTable.Raw + SmbiosTable.Hdr->Length), StrIndex, &NewString);
      GetOptionalStringByIndex ((CHAR8*)((UINT8*)SmbiosTable.Raw + SmbiosTable.Hdr->Length), Str2Index, &NewString2);
       if (!StrCmp(NewString2, L"GOOGLE") || !StrCmp(NewString2, L"Google")) {
          NewString2 = AllocateZeroPool (0x60);
          GetDeviceNameFromProduct(NewString, &NewString2);
          StrCatS (NewString3, 0x60 / sizeof (CHAR16), NewString2);
          StrCatS (NewString3, 0x60 / sizeof (CHAR16), L" (");
          StrCatS (NewString3, 0x60 / sizeof (CHAR16), NewString);
          StrCatS (NewString3, 0x60 / sizeof (CHAR16), L")");
       } else {
          StrCatS (NewString3, 0x60 / sizeof (CHAR16), NewString2);
          StrCatS (NewString3, 0x60 / sizeof (CHAR16), L" ");
          StrCatS (NewString3, 0x60 / sizeof (CHAR16), NewString);
       }
      TokenToUpdate = STRING_TOKEN (STR_FRONT_PAGE_COMPUTER_MODEL);
      HiiSetString (gFrontPagePrivate.HiiHandle, TokenToUpdate, NewString3, NULL);
      FreePool (NewString);
    }

    SmbiosTable = GetSmbiosTableFromType (EntryPoint, SMBIOS_TYPE_PROCESSOR_INFORMATION , 0);
    if (SmbiosTable.Raw != NULL) {
      NewString3 = AllocateZeroPool (0x60);
      StrIndex = SmbiosTable.Type4->ProcessorVersion;
      GetOptionalStringByIndex ((CHAR8*)((UINT8*)SmbiosTable.Raw + SmbiosTable.Hdr->Length), StrIndex, &NewString);
      StrCatS (NewString3, 0x60 / sizeof (CHAR16), NewString);
      while (NewString3[0] == 0x20) {
        NewString3 = &NewString3[1];
      }
      TokenToUpdate = STRING_TOKEN (STR_FRONT_PAGE_CPU_MODEL);
      HiiSetString (gFrontPagePrivate.HiiHandle, TokenToUpdate, NewString3, NULL);
      FreePool (NewString);
    }

    SmbiosTable = GetSmbiosTableFromType (EntryPoint, SMBIOS_TYPE_MEMORY_ARRAY_MAPPED_ADDRESS , 0);
    if (SmbiosTable.Raw != NULL) {
      if (SmbiosTable.Type19->StartingAddress != 0xFFFFFFFF ) {
        InstalledMemory += RShiftU64(SmbiosTable.Type19->EndingAddress -
                                     SmbiosTable.Type19->StartingAddress + 1, 10);
      } else {
        InstalledMemory += RShiftU64(SmbiosTable.Type19->ExtendedEndingAddress -
                                     SmbiosTable.Type19->ExtendedStartingAddress + 1, 20);
      }

      // now update the total installed RAM size
      ConvertMemorySizeToString ((UINT32)InstalledMemory, &NewString );
      TokenToUpdate = STRING_TOKEN (STR_FRONT_PAGE_MEMORY_SIZE);
      HiiSetString (gFrontPagePrivate.HiiHandle, TokenToUpdate, NewString, NULL);
      FreePool (NewString);
    }
  }
}

/**
  The user Entry Point for Application. The user code starts with this function
  as the real entry point for the image goes into a library that calls this
  function.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializeUserInterface (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_HII_HANDLE                     HiiHandle;
  EFI_STATUS                         Status;
  EFI_GRAPHICS_OUTPUT_PROTOCOL       *GraphicsOutput;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL    *SimpleTextOut;

  if (!mModeInitialized) {
    //
    // After the console is ready, set current video resolution
    // and text mode before launching setup at first time.
    //
    Status = gBS->HandleProtocol (
                    gST->ConsoleOutHandle,
                    &gEfiGraphicsOutputProtocolGuid,
                    (VOID**)&GraphicsOutput
                    );
    if (EFI_ERROR (Status)) {
      GraphicsOutput = NULL;
    }

    Status = gBS->HandleProtocol (
                    gST->ConsoleOutHandle,
                    &gEfiSimpleTextOutProtocolGuid,
                    (VOID**)&SimpleTextOut
                    );
    if (EFI_ERROR (Status)) {
      SimpleTextOut = NULL;
    }

    if (GraphicsOutput != NULL) {
      // Set native display resolution
      GraphicsOutput->SetMode(GraphicsOutput, 0);
    }

    if (SimpleTextOut != NULL) {
      // Set largest/highest text mode
      SimpleTextOut->SetMode (SimpleTextOut, SimpleTextOut->Mode->MaxMode);
    }

    mModeInitialized           = TRUE;
  }

  gBS->SetWatchdogTimer (0x0000, 0x0000, 0x0000, NULL);
  gST->ConOut->ClearScreen (gST->ConOut);

  //
  // Install customized fonts needed by Front Page
  //
  HiiHandle = ExportFonts ();
  ASSERT (HiiHandle != NULL);

  InitializeStringSupport ();

  UiEntry (FALSE);

  UninitializeStringSupport ();
  HiiRemovePackages (HiiHandle);

  return EFI_SUCCESS;
}

/**
  This function is the main entry of the UI entry.
  The function will present the main menu of the system UI.

  @param ConnectAllHappened Caller passes the value to UI to avoid unnecessary connect-all.

**/
VOID
EFIAPI
UiEntry (
  IN BOOLEAN                      ConnectAllHappened
  )
{
  EFI_STATUS                    Status;
  EFI_BOOT_LOGO_PROTOCOL        *BootLogo;

  //
  // Enter Setup page.
  //
  REPORT_STATUS_CODE (
    EFI_PROGRESS_CODE,
    (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_PC_USER_SETUP)
    );

  //
  // Indicate if the connect all has been performed before.
  // If has not been performed before, do here.
  //
  if (!ConnectAllHappened) {
    EfiBootManagerConnectAll ();
  }

  //
  // The boot option enumeration time is acceptable in Ui driver
  //
  EfiBootManagerRefreshAllBootOption ();

  //
  // Boot Logo is corrupted, report it using Boot Logo protocol.
  //
  Status = gBS->LocateProtocol (&gEfiBootLogoProtocolGuid, NULL, (VOID **) &BootLogo);
  if (!EFI_ERROR (Status) && (BootLogo != NULL)) {
    BootLogo->SetBootLogo (BootLogo, NULL, 0, 0, 0, 0);
  }

  InitializeFrontPage ();

  CallFrontPage ();

  FreeFrontPage ();

  if (mLanguageString != NULL) {
    FreePool (mLanguageString);
    mLanguageString = NULL;
  }

  //
  //Will leave browser, check any reset required change is applied? if yes, reset system
  //
  SetupResetReminder ();
}

//
//  Following are BDS Lib functions which contain all the code about setup browser reset reminder feature.
//  Setup Browser reset reminder feature is that an reset reminder will be given before user leaves the setup browser  if
//  user change any option setting which needs a reset to be effective, and  the reset will be applied according to  the user selection.
//





/**
  Record the info that  a reset is required.
  A  module boolean variable is used to record whether a reset is required.

**/
VOID
EFIAPI
EnableResetRequired (
  VOID
  )
{
  mResetRequired = TRUE;
}





/**
  Check if  user changed any option setting which needs a system reset to be effective.

**/
BOOLEAN
EFIAPI
IsResetRequired (
  VOID
  )
{
  return mResetRequired;
}


/**
  Check whether a reset is needed, and finish the reset reminder feature.
  If a reset is needed, Popup a menu to notice user, and finish the feature
  according to the user selection.

**/
VOID
EFIAPI
SetupResetReminder (
  VOID
  )
{
  EFI_INPUT_KEY                 Key;
  CHAR16                        *StringBuffer1;
  CHAR16                        *StringBuffer2;

  //
  //check any reset required change is applied? if yes, reset system
  //
  if (IsResetRequired ()) {

    StringBuffer1 = AllocateZeroPool (MAX_STRING_LEN * sizeof (CHAR16));
    ASSERT (StringBuffer1 != NULL);
    StringBuffer2 = AllocateZeroPool (MAX_STRING_LEN * sizeof (CHAR16));
    ASSERT (StringBuffer2 != NULL);
    StrCpyS (StringBuffer1, MAX_STRING_LEN, L"Configuration changed. Reset to apply it Now.");
    StrCpyS (StringBuffer2, MAX_STRING_LEN, L"Press ENTER to reset");
    //
    // Popup a menu to notice user
    //
    do {
      CreatePopUp (EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE, &Key, StringBuffer1, StringBuffer2, NULL);
    } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);

    FreePool (StringBuffer1);
    FreePool (StringBuffer2);

    gRT->ResetSystem (EfiResetCold, EFI_SUCCESS, 0, NULL);
  }
}

