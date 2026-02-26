//
//  Copyright (c) 2024 Autodesk, Inc. All Rights Reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
#include <RvCommon/DeepColorSupport.h>

#ifdef PLATFORM_DARWIN

#import <AppKit/AppKit.h>
#import <QuartzCore/QuartzCore.h>

#define DC_LOG(fmt, ...) \
    do { fprintf(stderr, "[DeepColor] " fmt "\n", ##__VA_ARGS__); } while(0)

void configureViewForDeepColor(unsigned long long nativeViewId)
{
    DC_LOG("configureViewForDeepColor called, nativeViewId=0x%llx", nativeViewId);

    NSView* view = (__bridge NSView*)(void*)nativeViewId;
    if (!view)
    {
        DC_LOG("nativeViewId is nil — winId() not yet valid, skipping");
        return;
    }

    // Ensure the view has a backing layer
    [view setWantsLayer:YES];
    CALayer* layer = view.layer;

    if (!layer)
    {
        DC_LOG("view has no backing layer after setWantsLayer:YES");
        return;
    }

    DC_LOG("backing layer class: %s", [NSStringFromClass([layer class]) UTF8String]);
    DC_LOG("responds to wantsExtendedDynamicRangeContent: %s",
           [layer respondsToSelector:@selector(wantsExtendedDynamicRangeContent)] ? "YES" : "NO");

    // Attempt 1: wantsExtendedDynamicRangeContent via KVC.
    // This property exists on CAOpenGLLayer and CAMetalLayer but not on plain CALayer,
    // so guard with respondsToSelector: for safe degradation.
    if ([layer respondsToSelector:@selector(wantsExtendedDynamicRangeContent)])
    {
        [layer setValue:@YES forKey:@"wantsExtendedDynamicRangeContent"];
        DC_LOG("set wantsExtendedDynamicRangeContent = YES via KVC");
    }

    // Attempt 2: kCAContentsFormatRGBA16Float — a valid macOS constant (unlike the
    // iOS-only kCAContentsFormatRGBA10XR) that tells Core Animation to keep the
    // layer contents in a 16-bit float format instead of the default RGBA8.
    DC_LOG("responds to setContentsFormat: %s",
           [layer respondsToSelector:@selector(setContentsFormat:)] ? "YES" : "NO");
    if ([layer respondsToSelector:@selector(setContentsFormat:)])
    {
        layer.contentsFormat = kCAContentsFormatRGBA16Float;
        DC_LOG("set contentsFormat = kCAContentsFormatRGBA16Float");
    }

    // Note: colorspace is a CAMetalLayer property, not available on
    // _NSOpenGLViewBackingLayer (CAOpenGLLayer), so we do not attempt to set it.
}

#endif // PLATFORM_DARWIN
