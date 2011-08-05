//
//  ThrustmasterPref.h
//  ThrustmasterPref
//
//  Created by Michael Milvich on Sat Jun 07 2003.
//  Copyright (c) 2003 __MyCompanyName__. All rights reserved.
//

#import <PreferencePanes/PreferencePanes.h>
#import <Security/Security.h>


@interface ThrustmasterPref : NSPreferencePane 
{
    IBOutlet NSButton		*oEnableThrottle;
    IBOutlet NSButton           *oEnableRudders;
    IBOutlet NSButton           *oTwistRudder;
    IBOutlet NSButton		*oRockerIsModifier;
    IBOutlet NSButton		*oReloadButton;
    IBOutlet NSButton           *oSaveSettings;
    IBOutlet NSTextField	*oInfoTextField;
    IBOutlet NSButton           *oHatSwitch;
    IBOutlet NSMatrix           *oButtonMatrix;

    NSMutableDictionary         *fInfoDict;
    NSMutableDictionary         *fInfoEntry;
    NSMutableArray              *fButtonArray;
    
    BOOL                        fModified;
    
    AuthorizationRef            fAuthorization;
}

- (void)mainViewDidLoad;

- (void)updateStatus;
- (void)updateButtons;

- (void)loadInfoDict;
- (void)saveInfoDict;

- (void)restartDriver;

- (BOOL)isThrustmasterInstalled;
- (BOOL)isThrustmasterLoaded;

- (void)setModified:(BOOL)modified;

- (IBAction)enableThrottleAction:(id)sender;
- (IBAction)enableRudderAction:(id)sender;
- (IBAction)twistRudderAction:(id)sender;
- (IBAction)rockerAction:(id)sender;
- (IBAction)restartAction:(id)sender;
- (IBAction)buttonAction:(id)sender;
- (IBAction)hatSwitchAction:(id)sender;
- (IBAction)saveAction:(id)sender;

- (void)restartSheetDidEnd:(NSWindow *)sheet returnCode:(int)returnCode contextInfo:(void *)contextInfo;

- (void)setupAuthorization;
- (void)cleanUpAuthorization;

- (void)readToEOF:(FILE*)file;

@end
