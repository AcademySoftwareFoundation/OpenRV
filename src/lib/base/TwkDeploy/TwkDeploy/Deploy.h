//******************************************************************************
// Copyright (c) 2003 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkDeploy__Deploy__h__
#define __TwkDeploy__Deploy__h__
#include <iostream>
#include <string>
#include <stdlib.h>
#include <vector>

#ifndef PLATFORM_WINDOWS
#include <unistd.h>
#endif

typedef void (*ExitFunction)(int);

void TWK_DEPLOY_SHOW_PROGRAM_BANNER(std::ostream&, const char* c = 0);
void TWK_DEPLOY_SHOW_COPYRIGHT_BANNER(std::ostream&);
void TWK_DEPLOY_SHOW_LOCAL_BANNER(std::ostream&);
std::string TWK_DEPLOY_SHORT_APP_NAME();

int TWK_DEPLOY_GET_LICENSE_STATE();

// DONT CALL THIS DIRECTLY ANYMORE
void TWK_DEPLOY_INITIALIZE(int majorVersion, int minorVersion, int patchLevel,
                           int argc, char** argv,
                           const char* release_description, const char* scm_id,
                           const char* date = __DATE__,
                           const char* time = __TIME__);
void TWK_DEPLOY_SET_HOME(const char* dir);

void TWK_DEPLOY_SET_CRASHLOG_FILE(const char* localpath,
                                  const char* remotename);

int TWK_DEPLOY_MAJOR_VERSION();
int TWK_DEPLOY_MINOR_VERSION();
int TWK_DEPLOY_PATCH_LEVEL();
const char* TWK_DEPLOY_COMPILE_DATE();
const char* TWK_DEPLOY_COMPILE_TIME();
std::string TWK_DEPLOY_TYPE_DESCRIPTION();
const char* TWK_DEPLOY_SCM_ID();
void TWK_DEPLOY_FINISH();

//
// Make one of these on the stack instead of calling init yourself
//

struct TWK_DEPLOY_APP_OBJECT
{
    TWK_DEPLOY_APP_OBJECT(int majorVersion, int minorVersion, int patchLevel,
                          int argc, char** argv,
                          const char* release_description, const char* scm_id,
                          const char* date = __DATE__,
                          const char* time = __TIME__)
    {
        TWK_DEPLOY_INITIALIZE(majorVersion, minorVersion, patchLevel, argc,
                              argv, release_description, scm_id, date, time);
    }

    ~TWK_DEPLOY_APP_OBJECT() { TWK_DEPLOY_FINISH(); }
};

#endif // __TwkDeploy__Deploy__h__
