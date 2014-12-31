Intel HD Mobile Graphics
=====================

On OS X Yosemite Intel HD4600 Mobile graphic cards are no longer supported out of the box, even when injecting a different device ID through DSDT.

A [solution](http://pikeralpha.wordpress.com/2014/09/10/experimental-bin-patch-for-hd4600-mobile-gt2/) was found by Pike R. Alpha, by means of a binary file patch to AppleIntelHD5000Graphics.kext
This patch can still be applied through Clover to prevent changes to the operating system.

However an [additional patch](http://www.tonymacx86.com/yosemite-laptop-support/145427-fix-intel-hd4400-hd4600-mobile-yosemite.html) is required to enable OpenCL/OpenGL without problems.

When looking in detail, both AppleIntelHD5000Graphics and OpenCL.Framework retrieve the graphics device and product ID directly from the IOPCIDevice configuration space.
Due to this any injection though kext or DSDT is ineffective.

IntelHDMobileGraphics however is able to act as a layer in between AppleIntelHD5000Graphics and the IOPCIDevice representing the graphics card.
This allows modifying the [PCI configuration space data](http://en.wikipedia.org/wiki/PCI_configuration_space) (specifically the `VID` and `PID`) as it is read from the device, successfully making it appear as Intel HD4600 Desktop.

Due to this no patches are needed to either AppleIntelHD5000Graphics or OpenCL.Framework and the Intel HD4600 should work out of the box.

Currently this is in proof of concept stage, but it can be enhanced to also take care of the device ID matching.

A pre-release testing build can be found in the releases section.
