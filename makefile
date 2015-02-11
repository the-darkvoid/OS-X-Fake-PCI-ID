# really just some handy scripts...

KEXT=FakePCIID.kext
KEXT_WIFI=FakePCIID_AR9280_as_AR946x.kext
KEXT_GFX=FakePCIID_HD4600_HD4400.kext
DIST=RehabMan-FakePCIID
BUILDDIR=./Build
INSTDIR=/System/Library/Extensions

ifeq ($(findstring 32,$(BITS)),32)
OPTIONS:=$(OPTIONS) -arch i386
endif

ifeq ($(findstring 64,$(BITS)),64)
OPTIONS:=$(OPTIONS) -arch x86_64
endif

OPTIONS:=$(OPTIONS) -scheme FakePCIID

.PHONY: all
all:
	xcodebuild build $(OPTIONS) -configuration Debug
	xcodebuild build $(OPTIONS) -configuration Release

.PHONY: clean
clean:
	xcodebuild clean $(OPTIONS) -configuration Debug
	xcodebuild clean $(OPTIONS) -configuration Release

.PHONY: update_kernelcache
update_kernelcache:
	sudo touch /System/Library/Extensions
	sudo kextcache -update-volume /

.PHONY: install_debug
install_debug:
	sudo rm -Rf $(INSTDIR)/$(KEXT)
	sudo cp -R $(BUILDDIR)/Debug/$(KEXT) $(INSTDIR)
	sudo rm -Rf $(INSTDIR)/$(KEXT_GFX)
	sudo cp -R $(BUILDDIR)/Debug/$(KEXT_GFX) $(INSTDIR)
	make update_kernelcache

.PHONY: install
install:
	sudo rm -Rf $(INSTDIR)/$(KEXT)
	sudo cp -R $(BUILDDIR)/Release/$(KEXT) $(INSTDIR)
	sudo rm -Rf $(INSTDIR)/$(KEXT_GFX)
	sudo cp -R $(BUILDDIR)/Release/$(KEXT_GFX) $(INSTDIR)
	make update_kernelcache

.PHONY: install_debug_wifi
install_debug_wifi:
	sudo rm -Rf $(INSTDIR)/$(KEXT_WIFI)
	sudo cp -R $(BUILDDIR)/Debug/$(KEXT_WIFI) $(INSTDIR)
	make install_debug

.PHONY: install_wifi
install_wifi:
	sudo rm -Rf $(INSTDIR)/$(KEXT_WIFI)
	sudo cp -R $(BUILDDIR)/Release/$(KEXT_WIFI) $(INSTDIR)
	make install

.PHONY: distribute
distribute:
	if [ -e ./Distribute ]; then rm -r ./Distribute; fi
	mkdir ./Distribute
	cp -R $(BUILDDIR)/Debug ./Distribute
	cp -R $(BUILDDIR)/Release ./Distribute
	find ./Distribute -path *.DS_Store -delete
	find ./Distribute -path *.dSYM -exec echo rm -r {} \; >/tmp/org.voodoo.rm.dsym.sh
	chmod +x /tmp/org.voodoo.rm.dsym.sh
	/tmp/org.voodoo.rm.dsym.sh
	rm /tmp/org.voodoo.rm.dsym.sh
	ditto -c -k --sequesterRsrc --zlibCompressionLevel 9 ./Distribute ./Archive.zip
	mv ./Archive.zip ./Distribute/`date +$(DIST)-%Y-%m%d.zip`
