// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#pragma once

#include <cstdint>

// The following types, structures and constants are ported from EDKII,
// see UEFI Platform Initialization Specification for details.

// clang-format off
#pragma pack(1)

using UINT64 = uint64_t;
using UINT32 = uint32_t;
using UINT16 = uint16_t;
using UINT8 = uint8_t;

#define EFI_FIRMWARE_FILE_SYSTEM2_GUID \
        { 0x8c8ce578, 0x8a3d, 0x4f1c, { 0x99, 0x35, 0x89, 0x61, 0x85, 0xc3, 0x2d, 0xd3 } }

#define EFI_VARIABLE_NON_VOLATILE                0x00000001
#define EFI_VARIABLE_BOOTSERVICE_ACCESS          0x00000002
#define EFI_VARIABLE_RUNTIME_ACCESS              0x00000004
#define EFI_VARIABLE_HARDWARE_ERROR_RECORD       0x00000008
#define EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS  0x00000010

typedef struct {
  UINT32  Data1;
  UINT16  Data2;
  UINT16  Data3;
  UINT8   Data4[8];
} EFI_GUID;

typedef struct {
  UINT32 NumBlocks;
  UINT32 Length;
} EFI_FV_BLOCK_MAP_ENTRY;

typedef UINT32  EFI_FVB_ATTRIBUTES_2;

typedef struct {
  UINT8                     ZeroVector[16];
  EFI_GUID                  FileSystemGuid;
  UINT64                    FvLength;
  UINT32                    Signature;
  EFI_FVB_ATTRIBUTES_2      Attributes;
  UINT16                    HeaderLength;
  UINT16                    Checksum;
  UINT16                    ExtHeaderOffset;
  UINT8                     Reserved[1];
  UINT8                     Revision;
  EFI_FV_BLOCK_MAP_ENTRY    BlockMap[1];
} EFI_FIRMWARE_VOLUME_HEADER;

typedef struct {
  EFI_GUID  FvName;
  UINT32    ExtHeaderSize;
} EFI_FIRMWARE_VOLUME_EXT_HEADER;

typedef union {
  struct {
    UINT8   Header;
    UINT8   File;
  } Checksum;
  UINT16    Checksum16;
} EFI_FFS_INTEGRITY_CHECK;

typedef UINT8 EFI_FV_FILETYPE;
typedef UINT8 EFI_FFS_FILE_ATTRIBUTES;
typedef UINT8 EFI_FFS_FILE_STATE;

typedef struct {
  EFI_GUID                Name;
  EFI_FFS_INTEGRITY_CHECK IntegrityCheck;
  EFI_FV_FILETYPE         Type;
  EFI_FFS_FILE_ATTRIBUTES Attributes;
  UINT8                   Size[3];
  EFI_FFS_FILE_STATE      State;
} EFI_FFS_FILE_HEADER;

#pragma pack()
// clang-format on
