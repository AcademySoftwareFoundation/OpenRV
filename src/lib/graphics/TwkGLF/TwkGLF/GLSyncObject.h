///*****************************************************************************/
// Copyright (c) 2019 Autodesk, Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//*****************************************************************************/

#ifndef __TwkGLF__GLSyncObject__h__
#define __TwkGLF__GLSyncObject__h__

#include <TwkGLF/GL.h>

//==============================================================================
// CLASS GLSyncObject
//==============================================================================

class GLSyncObject
{
public:
   GLSyncObject();
   GLSyncObject( GLSyncObject&& rhs );
   GLSyncObject& operator=(GLSyncObject&&);
   ~GLSyncObject();

   GLSyncObject( const GLSyncObject& rhs ) = delete;
   GLSyncObject& operator=(const GLSyncObject&) = delete;
 
   // Set a fence within the GL command stream.
   void setFence();

   // Remove a fence within the GL command stream.
   void removeFence();

   // Wait until all commands before the fence in GL command stream
   // are executed.
   void waitFence() const;

   // Check if all commands before the fence in GL command stream
   // are executed.
   bool testFence() const;

   // If fence set, check if all commands before the fence in GL
   // command stream are executed.
   bool tryTestFence() const;

   // Set a fence within the GL command stream and wait until all
   // command before the fence are executed.
   void wait();

public:
   class Imp;

private:
   Imp * _imp;
};

#endif // __TwkGLF__GLSyncObject__h__
