//******************************************************************************
// Copyright (c) 2003 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <TwkDeploy/Deploy.h>
#include <time.h>
#include <string>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _MSC_VER
#include <unistd.h>
#include <pwd.h>
#endif

#ifdef WIN32
#define snprintf _snprintf
#endif

using namespace std;

static int         Vmajor       = 0;
static int         Vminor       = 0;
static int         Vpatch_level = 0;
static const char* Vdesc       = "";
static const char* Cdate        = "someday";
static const char* Ctime        = "sometime";
static int         Pargc        = 0;
static char**      Pargv        = 0;
static const char* crashlog     = ".tweak_crash_log";
static const char* remotelog    = "tweak_crash_log";
static const char* homeloc      = 0;
static const char* SCM_ID       = "";
static int         lstate       = 2;

int
TWK_DEPLOY_GET_LICENSE_STATE()
{
    return lstate;
}

string
TWK_DEPLOY_SHORT_APP_NAME (void)
{
    return "rv";
}

void
TWK_DEPLOY_SET_HOME(const char* dir)
{
    homeloc = strdup(dir);
}

void
TWK_DEPLOY_SEGV(int sig)
{
    fprintf(stderr, "ERROR: segmentation violation\n");

    _exit(-1);
}

void
TWK_DEPLOY_SHOW_COPYRIGHT_BANNER(ostream& o)
{
    o << COPYRIGHT_TEXT;
    o << endl;
}

void
TWK_DEPLOY_SHOW_LOCAL_BANNER(ostream& o)
{
    // nothing
}

int TWK_DEPLOY_MAJOR_VERSION() { return Vmajor; }
int TWK_DEPLOY_MINOR_VERSION() { return Vminor; }
int TWK_DEPLOY_PATCH_LEVEL()   { return Vpatch_level; }
const char* TWK_DEPLOY_COMPILE_DATE() { return Cdate; }
const char* TWK_DEPLOY_COMPILE_TIME() { return Ctime; }
const char* TWK_DEPLOY_SCM_ID() { return SCM_ID; }

void TWK_DEPLOY_SET_CRASHLOG_FILE(const char* c,
                                  const char* r)
{
    crashlog = c;
    remotelog = r;
}

void TWK_DEPLOY_INITIALIZE(int major, int minor, int patch_level,
                           int argc, char** argv,
                           const char* release_description,
                           const char* scm_id,
                           const char* d,
                           const char* t)
{
    Vmajor = major;
    Vminor = minor;
    Vpatch_level = patch_level;
    Vdesc = release_description;
    Pargc = argc;
    Pargv = argv;
    Cdate = d;
    Ctime = t;
    SCM_ID = scm_id;
}

void
TWK_DEPLOY_SHOW_PROGRAM_BANNER(ostream& o, const char *m)
{
    if (m) o << m;
    else o << Pargv[0];

    o << endl;

    o << "Version " << Vmajor << "." << Vminor << "." << Vpatch_level
      << " (" << Vdesc << "), built on " << Cdate
      //<< " at " << Ctime << " (" << 8*sizeof(void*) << "bit).";
      << " at " << Ctime << " (" << SCM_ID << ").";

    o << endl;
}

void
TWK_DEPLOY_FINISH()
{
}
