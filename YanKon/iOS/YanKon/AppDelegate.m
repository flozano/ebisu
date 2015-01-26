//
//  AppDelegate.m
//  YanKon
//
//  Created by Evan JIANG on 14/11/19.
//  Copyright (c) 2014年 Kii Inc. All rights reserved.
//

#import "AppDelegate.h"
#import "CommandDaemon.h"
#import <KiiSDK/Kii.h>

@interface AppDelegate ()

@end

@implementation AppDelegate

- (BOOL)application:(UIApplication*)application didFinishLaunchingWithOptions:(NSDictionary*)launchOptions
{
    // Override point for customization after application launch.
    [Kii beginWithID:@"06e806e2" andKey:@"31afdcdfd72ade025559176a40a20875" andSite:kiiSiteJP];
    [[CommandDaemon getInstance] willEnterForeground];
    return YES;
}

- (void)applicationWillResignActive:(UIApplication*)application
{
    // Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
    // Use this method to pause ongoing tasks, disable timers, and throttle down OpenGL ES frame rates. Games should use this method to pause the game.
}

- (void)applicationDidEnterBackground:(UIApplication*)application
{
    // Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later.
    // If your application supports background execution, this method is called instead of applicationWillTerminate: when the user quits.
    [[CommandDaemon getInstance] didEnterBackground];
}

- (void)applicationWillEnterForeground:(UIApplication*)application
{
    [[CommandDaemon getInstance] willEnterForeground];
}

- (void)applicationDidBecomeActive:(UIApplication*)application
{
    // Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
}

- (void)applicationWillTerminate:(UIApplication*)application
{
    // Called when the application is about to terminate. Save data if appropriate. See also applicationDidEnterBackground:.
}

@end
