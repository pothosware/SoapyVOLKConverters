// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: GPL-3.0

#include <SoapySDR/ConverterRegistry.hpp>
#include <SoapySDR/Formats.hpp>
#include <SoapySDR/Modules.hpp>
#include <SoapySDR/Version.hpp>

#include <volk/constants.h>
#include <volk/volk.h>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <memory>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

using SoapySDR::ConverterRegistry;

static constexpr size_t numElements = 16384;
static constexpr size_t numIterations = 10000;
static constexpr double scalarRatio = 10.0;

//
// RAII volk_malloc
//

using VolkPtr = std::unique_ptr<void, void(*)(void*)>;

static VolkPtr getVolkPtr(size_t size)
{
    return VolkPtr(
               volk_malloc(size, volk_get_alignment()),
               volk_free);
}

//
// Utility functions
//

static VolkPtr getRandomMemory(size_t numElements, size_t elementSize)
{
    const size_t memSize = numElements * elementSize;
    auto mem = getVolkPtr(memSize);

    // Fill the memory with "random" data. Not truly random, but enough for
    // our purpose.
    int* memAsInt = reinterpret_cast<int*>(mem.get());
    for(size_t i = 0; i < (memSize/sizeof(int)); ++i)
    {
        memAsInt[i] = rand();
    }

    return mem;
}

double median(const std::vector<double>& inputs)
{
    std::vector<double> sortedInputs(inputs);
    std::sort(sortedInputs.begin(), sortedInputs.end());

    return sortedInputs[sortedInputs.size()/2];
}

double medAbsDev(const std::vector<double>& inputs)
{
    const double med = median(inputs);

    std::vector<double> diffs;
    std::transform(
        inputs.begin(),
        inputs.end(),
        std::back_inserter(diffs),
        [&med](double val){return std::abs(val-med);});

    return median(diffs);
}

static inline double getScalar(
    const std::string& source,
    const std::string& target)
{
    const auto sourceSize = SoapySDR::formatToSize(source);
    const auto targetSize = SoapySDR::formatToSize(target);

    if(sourceSize == targetSize) return 1.0;

    return (sourceSize > targetSize) ? (1.0 / scalarRatio) : scalarRatio;
}

static void benchmarkConverter(
    const std::string& source,
    const std::string& target,
    ConverterRegistry::FunctionPriority priority,
    double* pMedian,
    double* pMedAbsDev)
{
    using Microseconds = std::chrono::duration<double, std::micro>;

    auto converterFunc = ConverterRegistry::getFunction(
                             source,
                             target,
                             priority);

    std::vector<double> times;
    times.reserve(numIterations);

    auto output = getVolkPtr(numElements * SoapySDR::formatToSize(target));
    const auto scalar = getScalar(source, target);

    for(size_t i = 0; i < numIterations; ++i)
    {
        auto input = getRandomMemory(numElements, SoapySDR::formatToSize(source));

        auto startTime = std::chrono::system_clock::now();

        converterFunc(
            input.get(),
            output.get(),
            numElements,
            scalar);

        auto endTime = std::chrono::system_clock::now();
        Microseconds iterationTime = endTime-startTime;
        times.emplace_back(iterationTime.count());
    }

    (*pMedian) = median(times);
    (*pMedAbsDev) = medAbsDev(times);
}

static void compareConverters(
    const std::string& source,
    const std::string& target)
{
    double genericMedianTime, genericMedAbsDevTime;
    double vectorizedMedianTime, vectorizedMedAbsDevTime;

    std::cout << std::endl << source << " -> " << target << std::endl;
    benchmarkConverter(
        source,
        target,
        ConverterRegistry::GENERIC,
        &genericMedianTime,
        &genericMedAbsDevTime);
    benchmarkConverter(
        source,
        target,
        ConverterRegistry::VECTORIZED,
        &vectorizedMedianTime,
        &vectorizedMedAbsDevTime);
    std::cout << "Generic:    " << genericMedianTime << " us (MAD = " << genericMedAbsDevTime << " us)" << std::endl;
    std::cout << "Vectorized: " << vectorizedMedianTime << " us (MAD = " << vectorizedMedAbsDevTime << " us)" << std::endl;
    std::cout << (genericMedianTime / vectorizedMedianTime) << "x faster" << std::endl;
}

static void benchmarkVectorizedOnly(
    const std::string& source,
    const std::string& target)
{
    double medianTime, medAbsDevTime;

    std::cout << std::endl << source << " -> " << target << std::endl;
    benchmarkConverter(
        source,
        target,
        ConverterRegistry::VECTORIZED,
        &medianTime,
        &medAbsDevTime);
    std::cout << "Vectorized: " << medianTime << " us (MAD = " << medAbsDevTime << " us)" << std::endl;
}

int main(int, char**)
{
    try
    {
        srand(time(0));

        // TODO: portability
        const std::string modulePath = "./libvolkConverters.so";
        SoapySDR::loadModule(modulePath);

        std::cout << "SoapyVOLKConverters " << SoapySDR::getModuleVersion(modulePath) << std::endl;
        std::cout << "SoapySDR            " << SoapySDR::getLibVersion() << std::endl;
        std::cout << "VOLK                " << volk_version() << std::endl;

        std::cout << std::endl;
        std::cout << "Stats:" << std::endl;
        std::cout << " * Buffer size:  " << numElements << std::endl;
        std::cout << " * # iterations: " << numIterations << std::endl;
        std::cout << " * Scalar ratio: " << scalarRatio << std::endl;

        compareConverters(SOAPY_SDR_CS16, SOAPY_SDR_CF32);
        compareConverters(SOAPY_SDR_S16, SOAPY_SDR_S8);
        compareConverters(SOAPY_SDR_CF32, SOAPY_SDR_CS16);
        compareConverters(SOAPY_SDR_S8, SOAPY_SDR_S16);
        compareConverters(SOAPY_SDR_S16, SOAPY_SDR_F32);
        compareConverters(SOAPY_SDR_F32, SOAPY_SDR_S16);
        compareConverters(SOAPY_SDR_F32, SOAPY_SDR_S8);
        compareConverters(SOAPY_SDR_S8, SOAPY_SDR_F32);

        benchmarkVectorizedOnly(SOAPY_SDR_F32, SOAPY_SDR_F64);
        benchmarkVectorizedOnly(SOAPY_SDR_F64, SOAPY_SDR_F32);
        benchmarkVectorizedOnly(SOAPY_SDR_F32, SOAPY_SDR_S32);
        benchmarkVectorizedOnly(SOAPY_SDR_S32, SOAPY_SDR_F32);
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
