/** @file
  Helper functions to parse HID report descriptor and items.

Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UsbMouseAbsolutePointer.h"

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

#define HID_UP_GENERIC_DESKTOP  0x01
#define HID_UP_BUTTON           0x09

#define HID_GD_MOUSE  0x02
#define HID_GD_X      0x30
#define HID_GD_Y      0x31
#define HID_GD_WHEEL  0x38

#define HID_MOUSE_MAX_USAGES  16

///
/// Transient state while walking a HID report descriptor.
///
typedef struct {
  UINT32    UsagePage;
  UINT32    ReportSize;
  UINT32    ReportCount;
  INT32     LogicalMinimum;
  INT32     LogicalMaximum;
  BOOLEAN   HasReportId;
  UINT8     ReportId;
  UINT32    BitCursor;
  UINT32    Usages[HID_MOUSE_MAX_USAGES];
  UINTN     UsageCount;
  UINT32    UsageMinimum;
  UINT32    UsageMaximum;
  BOOLEAN   UsageRangeValid;
  UINTN     CollectionDepth;
  UINTN     MouseAppDepth;       ///< Non-zero while inside Mouse Application collection.
  BOOLEAN   HaveX;
  BOOLEAN   HaveY;
  BOOLEAN   HaveButtons;
  BOOLEAN   HaveWheel;
} HID_MOUSE_PARSER_STATE;

/**
  Initialize ReportLayout to the classic HID boot-protocol mouse format.

  @param  Layout  Layout to initialize.

**/
VOID
InitializeDefaultBootMouseLayout (
  OUT USB_MOUSE_REPORT_LAYOUT  *Layout
  )
{
  ZeroMem (Layout, sizeof (*Layout));
  Layout->Valid           = TRUE;
  Layout->HasReportId     = FALSE;
  Layout->ReportId        = 0;
  Layout->ButtonBitOffset = 0;
  Layout->ButtonBitCount  = 3;
  Layout->XBitOffset      = 8;
  Layout->XBitSize        = 8;
  Layout->XRelative       = TRUE;
  Layout->YBitOffset      = 16;
  Layout->YBitSize        = 8;
  Layout->YRelative       = TRUE;
  Layout->WheelBitOffset  = 24;
  Layout->WheelBitSize    = 8;
  Layout->MinPacketBytes  = 3;
}

/**
  Return TRUE if Layout matches a boot-protocol mouse report.

  @param  Layout  Parsed layout.

  @retval TRUE   Boot protocol is appropriate.
  @retval FALSE  Report protocol must be used.

**/
BOOLEAN
MouseReportLayoutIsBootCompatible (
  IN CONST USB_MOUSE_REPORT_LAYOUT  *Layout
  )
{
  if (!Layout->Valid || Layout->HasReportId) {
    return FALSE;
  }

  if ((Layout->XBitSize != 8) || (Layout->YBitSize != 8)) {
    return FALSE;
  }

  if ((Layout->XBitOffset != 8) || (Layout->YBitOffset != 16)) {
    return FALSE;
  }

  if (Layout->ButtonBitOffset != 0) {
    return FALSE;
  }

  return TRUE;
}

/**
  Update MinPacketBytes from the furthest mapped field.

  @param  Layout  Layout to update.

**/
STATIC
VOID
UpdateMinPacketBytes (
  IN OUT USB_MOUSE_REPORT_LAYOUT  *Layout
  )
{
  UINT32  EndBit;
  UINT32  PayloadBytes;

  EndBit = 0;
  if (Layout->ButtonBitCount > 0) {
    EndBit = MAX (EndBit, (UINT32)Layout->ButtonBitOffset + Layout->ButtonBitCount);
  }

  if (Layout->XBitSize > 0) {
    EndBit = MAX (EndBit, (UINT32)Layout->XBitOffset + Layout->XBitSize);
  }

  if (Layout->YBitSize > 0) {
    EndBit = MAX (EndBit, (UINT32)Layout->YBitOffset + Layout->YBitSize);
  }

  if ((Layout->WheelBitOffset != USB_MOUSE_REPORT_FIELD_ABSENT) && (Layout->WheelBitSize > 0)) {
    EndBit = MAX (EndBit, (UINT32)Layout->WheelBitOffset + Layout->WheelBitSize);
  }

  PayloadBytes = (EndBit + 7) / 8;
  Layout->MinPacketBytes = (UINT16)(PayloadBytes + (Layout->HasReportId ? 1 : 0));
  if (Layout->MinPacketBytes < 3) {
    Layout->MinPacketBytes = 3;
  }
}

/**
  Get next HID item from report descriptor.

  This function retrieves next HID item from report descriptor, according to
  the start position.
  According to USB HID Specification, An item is piece of information
  about the device. All items have a one-byte prefix that contains
  the item tag, item type, and item size.
  There are two basic types of items: short items and long items.
  If the item is a short item, its optional data size may be 0, 1, 2, or 4 bytes.
  Only short item is supported here.

  @param  StartPos          Start position of the HID item to get.
  @param  EndPos            End position of the range to get the next HID item.
  @param  HidItem           Buffer for the HID Item to return.

  @return Pointer to end of the HID item returned.
          NULL if no HID item retrieved.

**/
UINT8 *
GetNextHidItem (
  IN  UINT8     *StartPos,
  IN  UINT8     *EndPos,
  OUT HID_ITEM  *HidItem
  )
{
  UINT8  Temp;

  if (EndPos <= StartPos) {
    return NULL;
  }

  Temp = *StartPos;
  StartPos++;

  //
  // Bit format of prefix byte:
  // Bits 0-1: Size
  // Bits 2-3: Type
  // Bits 4-7: Tag
  //
  HidItem->Type = BitFieldRead8 (Temp, 2, 3);
  HidItem->Tag  = BitFieldRead8 (Temp, 4, 7);

  if (HidItem->Tag == HID_ITEM_TAG_LONG) {
    //
    // Long Items are not supported, although we try to parse it.
    //
    HidItem->Format = HID_ITEM_FORMAT_LONG;

    if ((EndPos - StartPos) >= 2) {
      HidItem->Size = *StartPos++;
      HidItem->Tag  = *StartPos++;

      if ((EndPos - StartPos) >= HidItem->Size) {
        HidItem->Data.LongData = StartPos;
        StartPos              += HidItem->Size;
        return StartPos;
      }
    }
  } else {
    HidItem->Format = HID_ITEM_FORMAT_SHORT;
    HidItem->Size   = BitFieldRead8 (Temp, 0, 1);

    switch (HidItem->Size) {
      case 0:
        //
        // No data
        //
        return StartPos;

      case 1:
        //
        // 1-byte data
        //
        if ((EndPos - StartPos) >= 1) {
          HidItem->Data.Uint8 = *StartPos++;
          return StartPos;
        }

        break;

      case 2:
        //
        // 2-byte data
        //
        if ((EndPos - StartPos) >= 2) {
          CopyMem (&HidItem->Data.Uint16, StartPos, sizeof (UINT16));
          StartPos += 2;
          return StartPos;
        }

        break;

      case 3:
        //
        // 4-byte data, adjust size
        //
        HidItem->Size = 4;
        if ((EndPos - StartPos) >= 4) {
          CopyMem (&HidItem->Data.Uint32, StartPos, sizeof (UINT32));
          StartPos += 4;
          return StartPos;
        }

        break;
    }
  }

  return NULL;
}

/**
  Get unsigned data from HID item.

  @param  HidItem       Pointer to the HID item.

  @return The data of HID item.

**/
UINT32
GetItemData (
  IN  HID_ITEM  *HidItem
  )
{
  switch (HidItem->Size) {
    case 1:
      return HidItem->Data.Uint8;
    case 2:
      return HidItem->Data.Uint16;
    case 4:
      return HidItem->Data.Uint32;
  }

  return 0;
}

/**
  Get signed data from HID item.

  @param  HidItem       Pointer to the HID item.

  @return The signed data of HID item.

**/
STATIC
INT32
GetItemDataSigned (
  IN  HID_ITEM  *HidItem
  )
{
  switch (HidItem->Size) {
    case 1:
      return (INT32)(INT8)HidItem->Data.Uint8;
    case 2:
      return (INT32)(INT16)HidItem->Data.Uint16;
    case 4:
      return (INT32)HidItem->Data.Uint32;
  }

  return 0;
}

/**
  Clear local items after a Main item consumes them.

  @param  Parser  Parser state.

**/
STATIC
VOID
ClearLocalState (
  IN OUT HID_MOUSE_PARSER_STATE  *Parser
  )
{
  Parser->UsageCount      = 0;
  Parser->UsageRangeValid = FALSE;
  Parser->UsageMinimum    = 0;
  Parser->UsageMaximum    = 0;
}

/**
  Resolve the Usage ID for field Index of an Input item.

  @param  Parser  Parser state.
  @param  Index   Field index within ReportCount.

  @return Usage ID, or 0 if unknown.

**/
STATIC
UINT32
GetUsageForField (
  IN HID_MOUSE_PARSER_STATE  *Parser,
  IN UINTN                   Index
  )
{
  if (Index < Parser->UsageCount) {
    return Parser->Usages[Index];
  }

  if (Parser->UsageRangeValid) {
    return Parser->UsageMinimum + (UINT32)Index;
  }

  if (Parser->UsageCount > 0) {
    return Parser->Usages[Parser->UsageCount - 1];
  }

  return 0;
}

/**
  Record a mapped field into the mouse report layout.

  @param  UsbMouse  Device instance.
  @param  Parser    Parser state.
  @param  Usage     HID usage (Generic Desktop).
  @param  BitSize   Field width in bits.
  @param  Relative  TRUE if relative Input flag set.
  @param  BitOffset Bit offset within the report payload.

**/
STATIC
VOID
RecordPointerField (
  IN OUT USB_MOUSE_ABSOLUTE_POINTER_DEV  *UsbMouse,
  IN OUT HID_MOUSE_PARSER_STATE          *Parser,
  IN     UINT32                          Usage,
  IN     UINT8                           BitSize,
  IN     BOOLEAN                         Relative,
  IN     UINT32                          BitOffset
  )
{
  USB_MOUSE_REPORT_LAYOUT  *Layout;

  Layout = &UsbMouse->ReportLayout;

  switch (Usage) {
    case HID_GD_X:
      if (!Parser->HaveX) {
        Layout->XBitOffset = (UINT16)BitOffset;
        Layout->XBitSize   = BitSize;
        Layout->XRelative  = Relative;
        Parser->HaveX      = TRUE;
        UsbMouse->XLogicMin = Parser->LogicalMinimum;
        UsbMouse->XLogicMax = Parser->LogicalMaximum;
      }

      break;

    case HID_GD_Y:
      if (!Parser->HaveY) {
        Layout->YBitOffset = (UINT16)BitOffset;
        Layout->YBitSize   = BitSize;
        Layout->YRelative  = Relative;
        Parser->HaveY      = TRUE;
        UsbMouse->YLogicMin = Parser->LogicalMinimum;
        UsbMouse->YLogicMax = Parser->LogicalMaximum;
      }

      break;

    case HID_GD_WHEEL:
      if (!Parser->HaveWheel) {
        Layout->WheelBitOffset = (UINT16)BitOffset;
        Layout->WheelBitSize   = BitSize;
        Parser->HaveWheel      = TRUE;
        if (Layout->Valid) {
          UpdateMinPacketBytes (Layout);
        }
      }

      break;

    default:
      break;
  }

  if (Parser->HaveX && Parser->HaveY) {
    Layout->Valid       = TRUE;
    Layout->HasReportId = Parser->HasReportId;
    Layout->ReportId    = Parser->ReportId;
    UpdateMinPacketBytes (Layout);
  }
}

/**
  Handle a Main Input item.

  @param  UsbMouse  Device instance.
  @param  Parser    Parser state.
  @param  Flags     Input item flags.

**/
STATIC
VOID
ParseInputItem (
  IN OUT USB_MOUSE_ABSOLUTE_POINTER_DEV  *UsbMouse,
  IN OUT HID_MOUSE_PARSER_STATE          *Parser,
  IN     UINT32                          Flags
  )
{
  UINT32   Index;
  UINT32   BitSize;
  UINT32   Count;
  BOOLEAN  Relative;
  BOOLEAN  Constant;
  UINT32   Usage;
  UINT8    ButtonCount;

  BitSize  = Parser->ReportSize;
  Count    = Parser->ReportCount;
  Relative = (Flags & HID_MAIN_ITEM_RELATIVE) != 0;
  Constant = (Flags & HID_MAIN_ITEM_CONSTANT) != 0;

  //
  // Only map fields from the Mouse Application collection.  Always advance
  // the bit cursor so subsequent fields stay aligned.
  //
  if ((Parser->MouseAppDepth == 0) || Constant || (BitSize == 0) || (Count == 0)) {
    Parser->BitCursor += BitSize * Count;
    ClearLocalState (Parser);
    return;
  }

  if (Parser->UsagePage == HID_UP_BUTTON) {
    if (!Parser->HaveButtons) {
      if (Parser->UsageRangeValid) {
        ButtonCount = (UINT8)(Parser->UsageMaximum - Parser->UsageMinimum + 1);
      } else if (Parser->UsageCount > 0) {
        ButtonCount = (UINT8)Parser->UsageCount;
      } else {
        ButtonCount = (UINT8)MIN (Count, 8);
      }

      if (ButtonCount > Count) {
        ButtonCount = (UINT8)Count;
      }

      UsbMouse->ReportLayout.ButtonBitOffset = (UINT16)Parser->BitCursor;
      UsbMouse->ReportLayout.ButtonBitCount  = ButtonCount;
      UsbMouse->PrivateData.ButtonDetected   = TRUE;
      if (Parser->UsageRangeValid) {
        UsbMouse->PrivateData.ButtonMinIndex = (UINT8)Parser->UsageMinimum;
        UsbMouse->PrivateData.ButtonMaxIndex = (UINT8)Parser->UsageMaximum;
      } else {
        UsbMouse->PrivateData.ButtonMinIndex = 1;
        UsbMouse->PrivateData.ButtonMaxIndex = ButtonCount;
      }

      Parser->HaveButtons = TRUE;
    }

    Parser->BitCursor += BitSize * Count;
    ClearLocalState (Parser);
    return;
  }

  if (Parser->UsagePage == HID_UP_GENERIC_DESKTOP) {
    for (Index = 0; Index < Count; Index++) {
      Usage = GetUsageForField (Parser, Index);
      RecordPointerField (
        UsbMouse,
        Parser,
        Usage,
        (UINT8)BitSize,
        Relative,
        Parser->BitCursor
        );
      Parser->BitCursor += BitSize;
    }

    ClearLocalState (Parser);
    return;
  }

  //
  // Consumer / vendor / other pages: skip.
  //
  Parser->BitCursor += BitSize * Count;
  ClearLocalState (Parser);
}

/**
  Parse HID item from report descriptor.

  @param  UsbMouse  The instance of USB_MOUSE_ABSOLUTE_POINTER_DEV
  @param  Parser    Parser state
  @param  HidItem   The HID item to parse

**/
STATIC
VOID
ParseHidItem (
  IN OUT USB_MOUSE_ABSOLUTE_POINTER_DEV  *UsbMouse,
  IN OUT HID_MOUSE_PARSER_STATE          *Parser,
  IN     HID_ITEM                        *HidItem
  )
{
  UINT32  Data;
  UINT32  CollectionType;
  UINT32  Usage;

  switch (HidItem->Type) {
    case HID_ITEM_TYPE_MAIN:
      switch (HidItem->Tag) {
        case HID_MAIN_ITEM_TAG_INPUT:
          ParseInputItem (UsbMouse, Parser, GetItemData (HidItem));
          break;

        case HID_MAIN_ITEM_TAG_OUTPUT:
        case HID_MAIN_ITEM_TAG_FEATURE:
          //
          // Still consume locals; do not touch BitCursor (Output/Feature are
          // not part of the interrupt Input report).
          //
          ClearLocalState (Parser);
          break;

        case HID_MAIN_ITEM_TAG_BEGIN_COLLECTION:
          CollectionType = GetItemData (HidItem);
          Parser->CollectionDepth++;
          Usage = 0;
          if (Parser->UsageCount > 0) {
            Usage = Parser->Usages[0];
          } else if (Parser->UsageRangeValid) {
            Usage = Parser->UsageMinimum;
          }

          if ((Parser->MouseAppDepth == 0) &&
              (CollectionType == HID_COLLECTION_APPLICATION) &&
              (Parser->UsagePage == HID_UP_GENERIC_DESKTOP) &&
              (Usage == HID_GD_MOUSE))
          {
            Parser->MouseAppDepth = Parser->CollectionDepth;
          }

          ClearLocalState (Parser);
          break;

        case HID_MAIN_ITEM_TAG_END_COLLECTION:
          if ((Parser->MouseAppDepth != 0) &&
              (Parser->CollectionDepth == Parser->MouseAppDepth))
          {
            Parser->MouseAppDepth = 0;
          }

          if (Parser->CollectionDepth > 0) {
            Parser->CollectionDepth--;
          }

          ClearLocalState (Parser);
          break;

        default:
          ClearLocalState (Parser);
          break;
      }

      return;

    case HID_ITEM_TYPE_GLOBAL:
      switch (HidItem->Tag) {
        case HID_GLOBAL_ITEM_TAG_USAGE_PAGE:
          Parser->UsagePage = GetItemData (HidItem);
          break;

        case HID_GLOBAL_ITEM_TAG_LOGICAL_MINIMUM:
          Parser->LogicalMinimum = GetItemDataSigned (HidItem);
          break;

        case HID_GLOBAL_ITEM_TAG_LOGICAL_MAXIMUM:
          Parser->LogicalMaximum = GetItemDataSigned (HidItem);
          break;

        case HID_GLOBAL_ITEM_TAG_REPORT_SIZE:
          Parser->ReportSize = GetItemData (HidItem);
          break;

        case HID_GLOBAL_ITEM_TAG_REPORT_COUNT:
          Parser->ReportCount = GetItemData (HidItem);
          break;

        case HID_GLOBAL_ITEM_TAG_REPORT_ID:
          Data = GetItemData (HidItem);
          Parser->HasReportId = TRUE;
          Parser->ReportId    = (UINT8)Data;
          Parser->BitCursor   = 0;
          break;

        default:
          break;
      }

      return;

    case HID_ITEM_TYPE_LOCAL:
      if (HidItem->Size == 0) {
        return;
      }

      Data = GetItemData (HidItem);

      switch (HidItem->Tag) {
        case HID_LOCAL_ITEM_TAG_USAGE:
          if (Parser->UsageCount < HID_MOUSE_MAX_USAGES) {
            Parser->Usages[Parser->UsageCount++] = Data;
          }

          return;

        case HID_LOCAL_ITEM_TAG_USAGE_MINIMUM:
          Parser->UsageMinimum    = Data;
          Parser->UsageRangeValid = TRUE;
          return;

        case HID_LOCAL_ITEM_TAG_USAGE_MAXIMUM:
          Parser->UsageMaximum    = Data;
          Parser->UsageRangeValid = TRUE;
          return;

        default:
          return;
      }
  }
}

/**
  Parse Mouse Report Descriptor.

  According to USB HID Specification, report descriptors are
  composed of pieces of information. Each piece of information
  is called an Item. This function retrieves each item from
  the report descriptor and updates USB_MOUSE_ABSOLUTE_POINTER_DEV.

  @param  UsbMouseAbsolutePointer  The instance of USB_MOUSE_ABSOLUTE_POINTER_DEV
  @param  ReportDescriptor         Report descriptor to parse
  @param  ReportSize               Report descriptor size

  @retval EFI_SUCCESS              Report descriptor successfully parsed.
  @retval EFI_UNSUPPORTED          Report descriptor contains long item.

**/
EFI_STATUS
ParseMouseReportDescriptor (
  OUT USB_MOUSE_ABSOLUTE_POINTER_DEV  *UsbMouseAbsolutePointer,
  IN  UINT8                           *ReportDescriptor,
  IN  UINTN                           ReportSize
  )
{
  UINT8                   *DescriptorEnd;
  UINT8                   *Ptr;
  HID_ITEM                HidItem;
  HID_MOUSE_PARSER_STATE  Parser;

  //
  // Default to boot-protocol layout so devices with incomplete descriptors
  // keep the historical behaviour.
  //
  InitializeDefaultBootMouseLayout (&UsbMouseAbsolutePointer->ReportLayout);
  UsbMouseAbsolutePointer->ReportLayout.WheelBitOffset = USB_MOUSE_REPORT_FIELD_ABSENT;
  UsbMouseAbsolutePointer->ReportLayout.WheelBitSize   = 0;
  UsbMouseAbsolutePointer->ReportLayout.Valid          = FALSE;

  ZeroMem (&Parser, sizeof (Parser));

  DescriptorEnd = ReportDescriptor + ReportSize;

  Ptr = GetNextHidItem (ReportDescriptor, DescriptorEnd, &HidItem);
  while (Ptr != NULL) {
    if (HidItem.Format != HID_ITEM_FORMAT_SHORT) {
      //
      // Long Item is not supported at current HID revision
      //
      return EFI_UNSUPPORTED;
    }

    ParseHidItem (UsbMouseAbsolutePointer, &Parser, &HidItem);

    Ptr = GetNextHidItem (Ptr, DescriptorEnd, &HidItem);
  }

  if (!UsbMouseAbsolutePointer->ReportLayout.Valid) {
    //
    // Could not locate X/Y — fall back to boot mouse layout.
    //
    InitializeDefaultBootMouseLayout (&UsbMouseAbsolutePointer->ReportLayout);
    DEBUG ((
      DEBUG_WARN,
      "UsbMouseAbsolutePointer: HID descriptor lacked X/Y; using boot layout\n"
      ));
  } else {
    if (!Parser.HaveWheel) {
      UsbMouseAbsolutePointer->ReportLayout.WheelBitOffset = USB_MOUSE_REPORT_FIELD_ABSENT;
      UsbMouseAbsolutePointer->ReportLayout.WheelBitSize   = 0;
    }

    if (!Parser.HaveButtons) {
      UsbMouseAbsolutePointer->ReportLayout.ButtonBitOffset = 0;
      UsbMouseAbsolutePointer->ReportLayout.ButtonBitCount  = 3;
    }

    UpdateMinPacketBytes (&UsbMouseAbsolutePointer->ReportLayout);

    DEBUG ((
      DEBUG_INFO,
      "UsbMouseAbsolutePointer: layout HasId=%d Id=0x%02x X@%u/%u Y@%u/%u Btn@%u/%u Wheel@%u/%u MinLen=%u\n",
      UsbMouseAbsolutePointer->ReportLayout.HasReportId,
      UsbMouseAbsolutePointer->ReportLayout.ReportId,
      UsbMouseAbsolutePointer->ReportLayout.XBitOffset,
      UsbMouseAbsolutePointer->ReportLayout.XBitSize,
      UsbMouseAbsolutePointer->ReportLayout.YBitOffset,
      UsbMouseAbsolutePointer->ReportLayout.YBitSize,
      UsbMouseAbsolutePointer->ReportLayout.ButtonBitOffset,
      UsbMouseAbsolutePointer->ReportLayout.ButtonBitCount,
      UsbMouseAbsolutePointer->ReportLayout.WheelBitOffset,
      UsbMouseAbsolutePointer->ReportLayout.WheelBitSize,
      UsbMouseAbsolutePointer->ReportLayout.MinPacketBytes
      ));
  }

  if (UsbMouseAbsolutePointer->PrivateData.ButtonDetected) {
    UsbMouseAbsolutePointer->NumberOfButtons = (UINT8)(
      UsbMouseAbsolutePointer->PrivateData.ButtonMaxIndex -
      UsbMouseAbsolutePointer->PrivateData.ButtonMinIndex + 1
      );
  } else {
    UsbMouseAbsolutePointer->NumberOfButtons =
      UsbMouseAbsolutePointer->ReportLayout.ButtonBitCount;
  }

  if ((UsbMouseAbsolutePointer->XLogicMax == 0) && (UsbMouseAbsolutePointer->XLogicMin == 0)) {
    UsbMouseAbsolutePointer->XLogicMax = 1023;
    UsbMouseAbsolutePointer->XLogicMin = -1023;
  }

  if ((UsbMouseAbsolutePointer->YLogicMax == 0) && (UsbMouseAbsolutePointer->YLogicMin == 0)) {
    UsbMouseAbsolutePointer->YLogicMax = 1023;
    UsbMouseAbsolutePointer->YLogicMin = -1023;
  }

  return EFI_SUCCESS;
}
