//
// Copyright (c) 2010, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __stl_ext_string_algo__h__
#define __stl_ext_string_algo__h__
#include <string>
#include <vector>
#include <stl_ext/stl_ext_config.h>

namespace stl_ext
{

    //
    //  Tokenize, like the C function. Returns a vector of strings with
    //  the tokens in it. Lifted from some linux help site.
    //

    STL_EXT_EXPORT void tokenize(std::vector<std::string>& tokens,
                                 const std::string& str,
                                 const std::string& delimiters = " ");

    //
    //  Wrap the text to some column width, break at char
    //

    STL_EXT_EXPORT std::string wrap(const std::string& text,
                                    char wrapChar = ' ',
                                    const std::string& breakString = "\n",
                                    std::string::size_type column = 65);

    // Returns the directory part of the path
    STL_EXT_EXPORT std::string dirname(std::string path);

    // Returns the file part of the path
    STL_EXT_EXPORT std::string basename(std::string path);

    // Returns "file" for "/tmp/file.tif"
    STL_EXT_EXPORT std::string prefix(std::string path);

    // Returns the file extension (if there is one, "" otherwise )
    STL_EXT_EXPORT std::string extension(std::string path);

    // Returns a hash value for the given string
    STL_EXT_EXPORT unsigned long hash(const std::string& s);

} // namespace stl_ext

#endif
