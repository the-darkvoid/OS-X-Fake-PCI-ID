## FakePCIID.kext

The purpose of this kext is to attach to any IOPCIDevice so it can provide alternate PCI ID when another driver attached to the same device requests them.  This technique can be used instead of patching binaries that may check for supported device-ids (or other PCI IDs) in their IOService::probe or IOService::start method.

In order to attach FakePCIID to a given IOPCIDevice, an injector kext must be built that IOKit can use to match against. 
The FakePCIID.kext Info.plist has no built-in IOKitPersonalities, as it is generic and not built to suit a specific purpose.  The distribution ZIP provide has two such injector kexts, one for Intel HD4400 and HD4600 mobile graphics and one for AR9280 WiFi (for use when a re-branded AR9280 is used to work around a WiFi device whitelist implemented in BIOS).  Custom injector kexts can be created for other devices.

Note: FakePCIID_HD4600_HD4400.kext works for HD4400 mobile, HD4600 mobile, HD4200 mobile, and HD4600 desktop.

In any case, a DSDT patch or FakeID configuration (Clover) will be required to inject the properties that FakePCIID can read on the IOPCIDevice.  Generally this is done with _DSM injection although there are a variety of ways to accomplish such injections.

### Downloads:

Downloads are available on Bitbucket:

https://bitbucket.org/RehabMan/os-x-fake-pci-id/downloads


### How to Install

In all cases, FakePCIID.kext must be installed with a kext installer (such as Kext Wizard).  The Release build should be used for normal installs.  It has a minimum of output to system.log.  For troubleshooting, the Debug build can be used.

In order to cause the kext to be loaded against a particular device, you must also install the appropriate injector kext.  Currently, three injectors are provided:

- FakePCIID_HD4600_HD4400.kext: 
  This will attach to `8086:0416`, `8086:0412` and `8086:0a16`.
  - `8086:0a16` is the native device-id for HD4400 mobile. 
  - `8086:0416` is the native device-id for HD4600 mobile.
  
  Normally, a fake device-id of `8086:0412` will be injected for Yosemite, as Yosemite does not natively recognize `8086:0416`.  `8086:0412` is the native device-id for HD4600 desktop.
  By injecting `0412`, `AppleIntelFramebufferAzul` and `AppleIntelHD5000Graphics` will load.
  And since, FakePCIID will also be attached to these devices, it will successfully fool both kexts that the device an Intel HD4600 Desktop IGPU (0412).

- FakePCIID_AR9280_as_AR946x:
This kext will attach to `168c:0034` or `168c:002a`.

  This particular application of FakePCIID.kext is used in a situation where you have an AR9280 re-branded as some other device.  For example, with the Lenovo u430, it is useful to rebrand an AR9280 ias an AR946x as that device is allowed by the BIOS whitelist where AR9280 is not.
By using FakePCIID, we can remap the PCI IDs back to AR9280 (`168c:002a`) even though the device itself is reporting `168c:0034`.

- FakePCIID_BCM94352Z_as_BCM94360CS2:
	  This kext will attach to `14e4:43b1` or `14e4:43a0`.

  This particular application of FakePCIID.kext is used to emulate an authentic Apple Airport Extreme (BCM94360CS2), when using a BCM94352Z NGFF M.2 WiFi module.

In order to create your own injector, you should be familiar with IOKit matching and kext Info.plist files.  There is ample documentation available on developer.apple.com.  Use the existing injectors as a template to build your own.


### DSDT patches

FakePCIID.kext will return the vendor-id, device-id, subsystem-vendor-id, and subsystem-id as found in the IO registry under the associated IOPCIDevice.  In order to provide the correct/supported values, _DSM injection is employed (or FakeID with Clover).

For example, this is one such patch that might be used for HD4600:
```c
into method label _DSM parent_adr 0x00020000 remove_entry;
into device name_adr 0x00020000 insert
begin
Method (_DSM, 4, NotSerialized)\n
{\n
    If (LEqual (Arg2, Zero)) { Return (Buffer() { 0x03 } ) }\n
    Return (Package()\n
    {\n
        "device-id", Buffer() { 0x12, 0x04, 0x00, 0x00 },\n
        "AAPL,ig-platform-id", Buffer() { 0x06, 0x00, 0x26, 0x0a },\n
        "hda-gfx", Buffer() { "onboard-1" },\n
        "model", Buffer() { "Intel HD 4600" },\n
    })\n
}\n
end;
```

Note that the only property read by FakePCIID in the patch above is "device-id".  Also the "device-id" injection could have been provided by Clover's config.plist (FakeID) or by (as an example) Chimera's IGPDeviceID flag.

The "device-id" property is used both by FakePCIID and by IOKit matching. Generally this is OK, but for flexibility you can specify a different IDs to be used by FakePCIID by using the "RM," prefixed properties.  

So, a minimalist patch would be as follows:
```c
into method label _DSM parent_adr 0x00020000 remove_entry;
into device name_adr 0x00020000 insert
begin
Method (_DSM, 4, NotSerialized)\n
{\n
    If (LEqual (Arg2, Zero)) { Return (Buffer() { 0x03 } ) }\n
    Return (Package()\n
    {\n
        "RM,device-id", Buffer() { 0x12, 0x04, 0x00, 0x00 },\n
    })\n
}\n
end;
```

You would have to inject "device-id" and "ig-platform-id" to have working HD4600 using some other mechanism, of course.  But FakePCIID.kext can do its work with only "RM,device-id".

And this is the patch used in the AR9280 as AR946x scenario:
```c
into method label _DSM parent_label PXSX remove_entry;
into device label PXSX parent_label RP03 insert
begin
Method (_DSM, 4, NotSerialized)\n
{\n
    If (LEqual (Arg2, Zero)) { Return (Buffer() { 0x03 } ) }\n
    Return (Package()\n
    {\n
        "vendor-id", Buffer() { 0x8c, 0x16, 0x00, 0x00 },\n
        "device-id", Buffer() { 0x2a, 0x00, 0x00, 0x00 },\n
        "subsystem-id", Buffer() { 0x8F, 0x00, 0x00, 0x00 },\n
        "subsystem-vendor-id", Buffer() { 0x6B, 0x10, 0x00, 0x00 },\n
        "compatible", "pci168c,2a",\n
        "IOName", "pci168c,2a",\n
        "name", "pci168c,2a",\n
        "AAPL,slot-name", Buffer() { "AirPort" },\n
        "device_type", Buffer() { "AirPort" },\n
        "model", Buffer() { "Atheros 928x 802.11 b/g/n Wireless Network Adapter" },\n
    })\n
}\n
end;
```

For BCM94352Z as BCM94360CS2 the following DSDT patch is used:
```c
into device Label PXSX parent_label RP03 replace_content begin
Method (_DSM, 4, NotSerialized)\n
{\n
	If (LEqual(Arg2, Zero)) { Return (Buffer() { 0x03 } ) }\n
	Return (Package()\n
	{\n
		"vendor-id", Buffer() { 0xe4, 0x14, 0x00, 0x00 },\n
		"device-id", Buffer() { 0xa0, 0x43, 0x00, 0x00 },\n
		"subsystem-vendor-id", Buffer() { 0x6b, 0x10, 0x00, 0x00 },\n
		"subsystem-id", Buffer() { 0x34, 0x01, 0x00, 0x00 },\n
		"compatible", "pci14e4,43a0",\n
		"IOName", "pci14e4,43a0",\n
		"name", "pci14e4,43a0"
	})\n
}\n
end;
```

Please realize that the nodes PXSX and RP03 are specific to the subject DSDT.  In this case a Lenovo u430 laptop.

Again, a minimalist patch for the WiFi scenario would look like this:
```c
into method label _DSM parent_label PXSX remove_entry;
into device label PXSX parent_label RP03 insert
begin
Method (_DSM, 4, NotSerialized)\n
{\n
    If (LEqual (Arg2, Zero)) { Return (Buffer() { 0x03 } ) }\n
    Return (Package()\n
    {\n
        "RM,vendor-id", Buffer() { 0x8c, 0x16, 0x00, 0x00 },\n
        "RM,device-id", Buffer() { 0x2a, 0x00, 0x00, 0x00 },\n
        "RM,subsystem-id", Buffer() { 0x8F, 0x00, 0x00, 0x00 },\n
        "RM,subsystem-vendor-id", Buffer() { 0x6B, 0x10, 0x00, 0x00 },\n
    })\n
}\n
end;
```

Assuming that the function "compatible" served in the original example is with some other mechanism (an injector kext, or Clover configuration).

Properties supported by FakePCIID and their corresponding PCI configuration space offsets are listed below:

- Offset `0x00`: "vendor-id", "RM,vendor-id"
- Offset `0x02`: "device-id", "RM,device-id"
- Offset `0x2c`: "subsystem-vendor-id", "RM,subsystem-vendor-id"
- Offset `0x2e`: "subsystem-id", "RM,subsystem-id"

For more information on the PCI configuration space: http://en.wikipedia.org/wiki/PCI_configuration_space

### Build Environment

My build environment is currently Xcode 6.1, using SDK 10.6, targeting OS X 10.6.

### 32-bit Builds

This project does not support 32-bit builds, although it is probably not difficult to build one given the proper tools.

### Source Code:

The source code is maintained at the following sites:

https://bitbucket.org/RehabMan/os-x-fake-pci-id

https://github.com/RehabMan/OS-X-Fake-PCI-ID

### History

This kext was forked from the project originally named IntelHDMobileGraphics, and was first discussed here: http://www.tonymacx86.com/yosemite-laptop-support/145427-fix-intel-hd4400-hd4600-mobile-yosemite-47.html#post952079

The original repo is now renamed: https://github.com/the-darkvoid/OS-X-Fake-PCI-ID

So, originally a single purpose kext for Intel HD46000 graphics, it has been modified into a general purpose kext that can be used in many different scenarios.

Note: So far, https://github.com/the-darkvoid/OS-X-Fake-PCI-ID, and https://github.com/RehabMan/OS-X-Fake-PCI-ID are being kept in sync.

