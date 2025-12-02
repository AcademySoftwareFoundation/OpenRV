// Minimal RVLinkLauncher for rvlink protocol
// Copyright (c) 2025 Autodesk
// SPDX-License-Identifier: Apache-2.0


#import <Cocoa/Cocoa.h>
#import <CoreServices/CoreServices.h>

@interface RVLinkURLHandler : NSObject {
    BOOL urlProcessed;
    NSAlert *currentAlert;
    NSString *latestRVLinkURL;
}
- (void)handleGetURLEvent:(NSAppleEventDescriptor *)event withReplyEvent:(NSAppleEventDescriptor *)replyEvent;
- (void)processRVLinkURL:(NSString *)rvlinkURL;
- (BOOL)hasProcessedURL;
- (NSMutableArray<NSURL *> *)findRVAppsUsingMDFind;
- (NSMutableArray<NSURL *> *)findRVAppsUsingWorkspace:(NSString *)rvlinkURL;
@end

@implementation RVLinkURLHandler
- (instancetype)init {
    self = [super init];
    if (self) {
        urlProcessed = NO;
        currentAlert = nil;
        latestRVLinkURL = nil;
    }
    return self;
}

- (BOOL)hasProcessedURL {
    return urlProcessed;
}

- (void)dealloc {
    if (latestRVLinkURL != nil) {
        [latestRVLinkURL release];
        latestRVLinkURL = nil;
    }
    [super dealloc];
}

- (void)handleGetURLEvent:(NSAppleEventDescriptor *)event withReplyEvent:(NSAppleEventDescriptor *)replyEvent
{
    NSLog(@"*** RVLinkLauncher Apple Event handler called! ***");

    @try {
        NSAppleEventDescriptor *directObjectDescriptor = [event paramDescriptorForKeyword:keyDirectObject];
        if (directObjectDescriptor != nil) {
            NSString *url = [directObjectDescriptor stringValue];
            if (url != nil && [url hasPrefix:@"rvlink://"]) {
                NSLog(@"RVLinkLauncher received URL: %@", url);

                // Track the most recent rvlink URL
                if (latestRVLinkURL != nil) {
                    [latestRVLinkURL release];
                    latestRVLinkURL = nil;
                }
                latestRVLinkURL = [url copy];

                if (currentAlert != nil) {
                    // Update the existing dialog in place and bring it to the front
                    [currentAlert setInformativeText:[NSString stringWithFormat:@"Opening: %@", latestRVLinkURL]];
                    [[currentAlert window] makeKeyAndOrderFront:nil];
                } else {
                    // No dialog is currently shown – process normally
                    [self processRVLinkURL:latestRVLinkURL];
                }

                urlProcessed = YES;
            } else {
                NSLog(@"RVLinkLauncher received invalid or non-rvlink URL: %@", url);
            }
        } else {
            NSLog(@"RVLinkLauncher: No direct object found in Apple Event");
        }
    } @catch (NSException *exception) {
        NSLog(@"RVLinkLauncher Apple Event handling error: %@", exception);
    }
}

- (NSMutableArray<NSURL *> *)findRVAppsUsingMDFind
{
    NSMutableArray<NSURL *> *appURLs = [NSMutableArray array];
    @try {
        NSTask *task = [[NSTask alloc] init];
        [task setLaunchPath:@"/usr/bin/mdfind"];
        [task setArguments:@[ @"kMDItemCFBundleIdentifier == 'com.autodesk.RV'" ]];

        NSPipe *pipe = [NSPipe pipe];
        [task setStandardOutput:pipe];
        [task launch];
        [task waitUntilExit];

        NSData *data = [[pipe fileHandleForReading] readDataToEndOfFile];
        NSString *output = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
        NSArray<NSString *> *lines = [output componentsSeparatedByString:@"\n"];
        for (NSString *line in lines) {
            if ([line length] > 0) {
                NSURL *url = [NSURL fileURLWithPath:line];
                if (url != nil) {
                    NSString *displayName = [[NSFileManager defaultManager] displayNameAtPath:line];
                    NSBundle *bundle = [NSBundle bundleWithURL:url];
                    NSString *bundleID = [bundle bundleIdentifier];
                    
                    [appURLs addObject:url];
                    NSLog(@"  App: %@ | Bundle ID: %@ | Path: %@", displayName, bundleID, line);
                }
            }
        }
        NSLog(@"Found %lu RV apps using mdfind", (unsigned long)[appURLs count]);
    } @catch (NSException *e) {
        NSLog(@"mdfind failed: %@", e);
    }
    return appURLs;
}

- (NSMutableArray<NSURL *> *)findRVAppsUsingWorkspace:(NSString *)rvlinkURL
{
    NSURL *testURL = [NSURL URLWithString:rvlinkURL];
    NSArray<NSURL *> *registeredApps = [[NSWorkspace sharedWorkspace] URLsForApplicationsToOpenURL:testURL];
    NSMutableArray<NSURL *> *appURLs = [NSMutableArray array];
    
    // Filter out RVLinkLauncher to avoid selection loops
    for (NSURL *appURL in registeredApps) {
        NSString *displayName = [[NSFileManager defaultManager] displayNameAtPath:[appURL path]];
        NSBundle *bundle = [NSBundle bundleWithURL:appURL];
        NSString *bundleID = [bundle bundleIdentifier];
        
        // Skip RVLinkLauncher apps - we don't want users selecting the launcher itself
        if ([displayName containsString:@"RVLinkLauncher"] || 
            [bundleID isEqualToString:@"com.autodesk.RVLinkLauncher"]) {
            NSLog(@"  Skipping RVLinkLauncher: %@ | Bundle ID: %@", displayName, bundleID);
            continue;
        }
        
        [appURLs addObject:appURL];
        NSLog(@"  App: %@ | Bundle ID: %@ | Path: %@", displayName, bundleID, [appURL path]);
    }
    
    NSLog(@"Found %lu valid rvlink handlers using URLsForApplicationsToOpenURL (excluding RVLinkLauncher)", (unsigned long)[appURLs count]);
    return appURLs;
}

- (void)processRVLinkURL:(NSString *)rvlinkURL
{
    // Remember the URL we are currently processing so that if new URLs arrive
    // while a chooser dialog is visible, we can still open the most recent one.
    if (rvlinkURL != nil) {
        if (latestRVLinkURL != nil) {
            [latestRVLinkURL release];
            latestRVLinkURL = nil;
        }
        latestRVLinkURL = [rvlinkURL copy];
    }

    // Find all RV apps using both discovery methods
    NSLog(@"Discovering RV applications using multiple methods...");
    
    // Method 1: Find apps by bundle ID (com.autodesk.RV)
    NSMutableArray<NSURL *> *mdfindApps = [self findRVAppsUsingMDFind];
    
    // Method 2: Find apps registered for rvlink:// URLs
    NSMutableArray<NSURL *> *registeredApps = [self findRVAppsUsingWorkspace:rvlinkURL];
    
    // Combine results and remove duplicates
    NSMutableArray<NSURL *> *appURLs = [NSMutableArray array];
    NSMutableSet<NSString *> *seenPaths = [NSMutableSet set];
    
    // Add mdfind results
    for (NSURL *appURL in mdfindApps) {
        NSString *path = [appURL path];
        if (![seenPaths containsObject:path]) {
            [appURLs addObject:appURL];
            [seenPaths addObject:path];
        }
    }
    
    // Add workspace results (skip duplicates)
    for (NSURL *appURL in registeredApps) {
        NSString *path = [appURL path];
        if (![seenPaths containsObject:path]) {
            [appURLs addObject:appURL];
            [seenPaths addObject:path];
        }
    }
    
    NSLog(@"Total unique RV applications found: %lu", (unsigned long)[appURLs count]);

    // Check if we found any RV apps
    if ([appURLs count] == 0) {
        currentAlert = [[NSAlert alloc] init];
        [currentAlert setMessageText:@"No RV Applications Found"];
        [currentAlert setInformativeText:@"Could not find any installed RV applications to open the rvlink URL."];
        [currentAlert addButtonWithTitle:@"OK"];
        [currentAlert runModal];
        [currentAlert release];
        currentAlert = nil;
        return;
    }

    // If only one RV app found, launch it directly without showing chooser
    if ([appURLs count] == 1) {
        NSURL *selectedAppURL = [appURLs firstObject];
        NSString *displayName = [[NSFileManager defaultManager] displayNameAtPath:[selectedAppURL path]];
        NSLog(@"Only one RV application found, launching directly: %@", displayName);
        
        NSURL *targetURL = [NSURL URLWithString:latestRVLinkURL];
        if (targetURL != nil) {
            NSWorkspaceOpenConfiguration *config = [NSWorkspaceOpenConfiguration configuration];
            [[NSWorkspace sharedWorkspace] openURLs:@[targetURL]
                              withApplicationAtURL:selectedAppURL
                                    configuration:config
                               completionHandler:^(NSRunningApplication *app, NSError *err) {
                if (err != nil) {
                    NSLog(@"Failed to launch app: %@", [selectedAppURL path]);
                    NSLog(@"Error: %@", [err localizedDescription]);
                } else {
                    NSLog(@"Successfully opened URL: %@ with app: %@", latestRVLinkURL, [selectedAppURL path]);
                }
            }];
        } else {
            NSLog(@"Invalid rvlink URL: %@", latestRVLinkURL);
        }
        return;
    }

    // Present chooser UI for multiple apps
    currentAlert = [[NSAlert alloc] init];
    [currentAlert setMessageText:@"Choose RV Application"];
    [currentAlert setInformativeText:[NSString stringWithFormat:@"Opening: %@", latestRVLinkURL]];
    [currentAlert addButtonWithTitle:@"Open"];
    [currentAlert addButtonWithTitle:@"Cancel"];

    // Sort apps by path for consistent ordering
    NSArray *sortedAppURLs = [appURLs sortedArrayUsingComparator:^NSComparisonResult(NSURL *url1, NSURL *url2) {
        return [[url1 path] compare:[url2 path]];
    }];
    
    NSPopUpButton *popup = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(0,0,400,24)];
    for (NSURL *appURL in sortedAppURLs) {
        NSString *displayName = [[NSFileManager defaultManager] displayNameAtPath:[appURL path]];
        NSString *appPath = [appURL path];
        
        // Create a more readable title that shows distinguishing path info
        NSString *itemTitle;
        if ([appPath hasPrefix:@"/Applications/"]) {
            // For apps in Applications folder, show "RV.app - /Applications/RV.app"
            itemTitle = [NSString stringWithFormat:@"%@ - %@", displayName, appPath];
        } else if ([appPath hasPrefix:@"/Users/"]) {
            // For user-specific installations, show "RV.app - ~/path/to/RV.app"
            NSString *homeDir = NSHomeDirectory();
            if ([appPath hasPrefix:homeDir]) {
                NSString *relativePath = [appPath substringFromIndex:[homeDir length]];
                itemTitle = [NSString stringWithFormat:@"%@ - ~%@", displayName, relativePath];
            } else {
                itemTitle = [NSString stringWithFormat:@"%@ - %@", displayName, appPath];
            }
        } else {
            // For other locations, show full path
            itemTitle = [NSString stringWithFormat:@"%@ - %@", displayName, appPath];
        }
        
        [popup addItemWithTitle:itemTitle];
        [[popup lastItem] setRepresentedObject:appURL];
    }
    [currentAlert setAccessoryView:popup];

    NSInteger result = [currentAlert runModal];
    NSAlert *alert = currentAlert;
    currentAlert = nil;
    
    if (result == NSAlertFirstButtonReturn) {
        NSURL *selectedAppURL = [[popup selectedItem] representedObject];
        NSURL *targetURL = [NSURL URLWithString:latestRVLinkURL];
        if (targetURL != nil) {
            NSWorkspaceOpenConfiguration *config = [NSWorkspaceOpenConfiguration configuration];
            [[NSWorkspace sharedWorkspace] openURLs:@[targetURL]
                              withApplicationAtURL:selectedAppURL
                                    configuration:config
                               completionHandler:^(NSRunningApplication *app, NSError *err) {
                if (err != nil) {
                    NSLog(@"Failed to launch app: %@", [selectedAppURL path]);
                    NSLog(@"Error: %@", [err localizedDescription]);
                } else {
                    NSLog(@"Successfully opened URL: %@ with app: %@", rvlinkURL, [selectedAppURL path]);
                }
            }];
        } else {
            NSLog(@"Invalid rvlink URL: %@", rvlinkURL);
        }
    }
    [popup release];
    [alert release];
}
@end

int main(int argc, const char * argv[])
{
    @autoreleasepool {
        // Register Apple Event handler for URL events
        RVLinkURLHandler *urlHandler = [[RVLinkURLHandler alloc] init];
        [[NSAppleEventManager sharedAppleEventManager] 
            setEventHandler:urlHandler 
            andSelector:@selector(handleGetURLEvent:withReplyEvent:)
            forEventClass:kInternetEventClass 
            andEventID:kAEGetURL];

        // Register as default handler for rvlink:// URLs
        NSString *bundleID = [[NSBundle mainBundle] bundleIdentifier];
        CFStringRef scheme = CFSTR("rvlink");
        CFStringRef handler = (__bridge CFStringRef)bundleID;
        LSSetDefaultHandlerForURLScheme(scheme, handler);

        // Warn if not the default handler
        CFStringRef currentHandler = LSCopyDefaultHandlerForURLScheme(scheme);
        if (currentHandler != NULL && !CFEqual(currentHandler, handler)) {
            NSAlert *warnAlert = [[NSAlert alloc] init];
            [warnAlert setMessageText:@"RVLinkLauncher is not the default handler for rvlink URLs."];
            [warnAlert setInformativeText:@"Please set RVLinkLauncher as the default handler to ensure correct behavior."];
            [warnAlert addButtonWithTitle:@"OK"];
            [warnAlert runModal];
            [warnAlert release];
        }
        if (currentHandler != NULL) {
            CFRelease(currentHandler);
        }

        // Check for command line URL
        NSString *rvlinkURL = nil;
        for (int i = 1; i < argc; i++) {
            NSString *arg = [NSString stringWithUTF8String:argv[i]];
            if ([arg hasPrefix:@"rvlink://"]) {
                rvlinkURL = arg;
                break;
            }
        }
        
        // If a URL was provided via command line, forward it to LaunchServices and exit.
        // This avoids creating a second launcher instance with its own UI when the
        // binary is invoked directly from the terminal.
        if (rvlinkURL != nil) {
            NSLog(@"Forwarding rvlink URL from command line to LaunchServices: %@", rvlinkURL);
            NSURL *url = [NSURL URLWithString:rvlinkURL];
            if (url != nil) {
                [[NSWorkspace sharedWorkspace] openURL:url];
            } else {
                NSLog(@"Invalid rvlink URL from command line: %@", rvlinkURL);
            }
            return 0;
        }
        
        // No immediate URL - register handler and wait briefly for Apple Events
        NSLog(@"RVLinkLauncher registered and ready to handle rvlink:// URLs");
        
        // Run the event loop briefly to allow Apple Events to be received
        // This is needed when the app is launched by clicking a rvlink:// URL
        NSDate *timeout = [NSDate dateWithTimeIntervalSinceNow:2.0];
        while ([timeout timeIntervalSinceNow] > 0 && ![urlHandler hasProcessedURL]) {
            NSEvent *event = [[NSApplication sharedApplication] 
                nextEventMatchingMask:NSEventMaskAny 
                untilDate:[NSDate dateWithTimeIntervalSinceNow:0.1]
                inMode:NSDefaultRunLoopMode 
                dequeue:YES];
            if (event) {
                [[NSApplication sharedApplication] sendEvent:event];
            }
            
            // Process any pending Apple Events
            [[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.1]];
        }
        
        if (![urlHandler hasProcessedURL]) {
            // No URL was processed - this was a manual launch, show registration dialog
            NSLog(@"Manual launch detected - showing registration complete dialog");
            
            NSAlert *registrationAlert = [[NSAlert alloc] init];
            [registrationAlert setMessageText:@"RVLinkLauncher Registration Complete"];
            [registrationAlert setInformativeText:@"RVLinkLauncher is now registered to handle rvlink:// URLs. Future rvlink:// links will automatically open with your choice of RV application."];
            [registrationAlert addButtonWithTitle:@"OK"];
            [registrationAlert runModal];
            [registrationAlert release];
        } else {
            // URL was processed - this was an automatic launch via rvlink://, no dialog needed
            NSLog(@"rvlink:// URL processed successfully - exiting silently");
        }
    }
    return 0;
}
