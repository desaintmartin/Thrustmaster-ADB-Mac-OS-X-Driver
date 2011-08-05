//
//  ThrustmasterPref.m
//  ThrustmasterPref
//
//  Created by Michael Milvich on Sat Jun 07 2003.
//  Copyright (c) 2003 __MyCompanyName__. All rights reserved.
//

#import "ThrustmasterPref.h"
#include <stdio.h>

NSString	*kThrustmasterPath = @"/System/Library/Extensions/Thrustmaster.kext";
NSString	*kThrustmasterName = @"Thrustmaster";
NSString        *kThrustmanterPersonality = @"TM-IMateDriver";
NSString	*kThrustmasterInfoPath = @"/System/Library/Extensions/Thrustmaster.kext/Contents/Info.plist";
NSString        *kExtensionCachePath = @"/System/Library/Extensions.kextcache";
NSString        *kEnableThrottleKey = @"HasThrottle";
NSString        *kEnableRuddersKey = @"HasRudder";
NSString        *kRockerKey = @"RockerIsModifier";
NSString        *kTempPath = @"/tmp/newTMInfo.plist";
NSString        *kButtonsKey = @"Buttons";
NSString        *kHatKey = @"ModifierEffectsHat";
NSString        *kTwistKey = @"TwistRudder";

char* kKextload = "/sbin/kextload";
char* kKextunload = "/sbin/kextunload";
char* kCpPath = "/bin/cp";
char* kRmPath = "/bin/rm";

@implementation ThrustmasterPref


- (void)dealloc
{
    [fInfoDict release];
    [self cleanUpAuthorization];
    [super dealloc];
}

- (void) mainViewDidLoad
{
    fModified = NO;
    [self setupAuthorization];
    [self loadInfoDict];
    [self updateStatus];
    [self updateButtons];
}

- (void)updateStatus
{
    NSString	*installed, *loaded;

    installed = ([self isThrustmasterInstalled]) ? @"Yes" : @"No";
    loaded = ([self isThrustmasterLoaded]) ? @"Yes" : @"No";

    [oInfoTextField setStringValue:[NSString stringWithFormat:@"Installed: %@   Loaded: %@", installed, loaded]];
}

- (void)updateButtons
{
    BOOL    enabled = [self isThrustmasterInstalled];
    
    [oReloadButton setEnabled:enabled];
    [oEnableThrottle setEnabled:enabled];
    [oEnableRudders setEnabled:enabled];
    [oTwistRudder setEnabled:enabled];
    [oRockerIsModifier setEnabled:enabled];
    [oSaveSettings setEnabled:fModified && enabled];
    [oHatSwitch setEnabled:enabled];
    [oButtonMatrix setEnabled:enabled];
    
    [oEnableThrottle setState:[[fInfoEntry objectForKey:kEnableThrottleKey] boolValue]];
    [oEnableRudders setState:[[fInfoEntry objectForKey:kEnableRuddersKey] boolValue]];
    [oTwistRudder setState:[[fInfoEntry objectForKey:kTwistKey] boolValue]];
    [oRockerIsModifier setState:[[fInfoEntry objectForKey:kRockerKey] boolValue]];
    [oHatSwitch setState:[[fInfoEntry objectForKey:kHatKey] boolValue]];
    
    for(int i = 0; i < 10; i++)
    {
        [oButtonMatrix setState:[[fButtonArray objectAtIndex:i] boolValue] atRow:0 column:i];
    }
}

- (void)loadInfoDict
{
    [fInfoDict release];
    fInfoDict = [[NSMutableDictionary dictionaryWithContentsOfFile:kThrustmasterInfoPath] retain];
    fInfoEntry = [[fInfoDict objectForKey:@"IOKitPersonalities"] objectForKey:kThrustmanterPersonality];
    fButtonArray = [fInfoEntry objectForKey:kButtonsKey];
    
    if([fButtonArray count] != 10)
    {
        NSLog(@"The button array is not size 10, creating a blank version");
        fButtonArray = [NSMutableArray array];
        [fInfoEntry setObject:fButtonArray forKey:kButtonsKey];
        for(int i = 0; i < 10; i++)
        {
            [fButtonArray addObject:[NSNumber numberWithBool:NO]]; 
        }
    }
}

- (void)saveInfoDict
{
    AuthorizationItem   items[4];
    AuthorizationRights rights;
    AuthorizationFlags  flags;
    OSStatus            result;
    char                *args[3];
    FILE                *file;
        
    items[0].name = kAuthorizationRightExecute;
    items[0].value = kCpPath;
    items[0].valueLength = strlen(kCpPath);
    items[0].flags = 0;
    
    flags = kAuthorizationFlagInteractionAllowed | kAuthorizationFlagExtendRights;
    
    rights.count = 1;
    rights.items = items;
    
    // try to get the authorization for this mv call
    result = AuthorizationCopyRights(fAuthorization, &rights, kAuthorizationEmptyEnvironment, flags, nil);
    if(result != errAuthorizationSuccess)
    {
        NSLog(@"Failed to authenticate: %d", result);
        return;
    }
    
    // write to the temp folder
    [fInfoDict writeToFile:kTempPath atomically:NO];
        
    // then run the mv command
    args[0] = (char*)[kTempPath UTF8String];
    args[1] = (char*)[kThrustmasterInfoPath UTF8String];
    args[2] = nil;
    result = AuthorizationExecuteWithPrivileges(fAuthorization, kCpPath, kAuthorizationFlagDefaults, args, &file);
    if(result != errAuthorizationSuccess)
    {
        NSLog(@"Failed to execute the mv command: %d", result);
        NSBeginAlertSheet(@"Save Failed", @"OK", nil, nil, [oReloadButton window], nil, nil, nil, nil, @"Failed to save the new settings.");
        return;
    }
    [self readToEOF:file];
    fclose(file);
    
    [[NSFileManager defaultManager] removeFileAtPath:kTempPath handler:nil];
    [self setModified:NO];
}

- (void)restartDriver
{
    AuthorizationItem   items[4];
    AuthorizationRights rights;
    AuthorizationFlags  flags;
    OSStatus            result;
    char                *args[2];
    FILE                *file;
        
    items[0].name = kAuthorizationRightExecute;
    items[0].value = kKextload;
    items[0].valueLength = strlen(kKextload);
    items[0].flags = 0;
    
    items[1].name = kAuthorizationRightExecute;
    items[1].value = kKextunload;
    items[1].valueLength = strlen(kKextunload);
    items[1].flags = 0;
    
    items[2].name = kAuthorizationRightExecute;
    items[2].value = kRmPath;
    items[2].valueLength = strlen(kRmPath);
    items[2].flags = 0;
    
    flags = kAuthorizationFlagInteractionAllowed | kAuthorizationFlagExtendRights;
    
    rights.count = 3;
    rights.items = items;
    
    // get the privlages
    result = AuthorizationCopyRights(fAuthorization, &rights, kAuthorizationEmptyEnvironment, flags, nil);
    
    if(result != errAuthorizationSuccess)
    {
        NSLog(@"Failed to authenticate: %d", result);
        return;
    }
        
    // unload the thrustmaster
    args[0] = (char*)[kThrustmasterPath UTF8String];
    args[1] = nil;
    result = AuthorizationExecuteWithPrivileges(fAuthorization, kKextunload, kAuthorizationFlagDefaults, args, &file);
    if(result != errAuthorizationSuccess)
    {
        NSLog(@"Failed to restart the driver: %d", result);
        NSBeginAlertSheet(@"Restart Failed", @"OK", nil, nil, [oReloadButton window], nil, nil, nil, nil, @"Failed to unload the driver. Try restarting the computer instead.");
        return;
    }
    [self readToEOF:file];
    fclose(file);
        
    // remove the cache
    args[0] = (char*)[kExtensionCachePath UTF8String];
    args[1] = nil;
    result = AuthorizationExecuteWithPrivileges(fAuthorization, kRmPath, kAuthorizationFlagDefaults, args, &file);
    if(result != errAuthorizationSuccess)
    {
        NSLog(@"Failed to restart the driver: %d", result);
        NSBeginAlertSheet(@"Restart Failed", @"OK", nil, nil, [oReloadButton window], nil, nil, nil, nil, @"Failed to delete the extension cache. Try restarting the computer instead.");
        return;
    }
    [self readToEOF:file];
    fclose(file); 
        
    // load the driver
    args[0] = (char*)[kThrustmasterPath UTF8String];
    args[1] = nil;
    result = AuthorizationExecuteWithPrivileges(fAuthorization, kKextload, kAuthorizationFlagDefaults, args, &file);
    if(result != errAuthorizationSuccess)
    {
        NSLog(@"Failed to restart the driver: %d", result);
        NSBeginAlertSheet(@"Restart Failed", @"OK", nil, nil, [oReloadButton window], nil, nil, nil, nil, @"Failed to load the driver. Try restarting the computer instead.");
        return;
    }
    [self readToEOF:file];
    fclose(file);
}

- (BOOL)isThrustmasterInstalled
{
    return fInfoDict != nil;	// we can't read it if it doesn't exist
}

- (BOOL)isThrustmasterLoaded
{
    NSTask		*task = [[NSTask alloc] init];
    NSString		*string;
    NSPipe		*pipe = [NSPipe pipe];
    NSFileHandle	*output = [pipe fileHandleForReading];
    
    [task setLaunchPath:@"/usr/sbin/kextstat"];
    [task setStandardOutput:pipe];

    [task launch];
    
    string = [[[NSString alloc] initWithData:[output readDataToEndOfFile] encoding:NSUTF8StringEncoding] autorelease];
    
    [task waitUntilExit];
    [task release];
    
    return [string rangeOfString:kThrustmasterName].location != NSNotFound;
}


- (void)setModified:(BOOL)modified
{
    fModified = modified;
    [oSaveSettings setEnabled:modified];
}

- (IBAction)enableThrottleAction:(id)sender
{
    [fInfoEntry setObject:[NSNumber numberWithBool:[oEnableThrottle state] == NSOnState] forKey:kEnableThrottleKey];
    [self setModified:YES];
}

- (IBAction)enableRudderAction:(id)sender
{
    [fInfoEntry setObject:[NSNumber numberWithBool:[oEnableRudders state] == NSOnState] forKey:kEnableRuddersKey];
    [self setModified:YES];
}

- (IBAction)twistRudderAction:(id)sender
{
    [fInfoEntry setObject:[NSNumber numberWithBool:[oTwistRudder state] == NSOnState] forKey:kTwistKey];
    [self setModified:YES];
}

- (IBAction)rockerAction:(id)sender
{
    [fInfoEntry setObject:[NSNumber numberWithBool:[oRockerIsModifier state] == NSOnState] forKey:kRockerKey];
    [self setModified:YES];
}

- (IBAction)buttonAction:(id)sender
{
    NSButtonCell    *cell = [sender selectedCell];
    
    [fButtonArray replaceObjectAtIndex:[cell tag] withObject:[NSNumber numberWithBool:[cell state] == NSOnState]];
    
    [self setModified:YES];
}

- (IBAction)hatSwitchAction:(id)sender
{
    [fInfoEntry setObject:[NSNumber numberWithBool:[oHatSwitch state] == NSOnState] forKey:kHatKey];
    [self setModified:YES];
}

- (IBAction)restartAction:(id)sender
{
    // first save... if needed
    if(fModified)
    {
        [self saveAction:nil];
    }
    NSBeginAlertSheet(@"Are you sure you want to restart the driver?", @"OK", @"Cancel", nil, [oReloadButton window], self, @selector(restartSheetDidEnd:returnCode:contextInfo:), nil, nil, @"Please make sure to quit all applications that could be using your Thrustmaster stick. If you fail to do so, you will most likely crash your computer.");
}

- (IBAction)saveAction:(id)sender
{
    [self saveInfoDict];
}

- (void)restartSheetDidEnd:(NSWindow *)sheet returnCode:(int)returnCode contextInfo:(void *)contextInfo
{
    if(returnCode == NSAlertDefaultReturn)
    {
        [self restartDriver];
        [self updateStatus];
    }
}

- (void)setupAuthorization
{
    OSStatus    result;
    
    result = AuthorizationCreate(nil, kAuthorizationEmptyEnvironment, kAuthorizationFlagDefaults, &fAuthorization);
    if(result != errAuthorizationSuccess)
    {
        NSLog(@"Failed to create an authorization record: %d", result);
        fAuthorization = nil;
    }
}

- (void)cleanUpAuthorization
{
    OSStatus result;
    
    result = AuthorizationFree(fAuthorization, kAuthorizationFlagDestroyRights);
    
    if(result != errAuthorizationSuccess)
    {
        NSLog(@"Failed to create an authorization record: %d", result);
    }
}

- (void)readToEOF:(FILE*)file
{
    while(fgetc(file) != EOF);
}

@end
