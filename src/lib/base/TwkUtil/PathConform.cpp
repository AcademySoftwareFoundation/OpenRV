//******************************************************************************
// Copyright (c) 2007 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <TwkUtil/PathConform.h>
#include <TwkUtil/TwkRegEx.h>
#include <iostream>
#include <sstream>
#include <algorithm>

namespace TwkUtil
{
    using namespace std;

    bool pathIsURL(const std::string& path)
    {
        RegEx regEx("^[a-z]*://");
        Match m(regEx, path);
        return m.foundMatch();
    }

    string pathConform(const std::string& path)
    {
#ifdef WIN32
        string cygdrive = "/cygdrive/";

        string s = path;
        if (pathIsURL(path))
            return s;
        replace(s.begin(), s.end(), '\\', '/');

        if (s.compare(0, cygdrive.size(), cygdrive) == 0)
        {
            //
            //      Cygwin path
            //

            s.replace(0, cygdrive.size(), string(""));
            string::size_type p = s.find('/');
            if (p != string::npos)
                s.replace(p, 1, ":/");
            // cout << "INFO: converting cygwin path " << path << endl;
        }
        else if (s.size() > 2 && s[0] == '/' && s[1] == '/')
        {
            //
            //      UNC path. Remove and preceeding slashes before the
            //      double slash
            //

            while (s.size() > 2 && s[0] == '/' && s[1] == '/' && s[2] == '/')
            {
                s.erase(0, 1);
            }
        }

        //
        //  This is causing rv to make sequence out of lists of
        //  directories whose names suggest frame numbers (the trailing
        //  slash is used to indicate directories).  So take it out for
        //  now, but look out for other weirdness.
        //
        //  if (s.size() && s[s.size()-1] == '/') s.erase(s.size()-1);

        return s;

#else
        //
        //  Remove doubled slashes from path.
        //
        string s = path;
        if (pathIsURL(path))
            return s;
        size_t pos;
        while ((pos = s.find("//")) != string::npos)
            s.replace(pos, 2, "/");
        return s;
#endif
    }

} // namespace TwkUtil
