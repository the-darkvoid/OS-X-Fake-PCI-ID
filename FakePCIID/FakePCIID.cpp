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
#include "PCIDeviceStub.h"

static inline const void *getVTable(const IOPCIDevice *object)
{
    return *(const void *const *)object;
}

static inline void setVTable(IOPCIDevice *object, const void *vtable)
{
    *(const void **)object = vtable;
}

#define super IOService
OSDefineMetaClassAndStructors(FakePCIID, IOService);

bool FakePCIID::init(OSDictionary *propTable)
{
    DebugLog("FakePCIID::init()\n");
    
    bool ret = super::init(propTable);
    if (!ret)
    {
        AlwaysLog("super::init returned false\n");
        return false;
    }
    
    IOLog("FakePCIID v1.0 starting.\n");
    
    // capture vtable pointer for PCIDeviceStub
    PCIDeviceStub *stub = OSTypeAlloc(PCIDeviceStub);
    mStubVtable = getVTable(stub);
    stub->release();

    return true;
}

#ifdef DEBUG
void FakePCIID::free()
{
    DebugLog("FakePCIID::free()\n");
    
    super::free();
}
#endif

bool FakePCIID::attach(IOService* provider)
{
    DebugLog("FakePCIID::attach()\n");

    IOPCIDevice *device = OSDynamicCast(IOPCIDevice, provider);
    if (!device)
    {
        AlwaysLog("provider is not a IOPCIDevice: %s\n", provider->getMetaClass()->getClassName());
        return false;
    }

    //mDeviceVtable = getVTable(device);
    setVTable(device, mStubVtable);
    
    return super::attach(provider);
}

#ifdef DEBUG
bool FakePCIID::start(IOService *provider)
{
    DebugLog("FakePCIID::start()\n");
    
    if (!super::start(provider))
    {
        AlwaysLog("super::start returned false\n");
        return false;
    }

    return true;
}

void FakePCIID::stop(IOService *provider)
{
    DebugLog("FakePCIID::stop()\n");
    
    //setVTable(OSDynamicCast(IOPCIDevice, provider), mDeviceVtable);
    super::stop(provider);
}
#endif

