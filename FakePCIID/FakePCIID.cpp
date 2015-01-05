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

#ifdef DEBUG
    // merge overriding properties into the provider
    OSDictionary *providerDict = (OSDictionary*)getProperty("FakeProperties");
    
    if (providerDict != NULL)
        MergeDictionaryIntoProvider(provider, providerDict);
#endif
    
    mProvider = device;
    device->retain();

    mDeviceVtable = getVTable(device);
    setVTable(device, mStubVtable);

    return true;
}

void FakePCIID::unhookProvider()
{
    if (!mDeviceVtable)
        return; // not hooked

    // restore provider IOPCIDevice vtable on stop
    setVTable(mProvider, mDeviceVtable);
    mDeviceVtable = NULL;

    mProvider->release();
    mProvider = NULL;
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
    mProvider = NULL;
    
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

    unhookProvider();

    super::stop(provider);
}

void FakePCIID::free()
{
    DebugLog("FakePCIID::free() %p\n", this);

    unhookProvider();

    super::free();
}

#ifdef DEBUG
void FakePCIID::detach(IOService *provider)
{
    DebugLog("FakePCIID::detach() %p\n", this);

    return super::detach(provider);
}
#endif

OSDefineMetaClassAndStructors(FakePCIID_HD4600_HD4400, FakePCIID);

bool FakePCIID_HD4600_HD4400::init(OSDictionary *propTable)
{
    if (!super::init(propTable))
        return false;

    // capture vtable pointer for PCIDeviceStub_HD4600_HD4400
    PCIDeviceStub *stub = OSTypeAlloc(PCIDeviceStub_HD4600_HD4400);
    mStubVtable = getVTable(stub);
    stub->release();

    return true;
}


static bool haveCreatedRef = false;

//================================================================================================
//
//  MergeDictionaryIntoProvider
//
//  We will iterate through the dictionary that we want to merge into our provider.  If
//  the dictionary entry is not an OSDictionary, we will set that property into our provider.  If it is a
//  OSDictionary, we will get our provider's entry and merge our entry into it, recursively.
//
//================================================================================================
//
bool
FakePCIID::MergeDictionaryIntoProvider(IOService * provider, OSDictionary * dictionaryToMerge)
{
    const OSSymbol * 		dictionaryEntry = NULL;
    OSCollectionIterator * 	iter = NULL;
    bool			result = false;
    
    DebugLog("+%s[%p]::MergeDictionary(%p)IntoProvider(%p)", getName(), this, dictionaryToMerge, provider);
    
    if (!provider || !dictionaryToMerge)
        return false;
    
    //
    // rdar://4041566 -- Trick the C++ run-time into keeping us loaded.
    //
    if (haveCreatedRef == false)
    {
        haveCreatedRef = true;
        getMetaClass()->instanceConstructed();
    }
    
    // Get the dictionary whose entries we need to merge into our provider and get
    // an iterator to it.
    //
    iter = OSCollectionIterator::withCollection((OSDictionary *)dictionaryToMerge);
    if ( iter != NULL )
    {
        // Iterate through the dictionary until we run out of entries
        //
        while ( NULL != (dictionaryEntry = (const OSSymbol *)iter->getNextObject()) )
        {
            const char *	str = NULL;
            OSDictionary *	sourceDictionary = NULL;
            OSDictionary *	providerDictionary = NULL;
            OSObject *		providerProperty = NULL;
            
            // Get the symbol name for debugging
            //
            str = dictionaryEntry->getCStringNoCopy();
            DebugLog("%s[%p]::MergeDictionaryIntoProvider  merging \"%s\"\n", getName(), this, str);
            
            // Check to see if our destination already has the same entry.  If it does
            // we assume that it is a dictionary.  Perhaps we should check that
            //
            providerProperty = provider->getProperty(dictionaryEntry);
            if ( providerProperty )
            {
                DebugLog("%s[%p]::MergeDictionaryIntoProvider  provider already had property %s\n", getName(), this, str);
                providerDictionary = OSDynamicCast(OSDictionary, providerProperty);
                if ( providerDictionary )
                {
                    DebugLog("%s[%p]::MergeDictionaryIntoProvider  provider's %s is also a dictionary (%p)\n", getName(), this, str, providerDictionary);
                }
            }
            
            // See if our source entry is also a dictionary
            //
            sourceDictionary = OSDynamicCast(OSDictionary, dictionaryToMerge->getObject(dictionaryEntry));
            if ( sourceDictionary )
            {
                DebugLog("%s[%p]::MergeDictionaryIntoProvider  source dictionary had %s as a dictionary (%p)\n", getName(), this, str, sourceDictionary);
            }
            
            if ( providerDictionary &&  sourceDictionary )
            {
                DebugLog("%s[%p]::MergeDictionaryIntoProvider  merging dictionary(%p) into (%p)\n", getName(), this, sourceDictionary, providerDictionary);
                
                // Need to merge our entry into the provider's dictionary.  However, we don't have a copy of our dictionary, just
                // a reference to it.  So, we need to make a copy of our provider's dictionary
                //
                OSDictionary *		localCopyOfProvidersDictionary;
                UInt32			providerSize;
                UInt32			providerSizeAfterMerge;
                
                localCopyOfProvidersDictionary = OSDictionary::withDictionary( providerDictionary, 0);
                if ( localCopyOfProvidersDictionary == NULL )
                {
                    AlwaysLog("%s::MergeDictionaryIntoProvider  could not copy our provider's dictionary\n",getName());
                    break;
                }
                
                // Get the size of our provider's dictionary so that we can check later whether it changed
                //
                providerSize = providerDictionary->getCapacity();
                DebugLog("%s[%p]::MergeDictionaryIntoProvider  Created a local copy(%p) of dictionary (%p), size %d\n", getName(), this, localCopyOfProvidersDictionary, providerDictionary, (uint32_t)providerSize);
                
                // Note that our providerDictionary *might* change
                // between the time we copied it and when we write it out again.  If so, we will obviously overwrite anychanges
                //
                DebugLog("%s[%p]::MergeDictionaryIntoProvider  need to merge a dictionary \"%s\" %p into %p\n", getName(), this, str, sourceDictionary, localCopyOfProvidersDictionary);
                result = MergeDictionaryIntoDictionary(  sourceDictionary, localCopyOfProvidersDictionary);
                
                if ( result )
                {
                    // Get the size of our provider's dictionary so to see if it's changed  (Yes, the size could remain the same but the contents
                    // could have changed, but this gives us a first approximation.  We're not doing anything with this result, although we could
                    // remerge
                    //
                    providerSizeAfterMerge = providerDictionary->getCapacity();
                    if ( providerSizeAfterMerge != providerSize )
                    {
                        AlwaysLog("%s::MergeDictionaryIntoProvider  our provider's dictionary size changed (%d,%d)\n\n",getName(), (uint32_t) providerSize, (uint32_t) providerSizeAfterMerge);
                    }
                    
                    DebugLog("%s[%p]::MergeDictionaryIntoProvider  setting  property %s from merged dictionary (%p)", getName(), this, str, providerDictionary);
                    result = provider->setProperty( dictionaryEntry, localCopyOfProvidersDictionary );
                    if ( !result )
                    {
                        DebugLog("%s[%p]::MergeDictionaryIntoProvider  setProperty %s , returned false\n", getName(), this, str);
                        break;
                    }
                }
                else
                {
                    // If we got an error merging dictionaries, then just bail out without doing anything
                    //
                    DebugLog("%s[%p]::MergeDictionaryIntoProvider  MergeDictionaryIntoDictionary(%p,%p) returned false\n", getName(), this, sourceDictionary, providerDictionary);
                    break;
                }
            }
            else
            {
                DebugLog("%s[%p]::MergeDictionaryIntoProvider  setting property %s\n", getName(), this, str);
                result = provider->setProperty(dictionaryEntry, dictionaryToMerge->getObject(dictionaryEntry));
                if ( !result )
                {
                    DebugLog("%s[%p]::MergeDictionaryIntoProvider  setProperty %s, returned false\n", getName(), this, str);
                    break;
                }
            }
        }
        iter->release();
    }
    DebugLog("-%s[%p]::MergeDictionaryIntoProvider(%p, %p)  result %d\n", getName(), this, provider, dictionaryToMerge, result);
    
    return result;
}


//================================================================================================
//
//  MergeDictionaryIntoDictionary( parentSourceDictionary, parentTargetDictionary)
//
//  This routine will merge the contents of parentSourceDictionary into the targetDictionary, recursively.
//  Note that we are only modifying copies of the parentTargetDictionary, so we don't expect anybody
//  else to be accessing them at the same time.
//
//================================================================================================
//
bool
FakePCIID::MergeDictionaryIntoDictionary(OSDictionary * parentSourceDictionary,  OSDictionary * parentTargetDictionary)
{
    OSCollectionIterator*	srcIterator = NULL;
    OSSymbol*			keyObject = NULL ;
    OSObject*			targetObject = NULL ;
    bool			result = false;
    
    DebugLog("+%s[%p]::MergeDictionaryIntoDictionary(%p => %p)\n", getName(), this, parentSourceDictionary, parentTargetDictionary);
    
    if (!parentSourceDictionary || !parentTargetDictionary)
        return false ;
    
    // Get our source dictionary
    //
    srcIterator = OSCollectionIterator::withCollection(parentSourceDictionary) ;
    
    while (NULL != (keyObject = OSDynamicCast(OSSymbol, srcIterator->getNextObject())))
    {
        const char *	str;
        OSDictionary *	childSourceDictionary = NULL;
        OSDictionary *	childTargetDictionary = NULL;
        OSObject *	childTargetObject = NULL;
        
        // Get the symbol name for debugging
        //
        str = keyObject->getCStringNoCopy();
        DebugLog("%s[%p]::MergeDictionaryIntoDictionary  merging \"%s\"\n", getName(), this, str);
        
        // Check to see if our destination already has the same entry.
        //
        childTargetObject = parentTargetDictionary->getObject(keyObject);
        if ( childTargetObject )
        {
            childTargetDictionary = OSDynamicCast(OSDictionary, childTargetObject);
            if ( childTargetDictionary )
            {
                DebugLog("%s[%p]::MergeDictionaryIntoDictionary  target object %s is a dictionary (%p)\n", getName(), this, str, childTargetDictionary);
            }
        }
        
        // See if our source entry is also a dictionary
        //
        childSourceDictionary = OSDynamicCast(OSDictionary, parentSourceDictionary->getObject(keyObject));
        if ( childSourceDictionary )
        {
            DebugLog("%s[%p]::MergeDictionaryIntoDictionary  source dictionary had %s as a dictionary (%p)\n", getName(), this, str, childSourceDictionary);
        }
        
        if ( childTargetDictionary && childSourceDictionary)
        {
            // Our target dictionary already has the entry for this same object AND our
            // source is also a dictionary, so we need to recursively add it.
            //
            // Need to merge our entry into the provider's dictionary.  However, we don't have a copy of our dictionary, just
            // a reference to it.  So, we need to make a copy of our target's dictionary
            //
            OSDictionary *		localCopyOfTargetDictionary;
            UInt32			targetSize;
            UInt32			targetSizeAfterMerge;
            
            localCopyOfTargetDictionary = OSDictionary::withDictionary( childTargetDictionary, 0);
            if ( localCopyOfTargetDictionary == NULL )
            {
                AlwaysLog("%s::MergeDictionaryIntoDictionary  could not copy our target's dictionary\n",getName());
                break;
            }
            
            // Get the size of our provider's dictionary so that we can check later whether it changed
            //
            targetSize = childTargetDictionary->getCapacity();
            DebugLog("%s[%p]::MergeDictionaryIntoDictionary  Created a local copy(%p) of dictionary (%p), size %d\n", getName(), this, localCopyOfTargetDictionary, childTargetDictionary, (uint32_t)targetSize);
            
            // Note that our targetDictionary *might* change
            // between the time we copied it and when we write it out again.  If so, we will obviously overwrite anychanges
            //
            DebugLog("%s[%p]::MergeDictionaryIntoDictionary  recursing to merge a dictionary \"%s\" %p into %p\n", getName(), this, str, childSourceDictionary, localCopyOfTargetDictionary);
            result = MergeDictionaryIntoDictionary(childSourceDictionary, localCopyOfTargetDictionary) ;
            if ( result )
            {
                // Get the size of our provider's dictionary so to see if it's changed  (Yes, the size could remain the same but the contents
                // could have changed, but this gives us a first approximation.  We're not doing anything with this result, although we could
                // remerge
                //
                targetSizeAfterMerge = childTargetDictionary->getCapacity();
                if ( targetSizeAfterMerge != targetSize )
                {
                    AlwaysLog("%s::MergeDictionaryIntoDictionary  our target's dictionary size changed (%d,%d)\n",getName(), (uint32_t) targetSize, (uint32_t) targetSizeAfterMerge);
                }
                
                DebugLog("%s[%p]::MergeDictionaryIntoDictionary  setting  dictionary %s from merged dictionary (%p)\n", getName(), this, str, localCopyOfTargetDictionary);
                result = parentTargetDictionary->setObject(keyObject, localCopyOfTargetDictionary);
                if ( !result )
                {
                    DebugLog("%s[%p]::MergeDictionaryIntoDictionary  setProperty %s , returned false\n", getName(), this, str);
                    break;
                }
            }
            else
            {
                // If we got an error merging dictionaries, then just bail out without doing anything
                //
                DebugLog("%s[%p]::MergeDictionaryIntoDictionary  MergeDictionaryIntoDictionary(%p,%p) returned false\n", getName(), this, childSourceDictionary, localCopyOfTargetDictionary);
                break;
            }
        }
        else
        {
            // We have a property that we need to merge into our parent dictionary.
            //
            DebugLog("%s[%p]::MergeDictionaryIntoDictionary  setting object %s into dictionary %p\n", getName(), this, str, parentTargetDictionary);
            result = parentTargetDictionary->setObject(keyObject, parentSourceDictionary->getObject(keyObject)) ;
            if ( !result )
            {
                DebugLog("%s[%p]::MergeDictionaryIntoDictionary  setObject %s, returned false\n", getName(), this, str);
                break;
            }
        }
        
    }
    
    srcIterator->release();
    
    DebugLog("-%s[%p]::MergeDictionaryIntoDictionary(%p=>(%p)  result %d\n", getName(), this, parentSourceDictionary, parentTargetDictionary, result);
    return result;
}
