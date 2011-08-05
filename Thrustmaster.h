/*
 File:		Thrustmaster.h
 Creater:	Michael Milvich, michael@milvich.com
 */

#include <IOKit/usb/IOUSBInterface.h>
#include <IOKit/hid/IOHIDDevice.h>
#include <IOKit/IOBufferMemoryDescriptor.h>
#include <IOKit/IOLib.h>
#include "Constants.h"

class com_milvich_driver_Thrustmaster : public IOHIDDevice
{
    OSDeclareDefaultStructors(com_milvich_driver_Thrustmaster);

public:
    IOBufferMemoryDescriptor    *fReport;
    bool                        fHasRudders;
    bool                        fHasThrottle;
    bool                        fEndThread;
    bool                        fRockerIsModifier;
    char                        fButtonShifts[kNumOfButtons * kNumModifiers];
    char                        fHatSwitchShifts[kNumModifiers];
    int                         fNumButtons;
    bool                        fHatIsModified;
    bool                        fTwistRudder;
    
    IOUSBInterface  *fIface;
    IOUSBPipe       *fPipe;
    int             fOutstandingIOOps;
    IOUSBCompletion fReadCompletion;
    IOUSBCompletion fInitCompletion;
    IOBufferMemoryDescriptor *fBuffer;
    bool            fNeedToClose;
    bool            fFinishedInit;
    unsigned char   fControlData[8];
    IOCommandGate   *fGate;

public:
        
    virtual OSString* newTransportString() const;
    virtual OSString* newProductString() const;

    virtual OSNumber* newPrimaryUsageNumber() const;
    virtual OSNumber* newPrimaryUsagePageNumber() const;

    virtual IOReturn getReport(IOMemoryDescriptor *report, IOHIDReportType reportType, IOOptionBits options);
    virtual IOReturn getReport(IOMemoryDescriptor *report, UInt8 *data, IOByteCount length);
    virtual void packet(UInt8 *data, IOByteCount length);

    virtual IOReturn newReportDescriptor(IOMemoryDescriptor ** descriptor ) const;
    
    
    // USB functions...
    virtual bool init(OSDictionary *properties);
    virtual IOService* probe(IOService *provider, SInt32 *score );
    virtual bool handleStart( IOService * provider );
    virtual void handleStop(IOService *provider);
    virtual bool willTerminate(IOService *provider, IOOptionBits options ); 
    virtual bool didTerminate(IOService *provider, IOOptionBits options, bool *defer );
    
    virtual void handleInit();
    static void initThread(void *arg);
    virtual void handleInitFinshed();
    static IOReturn initFinished(OSObject *obj, void *arg0, void *arg1, void *arg2, void *arg3);
    
    virtual IOReturn startReadLoop();
    virtual void handleRead(IOReturn status);
    static void readCallback(void *target, void *parameter, IOReturn status, UInt32 bufferSizeRemaining);
    virtual void incrementOutstandingIO();
    virtual void decrementOutstandingIO();
};