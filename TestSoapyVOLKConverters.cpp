// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: GPL-3.0

#include <SoapySDR/ConverterRegistry.hpp>
#include <SoapySDR/Formats.hpp>
#include <SoapySDR/Modules.hpp>

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using SoapySDR::ConverterRegistry;
using ByteVector = std::vector<uint8_t>;

static void checkConverterExecutes(
    const std::string& source,
    const std::string& target)
{
    ConverterRegistry::ConverterFunction converterFunc = nullptr;
    std::cout << "Testing " << source << " -> " << target << "..." << std::endl;

    try
    {
        converterFunc = ConverterRegistry::getFunction(
                            source,
                            target,
                            ConverterRegistry::VECTORIZED);
    }
    catch(const std::runtime_error& ex)
    {
        std::cerr << ex.what() << std::endl;
        exit(EXIT_FAILURE);
    }

    if(!converterFunc)
    {
        std::cerr << "getFunction() returned null function." << std::endl;
        exit(EXIT_FAILURE);
    }

    // At this point, make sure the function can be called and executed without
    // crashing.
    static constexpr size_t numElements = 256;
    static constexpr double scalar = 1.0;

    ByteVector sourceVector(numElements * SoapySDR::formatToSize(source));
    ByteVector targetVector(numElements * SoapySDR::formatToSize(target));
    converterFunc(
        sourceVector.data(),
        targetVector.data(),
        numElements,
        scalar);
}

int main(int, char**)
{
    // TODO: portability
    SoapySDR::loadModule("./libvolkConverters.so");

    // Make sure all expected converters exist and execute.
    checkConverterExecutes(SOAPY_SDR_CS16, SOAPY_SDR_CF32);
    checkConverterExecutes(SOAPY_SDR_S16,  SOAPY_SDR_S8);
    checkConverterExecutes(SOAPY_SDR_CF32, SOAPY_SDR_CS16);
    checkConverterExecutes(SOAPY_SDR_F32,  SOAPY_SDR_F64);
    checkConverterExecutes(SOAPY_SDR_F64,  SOAPY_SDR_F32);
    checkConverterExecutes(SOAPY_SDR_S8,   SOAPY_SDR_S16);
    checkConverterExecutes(SOAPY_SDR_S16,  SOAPY_SDR_F32);
    checkConverterExecutes(SOAPY_SDR_F32,  SOAPY_SDR_S16);
    checkConverterExecutes(SOAPY_SDR_F32,  SOAPY_SDR_S32);
    checkConverterExecutes(SOAPY_SDR_F32,  SOAPY_SDR_S8);
    checkConverterExecutes(SOAPY_SDR_S32,  SOAPY_SDR_F32);
    checkConverterExecutes(SOAPY_SDR_S8,   SOAPY_SDR_F32);

    return EXIT_SUCCESS;
}
