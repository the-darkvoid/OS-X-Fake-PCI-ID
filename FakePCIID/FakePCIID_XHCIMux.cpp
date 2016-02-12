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
#include "FakePCIID.h"
#include "FakePCIID_XHCIMux.h"
#include "PCIDeviceStub.h"

//////////////////////////////////////////////////////////////////////////////

OSDefineMetaClassAndStructors(FakePCIID_XHCIMux, FakePCIID);

bool FakePCIID_XHCIMux::init(OSDictionary *propTable)
{
    DebugLog("FakePCIID_XHCIMux::init\n");

    if (!super::init(propTable))
        return false;

    // capture vtable pointer for PCIDeviceStub_XHCIMux
    PCIDeviceStub *stub = OSTypeAlloc(PCIDeviceStub_XHCIMux);
    mStubVtable = getVTable(stub);
    stub->release();

    return true;
}

bool FakePCIID_XHCIMux::hookProvider(IOService *provider)
{
    DebugLog("FakePCIID_XHCIMux::hookProvider\n");

    // need to run hookProvider first as it injects properties for startup
    bool init = !mDeviceVtable;
    bool result = super::hookProvider(provider);

    // write initial value to PR2 early...
    if (init)
        ((PCIDeviceStub_XHCIMux*)provider)->startup();

    return result;
}

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
            UInt32 mask;
            if (getBoolProperty(kPR2HonorPR2M, true))
                mask = super::configRead32(super::space, kXHCI_PCIConfig_PR2M);
            else
                mask = getUInt32Property(kPR2ChipsetMask);
            newData = super::configRead32(space, kXHCI_PCIConfig_PR2);
            newData &= ~mask;
            newData |= getUInt32Property(kPR2Force) & mask;
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

    UInt32 mask;
    if (getBoolProperty(kPR2HonorPR2M, true))
        mask = super::configRead32(super::space, kXHCI_PCIConfig_PR2M);
    else
        mask = getUInt32Property(kPR2ChipsetMask);
    UInt32 newData = super::configRead32(super::space, kXHCI_PCIConfig_PR2);
    newData &= ~mask;
    newData |= getUInt32Property(kPR2Force) & mask;
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
