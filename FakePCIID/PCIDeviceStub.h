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

#ifndef PCIDeviceStub_h
#define PCIDeviceStub_h

#ifdef DEBUG
#define DebugLog(args...) do { IOLog("FakePCIID: " args); } while (0)
#else
#define DebugLog(args...) do { } while (0)
#endif
#define AlwaysLog(args...) do { IOLog("FakePCIID: " args); } while (0)

#include <IOKit/pci/IOPCIDevice.h>

//#define HOOK_ALL

class PCIDeviceStub : public IOPCIDevice
{
    OSDeclareDefaultStructors(PCIDeviceStub);
    typedef IOPCIDevice super;

protected:
    int getIntegerProperty(const char* aKey, const char* alternateKey);
    
public:
    virtual UInt32 configRead32(IOPCIAddressSpace space, UInt8 offset);
    virtual UInt16 configRead16(IOPCIAddressSpace space, UInt8 offset);
#ifdef HOOK_ALL
    virtual UInt8 configRead8(IOPCIAddressSpace space, UInt8 offset);

    /*! @function configRead32
     @abstract Reads a 32-bit value from the PCI device's configuration space.
     @discussion This method reads a 32-bit configuration space register on the device and returns its value.
     @param offset An 8-bit offset into configuration space, of which bits 0-1 are ignored.
     @result An 32-bit value in host byte order (big endian on PPC). */
    virtual UInt32 configRead32(UInt8 offset);
    
    /*! @function configRead16
     @abstract Reads a 16-bit value from the PCI device's configuration space.
     @discussion This method reads a 16-bit configuration space register on the device and returns its value.
     @param offset An 8-bit offset into configuration space, of which bit 0 is ignored.
     @result An 16-bit value in host byte order (big endian on PPC). */
    virtual UInt16 configRead16(UInt8 offset);
    
    /*! @function configRead8
     @abstract Reads a 8-bit value from the PCI device's configuration space.
     @discussion This method reads a 8-bit configuration space register on the device and returns its value.
     @param offset An 8-bit offset into configuration space.
     @result An 8-bit value. */
    virtual UInt8 configRead8(UInt8 offset);
    
    /*! @function extendedConfigRead32
     @abstract Reads a 32-bit value from the PCI device's configuration space.
     @discussion This method reads a 32-bit configuration space register on the device and returns its value.
     @param offset A byte offset into configuration space, of which bits 0-1 are ignored.
     @result An 32-bit value in host byte order (big endian on PPC). */
    UInt32 extendedConfigRead32(IOByteCount offset);
    
    /*! @function extendedConfigRead16
     @abstract Reads a 16-bit value from the PCI device's configuration space.
     @discussion This method reads a 16-bit configuration space register on the device and returns its value.
     @param offset A byte offset into configuration space, of which bit 0 is ignored.
     @result An 16-bit value in host byte order (big endian on PPC). */
    UInt16 extendedConfigRead16(IOByteCount offset);
    
    /*! @function extendedConfigRead8
     @abstract Reads a 8-bit value from the PCI device's configuration space.
     @discussion This method reads a 8-bit configuration space register on the device and returns its value.
     @param offset A byte offset into configuration space.
     @result An 8-bit value. */
    UInt8 extendedConfigRead8(IOByteCount offset);
    
    /*! @function ioRead32
     @abstract Reads a 32-bit value from an I/O space aperture.
     @discussion This method will read a 32-bit value from a 4 byte aligned offset in an I/O space aperture. If a map object is passed in, the value is read relative to it, otherwise to the value is read relative to the I/O space aperture for the bus. This function encapsulates the differences between architectures in generating I/O space operations. An eieio instruction is included on PPC.
     @param offset An offset into a bus or device's I/O space aperture.
     @param map If the offset is relative to the beginning of a device's aperture, an IOMemoryMap object for that object should be passed in. Otherwise, passing zero will write the value relative to the beginning of the bus' I/O space.
     @result The value read in host byte order (big endian on PPC). */
    
    virtual UInt32 ioRead32( UInt16 offset, IOMemoryMap * map = 0 );
    
    /*! @function ioRead16
     @abstract Reads a 16-bit value from an I/O space aperture.
     @discussion This method will read a 16-bit value from a 2 byte aligned offset in an I/O space aperture. If a map object is passed in, the value is read relative to it, otherwise to the value is read relative to the I/O space aperture for the bus. This function encapsulates the differences between architectures in generating I/O space operations. An eieio instruction is included on PPC.
     @param offset An offset into a bus or device's I/O space aperture.
     @param map If the offset is relative to the beginning of a device's aperture, an IOMemoryMap object for that object should be passed in. Otherwise, passing zero will write the value relative to the beginning of the bus' I/O space.
     @result The value read in host byte order (big endian on PPC). */
    
    virtual UInt16 ioRead16( UInt16 offset, IOMemoryMap * map = 0 );
    
    /*! @function ioRead8
     @abstract Reads a 8-bit value from an I/O space aperture.
     @discussion This method will read a 8-bit value from an offset in an I/O space aperture. If a map object is passed in, the value is read relative to it, otherwise to the value is read relative to the I/O space aperture for the bus. This function encapsulates the differences between architectures in generating I/O space operations. An eieio instruction is included on PPC.
     @param offset An offset into a bus or device's I/O space aperture.
     @param map If the offset is relative to the beginning of a device's aperture, an IOMemoryMap object for that object should be passed in. Otherwise, passing zero will write the value relative to the beginning of the bus' I/O space.
     @result The value read. */
    
    virtual UInt8 ioRead8( UInt16 offset, IOMemoryMap * map = 0 );
#endif
};

class PCIDeviceStub_HD4600_HD4400 : public PCIDeviceStub
{
    OSDeclareDefaultStructors(PCIDeviceStub_HD4600_HD4400);
    typedef PCIDeviceStub super;

public:
    virtual UInt32 configRead32(IOPCIAddressSpace space, UInt8 offset);
    virtual UInt16 configRead16(IOPCIAddressSpace space, UInt8 offset);
};

#endif
