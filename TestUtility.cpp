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

#include <direct.h> // getcwd
#else
#define IS_UNIX

#include <unistd.h> // getcwd
#endif

#ifndef MAX_PATH
#define MAX_PATH 256
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

            char cwd[MAX_PATH] = {0};
            getcwd(cwd, sizeof(cwd));

#if IS_WIN32
            extension = ".dll";
            separator = "\\";
            subpath = getCWD() + separator + CMAKE_BUILD_TYPE;
#else
            extension = ".so";
            separator = "/";
            subpath.assign(cwd);
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
