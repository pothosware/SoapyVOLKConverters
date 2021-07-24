// Copyright (c) 2021 Nicholas Corgan
// SPDX-License-Identifier: GPL-3.0

#include "config.h"
#include "TestUtility.hpp"

#include <SoapySDR/Modules.hpp>

#include <cstdlib>
#include <iostream>
#include <stdexcept>

#if defined _WIN32 || defined __CYGWIN__
#define IS_WIN32
#else
#define IS_UNIX
#endif

#ifndef MAX_PATH
#define MAX_PATH 256
#endif

#ifdef IS_WIN32
#include <direct.h>

std::string getCWD()
{
    char cwdBuff[MAX_PATH] = { 0 };
    _getcwd(cwdBuff, sizeof(cwdBuff));

    return cwdBuff;
}
#else
#include <unistd.h>

std::string getCWD()
{
    char cwdBuff[MAX_PATH] = { 0 };
    getcwd(cwdBuff);

    return cwdBuff;
}
#endif

namespace TestUtility
{
    bool loadSoapyVOLK()
    {
        try
        {
            std::string extension;
            std::string separator;
            std::string subpath;

#if defined _WIN32 || defined __CYGWIN__
            extension = ".dll";
            separator = "\\";
            subpath = getCWD() + separator + CMAKE_BUILD_TYPE;
#else
            extension = ".so";
            separator = "/";
            subpath = getCWD();
#endif
            const std::string filepath = subpath + separator + "volkConverters" + extension;
            std::cout << "Loading " << filepath << " (" << SoapySDR::getModuleVersion(filepath) << ")..." << std::endl;

            SoapySDR::loadModule("Release\\volkConverters.dll");
        }
        catch (const std::exception& ex)
        {
            std::cerr << "Exception loading module: " << ex.what() << std::endl;
            return false;
        }

        return true;
    }
}