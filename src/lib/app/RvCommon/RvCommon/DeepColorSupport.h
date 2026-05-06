//
//  Copyright (c) 2026 Autodesk, Inc. All Rights Reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
#pragma once

#ifdef PLATFORM_DARWIN

/// Configure the native NSView backing layer to use a wide-color format
/// (kCAContentsFormatRGBA10XR) so that 10-bit pixel data written by the
/// OpenGL context reaches the display without being clipped to 8-bit by the
/// macOS compositor.
///
/// \param nativeViewId  WId of the QWindow (cast to unsigned long long);
///                      on macOS this is the NSView* of the backing view.
void configureViewForDeepColor(unsigned long long nativeViewId);

#endif // PLATFORM_DARWIN
