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
#include "IntelHDMobileGraphics.h"
#include "PCIDeviceStub.h"

static const void *getVTable(const IOPCIDevice *object)
{
    return *(const void *const *)object;
}

static void setVTable(IOPCIDevice *object, const void *vtable)
{
    *(const void **)object = vtable;
}

#define super IOService
OSDefineMetaClassAndStructors(IntelHDMobileGraphics, IOService);

bool IntelHDMobileGraphics::init(OSDictionary *propTable)
{
    IOLog("IntelHDMobileGraphics::init()\n");
    
    bool ret = super::init(propTable);
    
    if (ret)
    {
        PCIDeviceStub *stub = OSTypeAlloc(PCIDeviceStub);
        mStubVtable = getVTable(stub);
        stub->release();
    }
    
    return ret;
}

void IntelHDMobileGraphics::free()
{
    IOLog("IntelHDMobileGraphics::free()\n");
    
    super::free();
}

bool IntelHDMobileGraphics::start(IOService *provider)
{
    IOLog("IntelHDMobileGraphics::start()\n");
    
    if (!super::start(provider))
        return false;
    
    IOPCIDevice *device = OSDynamicCast(IOPCIDevice, provider);
    
    if (!device)
    {
        IOLog("IntelHDMobileGraphics: provider is not a IOPCIDevice: %s",
              provider->getMetaClass()->getClassName());
        
        return false;
    }
    
    mDeviceVtable = getVTable(device);
    setVTable(device, mStubVtable);
    
    return true;
}

void IntelHDMobileGraphics::stop(IOService *provider)
{
    IOLog("IntelHDMobileGraphics::stop()\n");
    //setVTable(OSDynamicCast(IOPCIDevice, provider), mDeviceVtable);
    super::stop(provider);
}