#ifndef __CONSTANTS__
#define __CONSTANTS__


// some constants for building the report descriptor
// I stole these from the HIDPriv.h from the darwin source code
enum {
    kZeroBytes					= 0,
    kOneByte					= 1,
    kTwoBytes					= 2,
    kFourBytes					= 3
};

// the item types (catagories)
enum {
    kHIDTypeMain				= 0 << 2,
    kHIDTypeGlobal				= 1 << 2,
    kHIDTypeLocal				= 2 << 2,
    kHIDTypeLong				= 3 << 2
};

// main tags
enum {
    kHIDTagInput				= 8 << 4,
    kHIDTagOutput				= 9 << 4,
    kHIDTagCollection			= 0x0A << 4,
    kHIDTagFeature				= 0x0B << 4,
    kHIDTagEndCollection		= 0x0C << 4
};

// global tags
enum {
    kHIDTagUsagePage			= 0 << 4,
    kHIDTagLogicalMinimum		= 1 << 4,
    kHIDTagLogicalMaximum		= 2 << 4,
    kHIDTagPhysicalMinimum		= 3 << 4,
    kHIDTagPhysicalMaximum		= 4 << 4,
    kHIDTagUnitExponent			= 5 << 4,
    kHIDTagUnit					= 6 << 4,
    kHIDTagReportSize			= 7 << 4,
    kHIDTagReportID				= 8 << 4,
    kHIDTagReportCount			= 9 << 4,
    kHIDTagPush					= 0x0A << 4,
    kHIDTagPop					= 0x0B << 4
};

// local tags
enum {
    kHIDTagUsage				= 0 << 4,
    kHIDTagUsageMinimum			= 1 << 4,
    kHIDTagUsageMaximum			= 2 << 4,
    kHIDTagDesignatorIndex		= 3 << 4,
    kHIDTagDesignatorMinimum	= 4 << 4,
    kHIDTagDesignatorMaximum	= 5 << 4,
    kHIDTagStringIndex			= 7 << 4,
    kHIDTagStringMinimum		= 8 << 4,
    kHIDTagStringMaximum		= 9 << 4,
    kHIDTagSetDelimiter			= 0x0A << 4
};

// these are the bit patterns of each button

const int   kNumOfFCSButtons = 4;
const int   kNumOfWCSButtons = 6;
const int   kNumOfButtons = 10;
const int   kNumModifiers = 3;

// FCS Buttons
enum {
    kFCSHatUpOffset			= 0,
    kFCSHatDownOffset			= 1,
    kFCSHatRightOffset			= 2,
    kFCSHatleftOffset			= 3,
    kFCSThumbHighOffset			= 4,
    kFCSTriggerOffset			= 5,
    kFCSThumbLowOffset			= 6,
    kFCSPinkyOffset			= 7
};

enum {
    kFCSHatUpMask			= 1,
    kFCSHatDownMask			= 1 << 1,
    kFCSHatRightMask			= 1 << 2,
    kFCSHatleftMask			= 1 << 3,
    kFCSThumbHighMask			= 1 << 4,
    kFCSTriggerMask			= 1 << 5,
    kFCSThumbLowMask			= 1 << 6,
    kFCSPinkyMask			= 1 << 7
};

enum {
    kWCSRockerUPMask			= 1 << 6,
    kWCSRockerDownMask			= 1 << 7
};


// what byte the controls are returned in
enum {
    kXAxisByte				= 0,
    kYAxisByte				= 1,
    kThrottleByte			= 2,
    kRuddersByte			= 3,
    kWCSButtonsByte			= 4,
    kFCSButtonsByte			= 5
};

enum {
    kFCSHatReportByte			= 4,
    kWCSHatReportByte			= 5,
    kXAxisReportByte			= 6,
    kYAxisReportByte			= 7,
    kRuddersReportByte			= 8,
    kThrottleReportByte			= 9
};

#endif