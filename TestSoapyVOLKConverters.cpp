// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: GPL-3.0

#include <SoapySDR/ConverterRegistry.hpp>
#include <SoapySDR/Formats.hpp>
#include <SoapySDR/Modules.hpp>

#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

using SoapySDR::ConverterRegistry;
using ByteVector = std::vector<uint8_t>;

static ByteVector getRandomMemory(size_t numElements, size_t elementSize)
{
    const size_t memSize = numElements * elementSize;
    ByteVector mem(memSize);

    // Fill the memory with "random" data. Not truly random, but enough for
    // our purpose.
    int* memAsInt = reinterpret_cast<int*>(mem.data());
    for(size_t i = 0; i < (memSize/sizeof(int)); ++i)
    {
        memAsInt[i] = rand();
    }

    return mem;
}

static void testConverter(
    const std::string& source,
    const std::string& target)
{
    ConverterRegistry::ConverterFunction vectorizedFunc = nullptr;

    std::cout << "Testing " << source << " -> " << target << "..." << std::endl;

    try
    {
        vectorizedFunc = ConverterRegistry::getFunction(
                             source,
                             target,
                             ConverterRegistry::VECTORIZED);
    }
    catch(const std::runtime_error& ex)
    {
        std::cerr << "Exception caught getting vectorized converter: "
                  << ex.what() << std::endl;
        exit(EXIT_FAILURE);
    }

    if(!vectorizedFunc)
    {
        std::cerr << "getFunction() returned null function." << std::endl;
        exit(EXIT_FAILURE);
    }

    // At this point, make sure the function can be called and executed without
    // crashing.
    static constexpr size_t numElements = 2048;
    static constexpr double scalar = 1.0;

    const auto sourceVector = getRandomMemory(
                                  numElements,
                                  SoapySDR::formatToSize(source));

    const size_t outputSize = numElements * SoapySDR::formatToSize(target);
    ByteVector vectorizedOutput(outputSize);

    vectorizedFunc(
        sourceVector.data(),
        vectorizedOutput.data(),
        numElements,
        scalar);
}

int main(int, char**)
{
    try
    {
        srand(time(0));

        // TODO: portability
        SoapySDR::loadModule("./libvolkConverters.so");

        testConverter(SOAPY_SDR_CS16, SOAPY_SDR_CF32);
        testConverter(SOAPY_SDR_S16,  SOAPY_SDR_S8);
        testConverter(SOAPY_SDR_CF32, SOAPY_SDR_CS16);
        testConverter(SOAPY_SDR_F32,  SOAPY_SDR_F64);
        testConverter(SOAPY_SDR_F64,  SOAPY_SDR_F32);
        testConverter(SOAPY_SDR_S8,   SOAPY_SDR_S16);
        testConverter(SOAPY_SDR_S16,  SOAPY_SDR_F32);
        testConverter(SOAPY_SDR_F32,  SOAPY_SDR_S16);
        testConverter(SOAPY_SDR_F32,  SOAPY_SDR_S32);
        testConverter(SOAPY_SDR_F32,  SOAPY_SDR_S8);
        testConverter(SOAPY_SDR_S32,  SOAPY_SDR_F32);
        testConverter(SOAPY_SDR_S8,   SOAPY_SDR_F32);
    }
    catch(const std::exception& ex)
    {
        std::cerr << "Uncaught exception: " << ex.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch(...)
    {
        std::cerr << "Unknown exception caught." << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
