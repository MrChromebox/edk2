## @file
# This file contains the script to build UniversalPayload
#
# Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

import argparse
import subprocess
import os
import shutil
import sys
from   ctypes import *

sys.dont_write_bytecode = True

class UPLD_INFO_HEADER(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ('Identifier',           ARRAY(c_char, 4)),
        ('HeaderLength',         c_uint32),
        ('SpecRevision',         c_uint16),
        ('Reserved',             c_uint16),
        ('Revision',             c_uint32),
        ('Attribute',            c_uint32),
        ('Capability',           c_uint32),
        ('ProducerId',           ARRAY(c_char, 16)),
        ('ImageId',              ARRAY(c_char, 16)),
        ]

    def __init__(self):
        self.Identifier     =  b'PLDH'
        self.HeaderLength   = sizeof(UPLD_INFO_HEADER)
        self.SpecRevision   = 0x0070
        self.Revision       = 0x0000010105
        self.ImageId        = b'UEFI'
        self.ProducerId     = b'INTEL'

def GenSpecRevision (Argument):
    try:
        (MajorStr, MinorStr) = Argument.split('.')
    except:
        raise argparse.ArgumentTypeError ('{} is not a valid SpecRevision format (Major[8-bits].Minor[8-bits]).'.format (Argument))
    #
    # Spec Revision Bits 15 : 8 - Major Version. Bits 7 : 0 - Minor Version.
    #
    if len(MinorStr) > 0 and len(MinorStr) < 3:
        try:
            Minor = int(MinorStr, 16) if len(MinorStr) == 2 else (int(MinorStr, 16) << 4)
        except:
            raise argparse.ArgumentTypeError ('{} Minor version of SpecRevision is not a valid integer value.'.format (Argument))
    else:
        raise argparse.ArgumentTypeError ('{} is not a valid SpecRevision format (Major[8-bits].Minor[8-bits]).'.format (Argument))

    if len(MajorStr) > 0 and len(MajorStr) < 3:
        try:
            Major = int(MajorStr, 16)
        except:
            raise argparse.ArgumentTypeError ('{} Major version of SpecRevision is not a valid integer value.'.format (Argument))
    else:
        raise argparse.ArgumentTypeError ('{} is not a valid SpecRevision format (Major[8-bits].Minor[8-bits]).'.format (Argument))

    return int('0x{0:02x}{1:02x}'.format(Major, Minor), 0)

def Validate32BitInteger (Argument):
    try:
        Value = int (Argument, 0)
    except:
        raise argparse.ArgumentTypeError ('{} is not a valid integer value.'.format (Argument))
    if Value < 0:
        raise argparse.ArgumentTypeError ('{} is a negative value.'.format (Argument))
    if Value > 0xffffffff:
        raise argparse.ArgumentTypeError ('{} is larger than 32-bits.'.format (Argument))
    return Value

def RunCommand(cmd):
    print(cmd)
    p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,cwd=os.environ['WORKSPACE'])
    while True:
        line = p.stdout.readline()
        if not line:
            break
        print(line.strip().decode(errors='ignore'))

    p.communicate()
    if p.returncode != 0:
        print("- Failed - error happened when run command: %s"%cmd)
        raise Exception("ERROR: when run command: %s"%cmd)

def BuildUniversalPayload(Args, MacroList):
    BuildTarget = Args.Target
    ToolChain = Args.ToolChain
    Quiet     = "--quiet"  if Args.Quiet else ""
    ElfToolChain = 'CLANGDWARF'
    BuildDir     = os.path.join(os.environ['WORKSPACE'], os.path.normpath("Build/UefiPayloadPkgX64"))
    if Args.Arch == 'X64':
        BuildArch      = "X64"
        ObjCopyFlag    = "elf64-x86-64"
        EntryOutputDir = os.path.join(BuildDir, "{}_{}".format (BuildTarget, ElfToolChain), os.path.normpath("X64/UefiPayloadPkg/UefiPayloadEntry/UniversalPayloadEntry/DEBUG/UniversalPayloadEntry.dll"))
    else:
        BuildArch      = "IA32 -a X64"
        ObjCopyFlag    = "elf32-i386"
        EntryOutputDir = os.path.join(BuildDir, "{}_{}".format (BuildTarget, ElfToolChain), os.path.normpath("IA32/UefiPayloadPkg/UefiPayloadEntry/UniversalPayloadEntry/DEBUG/UniversalPayloadEntry.dll"))

    if Args.PreBuildUplBinary is not None:
        EntryOutputDir = os.path.abspath(Args.PreBuildUplBinary)
    EntryModuleInf = os.path.normpath("UefiPayloadPkg/UefiPayloadEntry/UniversalPayloadEntry.inf")
    DscPath = os.path.normpath("UefiPayloadPkg/UefiPayloadPkg.dsc")
    DxeFvOutputDir = os.path.join(BuildDir, "{}_{}".format (BuildTarget, ToolChain), os.path.normpath("FV/DXEFV.Fv"))
    BdsFvOutputDir = os.path.join(BuildDir, "{}_{}".format (BuildTarget, ToolChain), os.path.normpath("FV/BDSFV.Fv"))
    PayloadReportPath = os.path.join(BuildDir, "UefiUniversalPayload.txt")
    ModuleReportPath = os.path.join(BuildDir, "UefiUniversalPayloadEntry.txt")
    UpldInfoFile = os.path.join(BuildDir, "UniversalPayloadInfo.bin")

    if "CLANG_BIN" in os.environ:
        LlvmObjcopyPath = os.path.join(os.environ["CLANG_BIN"], "llvm-objcopy")
    else:
        LlvmObjcopyPath = "llvm-objcopy"
    try:
        RunCommand('"%s" --version'%LlvmObjcopyPath)
    except:
        print("- Failed - Please check if LLVM is installed or if CLANG_BIN is set correctly")
        sys.exit(1)

    Pcds = ""
    if (Args.pcd != None):
        for PcdItem in Args.pcd:
            Pcds += " --pcd {}".format (PcdItem)

    Defines = ""
    for key in MacroList:
        Defines +=" -D {0}={1}".format(key, MacroList[key])

    #
    # Building DXE core and DXE drivers as DXEFV.
    #
    if Args.BuildEntryOnly == False:
        BuildPayload = "build -p {} -b {} -a X64 -t {} -y {} {}".format (DscPath, BuildTarget, ToolChain, PayloadReportPath, Quiet)
        BuildPayload += Pcds
        BuildPayload += Defines
        RunCommand(BuildPayload)
    #
    # Building Universal Payload entry.
    #
    if Args.PreBuildUplBinary is None:
        BuildModule = "build -p {} -b {} -a {} -m {} -t {} -y {} {}".format (DscPath, BuildTarget, BuildArch, EntryModuleInf, ElfToolChain, ModuleReportPath, Quiet)
        BuildModule += Pcds
        BuildModule += Defines
        RunCommand(BuildModule)
    #
    # Buid Universal Payload Information Section ".upld_info"
    #
    upld_info_hdr = UPLD_INFO_HEADER()
    upld_info_hdr.SpecRevision = Args.SpecRevision
    upld_info_hdr.Revision = Args.Revision
    upld_info_hdr.ProducerId = Args.ProducerId.encode()[:16]
    upld_info_hdr.ImageId = Args.ImageId.encode()[:16]
    upld_info_hdr.Attribute |= 1 if BuildTarget == "DEBUG" else 0
    fp = open(UpldInfoFile, 'wb')
    fp.write(bytearray(upld_info_hdr))
    fp.close()

    if Args.BuildEntryOnly == False:
        #
        # Copy the DXEFV as a section in elf format Universal Payload entry.
        #
        remove_section = '"{}" -I {} -O {} --remove-section .upld_info --remove-section .upld.uefi_fv --remove-section .upld.bds_fv {}'.format (
                           LlvmObjcopyPath,
                           ObjCopyFlag,
                           ObjCopyFlag,
                           EntryOutputDir
                           )
        add_section    = '"{}" -I {} -O {} --add-section .upld_info={} --add-section .upld.uefi_fv={} --add-section .upld.bds_fv={} {}'.format (
                           LlvmObjcopyPath,
                           ObjCopyFlag,
                           ObjCopyFlag,
                           UpldInfoFile,
                           DxeFvOutputDir,
                           BdsFvOutputDir,
                           EntryOutputDir
                           )
        set_section    = '"{}" -I {} -O {} --set-section-alignment .upld_info=4 --set-section-alignment .upld.uefi_fv=16 --set-section-alignment .upld.bds_fv=16 {}'.format (
                           LlvmObjcopyPath,
                           ObjCopyFlag,
                           ObjCopyFlag,
                           EntryOutputDir
                           )
        RunCommand(remove_section)
        RunCommand(add_section)
        RunCommand(set_section)

    shutil.copy (EntryOutputDir, os.path.join(BuildDir, 'UniversalPayload.elf'))

def main():
    parser = argparse.ArgumentParser(description='For building Universal Payload')
    parser.add_argument('-t', '--ToolChain')
    parser.add_argument('-b', '--Target', default='DEBUG')
    parser.add_argument('-a', '--Arch', choices=['IA32', 'X64'], help='Specify the ARCH for payload entry module. Default build X64 image.', default ='X64')
    parser.add_argument("-D", "--Macro", action="append", default=["UNIVERSAL_PAYLOAD=TRUE"])
    parser.add_argument('-i', '--ImageId', type=str, help='Specify payload ID (16 bytes maximal).', default ='UEFI')
    parser.add_argument('-q', '--Quiet', action='store_true', help='Disable all build messages except FATAL ERRORS.')
    parser.add_argument("-p", "--pcd", action="append")
    parser.add_argument("-s", "--SpecRevision", type=GenSpecRevision, default ='0.7', help='Indicates compliance with a revision of this specification in the BCD format.')
    parser.add_argument("-r", "--Revision", type=Validate32BitInteger, default ='0x0000010105', help='Revision of the Payload binary. Major.Minor.Revision.Build')
    parser.add_argument("-o", "--ProducerId", default ='INTEL', help='A null-terminated OEM-supplied string that identifies the payload producer (16 bytes maximal).')
    parser.add_argument("-e", "--BuildEntryOnly", action='store_true', help='Build UniversalPayload Entry file')
    parser.add_argument("-pb", "--PreBuildUplBinary", default=None, help='Specify the UniversalPayload file')
    MacroList = {}
    args = parser.parse_args()
    if args.Macro is not None:
        for Argument in args.Macro:
            if Argument.count('=') != 1:
                print("Unknown variable passed in: %s"%Argument)
                raise Exception("ERROR: Unknown variable passed in: %s"%Argument)
            tokens = Argument.strip().split('=')
            MacroList[tokens[0].upper()] = tokens[1]
    BuildUniversalPayload(args, MacroList)
    print ("Successfully build Universal Payload")

if __name__ == '__main__':
    main()
