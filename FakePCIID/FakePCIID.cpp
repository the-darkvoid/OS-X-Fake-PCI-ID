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

bool FakePCIID::hookProvider(IOService *provider)
{
    if (mDeviceVtable)
        return true;  // already hooked

    // hook provider IOPCIDevice vtable on attach/start
    IOPCIDevice *device = OSDynamicCast(IOPCIDevice, provider);
    if (!device)
    {
        AlwaysLog("provider is not a IOPCIDevice: %s\n", provider->getMetaClass()->getClassName());
        return false;
    }

    mDeviceVtable = getVTable(device);
    setVTable(device, mStubVtable);

    return true;
}

void FakePCIID::unhookProvider(IOService* provider)
{
    if (!mDeviceVtable)
        return; // not hooked

    // restore provider IOPCIDevice vtable on stop
    IOPCIDevice* device = OSDynamicCast(IOPCIDevice, provider);
    if (device)
        setVTable(device, mDeviceVtable);
    mDeviceVtable = NULL;
}

bool FakePCIID::init(OSDictionary *propTable)
{
    DebugLog("FakePCIID::init() %p\n", this);
    
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

    mDeviceVtable = NULL;
    
    return true;
}

bool FakePCIID::attach(IOService* provider)
{
    DebugLog("FakePCIID::attach() %p\n", this);

    if (!hookProvider(provider))
        return false;

    return super::attach(provider);
}

bool FakePCIID::start(IOService *provider)
{
    DebugLog("FakePCIID::start() %p\n", this);
    
    if (!super::start(provider))
    {
        AlwaysLog("super::start returned false\n");
        return false;
    }

    if (!hookProvider(provider))
        return false;

    return true;
}

void FakePCIID::stop(IOService *provider)
{
    DebugLog("FakePCIID::stop() %p\n", this);

    unhookProvider(provider);

    super::stop(provider);
}

#ifdef DEBUG
void FakePCIID::detach(IOService *provider)
{
    DebugLog("FakePCIID::detach() %p\n", this);

    return super::detach(provider);
}

void FakePCIID::free()
{
    DebugLog("FakePCIID::free() %p\n", this);

    super::free();
}
#endif

