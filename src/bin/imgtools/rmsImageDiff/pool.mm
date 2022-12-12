//******************************************************************************
// Copyright (c) 2005 Tweak Inc. 
// All rights reserved.
// 
// SPDX-License-Identifier: Apache-2.0
// 
//******************************************************************************

#include "pool.h"
#import <Foundation/Foundation.h>

static NSAutoreleasePool* pool = 0;

void initPool()
{
    pool = [[NSAutoreleasePool alloc] init];
}

void releasePool()
{
    [pool release];
}
