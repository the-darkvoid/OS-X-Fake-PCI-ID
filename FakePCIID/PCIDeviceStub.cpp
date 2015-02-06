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

int PCIDeviceStub::getIntegerProperty(const char *aKey, const char *alternateKey)
{
    OSData* data = OSDynamicCast(OSData, getProperty(aKey));
    if (!data || sizeof(UInt32) != data->getLength())
    {
        data = OSDynamicCast(OSData, getProperty(alternateKey));
        if (!data || sizeof(UInt32) != data->getLength())
            return -1;
    }
    UInt32 result = *static_cast<const UInt32*>(data->getBytesNoCopy());
    return result;
}

UInt32 PCIDeviceStub::configRead32(IOPCIAddressSpace space, UInt8 offset)
{
    UInt32 result = super::configRead32(space, offset);
    
    UInt16 vendorId = super::configRead16(super::space, kIOPCIConfigVendorID);
    UInt16 deviceId = super::configRead16(super::space, kIOPCIConfigDeviceID);
    
    DebugLog("[%04x:%04x] configRead32 address space(0x%08x, 0x%02x) result: 0x%08x\n",
             vendorId, deviceId, space.bits, offset, result);
    
    // Replace return value with injected vendor-id/device-id in ioreg
    UInt32 newResult = result;
    switch (offset)
    {
        case kIOPCIConfigVendorID:
        case kIOPCIConfigDeviceID: // OS X does a non-aligned read, which still returns full vendor / device ID
        {
            int vendor = getIntegerProperty("RM,vendor-id", "vendor-id");
            if (-1 != vendor)
                newResult = (newResult & 0xFFFF0000) | vendor;
            
            int device = getIntegerProperty("RM,device-id", "device-id");
            if (-1 != device)
                newResult = (device << 16) | (newResult & 0xFFFF);
            break;
        }
        case kIOPCIConfigSubSystemVendorID:
        {
            int vendor = getIntegerProperty("RM,subsystem-vendor-id", "subsystem-vendor-id");
            if (-1 != vendor)
                newResult = (newResult & 0xFFFF0000) | vendor;
            
            int device = getIntegerProperty("RM,subsystem-id", "subsystem-id");
            if (-1 != device)
                newResult = (device << 16) | (newResult & 0xFFFF);
            break;
        }
    }
    
    if (newResult != result)
        AlwaysLog("[%04x:%04x] configRead32(0x%02x), result 0x%08x -> 0x%08x\n",
                  vendorId, deviceId, offset, result, newResult);

    return newResult;
}

UInt16 PCIDeviceStub::configRead16(IOPCIAddressSpace space, UInt8 offset)
{
    UInt16 result = super::configRead16(space, offset);
    
    UInt16 vendorId = super::configRead16(super::space, kIOPCIConfigVendorID);
    UInt16 deviceId = super::configRead16(super::space, kIOPCIConfigDeviceID);
    
    DebugLog("[%04x:%04x] configRead16 address space(0x%08x, 0x%02x) result: 0x%04x\n",
             vendorId, deviceId, space.bits, offset, result);

    UInt16 newResult = result;
    switch (offset)
    {
        case kIOPCIConfigVendorID:
        {
            int vendor = getIntegerProperty("RM,vendor-id", "vendor-id");
            if (-1 != vendor)
                newResult = vendor;
            break;
        }
        case kIOPCIConfigDeviceID:
        {
            int device = getIntegerProperty("RM,device-id", "device-id");
            if (-1 != device)
                newResult = device;
            break;
        }
        case kIOPCIConfigSubSystemVendorID:
        {
            int vendor = getIntegerProperty("RM,subsystem-vendor-id", "subsystem-vendor-id");
            if (-1 != vendor)
                newResult = vendor;
            break;
        }
        case kIOPCIConfigSubSystemID:
        {
            int device = getIntegerProperty("RM,subsystem-id", "subsystem-id");
            if (-1 != device)
                newResult = device;
            break;
        }
    }

    if (newResult != result)
        AlwaysLog("[%04x:%04x] configRead16(0x%02x), result 0x%04x -> 0x%04x\n",
                  vendorId, deviceId, offset, result, newResult);

    return newResult;
}

#ifdef HOOK_ALL
UInt8 PCIDeviceStub::configRead8(IOPCIAddressSpace space, UInt8 offset)
{
    UInt8 result = super::configRead8(space, offset);
    
    UInt16 vendorId = super::configRead16(super::space, kIOPCIConfigVendorID);
    UInt16 deviceId = super::configRead16(super::space, kIOPCIConfigDeviceID);
    
    DebugLog("[%04x:%04x] configRead8 address space(0x%08x, 0x%02x) result: 0x%02x\n",
             vendorId, deviceId, space.bits, offset, result);
    
    return result;
}

UInt32 PCIDeviceStub::configRead32(UInt8 offset)
{
    UInt32 result = super::configRead32(offset);
    
    UInt16 vendorId = super::configRead16(super::space, kIOPCIConfigVendorID);
    UInt16 deviceId = super::configRead16(super::space, kIOPCIConfigDeviceID);
    
    DebugLog("[%04x:%04x] configRead32 address (0x%02x) result: 0x%08x\n",
             vendorId, deviceId, offset, result);
    
    return result;
}

UInt16 PCIDeviceStub::configRead16(UInt8 offset)
{
    UInt16 result = super::configRead16(offset);
    
    UInt16 vendorId = super::configRead16(super::space, kIOPCIConfigVendorID);
    UInt16 deviceId = super::configRead16(super::space, kIOPCIConfigDeviceID);
    
    DebugLog("[%04x:%04x] configRead16 address (0x%02x) result: 0x%04x\n",
             vendorId, deviceId, offset, result);
    
    return result;
}

UInt8 PCIDeviceStub::configRead8(UInt8 offset)
{
    UInt8 result = super::configRead8(offset);
    
    UInt16 vendorId = super::configRead16(super::space, kIOPCIConfigVendorID);
    UInt16 deviceId = super::configRead16(super::space, kIOPCIConfigDeviceID);
    
    DebugLog("[%04x:%04x] configRead8 address (0x%02x) result: 0x%02x\n",
             vendorId, deviceId, offset, result);
    
    return result;
}

UInt32 PCIDeviceStub::extendedConfigRead32(IOByteCount offset)
{
    UInt32 result = super::extendedConfigRead32(offset);
    
    UInt16 vendorId = super::configRead16(super::space, kIOPCIConfigVendorID);
    UInt16 deviceId = super::configRead16(super::space, kIOPCIConfigDeviceID);
    
    DebugLog("[%04x:%04x] extendedConfigRead32 address (0x%02llx) result: 0x%08x\n",
             vendorId, deviceId, offset, result);
    
    return result;
}

UInt16 PCIDeviceStub::extendedConfigRead16(IOByteCount offset)
{
    UInt16 result = super::extendedConfigRead16(offset);
    
    UInt16 vendorId = super::configRead16(super::space, kIOPCIConfigVendorID);
    UInt16 deviceId = super::configRead16(super::space, kIOPCIConfigDeviceID);
    
    DebugLog("[%04x:%04x] extendedConfigRead16 address (0x%02llx) result: 0x%04x\n",
             vendorId, deviceId, offset, result);
    
    return result;
}

UInt8 PCIDeviceStub::extendedConfigRead8(IOByteCount offset)
{
    UInt8 result = super::extendedConfigRead8(offset);
    
    UInt16 vendorId = super::configRead16(super::space, kIOPCIConfigVendorID);
    UInt16 deviceId = super::configRead16(super::space, kIOPCIConfigDeviceID);
    
    DebugLog("[%04x:%04x] extendedConfigRead8 address (0x%02llx) result: 0x%02x\n",
             vendorId, deviceId, offset, result);
    
    return result;
}

UInt32 PCIDeviceStub::ioRead32(UInt16 offset, IOMemoryMap* map)
{
    UInt32 result = super::ioRead32(offset);
    
    UInt16 vendorId = super::configRead16(super::space, kIOPCIConfigVendorID);
    UInt16 deviceId = super::configRead16(super::space, kIOPCIConfigDeviceID);
    
    DebugLog("[%04x:%04x] ioRead32 address (0x%04x) result: 0x%08x\n",
             vendorId, deviceId, offset, result);
    
    return result;
}

UInt16 PCIDeviceStub::ioRead16(UInt16 offset, IOMemoryMap* map)
{
    UInt16 result = super::ioRead16(offset);
    
    UInt16 vendorId = super::configRead16(super::space, kIOPCIConfigVendorID);
    UInt16 deviceId = super::configRead16(super::space, kIOPCIConfigDeviceID);
    
    DebugLog("[%04x:%04x] ioRead16 address (0x%04x) result: 0x%04x\n",
             vendorId, deviceId, offset, result);
    
    return result;
}

UInt8 PCIDeviceStub::ioRead8(UInt16 offset, IOMemoryMap* map)
{
    UInt8 result = super::ioRead8(offset);
    
    UInt16 vendorId = super::configRead16(super::space, kIOPCIConfigVendorID);
    UInt16 deviceId = super::configRead16(super::space, kIOPCIConfigDeviceID);
    
    DebugLog("[%04x:%04x] ioRead8 address (0x%04x) result: 0x%02x\n",
             vendorId, deviceId, offset, result);
    
    return result;
}
#endif

hack_OSDefineMetaClassAndStructors(PCIDeviceStub_HD4600_HD4400, PCIDeviceStub);

#define kHD4600_Desktop_DeviceID 0x0412

UInt32 PCIDeviceStub_HD4600_HD4400::configRead32(IOPCIAddressSpace space, UInt8 offset)
{
    UInt32 result = super::configRead32(space, offset);

    DebugLog("HD4600_HD4400: configRead32 address space(0x%08x, 0x%02x) result: 0x%08x\n", space.bits, offset, result);

    // Replace return value with injected vendor-id/device-id in ioreg
    UInt32 newResult = result;
    switch (offset)
    {
        case kIOPCIConfigVendorID:
        case kIOPCIConfigDeviceID: // OS X does a non-aligned read, which still returns full vendor / device ID
        {
            newResult = (kHD4600_Desktop_DeviceID << 16) | (newResult & 0xFFFF);
            break;
        }
    }

    if (newResult != result)
        AlwaysLog("HD4600_HD4400: configRead32(0x%02x), result 0x%08x -> 0x%08x\n", offset, result, newResult);

    return newResult;
}

UInt16 PCIDeviceStub_HD4600_HD4400::configRead16(IOPCIAddressSpace space, UInt8 offset)
{
    UInt16 result = super::configRead16(space, offset);

    DebugLog("HD4600_HD4400: configRead16 address space(0x%08x, 0x%02x) result: 0x%04x\n", space.bits, offset, result);

    UInt16 newResult = result;
    switch (offset)
    {
        case kIOPCIConfigDeviceID:
        {
            newResult = kHD4600_Desktop_DeviceID;
            break;
        }
    }

    if (newResult != result)
        AlwaysLog("HD4600_HD4400: configRead16(0x%02x), result 0x%04x -> 0x%04x\n", offset, result, newResult);

    return newResult;
}

