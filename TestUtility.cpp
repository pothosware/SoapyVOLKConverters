// Copyright (c) 2021 Nicholas Corgan
// SPDX-License-Identifier: GPL-3.0

#include "TestUtility.hpp"

#include <SoapySDR/Modules.hpp>

#include <iostream>
#include <stdexcept>

namespace TestUtility
{
    bool loadSoapyVOLK()
    {
        try
        {
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