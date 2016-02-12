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

#ifndef FakePCIID_XHCIMux_h
#define FakePCIID_XHCIMux_h

#include <IOKit/IOService.h>
#include <IOKit/pci/IOPCIDevice.h>
#include "FakePCIID.h"
#include "PCIDeviceStub.h"

class FakePCIID_XHCIMux : public FakePCIID
{
    OSDeclareDefaultStructors(FakePCIID_XHCIMux);
    typedef FakePCIID super;

public:
    virtual bool init(OSDictionary *propTable);
    virtual bool hookProvider(IOService *provider);
};

#define kXHCI_PCIConfig_PR2     0xd0
#define kXHCI_PCIConfig_PR2M    0xd4

#define kPR2Force       "RM,pr2-force"
#define kPR2Init        "RM,pr2-init"
#define kPR2Block       "RM,pr2-block"
#define kPR2MBlock      "RM,pr2m-block"
#define kPR2HonorPR2M   "RM,pr2-honor-pr2m"
#define kPR2ChipsetMask "RM,pr2-chipset-mask"

class PCIDeviceStub_XHCIMux : public PCIDeviceStub
{
    OSDeclareDefaultStructors(PCIDeviceStub_XHCIMux);
    typedef PCIDeviceStub super;

protected:
    bool getBoolProperty(const char* name, bool defValue);
    UInt32 getUInt32Property(const char* name);

public:
    virtual void configWrite32(IOPCIAddressSpace space, UInt8 offset, UInt32 data);
#ifdef HOOK_ALL
    virtual void configWrite16(IOPCIAddressSpace space, UInt8 offset, UInt16 data);
    virtual void configWrite8(IOPCIAddressSpace space, UInt8 offset, UInt8 data);
#endif

    void startup();
};

#endif
