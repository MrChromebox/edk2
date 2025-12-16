/** @file
  UserAuthentication DXE password wrapper.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UserAuthenticationDxe.h"

UINTN                           mAdminPasswordTryCount = 0;

BOOLEAN                         mNeedReVerify = TRUE;
BOOLEAN                         mPasswordVerified = FALSE;

/**
  Verify if the password is correct.

  @param[in]  Password               The user input password.
  @param[in]  PasswordSize           The size of Password in byte.
  @param[in]  UserPasswordVarStruct  The storage of password in variable.

  @retval EFI_SUCCESS              The password is correct.
  @retval EFI_SECURITY_VIOLATION   The password is incorrect.
**/
EFI_STATUS
CheckPassword (
  IN CHAR8                          *Password,
  IN UINTN                          PasswordSize,
  IN USER_PASSWORD_VAR_STRUCT       *UserPasswordVarStruct
  )
{
  BOOLEAN  HashOk;
  UINT8    HashData[PASSWORD_HASH_SIZE];

  HashOk = KeyLibGeneratePBKDF2Hash (
             HASH_TYPE_SHA256,
             (UINT8 *)Password,
             PasswordSize,
             UserPasswordVarStruct->PasswordSalt,
             sizeof(UserPasswordVarStruct->PasswordSalt),
             HashData,
             sizeof(HashData)
             );
  if (!HashOk) {
    return EFI_DEVICE_ERROR;
  }
  if (KeyLibSlowCompareMem (UserPasswordVarStruct->PasswordHash, HashData, PASSWORD_HASH_SIZE) == 0) {
    return EFI_SUCCESS;
  } else {
    return EFI_SECURITY_VIOLATION;
  }
}

/**
  Get hash data of password from non-volatile variable region.

  @param[in]   UserGuid               The user GUID of the password variable.
  @param[in]   Index                  The index of the password.
                                      0 means current password.
                                      Non-0 means the password history.
  @param[out]  UserPasswordVarStruct  The storage of password in variable.

  @retval EFI_SUCCESS             The password hash is returned successfully.
  @retval EFI_NOT_FOUND           The password hash is not found.
**/
EFI_STATUS
GetPasswordHashFromVariable (
  IN  EFI_GUID                       *UserGuid,
  IN  UINTN                          Index,
  OUT USER_PASSWORD_VAR_STRUCT       *UserPasswordVarStruct
  )
{
  UINTN                             DataSize;
  CHAR16                            PasswordName[sizeof(USER_AUTHENTICATION_VAR_NAME)/sizeof(CHAR16) + 5];

  if (Index != 0) {
    UnicodeSPrint (PasswordName, sizeof (PasswordName), L"%s%04x", USER_AUTHENTICATION_VAR_NAME, Index);
  } else {
    UnicodeSPrint (PasswordName, sizeof (PasswordName), L"%s", USER_AUTHENTICATION_VAR_NAME);
  }

  DataSize = sizeof(*UserPasswordVarStruct);
  return gRT->GetVariable (
                PasswordName,
                UserGuid,
                NULL,
                &DataSize,
                UserPasswordVarStruct
                );
}

/**
  Save password hash data to non-volatile variable region.

  @param[in]   UserGuid               The user GUID of the password variable.
  @param[in]   UserPasswordVarStruct  The storage of password in variable.

  @retval EFI_SUCCESS             The password hash is saved successfully.
  @retval EFI_OUT_OF_RESOURCES    Insufficient resources to save the password hash.
**/
EFI_STATUS
SavePasswordHashToVariable (
  IN EFI_GUID                       *UserGuid,
  IN USER_PASSWORD_VAR_STRUCT       *UserPasswordVarStruct
  )
{
  EFI_STATUS                        Status;

  if (UserPasswordVarStruct == NULL) {
    Status = gRT->SetVariable (
                    USER_AUTHENTICATION_VAR_NAME,
                    UserGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                    0,
                    NULL
                    );
  } else {
    Status = gRT->SetVariable (
                    USER_AUTHENTICATION_VAR_NAME,
                    UserGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                    sizeof(*UserPasswordVarStruct),
                    UserPasswordVarStruct
                    );
  }
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SavePasswordHashToVariable fails with %r\n", Status));
  }

  return Status;
}

/**
  Save old password hash data to non-volatile variable region as history.

  The number of password history variable is limited.
  If all the password history variables are used, the new password history
  will override the oldest one.

  @param[in]   UserGuid               The user GUID of the password variable.
  @param[in]   UserPasswordVarStruct  The storage of password in variable.

  @retval EFI_SUCCESS             The password hash is saved successfully.
  @retval EFI_OUT_OF_RESOURCES    Insufficient resources to save the password hash.
**/
EFI_STATUS
SaveOldPasswordToHistory (
  IN EFI_GUID                       *UserGuid,
  IN USER_PASSWORD_VAR_STRUCT       *UserPasswordVarStruct
  )
{
  EFI_STATUS                        Status;
  UINTN                             DataSize;
  UINT32                            LastIndex;
  CHAR16                            PasswordName[sizeof(USER_AUTHENTICATION_VAR_NAME)/sizeof(CHAR16) + 5];

  DEBUG ((DEBUG_INFO, "SaveOldPasswordToHistory\n"));

  DataSize = sizeof(LastIndex);
  Status = gRT->GetVariable (
                  USER_AUTHENTICATION_HISTORY_LAST_VAR_NAME,
                  UserGuid,
                  NULL,
                  &DataSize,
                  &LastIndex
                  );
  if (EFI_ERROR(Status)) {
    LastIndex = 0;
  }
  if (LastIndex >= PASSWORD_HISTORY_CHECK_COUNT) {
    LastIndex = 0;
  }

  LastIndex ++;
  UnicodeSPrint (PasswordName, sizeof (PasswordName), L"%s%04x", USER_AUTHENTICATION_VAR_NAME, LastIndex);


  Status = gRT->SetVariable (
                  PasswordName,
                  UserGuid,
                  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                  sizeof(*UserPasswordVarStruct),
                  UserPasswordVarStruct
                  );
  DEBUG ((DEBUG_INFO, "  -- to %s, %r\n", PasswordName, Status));
  if (!EFI_ERROR(Status)) {
    Status = gRT->SetVariable (
                    USER_AUTHENTICATION_HISTORY_LAST_VAR_NAME,
                    UserGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                    sizeof(LastIndex),
                    &LastIndex
                    );
    DEBUG ((DEBUG_INFO, " LastIndex - 0x%04x, %r\n", LastIndex, Status));
  }

  return Status;
}

/**
  Calculate password hash data and save it to non-volatile variable region.

  @param[in]  UserGuid               The user GUID of the password variable.
  @param[in]  Password               The user input password.
                                     NULL means delete the password variable.
  @param[in]  PasswordSize           The size of Password in byte.

  @retval EFI_SUCCESS             The password hash is calculated and saved.
  @retval EFI_OUT_OF_RESOURCES    Insufficient resources to save the password hash.
**/
EFI_STATUS
SavePasswordToVariable (
  IN  EFI_GUID                      *UserGuid,
  IN  CHAR8                         *Password,  OPTIONAL
  IN  UINTN                         PasswordSize
  )
{
  EFI_STATUS                        Status;
  USER_PASSWORD_VAR_STRUCT          UserPasswordVarStruct;
  BOOLEAN                           HashOk;

  //
  // If password is NULL, it means we want to clean password field saved in variable region.
  //
  if (Password != NULL) {
    KeyLibGenerateSalt (UserPasswordVarStruct.PasswordSalt, sizeof(UserPasswordVarStruct.PasswordSalt));
    HashOk = KeyLibGeneratePBKDF2Hash (
               HASH_TYPE_SHA256,
               (UINT8 *)Password,
               PasswordSize,
               UserPasswordVarStruct.PasswordSalt,
               sizeof(UserPasswordVarStruct.PasswordSalt),
               UserPasswordVarStruct.PasswordHash,
               sizeof(UserPasswordVarStruct.PasswordHash)
               );
    if (!HashOk) {
      return EFI_DEVICE_ERROR;
    }
    Status = SavePasswordHashToVariable (UserGuid, &UserPasswordVarStruct);
    //
    // Save Password data to history variable
    //
    if (!EFI_ERROR(Status)) {
      SaveOldPasswordToHistory (UserGuid, &UserPasswordVarStruct);
    }
  } else {
    Status = SavePasswordHashToVariable (UserGuid, NULL);
  }

  return Status;
}

/**
  Verify the password.
  If the password variable does not exist, it passes the verification.
  If the password variable exists, it does verification based upon password variable.

  @param[in]  UserGuid               The user GUID of the password variable.
  @param[in]  Password               The user input password.
  @param[in]  PasswordSize           The size of Password in byte.

  @retval TRUE    The verification passes.
  @retval FALSE   The verification fails.
**/
BOOLEAN
IsPasswordVerified (
  IN EFI_GUID                       *UserGuid,
  IN CHAR8                          *Password,
  IN UINTN                          PasswordSize
  )
{
  USER_PASSWORD_VAR_STRUCT          UserPasswordVarStruct;
  EFI_STATUS                        Status;
  UINTN                             *PasswordTryCount;

  PasswordTryCount = &mAdminPasswordTryCount;

  Status = GetPasswordHashFromVariable (UserGuid, 0, &UserPasswordVarStruct);
  if (EFI_ERROR(Status)) {
    return TRUE;
  }

  //
  // Old password exists
  //
  Status = CheckPassword (Password, PasswordSize, &UserPasswordVarStruct);
  if (EFI_ERROR(Status)) {
    if (Password[0] != 0) {
      *PasswordTryCount = *PasswordTryCount + 1;
    }
    return FALSE;
  }

  return TRUE;
}

/**
  Return if the password is set.

  @param[in]  UserGuid               The user GUID of the password variable.

  @retval TRUE    The password is set.
  @retval FALSE   The password is not set.
**/
BOOLEAN
IsPasswordSet (
  IN EFI_GUID                       *UserGuid
  )
{
  USER_PASSWORD_VAR_STRUCT          UserPasswordVarStruct;
  EFI_STATUS                        Status;

  Status = GetPasswordHashFromVariable(UserGuid, 0, &UserPasswordVarStruct);
  if (EFI_ERROR(Status)) {
    return FALSE;
  }
  return TRUE;
}

/**
  Return if the password is strong.
  Criteria:
  1) length >= PASSWORD_MIN_SIZE

  @param[in]  Password               The user input password.
  @param[in]  PasswordSize           The size of Password in byte.

  @retval TRUE    The password is strong.
  @retval FALSE   The password is weak.
**/
BOOLEAN
IsPasswordStrong (
  IN CHAR8   *Password,
  IN UINTN   PasswordSize
  )
{
  if (PasswordSize < PASSWORD_MIN_SIZE) {
    return FALSE;
  }

  return TRUE;
}

/**
  Return if the password is set before in PASSWORD_HISTORY_CHECK_COUNT.

  @param[in]  UserGuid               The user GUID of the password variable.
  @param[in]  Password               The user input password.
  @param[in]  PasswordSize           The size of Password in byte.

  @retval TRUE    The password is set before.
  @retval FALSE   The password is not set before.
**/
BOOLEAN
IsPasswordInHistory (
  IN EFI_GUID                       *UserGuid,
  IN CHAR8                          *Password,
  IN UINTN                          PasswordSize
  )
{
  EFI_STATUS                     Status;
  USER_PASSWORD_VAR_STRUCT       UserPasswordVarStruct;
  UINTN                          Index;

  for (Index = 1; Index <= PASSWORD_HISTORY_CHECK_COUNT; Index++) {
    Status = GetPasswordHashFromVariable (UserGuid, Index, &UserPasswordVarStruct);
    if (!EFI_ERROR(Status)) {
      Status = CheckPassword (Password, PasswordSize, &UserPasswordVarStruct);
      if (!EFI_ERROR(Status)) {
        return TRUE;
      }
    }
  }

  return FALSE;
}

/**
  Validate if the password is correct.

  @param[in] Password               The user input password.
  @param[in] PasswordSize           The size of Password in byte.

  @retval EFI_SUCCESS               The password is correct.
  @retval EFI_SECURITY_VIOLATION    The password is incorrect.
  @retval EFI_INVALID_PARAMETER     The password or size is invalid.
  @retval EFI_ACCESS_DENIED         Password retry count reach.
**/
EFI_STATUS
VerifyPassword (
  IN   CHAR16       *Password,
  IN   UINTN        PasswordSize
  )
{
  EFI_STATUS  Status;
  UINTN       *PasswordTryCount;
  UINTN       PasswordLen;
  EFI_GUID    *UserGuid;
  CHAR8       AsciiPassword[PASSWORD_MAX_SIZE];

  Status = UnicodeStrToAsciiStrS (Password, AsciiPassword, sizeof(AsciiPassword));
  if (EFI_ERROR(Status)) {
    goto EXIT;
  }

  PasswordLen = 0;
  PasswordTryCount = &mAdminPasswordTryCount;
  UserGuid = &gUserAuthenticationGuid;

  if (*PasswordTryCount >= PASSWORD_MAX_TRY_COUNT) {
    DEBUG ((DEBUG_ERROR, "PasswordHandler: VERIFY_PASSWORD try count reach!\n"));
    PasswordTryCount = NULL;
    Status = EFI_ACCESS_DENIED;
    goto EXIT;
  }

  PasswordLen = AsciiStrnLenS(AsciiPassword, sizeof(AsciiPassword));
  if (PasswordLen == sizeof(AsciiPassword)) {
    DEBUG ((DEBUG_ERROR, "PasswordHandler: Password invalid!\n"));
    Status = EFI_INVALID_PARAMETER;
    goto EXIT;
  }
  if (!IsPasswordVerified (UserGuid, AsciiPassword, PasswordLen + 1)) {
    DEBUG ((DEBUG_ERROR, "PasswordHandler: PasswordVerify - FAIL\n"));
    if (*PasswordTryCount >= PASSWORD_MAX_TRY_COUNT) {
      DEBUG ((DEBUG_ERROR, "PasswordHandler: VERIFY_PASSWORD try count reach!\n"));
      Status = EFI_ACCESS_DENIED;
    } else {
      Status = EFI_SECURITY_VIOLATION;
    }
    goto EXIT;
  }
  mPasswordVerified = TRUE;
  Status = EFI_SUCCESS;

EXIT:
  if (PasswordTryCount != NULL) {
    if (Status == EFI_SUCCESS) {
      *PasswordTryCount = 0;
    }
  }

  return Status;
}

/**
  Set a new password.

  @param[in] NewPassword            The user input new password.
                                    NULL means clear password.
  @param[in] NewPasswordSize        The size of NewPassword in byte.
  @param[in] OldPassword            The user input old password.
                                    NULL means no old password.
  @param[in] OldPasswordSize        The size of OldPassword in byte.

  @retval EFI_SUCCESS               The NewPassword is set successfully.
  @retval EFI_SECURITY_VIOLATION    The OldPassword is incorrect.
  @retval EFI_INVALID_PARAMETER     The password or size is invalid.
  @retval EFI_ACCESS_DENIED         Password retry count reach.
  @retval EFI_UNSUPPORTED           NewPassword is not strong enough.
  @retval EFI_ALREADY_STARTED       NewPassword is in history.
**/
EFI_STATUS
SetPassword (
  IN   CHAR16       *NewPassword,     OPTIONAL
  IN   UINTN        NewPasswordSize,
  IN   CHAR16       *OldPassword,     OPTIONAL
  IN   UINTN        OldPasswordSize
  )
{
  EFI_STATUS  Status;
  UINTN       *PasswordTryCount;
  UINTN       PasswordLen;
  EFI_GUID    *UserGuid;
  CHAR8       AsciiNewPassword[PASSWORD_MAX_SIZE];
  CHAR8       AsciiOldPassword[PASSWORD_MAX_SIZE];

  Status = UnicodeStrToAsciiStrS (NewPassword, AsciiNewPassword, sizeof(AsciiNewPassword));
  if (EFI_ERROR(Status)) {
    goto EXIT;
  }
  Status = UnicodeStrToAsciiStrS (OldPassword, AsciiOldPassword, sizeof(AsciiOldPassword));
  if (EFI_ERROR(Status)) {
    goto EXIT;
  }

  PasswordLen = 0;
  PasswordTryCount = &mAdminPasswordTryCount;
  UserGuid = &gUserAuthenticationGuid;

  if (NewPasswordSize > sizeof(AsciiNewPassword) * sizeof(CHAR16)) {
    return EFI_INVALID_PARAMETER;
  }
  if (OldPasswordSize > sizeof(AsciiOldPassword) * sizeof(CHAR16)) {
    return EFI_INVALID_PARAMETER;
  }

  if (*PasswordTryCount >= PASSWORD_MAX_TRY_COUNT) {
    DEBUG ((DEBUG_ERROR, "PasswordHandler: SET_PASSWORD try count reach!\n"));
    PasswordTryCount = NULL;
    Status = EFI_ACCESS_DENIED;
    goto EXIT;
  }

  PasswordLen = AsciiStrnLenS(AsciiOldPassword, sizeof(AsciiOldPassword));
  if (PasswordLen == sizeof(AsciiOldPassword)) {
    DEBUG ((DEBUG_ERROR, "PasswordHandler: OldPassword invalid!\n"));
    Status = EFI_INVALID_PARAMETER;
    goto EXIT;
  }

  if (!IsPasswordVerified (UserGuid, AsciiOldPassword, PasswordLen + 1)) {
    DEBUG ((DEBUG_ERROR, "PasswordHandler: PasswordVerify - FAIL\n"));
    if (*PasswordTryCount >= PASSWORD_MAX_TRY_COUNT) {
      DEBUG ((DEBUG_ERROR, "PasswordHandler: SET_PASSWORD try count reach!\n"));
      Status = EFI_ACCESS_DENIED;
    } else {
      Status = EFI_SECURITY_VIOLATION;
    }
    goto EXIT;
  }

  PasswordLen = AsciiStrnLenS(AsciiNewPassword, sizeof(AsciiNewPassword));
  if (PasswordLen == sizeof(AsciiNewPassword)) {
    DEBUG ((DEBUG_ERROR, "PasswordHandler: NewPassword invalid!\n"));
    Status = EFI_INVALID_PARAMETER;
    goto EXIT;
  }
  if (PasswordLen != 0 && !IsPasswordStrong (AsciiNewPassword, PasswordLen + 1)) {
    DEBUG ((DEBUG_ERROR, "PasswordHandler: NewPassword too weak!\n"));
    Status = EFI_UNSUPPORTED;
    goto EXIT;
  }
  if (PasswordLen != 0 && IsPasswordInHistory (UserGuid, AsciiNewPassword, PasswordLen + 1)) {
    DEBUG ((DEBUG_ERROR, "PasswordHandler: NewPassword in history!\n"));
    Status = EFI_ALREADY_STARTED;
    goto EXIT;
  }

  if (PasswordLen == 0) {
    Status = SavePasswordToVariable (UserGuid, NULL, 0);
  } else {
    Status = SavePasswordToVariable (UserGuid, AsciiNewPassword, PasswordLen + 1);
  }

EXIT:
  if (PasswordTryCount != NULL) {
    if (Status == EFI_SUCCESS) {
      *PasswordTryCount = 0;
    }
  }

  return Status;
}

/**
  Return if the password is set.

  @retval TRUE      The password is set.
  @retval FALSE     The password is not set.
**/
BOOLEAN
IsPasswordInstalled (
  VOID
  )
{
  return IsPasswordSet(&gUserAuthenticationGuid);
}

/**
  Get password verification policy.

  @param[out] VerifyPolicy          Verification policy.

  @retval EFI_SUCCESS               Get verification policy successfully.
**/
EFI_STATUS
GetPasswordVerificationPolicy (
  OUT PASSWORD_COMMUNICATE_VERIFY_POLICY    *VerifyPolicy
  )
{
  VerifyPolicy->NeedReVerify = mNeedReVerify;

  return EFI_SUCCESS;
}

/**
  Return if the password was verified.

  @retval TRUE      The password was verified.
  @retval FALSE     The password was not verified.
**/
BOOLEAN
WasPasswordVerified (
  VOID
  )
{
  return mPasswordVerified;
}
