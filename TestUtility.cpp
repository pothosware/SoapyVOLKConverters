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

#include <direct.h> // _getcwd

#define getcwd _getcwd
#else
#define IS_UNIX

#include <unistd.h> // getcwd
#endif

#ifndef MAX_PATH
#define MAX_PATH 256
#endif

// TODO: MinGW
namespace TestUtility
{
    bool loadSoapyVOLK()
    {
        try
        {
            std::string basename;
            std::string extension;
            std::string separator;
            std::string subpath;

            char cwd[MAX_PATH] = {0};
            char* _ = getcwd(cwd, sizeof(cwd)); // Suppress Clang warning
            (void)_;

#ifdef IS_WIN32
            basename = "volkConverters";
            extension = ".dll";
            separator = "\\";
            subpath = cwd + separator + CMAKE_BUILD_TYPE;
#else
            basename = "libvolkConverters";
            extension = ".so";
            separator = "/";
            subpath.assign(cwd);
#endif
            const std::string filepath = subpath + separator + basename + extension;
            std::cout << "Loading " << filepath << "..." << std::endl;
            SoapySDR::loadModule(filepath);
            std::cout << "Loaded version " << SoapySDR::getModuleVersion(filepath) << std::endl;
        }
        catch (const std::exception& ex)
        {
            std::cerr << "Exception loading module: " << ex.what() << std::endl;
            return false;
        }

        return true;
    }
}
