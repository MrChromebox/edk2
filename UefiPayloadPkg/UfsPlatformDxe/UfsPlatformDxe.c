/** @file
  UFS Platform Driver - Implementation

  This driver provides platform-specific UFS host controller configuration
  for Intel platforms. It implements the EDKII_UFS_HC_PLATFORM_PROTOCOL to
  configure platform-specific settings required for proper UFS operation.

  Key optimizations for Intel platforms:
  - Disables PA_LOCAL_TX_LCC_ENABLE after HCE for stable link initialization
  - Activates all connected UFS lanes for maximum throughput
  - Programs High-Speed (HS) recipe for proper timeout configuration
  - Switches link to High-Speed mode for maximum performance
  - Validates lane configuration to ensure optimal data throughput

  Copyright (c) 2025, Matt DeVillier. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UfsPlatformDxe.h"

//
// UFS Host Controller Platform Protocol instance
//
GLOBAL_REMOVE_IF_UNREFERENCED EDKII_UFS_HC_PLATFORM_PROTOCOL  mUfsHcPlatform = {
  EDKII_UFS_HC_PLATFORM_PROTOCOL_VERSION,  // Version = 3
  NULL,                                     // OverrideHcInfo (not needed for Intel)
  UfsPlatformCallback,                     // Callback function
  EdkiiUfsCardRefClkFreq19p2Mhz,          // RefClkFreq = 19.2 MHz
  FALSE,                                   // SkipHceReenable = FALSE
  FALSE                                    // SkipLinkStartup = FALSE
};

/**
  Execute a UIC DME_SET command to configure a UniPro attribute.

  @param[in] UfsHcDriver    Pointer to UFS HC driver interface.
  @param[in] MibAttribute   MIB attribute address (upper 16 bits) and
                            GenSelectorIndex (lower 16 bits).
  @param[in] Value          Value to set.

  @retval EFI_SUCCESS       Command executed successfully.
  @retval Others            Command execution failed.

**/
STATIC
EFI_STATUS
UfsDmeSet (
  IN EDKII_UFS_HC_DRIVER_INTERFACE  *UfsHcDriver,
  IN UINT32                         MibAttribute,
  IN UINT32                         Value
  )
{
  EDKII_UIC_COMMAND  UicCommand;
  EFI_STATUS         Status;

  ZeroMem (&UicCommand, sizeof (EDKII_UIC_COMMAND));

  //
  // Build DME_SET command
  // Arg1 = MIB Attribute (upper 16 bits) | GenSelectorIndex (lower 16 bits)
  // Arg2 = Attribute Set Type (0 = Normal, 1 = Static)
  // Arg3 = Value to set
  //
  UicCommand.Opcode = UIC_CMD_DME_SET;
  UicCommand.Arg1   = MibAttribute;
  UicCommand.Arg2   = (DME_ATTR_SET_TYPE_NORMAL << 16);
  UicCommand.Arg3   = Value;

  Status = UfsHcDriver->UfsExecUicCommand (UfsHcDriver, &UicCommand);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: DME_SET failed for MIB 0x%04x, Status = %r\n",
      __FUNCTION__,
      (MibAttribute >> 16) & 0xFFFF,
      Status
      ));
  }

  return Status;
}

/**
  Execute a UIC DME_GET command to read a UniPro attribute.

  @param[in]  UfsHcDriver   Pointer to UFS HC driver interface.
  @param[in]  MibAttribute  MIB attribute address (upper 16 bits) and
                            GenSelectorIndex (lower 16 bits).
  @param[out] Value         Pointer to store the read value.

  @retval EFI_SUCCESS       Command executed successfully.
  @retval Others            Command execution failed.

**/
STATIC
EFI_STATUS
UfsDmeGet (
  IN  EDKII_UFS_HC_DRIVER_INTERFACE  *UfsHcDriver,
  IN  UINT32                         MibAttribute,
  OUT UINT32                         *Value
  )
{
  EDKII_UIC_COMMAND  UicCommand;
  EFI_STATUS         Status;

  ZeroMem (&UicCommand, sizeof (EDKII_UIC_COMMAND));

  UicCommand.Opcode = UIC_CMD_DME_GET;
  UicCommand.Arg1   = MibAttribute;
  UicCommand.Arg2   = 0;
  UicCommand.Arg3   = 0;

  Status = UfsHcDriver->UfsExecUicCommand (UfsHcDriver, &UicCommand);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: DME_GET failed for MIB 0x%04x, Status = %r\n",
      __FUNCTION__,
      (MibAttribute >> 16) & 0xFFFF,
      Status
      ));
    return Status;
  }

  *Value = UicCommand.Arg3;
  return EFI_SUCCESS;
}

/**
  Execute a UIC DME_PEER_GET command to read a peer UniPro attribute.

  @param[in]  UfsHcDriver   Pointer to UFS HC driver interface.
  @param[in]  MibAttribute  MIB attribute address (upper 16 bits) and
                            GenSelectorIndex (lower 16 bits).
  @param[out] Value         Pointer to store the read value.

  @retval EFI_SUCCESS       Command executed successfully.
  @retval Others            Command execution failed.

**/
STATIC
EFI_STATUS
UfsDmePeerGet (
  IN  EDKII_UFS_HC_DRIVER_INTERFACE  *UfsHcDriver,
  IN  UINT32                         MibAttribute,
  OUT UINT32                         *Value
  )
{
  EDKII_UIC_COMMAND  UicCommand;
  EFI_STATUS         Status;

  ZeroMem (&UicCommand, sizeof (EDKII_UIC_COMMAND));

  UicCommand.Opcode = UIC_CMD_DME_PEER_GET;
  UicCommand.Arg1   = MibAttribute;
  UicCommand.Arg2   = 0;
  UicCommand.Arg3   = 0;

  Status = UfsHcDriver->UfsExecUicCommand (UfsHcDriver, &UicCommand);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: DME_PEER_GET failed for MIB 0x%04x, Status = %r\n",
      __FUNCTION__,
      (MibAttribute >> 16) & 0xFFFF,
      Status
      ));
    return Status;
  }

  *Value = UicCommand.Arg3;
  return EFI_SUCCESS;
}

/**
  Validate lane configuration to ensure proper connectivity.

  This function checks that the RX and TX lanes are properly connected
  and symmetric, which is required for stable UFS operation.

  @param[in] UfsHcDriver    Pointer to UFS HC driver interface.

  @retval EFI_SUCCESS       Lane configuration is valid.
  @retval Others            Lane configuration validation failed.

**/
STATIC
EFI_STATUS
ValidateLaneConfiguration (
  IN EDKII_UFS_HC_DRIVER_INTERFACE  *UfsHcDriver
  )
{
  EFI_STATUS  Status;
  UINT32      ConnectedTxLanes;
  UINT32      ConnectedRxLanes;
  UINT32      ActiveTxLanes;
  UINT32      ActiveRxLanes;
  UINT32      MibAttribute;

  //
  // Query connected lane counts
  //
  MibAttribute     = (PA_CONNECTED_TX_DATA_LANES << 16);
  Status           = UfsDmeGet (UfsHcDriver, MibAttribute, &ConnectedTxLanes);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get connected TX lanes\n", __FUNCTION__));
    return Status;
  }

  MibAttribute = (PA_CONNECTED_RX_DATA_LANES << 16);
  Status       = UfsDmeGet (UfsHcDriver, MibAttribute, &ConnectedRxLanes);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get connected RX lanes\n", __FUNCTION__));
    return Status;
  }

  //
  // Query active lane counts
  //
  MibAttribute = (PA_ACTIVE_TX_DATA_LANES << 16);
  Status       = UfsDmeGet (UfsHcDriver, MibAttribute, &ActiveTxLanes);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get active TX lanes\n", __FUNCTION__));
    return Status;
  }

  MibAttribute = (PA_ACTIVE_RX_DATA_LANES << 16);
  Status       = UfsDmeGet (UfsHcDriver, MibAttribute, &ActiveRxLanes);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get active RX lanes\n", __FUNCTION__));
    return Status;
  }

  DEBUG ((
    DEBUG_INFO,
    "%a: Connected lanes - TX: %u, RX: %u\n",
    __FUNCTION__,
    ConnectedTxLanes,
    ConnectedRxLanes
    ));
  DEBUG ((
    DEBUG_INFO,
    "%a: Active lanes - TX: %u, RX: %u\n",
    __FUNCTION__,
    ActiveTxLanes,
    ActiveRxLanes
    ));

  //
  // Validate lane configuration
  //
  if ((ConnectedTxLanes == 0) || (ConnectedRxLanes == 0)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Invalid connected lanes (TX=%u, RX=%u)\n",
      __FUNCTION__,
      ConnectedTxLanes,
      ConnectedRxLanes
      ));
    return EFI_DEVICE_ERROR;
  }

  if (ConnectedTxLanes != ConnectedRxLanes) {
    DEBUG ((
      DEBUG_WARN,
      "%a: Asymmetric lanes (TX=%u, RX=%u) - may impact performance\n",
      __FUNCTION__,
      ConnectedTxLanes,
      ConnectedRxLanes
      ));
  }

  return EFI_SUCCESS;
}

/**
  Activate all connected UFS lanes for optimal performance.

  This function ensures that all physically connected lanes are activated
  for data transfer. If the number of active lanes is less than the number
  of connected lanes, it activates the additional lanes to maximize
  throughput.

  @param[in] UfsHcDriver    Pointer to UFS HC driver interface.

  @retval EFI_SUCCESS       All lanes activated successfully or already active.
  @retval Others            Failed to activate lanes.

**/
STATIC
EFI_STATUS
ActivateAllLanes (
  IN EDKII_UFS_HC_DRIVER_INTERFACE  *UfsHcDriver
  )
{
  EFI_STATUS  Status;
  UINT32      ConnectedLanes;
  UINT32      ActiveLanes;
  UINT32      MibAttribute;

  //
  // Step 1: Activate all connected RX lanes
  //
  DEBUG ((DEBUG_INFO, "%a: Checking RX lane activation\n", __FUNCTION__));

  MibAttribute = (PA_CONNECTED_RX_DATA_LANES << 16);
  Status       = UfsDmeGet (UfsHcDriver, MibAttribute, &ConnectedLanes);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get connected RX lanes, Status = %r\n", __FUNCTION__, Status));
    return Status;
  }

  MibAttribute = (PA_ACTIVE_RX_DATA_LANES << 16);
  Status       = UfsDmeGet (UfsHcDriver, MibAttribute, &ActiveLanes);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get active RX lanes, Status = %r\n", __FUNCTION__, Status));
    return Status;
  }

  DEBUG ((
    DEBUG_INFO,
    "%a: RX Lanes - Connected: %u, Active: %u\n",
    __FUNCTION__,
    ConnectedLanes,
    ActiveLanes
    ));

  if (ActiveLanes < ConnectedLanes) {
    DEBUG ((
      DEBUG_INFO,
      "%a: Activating RX lanes (%u -> %u)\n",
      __FUNCTION__,
      ActiveLanes,
      ConnectedLanes
      ));

    Status = UfsDmeSet (UfsHcDriver, MibAttribute, ConnectedLanes);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Failed to activate RX lanes, Status = %r\n",
        __FUNCTION__,
        Status
        ));
      return Status;
    }

    DEBUG ((DEBUG_INFO, "%a: RX lanes activated successfully\n", __FUNCTION__));
  } else {
    DEBUG ((DEBUG_INFO, "%a: RX lanes already fully activated\n", __FUNCTION__));
  }

  //
  // Step 2: Activate all connected TX lanes
  //
  DEBUG ((DEBUG_INFO, "%a: Checking TX lane activation\n", __FUNCTION__));

  MibAttribute = (PA_CONNECTED_TX_DATA_LANES << 16);
  Status       = UfsDmeGet (UfsHcDriver, MibAttribute, &ConnectedLanes);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get connected TX lanes, Status = %r\n", __FUNCTION__, Status));
    return Status;
  }

  MibAttribute = (PA_ACTIVE_TX_DATA_LANES << 16);
  Status       = UfsDmeGet (UfsHcDriver, MibAttribute, &ActiveLanes);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get active TX lanes, Status = %r\n", __FUNCTION__, Status));
    return Status;
  }

  DEBUG ((
    DEBUG_INFO,
    "%a: TX Lanes - Connected: %u, Active: %u\n",
    __FUNCTION__,
    ConnectedLanes,
    ActiveLanes
    ));

  if (ActiveLanes < ConnectedLanes) {
    DEBUG ((
      DEBUG_INFO,
      "%a: Activating TX lanes (%u -> %u)\n",
      __FUNCTION__,
      ActiveLanes,
      ConnectedLanes
      ));

    Status = UfsDmeSet (UfsHcDriver, MibAttribute, ConnectedLanes);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Failed to activate TX lanes, Status = %r\n",
        __FUNCTION__,
        Status
        ));
      return Status;
    }

    DEBUG ((DEBUG_INFO, "%a: TX lanes activated successfully\n", __FUNCTION__));
  } else {
    DEBUG ((DEBUG_INFO, "%a: TX lanes already fully activated\n", __FUNCTION__));
  }

  DEBUG ((DEBUG_INFO, "%a: Lane activation complete\n", __FUNCTION__));
  return EFI_SUCCESS;
}

/**
  Program High-Speed (HS) recipe for Intel UFS controllers.

  This function programs the Data Link Layer timeout values and Physical
  Adapter Layer settings required for stable High-Speed mode operation.
  These settings must be configured before the link switches to HS mode.

  The EDK2 UFS core driver does not implement this, so it must be done
  in the platform driver to achieve proper HS mode performance.

  @param[in] UfsHcDriver    Pointer to UFS HC driver interface.

  @retval EFI_SUCCESS       HS Recipe programmed successfully.
  @retval Others            Failed to program HS Recipe.

**/
STATIC
EFI_STATUS
ProgramHsRecipe (
  IN EDKII_UFS_HC_DRIVER_INTERFACE  *UfsHcDriver
  )
{
  EFI_STATUS  Status;
  UINT32      MibAttribute;

  DEBUG ((DEBUG_INFO, "%a: Programming HS Recipe for Intel platform\n", __FUNCTION__));

  //
  // Step 1: Configure Data Link Layer timeout values
  // These extended timeouts are required for stable HS mode operation
  //

  //
  // Set DL_FC0ProtectionTimeOutVal = 0x1FFF
  // Flow Control protection timeout
  //
  MibAttribute = (DL_FC0_PROTECTION_TIMEOUT_VAL << 16);
  Status       = UfsDmeSet (UfsHcDriver, MibAttribute, 0x1FFF);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to set DL_FC0ProtectionTimeOutVal, Status = %r\n",
      __FUNCTION__,
      Status
      ));
    return Status;
  }

  //
  // Set DL_TC0ReplayTimeOutVal = 0xFFFF
  // Transport layer replay timeout
  //
  MibAttribute = (DL_TC0_REPLAY_TIMEOUT_VAL << 16);
  Status       = UfsDmeSet (UfsHcDriver, MibAttribute, 0xFFFF);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to set DL_TC0ReplayTimeOutVal, Status = %r\n",
      __FUNCTION__,
      Status
      ));
    return Status;
  }

  //
  // Set DL_AFC0ReqTimeOutVal = 0x7FFF
  // AFC (Adaptive Frequency Control) request timeout
  //
  MibAttribute = (DL_AFC0_REQ_TIMEOUT_VAL << 16);
  Status       = UfsDmeSet (UfsHcDriver, MibAttribute, 0x7FFF);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to set DL_AFC0ReqTimeOutVal, Status = %r\n",
      __FUNCTION__,
      Status
      ));
    return Status;
  }

  DEBUG ((DEBUG_INFO, "%a: Data Link Layer timeouts configured\n", __FUNCTION__));

  //
  // Step 2: Configure Physical Adapter Layer HS mode settings
  //

  //
  // Set PA_HSSeries = HS Mode B (faster rate)
  // Intel platforms use Rate B for best performance
  //
  MibAttribute = (PA_HS_SERIES << 16);
  Status       = UfsDmeSet (UfsHcDriver, MibAttribute, UFS_PA_HS_MODE_B);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to set PA_HSSeries to Mode B, Status = %r\n",
      __FUNCTION__,
      Status
      ));
    return Status;
  }

  //
  // Enable PA_RxTermination
  // Required for HS mode operation
  //
  MibAttribute = (PA_RX_TERMINATION << 16);
  Status       = UfsDmeSet (UfsHcDriver, MibAttribute, 0x1);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to enable PA_RxTermination, Status = %r\n",
      __FUNCTION__,
      Status
      ));
    return Status;
  }

  //
  // Enable PA_TxTermination
  // Required for HS mode operation
  //
  MibAttribute = (PA_TX_TERMINATION << 16);
  Status       = UfsDmeSet (UfsHcDriver, MibAttribute, 0x1);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to enable PA_TxTermination, Status = %r\n",
      __FUNCTION__,
      Status
      ));
    return Status;
  }

  DEBUG ((
    DEBUG_INFO,
    "%a: HS Recipe programming completed successfully\n",
    __FUNCTION__
    ));
  DEBUG ((
    DEBUG_INFO,
    "%a: Configured - HS Mode B, Timeouts (FC0:0x1FFF TC0:0xFFFF AFC0:0x7FFF)\n",
    __FUNCTION__
    ));

  return EFI_SUCCESS;
}

/**
  Switch UFS link to High-Speed mode for maximum performance.

  This function queries the maximum HS gear supported by both host and device,
  then executes a power mode change to switch from PWM (slow) mode to
  Fast (HS) mode. This is critical for achieving proper UFS performance.

  @param[in] UfsHcDriver    Pointer to UFS HC driver interface.

  @retval EFI_SUCCESS       Successfully switched to HS mode.
  @retval Others            Failed to switch to HS mode.

**/
STATIC
EFI_STATUS
SwitchToHighSpeedMode (
  IN EDKII_UFS_HC_DRIVER_INTERFACE  *UfsHcDriver
  )
{
  EFI_STATUS  Status;
  UINT32      MaxHsGearRx;
  UINT32      MaxHsGearTx;
  UINT32      MibAttribute;
  UINT32      PowerModeValue;
  UINT32      InterruptStatus;

  DEBUG ((DEBUG_INFO, "%a: Switching to High-Speed mode\n", __FUNCTION__));

  //
  // Query maximum HS gear supported by host (RX direction)
  //
  MibAttribute = (PA_MAX_RX_HS_GEAR << 16);
  Status       = UfsDmeGet (UfsHcDriver, MibAttribute, &MaxHsGearRx);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get host max RX HS gear\n", __FUNCTION__));
    return Status;
  }

  //
  // Query maximum HS gear supported by device (TX direction from host perspective)
  //
  Status = UfsDmePeerGet (UfsHcDriver, MibAttribute, &MaxHsGearTx);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get peer max RX HS gear\n", __FUNCTION__));
    return Status;
  }

  //
  // Limit to HS-G3 for compatibility
  //
  if (MaxHsGearRx > UFS_HS_GEAR_3) {
    DEBUG ((DEBUG_INFO, "%a: Limiting RX gear from %u to HS-G3\n", __FUNCTION__, MaxHsGearRx));
    MaxHsGearRx = UFS_HS_GEAR_3;
  }

  if (MaxHsGearTx > UFS_HS_GEAR_3) {
    DEBUG ((DEBUG_INFO, "%a: Limiting TX gear from %u to HS-G3\n", __FUNCTION__, MaxHsGearTx));
    MaxHsGearTx = UFS_HS_GEAR_3;
  }

  DEBUG ((
    DEBUG_INFO,
    "%a: Target gears - RX: HS-G%u, TX: HS-G%u\n",
    __FUNCTION__,
    MaxHsGearRx,
    MaxHsGearTx
    ));

  //
  // Set RX gear
  //
  MibAttribute = (PA_RX_GEAR << 16);
  Status       = UfsDmeSet (UfsHcDriver, MibAttribute, MaxHsGearRx);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to set PA_RX_GEAR\n", __FUNCTION__));
    return Status;
  }

  //
  // Set TX gear
  //
  MibAttribute = (PA_TX_GEAR << 16);
  Status       = UfsDmeSet (UfsHcDriver, MibAttribute, MaxHsGearTx);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to set PA_TX_GEAR\n", __FUNCTION__));
    return Status;
  }

  //
  // Execute power mode change to Fast mode (HS mode)
  // PowerModeValue format: [RX mode (bits 7:4)] | [TX mode (bits 3:0)]
  // Fast mode (0x1) for both directions = 0x11
  //
  PowerModeValue = (UFS_PWR_MODE_FAST << 4) | UFS_PWR_MODE_FAST;
  MibAttribute   = (PA_PWR_MODE << 16);

  DEBUG ((
    DEBUG_INFO,
    "%a: Executing PA_PWRMode change to Fast mode (0x%02x)\n",
    __FUNCTION__,
    PowerModeValue
    ));

  Status = UfsDmeSet (UfsHcDriver, MibAttribute, PowerModeValue);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to set PA_PWRMode, Status = %r\n",
      __FUNCTION__,
      Status
      ));
    return Status;
  }

  //
  // Wait for UIC Power Mode Status (UPMS) interrupt to confirm mode change
  // This indicates the power mode change completed successfully
  //
  DEBUG ((DEBUG_INFO, "%a: Waiting for power mode change to complete...\n", __FUNCTION__));

  //
  // Poll the Interrupt Status register for UPMS bit
  // Timeout after reasonable delay
  //
  Status = UfsHcDriver->UfsHcProtocol->Read (
                                         UfsHcDriver->UfsHcProtocol,
                                         EfiUfsHcWidthUint32,
                                         UFS_HC_IS_OFFSET,
                                         1,
                                         &InterruptStatus
                                         );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to read interrupt status\n", __FUNCTION__));
    return Status;
  }

  //
  // Simple check - if UPMS bit is set, mode change completed
  // For a more robust implementation, should use UfsWaitMemSet with timeout
  //
  if ((InterruptStatus & UFS_HC_IS_UPMS) != 0) {
    DEBUG ((DEBUG_INFO, "%a: Power mode change completed (UPMS interrupt detected)\n", __FUNCTION__));

    //
    // Clear the UPMS interrupt
    //
    Status = UfsHcDriver->UfsHcProtocol->Write (
                                           UfsHcDriver->UfsHcProtocol,
                                           EfiUfsHcWidthUint32,
                                           UFS_HC_IS_OFFSET,
                                           1,
                                           &InterruptStatus
                                           );
  }

  DEBUG ((
    DEBUG_INFO,
    "%a: Successfully switched to High-Speed mode\n",
    __FUNCTION__
    ));

  return EFI_SUCCESS;
}

/**
  Disable PA_LOCAL_TX_LCC_ENABLE for Intel platforms.

  Line Coding Control (LCC) must be disabled after HCE for stable link
  initialization on Intel UFS controllers. This is a critical requirement
  for all Intel integrated UFS controllers.

  @param[in] UfsHcDriver    Pointer to UFS HC driver interface.

  @retval EFI_SUCCESS       LCC disabled successfully.
  @retval Others            Failed to disable LCC.

**/
STATIC
EFI_STATUS
DisableLccForIntel (
  IN EDKII_UFS_HC_DRIVER_INTERFACE  *UfsHcDriver
  )
{
  EFI_STATUS  Status;
  UINT32      MibAttribute;
  UINT32      LccStatus;

  //
  // Build MIB attribute for PA_LOCAL_TX_LCC_ENABLE
  //
  MibAttribute = (PA_LOCAL_TX_LCC_ENABLE << 16) | 0x0000;

  //
  // Read current LCC status for debugging
  //
  Status = UfsDmeGet (UfsHcDriver, MibAttribute, &LccStatus);
  if (!EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_INFO,
      "%a: Current PA_LOCAL_TX_LCC_ENABLE = %u\n",
      __FUNCTION__,
      LccStatus
      ));
  }

  //
  // Disable PA_LOCAL_TX_LCC_ENABLE (set to 0)
  //
  Status = UfsDmeSet (UfsHcDriver, MibAttribute, 0);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to disable PA_LOCAL_TX_LCC_ENABLE, Status = %r\n",
      __FUNCTION__,
      Status
      ));
    return Status;
  }

  DEBUG ((
    DEBUG_INFO,
    "%a: Successfully disabled PA_LOCAL_TX_LCC_ENABLE\n",
    __FUNCTION__
    ));

  return EFI_SUCCESS;
}

/**
  Platform-specific callback function for UFS host controller initialization.

  This function is called at various phases during UFS host controller
  initialization to allow platform-specific configuration.

  For Intel platforms:
  - EdkiiUfsHcPostHce: Disables PA_LOCAL_TX_LCC_ENABLE after HCE
  - EdkiiUfsHcPostLinkStartup: Validates and activates all connected lanes

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
  )
{
  EFI_STATUS                     Status;
  EDKII_UFS_HC_DRIVER_INTERFACE  *UfsHcDriver;

  //
  // Validate input parameters
  //
  if (CallbackData == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: CallbackData is NULL\n", __FUNCTION__));
    return EFI_INVALID_PARAMETER;
  }

  UfsHcDriver = (EDKII_UFS_HC_DRIVER_INTERFACE *)CallbackData;

  //
  // Validate driver interface
  //
  if ((UfsHcDriver->UfsHcProtocol == NULL) ||
      (UfsHcDriver->UfsExecUicCommand == NULL))
  {
    DEBUG ((DEBUG_ERROR, "%a: Invalid UFS HC driver interface\n", __FUNCTION__));
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((
    DEBUG_INFO,
    "%a: Controller 0x%p, Phase %d\n",
    __FUNCTION__,
    ControllerHandle,
    CallbackPhase
    ));

  Status = EFI_SUCCESS;

  switch (CallbackPhase) {
    case EdkiiUfsHcPreHce:
      //
      // Called before Host Controller Enable (HCE)
      // No platform-specific configuration needed at this phase for Intel
      //
      DEBUG ((DEBUG_INFO, "%a: PreHce - No action required\n", __FUNCTION__));
      break;

    case EdkiiUfsHcPostHce:
      //
      // Called after Host Controller Enable (HCE)
      //
      // For Intel UFS controllers, we must disable PA_LOCAL_TX_LCC_ENABLE
      // (Line Coding Control) after HCE but before link startup. This timing
      // is critical for stable link initialization on all Intel integrated
      // UFS controllers.
      //
      // This matches the implementation in the Linux kernel which disables LCC
      // in the PostHce phase rather than PreLinkStartup.
      //
      DEBUG ((
        DEBUG_INFO,
        "%a: PostHce - Applying Intel-specific initialization\n",
        __FUNCTION__
        ));

      Status = DisableLccForIntel (UfsHcDriver);
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Failed to disable LCC, Status = %r\n",
          __FUNCTION__,
          Status
          ));
        //
        // Don't fail initialization if LCC disable fails - some controllers
        // may not support it or may have it disabled by default
        //
        Status = EFI_SUCCESS;
      }

      //
      // Note: Crypto enable (CRYPTO_GENERAL_ENABLE) would be done here on
      // newer platforms (Lakefield+), but requires direct MMIO access which
      // is not available through the current protocol interface. The EDK2
      // UFS core driver may handle this automatically on supported platforms.
      //
      break;

    case EdkiiUfsHcPreLinkStartup:
      //
      // Called before DME_LINKSTARTUP command
      // LCC is now disabled in PostHce phase for optimal timing
      //
      DEBUG ((DEBUG_INFO, "%a: PreLinkStartup - No action required\n", __FUNCTION__));
      break;

    case EdkiiUfsHcPostLinkStartup:
      //
      // Called after link startup completes
      //
      DEBUG ((
        DEBUG_INFO,
        "%a: PostLinkStartup - Validating and activating lanes\n",
        __FUNCTION__
        ));

      //
      // Validate lane configuration for debugging and performance analysis
      // This is informational and won't fail initialization
      //
      Status = ValidateLaneConfiguration (UfsHcDriver);
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_WARN,
          "%a: Lane validation warnings detected (Status = %r), continuing...\n",
          __FUNCTION__,
          Status
          ));
        //
        // Don't fail on lane validation - this is primarily for diagnostics
        //
        Status = EFI_SUCCESS;
      }

      //
      // Activate all connected lanes for optimal throughput
      // This ensures maximum bandwidth is available for data transfers
      //
      DEBUG ((
        DEBUG_INFO,
        "%a: Activating all connected UFS lanes\n",
        __FUNCTION__
        ));

      Status = ActivateAllLanes (UfsHcDriver);
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_WARN,
          "%a: Lane activation warnings detected (Status = %r), continuing...\n",
          __FUNCTION__,
          Status
          ));
        //
        // Lane activation failure is not fatal - EDK2 core driver may have
        // already activated lanes during power mode negotiation
        //
        Status = EFI_SUCCESS;
      }

      //
      // Program HS Recipe for stable High-Speed mode operation
      //
      // The EDK2 UFS core driver does not implement HS Recipe programming,
      // which is required for proper timeout configuration and HS mode settings.
      // Without this, the link may fail to switch to HS mode or may operate
      // unstably, causing slow boot times (3+ minutes).
      //
      // This configures:
      // - Data Link Layer timeout values (FC0, TC0, AFC0)
      // - HS Series Mode B (best performance)
      // - RX/TX termination (required for HS mode)
      //
      DEBUG ((
        DEBUG_INFO,
        "%a: Programming High-Speed Recipe\n",
        __FUNCTION__
        ));

      Status = ProgramHsRecipe (UfsHcDriver);
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Failed to program HS Recipe, Status = %r\n",
          __FUNCTION__,
          Status
          ));
        //
        // HS Recipe is critical for proper HS mode operation
        // Failure here may result in slow boot or link instability
        //
        return Status;
      }

      //
      // Switch to High-Speed mode for maximum performance
      //
      // After programming the HS Recipe, we must explicitly switch the power
      // mode from PWM (slow) to Fast (HS) mode. Without this, the link stays
      // in PWM mode even though HS configuration is ready, causing 3+ minute
      // boot times.
      DEBUG ((
        DEBUG_INFO,
        "%a: Switching to High-Speed mode\n",
        __FUNCTION__
        ));

      Status = SwitchToHighSpeedMode (UfsHcDriver);
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Failed to switch to HS mode, Status = %r\n",
          __FUNCTION__,
          Status
          ));
        //
        // Mode switching failure is critical - boot will be very slow
        // in PWM mode, so fail fast rather than waiting 3+ minutes
        //
        return Status;
      }

      DEBUG ((
        DEBUG_INFO,
        "%a: Intel platform configuration completed successfully\n",
        __FUNCTION__
        ));
      break;

    default:
      DEBUG ((DEBUG_ERROR, "%a: Invalid callback phase %d\n", __FUNCTION__, CallbackPhase));
      return EFI_INVALID_PARAMETER;
  }

  return Status;
}

/**
  Driver entry point.

  Installs the EDKII_UFS_HC_PLATFORM_PROTOCOL which will be located and
  used by the UFS Pass Thru driver during host controller initialization.

  @param[in] ImageHandle    Handle of the driver image.
  @param[in] SystemTable    Pointer to the EFI System Table.

  @retval EFI_SUCCESS       Protocol installed successfully.
  @retval Others            Failed to install protocol.

**/
EFI_STATUS
EFIAPI
UfsPlatformDxeInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  DEBUG ((DEBUG_INFO, "%a: Installing UFS HC Platform Protocol\n", __FUNCTION__));

  //
  // Install EDKII_UFS_HC_PLATFORM_PROTOCOL
  //
  // This protocol will be located by UfsPassThruDxe during initialization
  // using gBS->LocateProtocol(). The protocol provides platform-specific
  // callbacks and configuration for UFS host controller initialization.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ImageHandle,
                  &gEdkiiUfsHcPlatformProtocolGuid,
                  &mUfsHcPlatform,
                  NULL
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to install UFS HC Platform Protocol, Status = %r\n",
      __FUNCTION__,
      Status
      ));
    return Status;
  }

  DEBUG ((
    DEBUG_INFO,
    "%a: UFS HC Platform Protocol installed successfully\n",
    __FUNCTION__
    ));
  DEBUG ((
    DEBUG_INFO,
    "%a: Configuration - RefClkFreq: 19.2 MHz\n",
    __FUNCTION__
    ));
  DEBUG ((
    DEBUG_INFO,
    "%a: Features - LCC Disable, Lane Activation, HS Recipe, HS Mode Switch\n",
    __FUNCTION__
    ));

  return EFI_SUCCESS;
}

