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

#include <IOKit/IOLib.h>
#include "PCIDeviceStub.h"
#include "FakePCIID.h"

// We want ioreg to still see the normal class hierarchy for hooked
// provider IOPCIDevice
//
// Normal:
//  IOPCIDevice : IOService : IORegistryEntry : OSObject
//
// Without this hack:
//  PCIDeviceStub : IOPCIDevice : IOService : IORegistryEntry : OSObject
//

#define hack_OSDefineMetaClassAndStructors(className, superclassName) \
    hack_OSDefineMetaClassAndStructorsWithInit(className, superclassName, )

#define hack_OSDefineMetaClassAndStructorsWithInit(className, superclassName, init) \
    hack_OSDefineMetaClassWithInit(className, superclassName, init) \
    OSDefineDefaultStructors(className, superclassName)

// trick played with getMetaClass to return IOPCIDevice::gMetaClass...
#define hack_OSDefineMetaClassWithInit(className, superclassName, init)       \
    /* Class global data */                                                   \
    className ::MetaClass className ::gMetaClass;                             \
    const OSMetaClass * const className ::metaClass =                         \
        & className ::gMetaClass;                                             \
    const OSMetaClass * const className ::superClass =                        \
        & superclassName ::gMetaClass;                                        \
    /* Class member functions */                                              \
    className :: className(const OSMetaClass *meta)                           \
        : superclassName (meta) { }                                           \
    className ::~ className() { }                                             \
    const OSMetaClass * className ::getMetaClass() const                      \
        { return &IOPCIDevice::gMetaClass; }                                  \
    /* The ::MetaClass constructor */                                         \
    className ::MetaClass::MetaClass()                                        \
        : OSMetaClass(#className, className::superClass, sizeof(className))   \
        { init; }

hack_OSDefineMetaClassAndStructors(PCIDeviceStub, IOPCIDevice);

int PCIDeviceStub::getIntegerProperty(IORegistryEntry* entry, const char *aKey, const char *alternateKey)
{
    OSData* data = OSDynamicCast(OSData, entry->getProperty(aKey));
    if (!data || sizeof(UInt32) != data->getLength())
    {
        if (alternateKey)
            data = OSDynamicCast(OSData, entry->getProperty(alternateKey));
        if (!data || sizeof(UInt32) != data->getLength())
            return -1;
    }
    UInt32 result = *static_cast<const UInt32*>(data->getBytesNoCopy());
    return result;
}

UInt32 PCIDeviceStub::configRead32(IOPCIAddressSpace space, UInt8 offset)
{
    UInt32 result = super::configRead32(space, offset);
    
    UInt32 deviceInfo = super::configRead32(super::space, kIOPCIConfigVendorID);
   
    DebugLog("[%04x:%04x] configRead32 address space(0x%08x, 0x%02x) result: 0x%08x\n",
             deviceInfo & 0xFFFF, deviceInfo >> 16, space.bits, offset, result);
    
    // Replace return value with injected vendor-id/device-id in ioreg
    UInt32 newResult = result;
    switch (offset)
    {
        case kIOPCIConfigVendorID:
        case kIOPCIConfigDeviceID: // OS X does a non-aligned read, which still returns full vendor / device ID
        {
            int vendor = getIntegerProperty(this, "RM,vendor-id", "vendor-id");
            if (-1 != vendor)
                newResult = (newResult & 0xFFFF0000) | vendor;
            
            int device = getIntegerProperty(this, "RM,device-id", "device-id");
            if (-1 != device)
                newResult = (device << 16) | (newResult & 0xFFFF);
            break;
        }
        case kIOPCIConfigSubSystemVendorID:
        {
            int vendor = getIntegerProperty(this, "RM,subsystem-vendor-id", "subsystem-vendor-id");
            if (-1 != vendor)
                newResult = (newResult & 0xFFFF0000) | vendor;
            
            int device = getIntegerProperty(this, "RM,subsystem-id", "subsystem-id");
            if (-1 != device)
                newResult = (device << 16) | (newResult & 0xFFFF);
            break;
        }
        case kIOPCIConfigRevisionID:
        {
            int revision = getIntegerProperty(this, "RM,revision-id", "revision-id");
            
            if (-1 != revision)
                newResult = (newResult & 0xFFFFFF00) | revision;
            break;
        }
    }
    
    if (newResult != result)
        DebugLog("[%04x:%04x] configRead32(0x%02x), result 0x%08x -> 0x%08x\n",
                  deviceInfo & 0xFFFF, deviceInfo >> 16, offset, result, newResult);

    return newResult;
}

UInt16 PCIDeviceStub::configRead16(IOPCIAddressSpace space, UInt8 offset)
{
    UInt16 result = super::configRead16(space, offset);
    
    UInt32 deviceInfo = super::configRead32(super::space, kIOPCIConfigVendorID);
    
    DebugLog("[%04x:%04x] configRead16 address space(0x%08x, 0x%02x) result: 0x%04x\n",
             deviceInfo & 0xFFFF, deviceInfo >> 16, space.bits, offset, result);

    UInt16 newResult = result;
    switch (offset)
    {
        case kIOPCIConfigVendorID:
        {
            int vendor = getIntegerProperty(this, "RM,vendor-id", "vendor-id");
            if (-1 != vendor)
                newResult = vendor;
            break;
        }
        case kIOPCIConfigDeviceID:
        {
            int device = getIntegerProperty(this, "RM,device-id", "device-id");
            if (-1 != device)
                newResult = device;
            break;
        }
        case kIOPCIConfigSubSystemVendorID:
        {
            int vendor = getIntegerProperty(this, "RM,subsystem-vendor-id", "subsystem-vendor-id");
            if (-1 != vendor)
                newResult = vendor;
            break;
        }
        case kIOPCIConfigSubSystemID:
        {
            int device = getIntegerProperty(this, "RM,subsystem-id", "subsystem-id");
            if (-1 != device)
                newResult = device;
            break;
        }
        case kIOPCIConfigRevisionID:
        {
            int revision = getIntegerProperty(this, "RM,revision-id", "revision-id");
            
            if (-1 != revision)
                newResult = (newResult & 0xFF00) | revision;
            break;
        }
    }

    if (newResult != result)
        DebugLog("[%04x:%04x] configRead16(0x%02x), result 0x%04x -> 0x%04x\n",
                  deviceInfo & 0xFFFF, deviceInfo >> 16, offset, result, newResult);

    return newResult;
}

UInt8 PCIDeviceStub::configRead8(IOPCIAddressSpace space, UInt8 offset)
{
    UInt8 result = super::configRead8(space, offset);
 
    UInt32 deviceInfo = super::configRead32(super::space, kIOPCIConfigVendorID);
    
    DebugLog("[%04x:%04x] configRead8 address space(0x%08x, 0x%02x) result: 0x%02x\n",
             deviceInfo & 0xFFFF, deviceInfo >> 16, space.bits, offset, result);
    
    UInt8 newResult = result;
    switch (offset)
    {
        case kIOPCIConfigRevisionID:
        {
            int revision = getIntegerProperty(this, "RM,revision-id", "revision-id");
            
            if (-1 != revision)
                newResult = revision;
            break;
        }
    }
    
    if (newResult != result)
        DebugLog("[%04x:%04x] configRead8(0x%02x), result 0x%02x -> 0x%02x\n",
                  deviceInfo & 0xFFFF, deviceInfo >> 16, offset, result, newResult);
    
    return newResult;
}

#ifdef HOOK_ALL
void PCIDeviceStub::configWrite32(IOPCIAddressSpace space, UInt8 offset, UInt32 data)
{
    UInt32 deviceInfo = super::configRead32(super::space, kIOPCIConfigVendorID);
    
    DebugLog("[%04x:%04x] configWrite32 address space(0x%08x, 0x%02x) data: 0x%08x\n",
             deviceInfo & 0xFFFF, deviceInfo >> 16, space.bits, offset, data);
    
    super::configWrite32(space, offset, data);
}

void PCIDeviceStub::configWrite16(IOPCIAddressSpace space, UInt8 offset, UInt16 data)
{
    UInt32 deviceInfo = super::configRead32(super::space, kIOPCIConfigVendorID);
    
    DebugLog("[%04x:%04x] configWrite16 address space(0x%08x, 0x%02x) data: 0x%04x\n",
             deviceInfo & 0xFFFF, deviceInfo >> 16, space.bits, offset, data);
    
    super::configWrite16(space, offset, data);
}

void PCIDeviceStub::configWrite8(IOPCIAddressSpace space, UInt8 offset, UInt8 data)
{
    UInt32 deviceInfo = super::configRead32(super::space, kIOPCIConfigVendorID);
    
    DebugLog("[%04x:%04x] configWrite8 address space(0x%08x, 0x%02x) data: 0x%02x\n",
             deviceInfo & 0xFFFF, deviceInfo >> 16, space.bits, offset, data);
    
    super::configWrite8(space, offset, data);
}

UInt32 PCIDeviceStub::configRead32(UInt8 offset)
{
    UInt32 result = super::configRead32(offset);
    
    UInt32 deviceInfo = super::configRead32(super::space, kIOPCIConfigVendorID);
    
    DebugLog("[%04x:%04x] configRead32 address (0x%02x) result: 0x%08x\n",
             deviceInfo & 0xFFFF, deviceInfo >> 16, offset, result);
    
    return result;
}

UInt16 PCIDeviceStub::configRead16(UInt8 offset)
{
    UInt16 result = super::configRead16(offset);
    
    UInt32 deviceInfo = super::configRead32(super::space, kIOPCIConfigVendorID);
    
    DebugLog("[%04x:%04x] configRead16 address (0x%02x) result: 0x%04x\n",
             deviceInfo & 0xFFFF, deviceInfo >> 16, offset, result);
    
    return result;
}

UInt8 PCIDeviceStub::configRead8(UInt8 offset)
{
    UInt8 result = super::configRead8(offset);
    
    UInt32 deviceInfo = super::configRead32(super::space, kIOPCIConfigVendorID);
    
    DebugLog("[%04x:%04x] configRead8 address (0x%02x) result: 0x%02x\n",
             deviceInfo & 0xFFFF, deviceInfo >> 16, offset, result);
    
    return result;
}

UInt32 PCIDeviceStub::extendedConfigRead32(IOByteCount offset)
{
    UInt32 result = super::extendedConfigRead32(offset);
    
    UInt32 deviceInfo = super::configRead32(super::space, kIOPCIConfigVendorID);
    
    DebugLog("[%04x:%04x] extendedConfigRead32 address (0x%02llx) result: 0x%08x\n",
             deviceInfo & 0xFFFF, deviceInfo >> 16, offset, result);
    
    return result;
}

UInt16 PCIDeviceStub::extendedConfigRead16(IOByteCount offset)
{
    UInt16 result = super::extendedConfigRead16(offset);
    
    UInt32 deviceInfo = super::configRead32(super::space, kIOPCIConfigVendorID);
    
    DebugLog("[%04x:%04x] extendedConfigRead16 address (0x%02llx) result: 0x%04x\n",
             deviceInfo & 0xFFFF, deviceInfo >> 16, offset, result);
    
    return result;
}

UInt8 PCIDeviceStub::extendedConfigRead8(IOByteCount offset)
{
    UInt8 result = super::extendedConfigRead8(offset);
    
    UInt32 deviceInfo = super::configRead32(super::space, kIOPCIConfigVendorID);
    
    DebugLog("[%04x:%04x] extendedConfigRead8 address (0x%02llx) result: 0x%02x\n",
             deviceInfo & 0xFFFF, deviceInfo >> 16, offset, result);
    
    return result;
}

UInt32 PCIDeviceStub::ioRead32(UInt16 offset, IOMemoryMap* map)
{
    UInt32 result = super::ioRead32(offset);
    
    UInt32 deviceInfo = super::configRead32(super::space, kIOPCIConfigVendorID);
    
    DebugLog("[%04x:%04x] ioRead32 address (0x%04x) result: 0x%08x\n",
             deviceInfo & 0xFFFF, deviceInfo >> 16, offset, result);
    
    return result;
}

UInt16 PCIDeviceStub::ioRead16(UInt16 offset, IOMemoryMap* map)
{
    UInt16 result = super::ioRead16(offset);
    
    UInt32 deviceInfo = super::configRead32(super::space, kIOPCIConfigVendorID);
    
    DebugLog("[%04x:%04x] ioRead16 address (0x%04x) result: 0x%04x\n",
             deviceInfo & 0xFFFF, deviceInfo >> 16, offset, result);
    
    return result;
}

UInt8 PCIDeviceStub::ioRead8(UInt16 offset, IOMemoryMap* map)
{
    UInt8 result = super::ioRead8(offset);
    
    UInt32 deviceInfo = super::configRead32(super::space, kIOPCIConfigVendorID);
    
    DebugLog("[%04x:%04x] ioRead8 address (0x%04x) result: 0x%02x\n",
             deviceInfo & 0xFFFF, deviceInfo >> 16, offset, result);
    
    return result;
}

IODeviceMemory* PCIDeviceStub::getDeviceMemoryWithRegister( UInt8 reg )
{
    IODeviceMemory* result = super::getDeviceMemoryWithRegister(reg);
    
    UInt32 deviceInfo = super::configRead32(super::space, kIOPCIConfigVendorID);
    
    if (result)
        DebugLog("[%04x:%04x] getDeviceMemoryWithRegister address (0x%08llx) size (0x%08llx)\n",
                 deviceInfo & 0xFFFF, deviceInfo >> 16, result->getPhysicalAddress(), result->getLength());
    
    return result;
}

IOMemoryMap* PCIDeviceStub::mapDeviceMemoryWithRegister(UInt8 reg, IOOptionBits options)
{
    IOMemoryMap* result = super::mapDeviceMemoryWithRegister(reg, options);
    
    UInt32 deviceInfo = super::configRead32(super::space, kIOPCIConfigVendorID);
    
    if (result)
        DebugLog("[%04x:%04x] mapDeviceMemoryWithRegister address (0x%08llx) size (0x%08llx)\n",
                 deviceInfo & 0xFFFF, deviceInfo >> 16, result->getPhysicalAddress(), result->getLength());
    
    return result;
}

IODeviceMemory* PCIDeviceStub::ioDeviceMemory(void)
{
    IODeviceMemory* result = super::ioDeviceMemory();
    
    UInt32 deviceInfo = super::configRead32(super::space, kIOPCIConfigVendorID);
    
    if (result)
        DebugLog("[%04x:%04x] ioDeviceMemory address (0x%08llx) size (0x%08llx)\n",
                 deviceInfo & 0xFFFF, deviceInfo >> 16, result->getPhysicalAddress(), result->getLength());
    
    return result;
}

UInt32 PCIDeviceStub::extendedFindPCICapability( UInt32 capabilityID, IOByteCount* offset)
{
    UInt32 result = super::extendedFindPCICapability(capabilityID, offset);
    
    UInt32 deviceInfo = super::configRead32(super::space, kIOPCIConfigVendorID);
    
    DebugLog("[%04x:%04x] extendedFindPCICapability (0x%08x) offset (0x%08llx) result: 0x%08x\n",
             deviceInfo & 0xFFFF, deviceInfo >> 16, capabilityID, *offset, result);
    
    return result;
}

#endif

//////////////////////////////////////////////////////////////////////////////

hack_OSDefineMetaClassAndStructors(PCIDeviceStub_XHCIMux, PCIDeviceStub);

bool PCIDeviceStub_XHCIMux::getBoolProperty(const char* name, bool defValue)
{
    bool result = defValue;
    OSData* data = OSDynamicCast(OSData, getProperty(name));
    if (data && data->getLength() == 1)
        result = *static_cast<const UInt8*>(data->getBytesNoCopy());
    return result;
}

UInt32 PCIDeviceStub_XHCIMux::getUInt32Property(const char* name)
{
    UInt32 result = 0;
    OSData* data = OSDynamicCast(OSData, getProperty(name));
    if (data && data->getLength() == 4)
        result = *static_cast<const UInt32*>(data->getBytesNoCopy());
    return result;
}

void PCIDeviceStub_XHCIMux::configWrite32(IOPCIAddressSpace space, UInt8 offset, UInt32 data)
{
    UInt32 deviceInfo = super::configRead32(space, kIOPCIConfigVendorID);
    DebugLog("[%04x:%04x] XHCIMux::configWrite32 address space(0x%08x, 0x%02x) data: 0x%08x\n",
             deviceInfo & 0xFFFF, deviceInfo >> 16, space.bits, offset, data);

    UInt32 newData = data;
    switch (offset)
    {
        case kXHCI_PCIConfig_PR2:
        {
            if (getBoolProperty(kPR2Block, false))
            {
                AlwaysLog("[%04x:%04x] XHCIMux::configWrite32 address space(0x%08x, 0x%02x) data: 0x%08x blocked\n",
                         deviceInfo & 0xFFFF, deviceInfo >> 16, space.bits, offset, data);
                return;
            }
            UInt32 chipsetMask = getUInt32Property(kPR2ChipsetMask);
            UInt32 mask = getBoolProperty(kPR2HonorPR2M, true) ? super::configRead32(space, kXHCI_PCIConfig_PR2M) : chipsetMask;
            newData = super::configRead32(space, kXHCI_PCIConfig_PR2);
            newData &= ~mask;
            newData |= getUInt32Property(kPR2Force) & chipsetMask;
        }
        break;

        case kXHCI_PCIConfig_PR2M:
        {
            if (getBoolProperty(kPR2MBlock, false))
            {
                AlwaysLog("[%04x:%04x] XHCIMux::configWrite32 address space(0x%08x, 0x%02x) data: 0x%08x blocked\n",
                          deviceInfo & 0xFFFF, deviceInfo >> 16, space.bits, offset, data);
                return;
            }
        }
        break;
    }

    if (newData != data)
    {
        AlwaysLog("[%04x:%04x] XHCIMux::configWrite32 address space(0x%08x, 0x%02x) data: 0x%08x -> 0x%08x\n",
                 deviceInfo & 0xFFFF, deviceInfo >> 16, space.bits, offset, data, newData);
    }

    super::configWrite32(space, offset, newData);
}

void PCIDeviceStub_XHCIMux::startup()
{
    UInt32 deviceInfo = super::configRead32(super::space, kIOPCIConfigVendorID);

    UInt32 chipsetMask = getUInt32Property(kPR2ChipsetMask);
    UInt32 mask = getBoolProperty(kPR2HonorPR2M, true) ? super::configRead32(super::space, kXHCI_PCIConfig_PR2M) : chipsetMask;
    UInt32 newData = super::configRead32(super::space, kXHCI_PCIConfig_PR2);
    newData &= ~mask;
    newData |= getUInt32Property(kPR2Force) & chipsetMask;
    AlwaysLog("[%04x:%04x] XHCIMux::startup: newData for PR2: 0x%08x\n", deviceInfo & 0xFFFF, deviceInfo >> 16, newData);

    super::configWrite32(super::space, kXHCI_PCIConfig_PR2, newData);
}

#ifdef HOOK_ALL

void PCIDeviceStub_XHCIMux::configWrite16(IOPCIAddressSpace space, UInt8 offset, UInt16 data)
{
    UInt32 deviceInfo = super::configRead32(super::space, kIOPCIConfigVendorID);

    DebugLog("[%04x:%04x] configWrite16 address space(0x%08x, 0x%02x) data: 0x%04x\n",
             deviceInfo & 0xFFFF, deviceInfo >> 16, space.bits, offset, data);

    super::configWrite16(space, offset, data);
}

void PCIDeviceStub_XHCIMux::configWrite8(IOPCIAddressSpace space, UInt8 offset, UInt8 data)
{
    UInt32 deviceInfo = super::configRead32(super::space, kIOPCIConfigVendorID);

    DebugLog("[%04x:%04x] configWrite8 address space(0x%08x, 0x%02x) data: 0x%02x\n",
             deviceInfo & 0xFFFF, deviceInfo >> 16, space.bits, offset, data);

    super::configWrite8(space, offset, data);
}

#endif
