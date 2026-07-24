/** @file
  Coreboot CBMEM access and payload timestamp support.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Base.h>
#include <Coreboot.h>

#include <Uefi/UefiBaseType.h>
#include <Uefi/UefiSpec.h>
#include <Pi/PiMultiPhase.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/FdtLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>

#include <Library/CbMemLib.h>

#ifndef CBMEM_TIMESTAMPS
#define CBMEM_TIMESTAMPS  0
#endif

STATIC CONST EFI_GUID mCbMemTableHobGuid = {
  0x9e0d4b6f, 0xa8e8, 0x4d7e,
  { 0x9a, 0x2d, 0x31, 0x0d, 0x4c, 0x9a, 0x8f, 0x2b }
};

struct cbmem_table_hob {
  UINT64  Address;
  UINT32  Size;
  UINT32  Reserved;
};

STATIC
UINT64
CbMemUnpack64 (
  IN struct cbuint64  Value
  )
{
  return LShiftU64 (Value.hi, 32) | Value.lo;
}

STATIC
UINT64
CbMemUnpackReferenceAddress (
  IN UINT64  Value
  )
{
  struct cbuint64  Packed;

  CopyMem (&Packed, &Value, sizeof (Packed));
  return CbMemUnpack64 (Packed);
}

STATIC
UINT16
CorebootChecksum16 (
  IN CONST VOID  *Buffer,
  IN UINTN        Length
  )
{
  CONST UINT8  *Data;
  UINT32       Sum;
  UINT32       Value;
  UINTN        Index;

  Data = Buffer;
  Sum  = 0;
  for (Index = 0; Index < Length; Index++) {
    Value = Data[Index];
    if ((Index & 1) != 0) {
      Value <<= 8;
    }

    Sum += Value;
    if (Sum >= 0x10000) {
      Sum = (Sum + (Sum >> 16)) & 0xFFFF;
    }
  }

  return (UINT16)((~Sum) & 0xFFFF);
}

STATIC
RETURN_STATUS
ValidateCorebootTable (
  IN struct cb_header  *Header,
  IN UINTN             TableSize
  )
{
  UINTN  HeaderSize;
  UINTN  EntriesSize;

  if ((Header == NULL) || (TableSize < sizeof (*Header)) ||
      (Header->signature != CB_HEADER_SIGNATURE) ||
      (Header->header_bytes < sizeof (*Header)) ||
      (Header->header_bytes > TableSize) ||
      (Header->table_bytes < sizeof (struct cb_record)) ||
      (Header->table_bytes > (TableSize - Header->header_bytes)) ||
      (Header->table_entries > (Header->table_bytes / sizeof (struct cb_record))))
  {
    return RETURN_COMPROMISED_DATA;
  }

  HeaderSize = Header->header_bytes;
  EntriesSize = Header->table_bytes;
  if ((CorebootChecksum16 (Header, HeaderSize) != 0) ||
      (CorebootChecksum16 ((UINT8 *)Header + HeaderSize, EntriesSize) !=
       Header->table_checksum))
  {
    return RETURN_CRC_ERROR;
  }

  return RETURN_SUCCESS;
}

STATIC
struct cb_header *
GetCbTableFromFdt (
  IN VOID  *Fdt
  )
{
  INT32               CorebootNode;
  INT32               FirmwareNode;
  INT32               PropertyLength;
  CONST FDT_PROPERTY  *Property;
  CONST UINT64        *Reg;
  UINT64              CbmemAddress;
  UINT64              CbmemEnd;
  UINT64              CbmemSize;
  UINT64              TableAddress;
  UINT64              TableEnd;
  UINT64              TableSize;
  struct cb_header    *Header;

  if ((Fdt == NULL) || (FdtCheckHeader (Fdt) != 0)) {
    return NULL;
  }

  FirmwareNode = FdtSubnodeOffsetNameLen (Fdt, 0, "firmware", sizeof ("firmware") - 1);
  if (FirmwareNode < 0) {
    return NULL;
  }

  CorebootNode = FdtSubnodeOffsetNameLen (
                   Fdt,
                   FirmwareNode,
                   "coreboot",
                   sizeof ("coreboot") - 1
                   );
  if (CorebootNode < 0) {
    return NULL;
  }

  Property = FdtGetProperty (Fdt, CorebootNode, "reg", &PropertyLength);
  if ((Property == NULL) || (PropertyLength != (4 * sizeof (UINT64)))) {
    return NULL;
  }

  Reg          = (CONST UINT64 *)Property->Data;
  TableAddress = Fdt64ToCpu (ReadUnaligned64 (&Reg[0]));
  TableSize    = Fdt64ToCpu (ReadUnaligned64 (&Reg[1]));
  CbmemAddress = Fdt64ToCpu (ReadUnaligned64 (&Reg[2]));
  CbmemSize    = Fdt64ToCpu (ReadUnaligned64 (&Reg[3]));

  if ((TableAddress > MAX_UINTN) || (TableSize < sizeof (*Header)) ||
      (TableAddress > (MAX_UINT64 - TableSize)) ||
      (CbmemAddress > (MAX_UINT64 - CbmemSize)))
  {
    return NULL;
  }

  TableEnd = TableAddress + TableSize;
  CbmemEnd = CbmemAddress + CbmemSize;
  if ((TableAddress < CbmemAddress) || (TableEnd > CbmemEnd)) {
    return NULL;
  }

  Header = (struct cb_header *)(UINTN)TableAddress;
  return RETURN_ERROR (ValidateCorebootTable (Header, (UINTN)TableSize)) ? NULL : Header;
}

STATIC
RETURN_STATUS
GetCorebootTable (
  OUT struct cb_header  **Header
  )
{
  struct cb_header  *Candidate;
  struct cbmem_table_hob  *TableHob;
  VOID                *GuidHob;
  RETURN_STATUS      Status;

  if (Header == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  *Header   = NULL;
  Candidate = (struct cb_header *)(UINTN)PcdGet64 (PcdBootloaderParameter);
  Status = ValidateCorebootTable (Candidate, MAX_UINTN);
  if (!RETURN_ERROR (Status)) {
    *Header = Candidate;
    return RETURN_SUCCESS;
  }

  Candidate = GetCbTableFromFdt ((VOID *)(UINTN)PcdGet64 (PcdBootloaderParameter));
  if (Candidate != NULL) {
    Status = PcdSet64S (PcdBootloaderParameter, (UINT64)(UINTN)Candidate);
    if (RETURN_ERROR (Status)) {
      return Status;
    }

    *Header = Candidate;
    return RETURN_SUCCESS;
  }

  GuidHob = GetFirstGuidHob (&mCbMemTableHobGuid);
  if (GuidHob == NULL ||
      GET_GUID_HOB_DATA_SIZE (GuidHob) < sizeof (*TableHob))
  {
    return RETURN_NOT_FOUND;
  }

  TableHob = (struct cbmem_table_hob *)GET_GUID_HOB_DATA (GuidHob);
  if ((TableHob->Address > MAX_UINTN) ||
      RETURN_ERROR (ValidateCorebootTable (
        (struct cb_header *)(UINTN)TableHob->Address,
        TableHob->Size
        )))
  {
    return RETURN_COMPROMISED_DATA;
  }

  *Header = (struct cb_header *)(UINTN)TableHob->Address;
  return RETURN_SUCCESS;
}

STATIC
RETURN_STATUS
FindCorebootRecord (
  IN  UINT32         Tag,
  OUT struct cb_record **Record,
  OUT UINT32         *RecordSize
  )
{
  struct cb_header  *Header;
  struct cb_record  *Current;
  UINT8             *Cursor;
  UINT8             *End;
  UINTN             Index;
  RETURN_STATUS     Status;

  if (Record == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  *Record = NULL;
  if (RecordSize != NULL) {
    *RecordSize = 0;
  }

  Status = GetCorebootTable (&Header);
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  Cursor = (UINT8 *)Header + Header->header_bytes;
  End    = Cursor + Header->table_bytes;
  if (End < Cursor) {
    return RETURN_COMPROMISED_DATA;
  }

  for (Index = 0; Index < Header->table_entries; Index++) {
    if ((UINTN)(End - Cursor) < sizeof (struct cb_record)) {
      return RETURN_COMPROMISED_DATA;
    }

    Current = (struct cb_record *)Cursor;
    if ((Current->size < sizeof (*Current)) ||
        (Current->size > (UINTN)(End - Cursor)))
    {
      return RETURN_COMPROMISED_DATA;
    }

    if (Current->tag == Tag) {
      *Record = Current;
      if (RecordSize != NULL) {
        *RecordSize = Current->size;
      }
      return RETURN_SUCCESS;
    }

    Cursor += Current->size;
  }

  return RETURN_NOT_FOUND;
}

STATIC
RETURN_STATUS
GetLegacyCbmemPointer (
  IN  UINT32  Id,
  OUT VOID    **Address,
  OUT UINT32  *Size
  )
{
  struct cb_record     *Record;
  struct cb_cbmem_ref  *Reference;
  UINT32               Tag;
  RETURN_STATUS        Status;

  switch (Id) {
    case CBMEM_ID_CONSOLE:
      Tag = CB_TAG_CBMEM_CONSOLE;
      break;
    case CBMEM_ID_TIMESTAMP:
      Tag = CB_TAG_TIMESTAMPS;
      break;
    default:
      return RETURN_NOT_FOUND;
  }

  Status = FindCorebootRecord (Tag, &Record, NULL);
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  if ((Record->size < sizeof (*Reference)) || (Address == NULL)) {
    return RETURN_COMPROMISED_DATA;
  }

  Reference = (struct cb_cbmem_ref *)Record;
  *Address  = (VOID *)(UINTN)CbMemUnpackReferenceAddress (Reference->cbmem_addr);
  if (Size != NULL) {
    *Size = 0;
  }

  return (*Address == NULL) ? RETURN_COMPROMISED_DATA : RETURN_SUCCESS;
}

RETURN_STATUS
EFIAPI
CbMemFind (
  IN  UINT32  Id,
  OUT VOID    **Address,
  OUT UINT32  *Size
  )
{
  struct cb_record       *Record;
  struct cb_cbmem_entry  *Entry;
  struct cb_header       *Header;
  UINT8                  *Cursor;
  UINT8                  *End;
  UINTN                  Index;
  RETURN_STATUS          Status;

  if ((Address == NULL) || (Size == NULL)) {
    return RETURN_INVALID_PARAMETER;
  }

  *Address = NULL;
  *Size    = 0;

  Status = GetCorebootTable (&Header);
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  Cursor = (UINT8 *)Header + Header->header_bytes;
  End    = Cursor + Header->table_bytes;
  if (End < Cursor) {
    return RETURN_COMPROMISED_DATA;
  }

  for (Index = 0; Index < Header->table_entries; Index++) {
    if ((UINTN)(End - Cursor) < sizeof (*Record)) {
      return RETURN_COMPROMISED_DATA;
    }

    Record = (struct cb_record *)Cursor;
    if ((Record->size < sizeof (*Record)) ||
        (Record->size > (UINTN)(End - Cursor)))
    {
      return RETURN_COMPROMISED_DATA;
    }

    if ((Record->tag == CB_TAG_CBMEM_ENTRY) &&
        (Record->size >= sizeof (*Entry)))
    {
      Entry = (struct cb_cbmem_entry *)Record;
      if (Entry->id == Id) {
        *Address = (VOID *)(UINTN)CbMemUnpack64 (Entry->address);
        *Size    = Entry->entry_size;
        return (*Address == NULL) ? RETURN_COMPROMISED_DATA : RETURN_SUCCESS;
      }
    }

    Cursor += Record->size;
  }

  return GetLegacyCbmemPointer (Id, Address, Size);
}

RETURN_STATUS
EFIAPI
CbMemPublishTableHob (
  VOID
  )
{
  struct cb_header       *Header;
  struct cbmem_table_hob  TableHob;
  RETURN_STATUS           Status;

  Status = GetCorebootTable (&Header);
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  TableHob.Address  = (UINT64)(UINTN)Header;
  TableHob.Size     = Header->header_bytes + Header->table_bytes;
  TableHob.Reserved = 0;
  return (BuildGuidDataHob (
            &mCbMemTableHobGuid,
            &TableHob,
            sizeof (TableHob)
            ) == NULL) ? RETURN_OUT_OF_RESOURCES : RETURN_SUCCESS;
}

RETURN_STATUS
EFIAPI
CbMemLogSummary (
  VOID
  )
{
  struct cb_timestamp_table  *Table;
  UINT32                     Size;
  RETURN_STATUS              Status;

  Status = CbMemFind (CBMEM_ID_TIMESTAMP, (VOID **)&Table, &Size);
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  if ((Size != 0) && (Size < sizeof (*Table))) {
    return RETURN_COMPROMISED_DATA;
  }

  if ((Table->max_entries == 0) ||
      (Table->num_entries > Table->max_entries) ||
      ((Size != 0) &&
       (Table->max_entries > ((Size - sizeof (*Table)) / sizeof (Table->entries[0])))))
  {
    return RETURN_COMPROMISED_DATA;
  }

  DEBUG ((
    DEBUG_INFO,
    "CBMEM: timestamp table=%p size=0x%x base=0x%lx entries=%u/%u freq=%uMHz\n",
    Table,
    Size,
    Table->base_time,
    Table->num_entries,
    Table->max_entries,
    Table->tick_freq_mhz
    ));

  return RETURN_SUCCESS;
}

RETURN_STATUS
EFIAPI
CbMemTimestampAdd (
  IN UINT32  Id
  )
{
#if CBMEM_TIMESTAMPS && (defined (MDE_CPU_X64) || defined (MDE_CPU_IA32))
  struct cb_timestamp_table  *Table;
  UINT32                     Size;
  UINT64                     Timestamp;
  RETURN_STATUS              Status;

  Status = CbMemFind (CBMEM_ID_TIMESTAMP, (VOID **)&Table, &Size);
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  if ((Size < sizeof (*Table)) || (Table->num_entries >= Table->max_entries) ||
      (Table->max_entries > ((Size - sizeof (*Table)) / sizeof (Table->entries[0]))))
  {
    return RETURN_COMPROMISED_DATA;
  }

  Timestamp = AsmReadTsc ();
  if (Timestamp < Table->base_time) {
    return RETURN_UNSUPPORTED;
  }

  Table->entries[Table->num_entries].entry_id    = Id;
  Table->entries[Table->num_entries].entry_stamp = (INT64)(Timestamp - Table->base_time);
  Table->num_entries++;
  return RETURN_SUCCESS;
#else
  (VOID)Id;
  return RETURN_UNSUPPORTED;
#endif
}
