/** @file
  GUID is for UserAuthentication SMM communication.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __USER_AUTHENTICATION_GUID_H__
#define __USER_AUTHENTICATION_GUID_H__

#define PASSWORD_MIN_SIZE    9  // MIN number of chars of password, including NULL.
#define PASSWORD_MAX_SIZE    65 // MAX number of chars of password, including NULL.

#define PASSWORD_SALT_SIZE   32
#define PASSWORD_HASH_SIZE   32 // SHA256_DIGEST_SIZE

#define PASSWORD_MAX_TRY_COUNT  3
#define PASSWORD_HISTORY_CHECK_COUNT  5

//
// Name of the variable
//
#define USER_AUTHENTICATION_VAR_NAME L"Password"
#define USER_AUTHENTICATION_HISTORY_LAST_VAR_NAME L"PasswordLast"

//
// Variable storage
//
typedef struct {
  UINT8        PasswordHash[PASSWORD_HASH_SIZE];
  UINT8        PasswordSalt[PASSWORD_SALT_SIZE];
} USER_PASSWORD_VAR_STRUCT;

#define USER_AUTHENTICATION_GUID \
  { 0xee24a7f7, 0x606b, 0x4724, { 0xb3, 0xc9, 0xf5, 0xae, 0x4a, 0x3b, 0x81, 0x65 } }

extern EFI_GUID gUserAuthenticationGuid;

typedef struct {
  CHAR8                                 NewPassword[PASSWORD_MAX_SIZE];
  CHAR8                                 OldPassword[PASSWORD_MAX_SIZE];
} PASSWORD_COMMUNICATE_SET_PASSWORD;

typedef struct {
  CHAR8                                 Password[PASSWORD_MAX_SIZE];
} PASSWORD_COMMUNICATE_VERIFY_PASSWORD;

typedef struct {
  BOOLEAN                               NeedReVerify;
} PASSWORD_COMMUNICATE_VERIFY_POLICY;

#endif
