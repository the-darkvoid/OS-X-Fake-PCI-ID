/*
 *  Released under "The GNU General Public License (GPL-2.0)"
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 *  Based on iSightDefender concept by Stephen Checkoway (https://github.com/stevecheckoway/iSightDefender)
 */

#include "PCIDeviceStub.h"

#define super IOPCIDevice
OSDefineMetaClassAndStructors(PCIDeviceStub, IOPCIDevice);

UInt32 PCIDeviceStub::configRead32(IOPCIAddressSpace space, UInt8 offset)
{
    UInt32 result = super::configRead32(space, offset);
    
    IOLog("IntelHDMobileGraphics: configRead32 address space 0x%08x --> 0x%08x\n", space.bits, result);
    
    // Replace Intel Mobile ID with Intel Desktop ID
    if (result == 0x04168086)
        return 0x04128086;
    
    return result;
}

#ifdef HOOK_ALL
UInt16 PCIDeviceStub::configRead16(IOPCIAddressSpace space, UInt8 offset)
{
    UInt16 result = super::configRead16(space, offset);
    
    IOLog("IntelHDMobileGraphics: configRead16 address space --> 0x%04x\n", result);
    
    return result;
}

UInt8 PCIDeviceStub::configRead8(IOPCIAddressSpace space, UInt8 offset)
{
    UInt8 result = super::configRead8(space, offset);
    
    IOLog("IntelHDMobileGraphics: configRead8 address space --> 0x%02x\n", result);
    
    return result;
}

UInt32 PCIDeviceStub::configRead32(UInt8 offset)
{
    UInt32 result = super::configRead32(offset);
    
    IOLog("IntelHDMobileGraphics: configRead32 offset 0x%08x --> 0x%08x\n", offset, result);
    
    return result;
}

UInt16 PCIDeviceStub::configRead16(UInt8 offset)
{
    UInt16 result = super::configRead16(offset);
    
    IOLog("IntelHDMobileGraphics: configRead16 offset 0x%08x --> 0x%04x\n", offset, result);
    
    return result;
}

UInt8 PCIDeviceStub::configRead8(UInt8 offset)
{
    UInt8 result = super::configRead8(offset);
    
    IOLog("IntelHDMobileGraphics: configRead8 offset 0x%08x --> 0x%02x\n", offset, result);
    
    return result;
}

UInt32 PCIDeviceStub::extendedConfigRead32(IOByteCount offset)
{
    UInt32 result = super::extendedConfigRead32(offset);
    
    IOLog("IntelHDMobileGraphics: extendedConfigRead32 offset 0x%08llx --> 0x%08x\n", offset, result);
    
    return result;
}

UInt16 PCIDeviceStub::extendedConfigRead16(IOByteCount offset)
{
    UInt16 result = super::extendedConfigRead16(offset);
    
    IOLog("IntelHDMobileGraphics: extendedConfigRead16 offset 0x%08llx --> 0x%04x\n", offset, result);
    
    return result;
}

UInt8 PCIDeviceStub::extendedConfigRead8(IOByteCount offset)
{
    UInt8 result = super::extendedConfigRead8(offset);
    
    IOLog("IntelHDMobileGraphics: extendedConfigRead8 offset 0x%08llx --> 0x%02x\n", offset, result);
    
    return result;
}
#endif
