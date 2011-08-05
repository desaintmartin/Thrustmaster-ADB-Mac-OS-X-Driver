/*
 File:		Thrustmaster.cpp
 Creater:	Michael Milvich, michael@milvich.com
 */

#include "Thrustmaster.h"
#include "Constants.h"
#include <IOKit/hidsystem/IOHIDTypes.h>
#include <IOKit/hidsystem/IOHIDParameter.h>
#include <IOKit/IOLib.h>
#include <IOKit/IOPlatformExpert.h>
#include <IOKit/hidsystem/IOHidUsageTables.h>
#include <IOKit/IOReturn.h>

#define NAME "TM"

// I don't know what these do, but I recorded this communication between
// the iMate driver and the iMate device. And replaying them with a short pause
// between them seems to get the iMate device to do what I want.
#define kNumInitCmds    15
static UInt8 gCmd1[] = {0x0f, 0xfe};
static UInt8 gCmd2[] = {0x07, 0xfe};
static IOUSBDevRequest gInitSequence[kNumInitCmds] =
{
    {USBmakebmRequestType(kUSBOut, kUSBVendor, kUSBInterface), 0x01, 0x0004, 0x00FF, 0, NULL, 0},
    {USBmakebmRequestType(kUSBOut, kUSBVendor, kUSBInterface), 0x01, 0x0002, 0x0000, 0, NULL, 0},
    {USBmakebmRequestType(kUSBOut, kUSBVendor, kUSBInterface), 0x00, 0x0030, 0x0000, 0, NULL, 0},
    {USBmakebmRequestType(kUSBOut, kUSBVendor, kUSBInterface), 0x00, 0x007f, 0x0000, 0, NULL, 0},
    {USBmakebmRequestType(kUSBOut, kUSBVendor, kUSBInterface), 0x00, 0x00ff, 0x0000, 0, NULL, 0},
    {USBmakebmRequestType(kUSBOut, kUSBVendor, kUSBInterface), 0x00, 0x007b, 0x0000, sizeof(gCmd1), gCmd1, 0},
    {USBmakebmRequestType(kUSBOut, kUSBVendor, kUSBInterface), 0x00, 0x00ff, 0x0000, 0, NULL, 0},
    {USBmakebmRequestType(kUSBOut, kUSBVendor, kUSBInterface), 0x00, 0x007f, 0x0000, 0, NULL, 0},
    {USBmakebmRequestType(kUSBOut, kUSBVendor, kUSBInterface), 0x00, 0x00fb, 0x0000, sizeof(gCmd2), gCmd2, 0},
    {USBmakebmRequestType(kUSBOut, kUSBVendor, kUSBInterface), 0x00, 0x007f, 0x0000, 0, NULL, 0},
    {USBmakebmRequestType(kUSBOut, kUSBVendor, kUSBInterface), 0x01, 0x0001, 0x8000, 0, NULL, 0},
    {USBmakebmRequestType(kUSBOut, kUSBVendor, kUSBInterface), 0x01, 0x0004, 0x00FF, 0, NULL, 0},
    {USBmakebmRequestType(kUSBOut, kUSBVendor, kUSBInterface), 0x01, 0x0004, 0x000a, 0, NULL, 0},
    {USBmakebmRequestType(kUSBOut, kUSBVendor, kUSBInterface), 0x01, 0x0003, 0x0001, 0, NULL, 0},
    {USBmakebmRequestType(kUSBOut, kUSBVendor, kUSBInterface), 0x00, 0x007e, 0x0000, 0, NULL, 0}
};

// this is the handler ID of the TM device
#define	kTMHandlerID	95

// this is the size of the HID report
#define kReportSize		10

// make sure our super is pointing to the right place...
#undef super
#define super IOHIDDevice

// create our constructors and stuff
OSDefineMetaClassAndStructors(com_milvich_driver_Thrustmaster, IOHIDDevice);

OSString* com_milvich_driver_Thrustmaster::newTransportString() const
{
    return OSString::withCString("ADB");
}

OSString* com_milvich_driver_Thrustmaster::newProductString() const
{
    return OSString::withCString("Thrustmaster");
}

OSNumber* com_milvich_driver_Thrustmaster::newPrimaryUsageNumber() const
{
    // report that we are a joystick
    return OSNumber::withNumber(kHIDUsage_GD_Joystick, 32);
}

OSNumber* com_milvich_driver_Thrustmaster::newPrimaryUsagePageNumber() const
{
    // report that we come from the generic desktop catagory of devices
    return OSNumber::withNumber(kHIDPage_GenericDesktop, 32);
}

IOReturn com_milvich_driver_Thrustmaster::getReport(IOMemoryDescriptor *report, IOHIDReportType reportType, IOOptionBits options)
{
    // I have no idea what the report types or option bits are... so I am just
    // ignoring them..

    // use the data from the last interrupt... they shouldn't have changed...
    return getReport(report, fControlData, sizeof(fControlData));
}

IOReturn com_milvich_driver_Thrustmaster::getReport(IOMemoryDescriptor *report, UInt8 *TMData, IOByteCount length)
{
    UInt8           data[kReportSize];
    int             rockerPosition;
    unsigned int    buttons = 0;
    UInt8           hat;

    if(TMData[kWCSButtonsByte] & kWCSRockerUPMask)
    {
        rockerPosition = 0;
    }
    else if(TMData[kWCSButtonsByte] & kWCSRockerDownMask)
    {
        rockerPosition = 2;
    }
    else
    {
        rockerPosition = 1;
    }

    // zero everything
    for(int i = 0; i < kReportSize; i++)
    {
        data[i] = 0;
    }

    // set up the buttons, reordering the bits so that they make more sense, trigger as button
    // six is just lame...
    
    // FCS Buttons
    buttons = buttons | ((bool)(TMData[kFCSButtonsByte] & kFCSTriggerMask)) << fButtonShifts[0 + rockerPosition];
    buttons = buttons | ((bool)(TMData[kFCSButtonsByte] & kFCSThumbHighMask)) << fButtonShifts[3 + rockerPosition];
    buttons = buttons | ((bool)(TMData[kFCSButtonsByte] & kFCSThumbLowMask)) << fButtonShifts[6 + rockerPosition];
    buttons = buttons | ((bool)(TMData[kFCSButtonsByte] & kFCSPinkyMask)) << fButtonShifts[9 + rockerPosition];
    
    // do the WCS
    if(fHasThrottle)
    {
        buttons = buttons |  ((bool)(TMData[kWCSButtonsByte] & 1)) << fButtonShifts[12 + rockerPosition];
        buttons = buttons |  ((bool)(TMData[kWCSButtonsByte] & 2)) << fButtonShifts[15 + rockerPosition];
        buttons = buttons |  ((bool)(TMData[kWCSButtonsByte] & 4)) << fButtonShifts[18 + rockerPosition];
        buttons = buttons |  ((bool)(TMData[kWCSButtonsByte] & 8)) << fButtonShifts[21 + rockerPosition];
        buttons = buttons |  ((bool)(TMData[kWCSButtonsByte] & 16)) << fButtonShifts[24 + rockerPosition];
        buttons = buttons |  ((bool)(TMData[kWCSButtonsByte] & 32)) << fButtonShifts[27 + rockerPosition];
    }
    
    // swap bytes around
    *((unsigned int*)data) = HostToUSBLong(buttons);

    // the hat switch is a pain
    // As near as I can tell 15 == null, 0 == up, 1 == up right, 2 == right, and so on...
    if(TMData[kFCSButtonsByte] & kFCSHatUpMask && TMData[kFCSButtonsByte] & kFCSHatRightMask)
    {
        hat = 2;
    }
    else if(TMData[kFCSButtonsByte] & kFCSHatRightMask && TMData[kFCSButtonsByte] & kFCSHatDownMask)
    {
        hat = 4;
    }
    else if(TMData[kFCSButtonsByte] & kFCSHatDownMask && TMData[kFCSButtonsByte] & kFCSHatleftMask)
    {
        hat = 6;
    }
    else if(TMData[kFCSButtonsByte] & kFCSHatleftMask && TMData[kFCSButtonsByte] & kFCSHatUpMask)
    {
        hat = 8;
    }
    else if(TMData[kFCSButtonsByte] & kFCSHatUpMask)
    {
        hat = 1;
    }
    else if(TMData[kFCSButtonsByte] & kFCSHatRightMask)
    {
        hat = 3;
    }
    else if(TMData[kFCSButtonsByte] & kFCSHatDownMask)
    {
        hat = 5;
    }
    else if(TMData[kFCSButtonsByte] & kFCSHatleftMask)
    {
        hat = 7;
    }
    else
    {
        hat = 0;	// should be null
    }

    // move the data around based on the rockers position
    if(fHatIsModified)
    {
        if(rockerPosition == 0)
        {
            data[kFCSHatReportByte] = 0xf0 | hat;
            data[kFCSHatReportByte + 1] = 0xff;
        }
        if(rockerPosition == 1)
        {
            data[kFCSHatReportByte] = (hat << 4) | 0x0f;
            data[kFCSHatReportByte + 1] = 0xff;
        }
        else if(rockerPosition == 2)
        {
            data[kFCSHatReportByte] = 0xff;
            data[kFCSHatReportByte + 1] = hat;
        }
    }
    else
    {
        data[kFCSHatReportByte] = hat;
    }

    if(!fRockerIsModifier)
    {
        // and the rocker swtich
        if(TMData[kWCSButtonsByte] & kWCSRockerUPMask)
        {
            hat = 1;
        }
        else if(TMData[kWCSButtonsByte] & kWCSRockerDownMask)
        {
            hat = 5;
        }
        else
        {
            hat = 0;
        }
        
        data[kWCSHatReportByte] = data[kWCSHatReportByte] | hat << 4;
    }

    // then do the axis
    // the x & y axis range from -128 to 127. I convert that to 0 - 255 because a few programs
    // don't seem to like negative values... and they would think the range is 0-127...
    // everyone seems happy with a range from 0-255, so thats what I report
    data[kXAxisReportByte] = (TMData[kXAxisByte] + 128);	// x axis
    data[kYAxisReportByte] = (TMData[kYAxisByte] + 128);	// y axis
    data[kThrottleReportByte] = (255 - TMData[kThrottleByte]);	// throttle (slider)
    data[kRuddersReportByte] = (TMData[kRuddersByte] + 128); 	// rudder (z)... I think
    
/*
    IOLog("%s: Input Data: %02x%02x %02x%02x %02x%02x %02x%02x\n", NAME, TMData[0], TMData[1], TMData[2], TMData[3], TMData[4], TMData[5], TMData[6], TMData[7]);
    IOLog("%s: Output Data: %02x%02x %02x%02x %02x%02x %02x%02x\n", NAME, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
*/

    // copy the data into the memory descriptor
    report->writeBytes(0, data, kReportSize);
    return kIOReturnSuccess;
}

void com_milvich_driver_Thrustmaster::packet(UInt8 *data, IOByteCount length)
{
    getReport(fReport, data, length);
    handleReport(fReport);
}

IOReturn com_milvich_driver_Thrustmaster::newReportDescriptor(IOMemoryDescriptor **descriptor) const
{
    UInt8	data[255];
    int		x = 0;
    void	*realData;

    IOLog("TM - Creating evil report descriptor\n");

    /*
     we need to build this very evil data structure that lets the hid device
     know what we look like (ie what buttons and what axis we have).

     Each entry in the data has an one byte op code which looks like this
     ----------------------------------------
     | Tag              |  Type  |   Size   |
     ----------------------------------------
      7    6    5    4    3    2    1    0

     The size is either 0, 1, 2 or 4 bytes.
     The type is either main, global, or local
     The tag specfies what in main global or local we are setting or executing.

     The main items are the ones that cause things to be executed, I am only
     going to use the inupt command, which takes data that has been set in local
     or global settings to create a field in the report. After the input
     command the local data is cleared, but the global data remains and will
     be used for the next input command unless it is changed.

     Local contains settings specific to a control, such as what it is. =)

     Global contains more general data, such as min and max values. This way
     if you create 10 different contols that have the same min and max values
     and you only have to set it once.

     following the op code is a data value, or paramater for the operation

     In the end this is what the report will look like
    ----------------------------------------------------
     0 | 1b2 | 0b2 | 2b1 | 1b1 | 0b1 | 2b0 | 1b0 | 0b0 |
     ----------------------------------------------------
     1 | 0b5 | 2b4 | 1b4 | 0b4 | 2b3 | 1b3 | 0b3 | 2b2 |
     ----------------------------------------------------
     2 | 2b7 | 1b7 | 0b7 | 2b6 | 1b6 | 0b6 | 2b5 | 1b5 |
     ----------------------------------------------------
     3 |  u  |  u  | 2b9 | 1b9 | 0b9 | 2b8 | 1b8 | 0b8 |
     ----------------------------------------------------
     4 |   FCS Hat Switch 1    |   FCS Hat Switch 0    |
     ----------------------------------------------------
     5 |     WCS Hat Switch    |   FCS Hat Switch 2    |
     ----------------------------------------------------
     6 |                      X Axis                   |
     ----------------------------------------------------
     7 |                      Y Axis                   |
     ----------------------------------------------------
     8 |                 Z (Rudder) Axis               |
     ----------------------------------------------------
     9 |             Slider (Throttle) Axis            |
     ----------------------------------------------------
     
     */
    
    
    // first we need to tell the hid thing what sort of device we are
    // in our case we are a joystick from the generic desktop catagory

    // set the catagory to the generic desktop
    // usage page (generic desktop)
    data[x++] = kHIDTagUsagePage | kHIDTypeGlobal | kOneByte;
    data[x++] = kHIDPage_GenericDesktop;
    // usaage (joystick)
    data[x++] = kHIDTagUsage | kHIDTypeLocal | kOneByte;
    data[x++] = kHIDUsage_GD_Joystick;

    // a collection is a grouping of inputs.

    // so start the joystick collection
    // collection (application)
    data[x++] = kHIDTagCollection | kHIDTypeMain | kOneByte;
    data[x++] = 0x01;	// application
    
    // setup buttons
    
    // switch to the button page
    data[x++] = kHIDTagUsagePage | kHIDTypeGlobal | kOneByte;
    data[x++] = kHIDPage_Button;
    // since it is a button our minimum value is 0
    data[x++] = kHIDTagLogicalMinimum | kHIDTypeGlobal | kOneByte;
    data[x++] = 0;
    data[x++] = kHIDTagPhysicalMinimum | kHIDTypeGlobal | kOneByte;
    data[x++] = 0;
    // button has a max value of 1
    data[x++] = kHIDTagLogicalMaximum | kHIDTypeGlobal | kOneByte;
    data[x++] = 1;
    data[x++] = kHIDTagPhysicalMaximum | kHIDTypeGlobal | kOneByte;
    data[x++] = 1;
    // we need one bit for each button
    data[x++] = kHIDTagReportSize | kHIDTypeGlobal | kOneByte;
    data[x++] = 1;
    // now tell it the minimum button 1 in our case
    data[x++] = kHIDTagUsageMinimum | kHIDTypeLocal | kOneByte;
    data[x++] = 1;
    // then tell it the maximum button number... which is what we counted when we started
    data[x++] = kHIDTagUsageMaximum | kHIDTypeLocal | kOneByte;
    data[x++] = fNumButtons;
    // we need how ever many fields for the buttons we have
    data[x++] = kHIDTagReportCount | kHIDTypeGlobal | kOneByte;
    data[x++] = fNumButtons;
    // now we need to create the input field...
    data[x++] = kHIDTagInput | kHIDTypeMain | kOneByte;
    data[x++] = 2;	// flag the data as being variable
    
    if(fNumButtons < 32)
    {
        // skip over however many buttons where left over, plus the extra
        // 2 buttons to round it to the nearest byte
        data[x++] = kHIDTagReportSize | kHIDTypeGlobal | kOneByte;
        data[x++] = 32 - fNumButtons;
        // and one count
        data[x++] = kHIDTagReportCount | kHIDTypeGlobal | kOneByte;
        data[x++] = 1;
        // create an empty input thing
        data[x++] = kHIDTagInput | kHIDTypeMain | kOneByte;
        data[x++] = 1;	// is constant
    }

    // setup the hat switch
    for(int i = 0; i < ((fHatIsModified) ? kNumModifiers : 1); i++)
    {
        // switch the generic desktop
        data[x++] = kHIDTagUsagePage | kHIDTypeGlobal | kOneByte;
        data[x++] = kHIDPage_GenericDesktop;
        // say that we are a hat switch
        data[x++] = kHIDTagUsage | kHIDTypeLocal | kOneByte;
        data[x++] = kHIDUsage_GD_Hatswitch;
        // both logical starts at 1, and physical starts at 0
        data[x++] = kHIDTagLogicalMinimum | kHIDTypeGlobal | kOneByte;
        data[x++] = 1;
        data[x++] = kHIDTagPhysicalMinimum | kHIDTypeGlobal | kOneByte;
        data[x++] = 0;
        // we report only 8 different values, but in reality, we move 315 degrees
        data[x++] = kHIDTagLogicalMaximum | kHIDTypeGlobal | kOneByte;
        data[x++] = 8;
        data[x++] = kHIDTagPhysicalMaximum | kHIDTypeGlobal | kTwoBytes;
        data[x++] = 0x3B;
        data[x++] = 0x01;
        // set the report size 4...
        data[x++] = kHIDTagReportSize | kHIDTypeGlobal | kOneByte;
        data[x++] = 4;
        // set the report count to 1
        data[x++] = kHIDTagReportCount | kHIDTypeGlobal | kOneByte;
        data[x++] = 1;
        // create the hat switch
        data[x++] = kHIDTagInput | kHIDTypeMain | kOneByte;
        data[x++] = 66;	// variable, and has a null state
    }

    if(!fHatIsModified)
    {
        // skip over the extra two entires for the hat switch
        
        // set to 8 bits
        data[x++] = kHIDTagReportSize | kHIDTypeGlobal | kOneByte;
        data[x++] = 8;
        // and one count
        data[x++] = kHIDTagReportCount | kHIDTypeGlobal | kOneByte;
        data[x++] = 1;
        // create an empty input thing
        data[x++] = kHIDTagInput | kHIDTypeMain | kOneByte;
        data[x++] = 1;	// is constant
    }


    if(!fRockerIsModifier && fHasThrottle)
    {
        // we also treat the rocker as a hat switch, if it isn't acting as a modifier
        // switch the generic desktop
        data[x++] = kHIDTagUsagePage | kHIDTypeGlobal | kOneByte;
        data[x++] = kHIDPage_GenericDesktop;
        // say that we are a hat switch
        data[x++] = kHIDTagUsage | kHIDTypeLocal | kOneByte;
        data[x++] = kHIDUsage_GD_Hatswitch;
        // both logical starts at 1, and physical starts at 0
        data[x++] = kHIDTagLogicalMinimum | kHIDTypeGlobal | kOneByte;
        data[x++] = 1;
        data[x++] = kHIDTagPhysicalMinimum | kHIDTypeGlobal | kOneByte;
        data[x++] = 0;
        // we report only 8 different values, but in reality, we do 2
        data[x++] = kHIDTagLogicalMaximum | kHIDTypeGlobal | kOneByte;
        data[x++] = 8;
        data[x++] = kHIDTagPhysicalMaximum | kHIDTypeGlobal | kTwoBytes;
        data[x++] = 0x3B;
        data[x++] = 0x01;
        // set the report size 8...
        data[x++] = kHIDTagReportSize | kHIDTypeGlobal | kOneByte;
        data[x++] = 4;
        // set the report count to 1
        data[x++] = kHIDTagReportCount | kHIDTypeGlobal | kOneByte;
        data[x++] = 1;
        // create the hat switch
        data[x++] = kHIDTagInput | kHIDTypeMain | kOneByte;
        data[x++] = 66;	// variable, and has a null state
    }
    else
    {
        // rocker was used as modifer or we have no throttle... don't let it show up
        
        // set 4 bits
        data[x++] = kHIDTagReportSize | kHIDTypeGlobal | kOneByte;
        data[x++] = 4;
        // and one count
        data[x++] = kHIDTagReportCount | kHIDTypeGlobal | kOneByte;
        data[x++] = 1;
        // create an empty input thing
        data[x++] = kHIDTagInput | kHIDTypeMain | kOneByte;
        data[x++] = 1;	// is constant
    }
    
    // do axis
    // set report size to 8
    data[x++] = kHIDTagReportSize | kHIDTypeGlobal | kOneByte;
    data[x++] = 8;
    // set min to 0
    data[x++] = kHIDTagLogicalMinimum | kHIDTypeGlobal | kOneByte;
    data[x++] = 0;
    data[x++] = kHIDTagPhysicalMinimum | kHIDTypeGlobal | kOneByte;
    data[x++] = 0;
    // button has a max value of 255
    data[x++] = kHIDTagLogicalMaximum | kHIDTypeGlobal | kTwoBytes;
    data[x++] = 255;
    data[x++] = 0;
    data[x++] = kHIDTagPhysicalMaximum | kHIDTypeGlobal | kTwoBytes;
    data[x++] = 255;
    data[x++] = 0;

    // say that we are coming from the generic desktop catagory
    data[x++] = kHIDTagUsagePage | kHIDTypeGlobal | kOneByte;
    data[x++] = kHIDPage_GenericDesktop;
    // say that we are using x, and y
    data[x++] = kHIDTagUsage | kHIDTypeLocal | kOneByte;
    data[x++] = kHIDUsage_GD_X;
    data[x++] = kHIDTagUsage | kHIDTypeLocal | kOneByte;
    data[x++] = kHIDUsage_GD_Y;
    // we need two spots
    data[x++] = kHIDTagReportCount | kHIDTypeGlobal | kOneByte;
    data[x++] = 2;
    // create the input
    data[x++] = kHIDTagInput | kHIDTypeMain | kOneByte;
    data[x++] = 2;	// flag as being variable

    if(fHasRudders)
    {
        // we need the z axis
        data[x++] = kHIDTagUsage | kHIDTypeLocal | kOneByte;
        
        if(fTwistRudder)
        {
            data[x++] = kHIDUsage_GD_Rz;
        }
        else
        {
            data[x++] = kHIDUsage_GD_Z;
        }
        // we need one spots
        data[x++] = kHIDTagReportCount | kHIDTypeGlobal | kOneByte;
        data[x++] = 1;
        // create the input
        data[x++] = kHIDTagInput | kHIDTypeMain | kOneByte;
        data[x++] = 2;	// flag as being variable
    }
    else
    {
        // even though we didn't have one, I still want to leave space to make it easier
        // when actually creating the report
        // and one count
        data[x++] = kHIDTagReportCount | kHIDTypeGlobal | kOneByte;
        data[x++] = 1;
        // create an empty input thing
        data[x++] = kHIDTagInput | kHIDTypeMain | kOneByte;
        data[x++] = 1;	// is constant
    }

    if(fHasThrottle)
    {
        // we need the slider
        data[x++] = kHIDTagUsage | kHIDTypeLocal | kOneByte;
        data[x++] = kHIDUsage_GD_Slider;
        // we need two spots
        data[x++] = kHIDTagReportCount | kHIDTypeGlobal | kOneByte;
        data[x++] = 1;
        // create the input
        data[x++] = kHIDTagInput | kHIDTypeMain | kOneByte;
        data[x++] = 2;	// flag as being variable
    }
    else
    {
        // even though we didn't have one, I still want to leave space to make it easier
        // when actually creating the report
        // and one count
        data[x++] = kHIDTagReportCount | kHIDTypeGlobal | kOneByte;
        data[x++] = 1;
        // create an empty input thing
        data[x++] = kHIDTagInput | kHIDTypeMain | kOneByte;
        data[x++] = 1;	// is constant
    }

    // and end the joystick collection
    data[x++] = kHIDTagEndCollection | kHIDTypeMain | kZeroBytes;

    // now lets create the memory for this descriptor
    *descriptor = IOBufferMemoryDescriptor::withCapacity(x, kIODirectionOutIn, true);

    // make sure we did get memory
    if(*descriptor == NULL)
    {
        return kIOReturnNoMemory;
    }

    // now lets grab the data in that memory buffer
    realData = ((IOBufferMemoryDescriptor*)(*descriptor))->getBytesNoCopy();

    // and copy our thing into it's buffer
    bcopy(data, realData, x);
    
    IOLog("TM - Done creating evil report descriptor\n");
    
    return kIOReturnSuccess;
}

//==============================================================================
// USB Stuff (Mainly...)
//==============================================================================
bool com_milvich_driver_Thrustmaster::init(OSDictionary *properties)
{
    IOLog("%s: init\n", NAME);
    
    if(!super::init(properties))
    {
        return false;
    }
    
    fIface = NULL;
    fPipe = NULL;
    fBuffer = NULL;
    fOutstandingIOOps = 0;
    fNeedToClose = false;
    fFinishedInit = false;
    
    for(unsigned int i = 0; i < sizeof(fControlData); i++)
    {
        fControlData[i] = 0;
    }
    
    OSBoolean		*result;
    int                 count;
    
    // create the buffer for the reports
    fReport = IOBufferMemoryDescriptor::withCapacity(kReportSize, kIODirectionOutIn, true);
    if(!fReport)
    {
        IOLog("%s: Failed to create the MemoryDescriptor for our report\n", NAME);
        return false;
    }
    
    // I need to know this info to create the device descriptor, but I can't
    // dynamicly look this up until I finish initing the iMate, but that blocks...
    // So we be stupid and just read from a config file.
    fHasRudders = false;
    fHasThrottle = true;
    result = OSDynamicCast(OSBoolean, getProperty("HasRudder"));
    if(result)
    {
        fHasRudders = result->getValue();
    }
    result = OSDynamicCast(OSBoolean, getProperty("HasThrottle"));
    if(result)
    {
        fHasThrottle = result->getValue();
    }
    
    // check to see if the rocker switch should act as a modifier
    if(fHasThrottle)
    {
        result = OSDynamicCast(OSBoolean, getProperty("RockerIsModifier"));
        if(!result)
        {
            IOLog("%s: Failed to find an entry for RockerIsModifier, assuming false.\n", NAME);
            fRockerIsModifier = false;
        }
        else
        {
            fRockerIsModifier = result->getValue();
        }
    }
    else
    {
        fRockerIsModifier = false;
    }
    
    // setup buttons
    if(fHasThrottle)
    {
        count = kNumOfButtons;
    }
    else
    {
        count = kNumOfFCSButtons;
    }
    OSArray *buttonArray = OSDynamicCast(OSArray, getProperty("Buttons"));
    if(fRockerIsModifier && buttonArray)
    {
        for(int i = 0; i < count; i++)
        {
            result = OSDynamicCast(OSBoolean, buttonArray->getObject(i));
            if(result && result->getValue())
            {
                for(int j = 0; j < kNumModifiers; j++)
                {
                    fButtonShifts[i * kNumModifiers + j] = fNumButtons;
                    fNumButtons++;
                }
            }
            else
            {
                for(int j = 0; j < kNumModifiers; j++)
                {
                    fButtonShifts[i * kNumModifiers + j] = fNumButtons;
                }
                fNumButtons++;
            }
        }
    }
    else
    {
        // the buttons and hatswitchs are not shifted, so set all 3 shift values
        // to the same thing
        fNumButtons = 0;
        for(int i = 0; i < count; i++)
        {
            for(int j = 0; j < kNumModifiers; j++)
            {
                fButtonShifts[i * kNumModifiers + j] = fNumButtons;
            }
            fNumButtons++;
        }
        // same thing with the hat switch
    }
    
    result = OSDynamicCast(OSBoolean, getProperty("ModifierEffectsHat"));
    fHatIsModified = result && result->getValue();
    if(fHatIsModified && fRockerIsModifier)
    {
        fHatSwitchShifts[0] = 0;
        fHatSwitchShifts[1] = 1;
        fHatSwitchShifts[2] = 2;
    }
    else
    {
        fHatSwitchShifts[0] = fHatSwitchShifts[1] = fHatSwitchShifts[2] = 0;
        fHatIsModified = false;
    }
    
    result = OSDynamicCast(OSBoolean, getProperty("TwistRudder"));
    fTwistRudder = result && result->getValue();
    
    return true;
}

IOService* com_milvich_driver_Thrustmaster::probe(IOService *provider, SInt32 *score)
{
    if(OSDynamicCast(IOUSBInterface, provider) == NULL)
    {
        IOLog("%s: Somehow we got matched to something that isn't a USB interface...\n", NAME);
        return NULL;
    }
    IOLog("%s: probe!\n", NAME);
    *score = 100000;
    return this;
}

bool com_milvich_driver_Thrustmaster::handleStart( IOService * provider )
{
    IOLog("%s: handleStart\n", NAME);
    
    // let the super do its thing
    if(!super::handleStart(provider))
    {
        IOLog("%s: super failed to start\n", NAME);
        return false;
    }
    
    // get the interface
    fIface = OSDynamicCast(IOUSBInterface, provider);
    if(!fIface)
    {
        IOLog("%s: The provider is not an IOUSBInterface..\n", NAME);
        return false;
    }
    
    // open the interface
    if(!fIface->open(this))
    {
        IOLog("%s: Failed to open the provider\n", NAME);
        fIface = NULL;
        return false;
    }
    
    // find the pipe to talk to the iMate over
    IOUSBFindEndpointRequest request;
    request.type = kUSBInterrupt;
    request.direction = kUSBIn;
    request.maxPacketSize = 0;
    request.interval = 0;
    
    fPipe = fIface->FindNextPipe(NULL, &request);
    if(fPipe)
    {
        //IOLog("%s: Found a pipe! type = %d, direction = %d, maxPacketSize = %04x, interval = %d\n", NAME, request.type, request.direction, request.maxPacketSize, request.interval);
    }
    else
    {
        IOLog("%s: Couldn't find a pipe\n", NAME);
        fIface->close(this);
        return false;
    }
    
    // lets retain things
    fIface->retain();
    fPipe->retain();
    
    // we want to setup a command gate to sync some actions
    fGate = IOCommandGate::commandGate(this);
    if(getWorkLoop()->addEventSource(fGate) != kIOReturnSuccess)
    {
        IOLog("%s: Failed to add the gate to the work loop\n", NAME);
        fIface->close(this);
        return false;
    }
    
    // kick off the read chain
    if(startReadLoop() != kIOReturnSuccess)
    {
        fIface->close(this);
        return false;
    }
    
    // kick off the init sequence
    IOCreateThread(initThread, this);
    
    return true;
}

void com_milvich_driver_Thrustmaster::handleInit()
{
    IOReturn status;
    
    // loop through each init command and issue them. Stop if the need to close
    // flag is set. (Due to threads, it might not be updated in time... hopefully
    // DeviceRequest is smart enough to detect a problem and stop us.)
    for(int i = 0; i < kNumInitCmds  && !fNeedToClose; i++)
    {
        IOSleep(50);
        status = fIface->DeviceRequest(&gInitSequence[i], NULL);
        if(status != kIOReturnSuccess)
        {
            IOLog("%s: Init sequence failed on command %d. Value = %d, Error = %08x\n", NAME, i, gInitSequence[i].wValue, status);
            break;
        }
    }
    
    //IOLog("%s: Finished init\n", NAME);
    
    // this is so we can hopefully safely close the provider if there was an error...
    fGate->runAction(initFinished);
}

void com_milvich_driver_Thrustmaster::initThread(void *arg)
{
    com_milvich_driver_Thrustmaster *dump = OSDynamicCast(com_milvich_driver_Thrustmaster, (OSObject*)arg);
    
    if(dump)
    {
        dump->handleInit();
    }
}

void com_milvich_driver_Thrustmaster::handleInitFinshed()
{
    // see if we need to close. This can happen if the user disconnected the iMate
    // before we finished the init sequence.
    if(fNeedToClose)
    {
        fIface->close(this);
    }
    else
    {
        fFinishedInit = true;
    }
}

IOReturn com_milvich_driver_Thrustmaster::initFinished(OSObject *obj, void *arg0, void *arg1, void *arg2, void *arg3)
{
    com_milvich_driver_Thrustmaster  *dump = OSDynamicCast(com_milvich_driver_Thrustmaster, obj);
    
    if(obj)
    {
        dump->handleInitFinshed();
    }
    return kIOReturnSuccess;
}


IOReturn com_milvich_driver_Thrustmaster::startReadLoop()
{
    IOReturn err;
    
    // setup the completion
    fReadCompletion.target = this;
    fReadCompletion.action = readCallback;
    fReadCompletion.parameter = NULL;
    
    // we need a buffer
    if(!fBuffer)
    {
        fBuffer = IOBufferMemoryDescriptor::withCapacity(8, kIODirectionIn);
        if(!fBuffer)
        {
            IOLog("%s: Failed to create the buffer\n", NAME);
            return kIOReturnNoMemory;
        }
    }
    
    // now lets kick off the chain of reads
    incrementOutstandingIO();
    err = fPipe->Read(fBuffer, &fReadCompletion);
    if(err != kIOReturnSuccess)
    {
        IOLog("%s: Failed to issue the first read request. Error = %08x\n", NAME, err);
        decrementOutstandingIO();
    }
    
    return err;
}

void com_milvich_driver_Thrustmaster::handleRead(IOReturn status)
{
    bool readAgain = false;
    
    switch(status)
    {
        case kIOReturnSuccess:
        {
            int index = 0;
            unsigned char *data = (unsigned char*)fBuffer->getBytesNoCopy();
            
            // the full 8 bytes of data is returned in two 4 byte chucks, pick
            // the right offset into the 8 byte control data
            if(data[2] == 0x98)
            {
                index = 4;
            }
            
            // check to see if there was a change
            bool changed = false;
            for(int i = 0; i < 4; i++)
            {
                if(fControlData[index + i] != data[4 + i])
                {
                    fControlData[index + i] = data[4 + i];
                    changed = true;
                }
            }
            
            if(changed)
            {
                packet(fControlData, sizeof(fControlData));
            }
            
            readAgain = true;
            break;
        }
        default:
            // assume some problem and stop reading
            IOLog("%s: handleRead - status = %08x\n", NAME, status);
            readAgain = false;
    }
    
    // reschedule the read if we are still reading...
    if(!fNeedToClose && readAgain)
    {
        incrementOutstandingIO();
        if(fPipe->Read(fBuffer, &fReadCompletion) != kIOReturnSuccess)
        {
            IOLog("%s: Failed to reschedule a read operation\n", NAME);
            decrementOutstandingIO();
        }
    }
    
    // update our IO op count
    decrementOutstandingIO();
}

void com_milvich_driver_Thrustmaster::readCallback(void *target, void *parameter, IOReturn status, UInt32 bufferSizeRemaining)
{
    com_milvich_driver_Thrustmaster *dump = OSDynamicCast(com_milvich_driver_Thrustmaster, (OSObject*)target);
    
    if(dump)
    {
        dump->handleRead(status);
    }
}

void com_milvich_driver_Thrustmaster::incrementOutstandingIO()
{
    fOutstandingIOOps++;
}

void com_milvich_driver_Thrustmaster::decrementOutstandingIO()
{
    fOutstandingIOOps--;
    
    // if we don't have any outstaind IO requests, and if we need to close, then
    // close. Except if we are still waiting for the init thread to finish. In that
    // case the finished init handler will take care of closing the provider.
    if(fOutstandingIOOps == 0 && fNeedToClose && !fFinishedInit)
    {
        fIface->close(this);
    }
}

void com_milvich_driver_Thrustmaster::handleStop(IOService *provider)
{
    //IOLog("%s: handleStop\n", NAME);
    
    // clear out any memory that we allocated
    if(fBuffer != NULL)
    {
        fBuffer->release();
        fBuffer = NULL;
    }
    
    if(fReport != NULL)
    {
        fReport->release();
        fReport = NULL;
    }
    
    if(fPipe != NULL)
    {
        fPipe->release();
        fPipe = NULL;
    }
    
    if(fIface != NULL)
    {
        fIface->release();
        fIface = NULL;
    }
    
    // remove our gate from the work loop
    if(fGate)
    {
        getWorkLoop()->removeEventSource(fGate);
        fGate->release();
        fGate = NULL;
    }
    
    super::handleStop(provider);
}

bool com_milvich_driver_Thrustmaster::willTerminate(IOService *provider, IOOptionBits options)
{
    //IOLog("%s: willTerminate\n", NAME);
    
    // stop any IO operation that might be scheduled
    if(fPipe)
    {
        fPipe->Abort();
    }
    
    return super::willTerminate(provider, options);
}

bool com_milvich_driver_Thrustmaster::didTerminate(IOService *provider, IOOptionBits options, bool *defer)
{
    //IOLog("%s: didTerminate\n", NAME);
    
    fNeedToClose = true;
    
    // we are done, so close the reference to our provider... assuming we don't
    // have an IO operation currently going on... if we do then delay the close
    // until the IO operation completes.
    if(fIface && fOutstandingIOOps == 0 && fFinishedInit)
    {
        fIface->close(this);
    }
    
    return super::didTerminate(provider, options, defer);
}

