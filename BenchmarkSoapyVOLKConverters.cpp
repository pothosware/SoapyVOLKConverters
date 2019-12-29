// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: GPL-3.0

#include <SoapySDR/ConverterRegistry.hpp>
#include <SoapySDR/Formats.hpp>
#include <SoapySDR/Modules.hpp>
#include <SoapySDR/Version.hpp>

#include <volk/constants.h>
#include <volk/volk.h>
#include <volk/volk_prefs.h>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
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

static size_t numVolkArchPrefs = 0;
static volk_arch_pref* volkArchPrefs = nullptr;
static volk_arch_pref* volkArchPrefsEnd = nullptr;

// This should only be called once, as volk_load_preferences resets the
// global struct each time, even if it's loaded with the same file.
static void volkLoadPreferences()
{
    numVolkArchPrefs = volk_load_preferences(&volkArchPrefs);
    volkArchPrefsEnd = volkArchPrefs + numVolkArchPrefs;
}

// Assumes preferences are loaded
static std::string getVolkMachineForFunc(
    const std::string& kernel,
    bool aligned)
{
    auto prefsIter = std::find_if(
                         volkArchPrefs,
                         volkArchPrefsEnd,
                         [&kernel](const volk_arch_pref& pref)
                         {
                            return (0 == strncmp(
                                             kernel.c_str(),
                                             pref.name,
                                             sizeof(pref.name)));
                         });
    if(volkArchPrefsEnd == prefsIter)
    {
        std::cerr << "Could not find preferences for kernel " << kernel << "." << std::endl;
        exit(EXIT_FAILURE);
    }

    return aligned ? std::string(prefsIter->impl_a) : std::string(prefsIter->impl_u);
}

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
    if(!volk_is_aligned(output.get()))
    {
        std::cerr << "volk_malloc didn't return aligned pointer for output buffer" << std::endl;
        exit(EXIT_FAILURE);
    }

    const auto scalar = getScalar(source, target);

    for(size_t i = 0; i < numIterations; ++i)
    {
        auto input = getRandomMemory(numElements, SoapySDR::formatToSize(source));
        if(!volk_is_aligned(input.get()))
        {
            std::cerr << "volk_malloc didn't return aligned pointer for input buffer" << std::endl;
            exit(EXIT_FAILURE);
        }

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
    const std::string& target,
    const std::string& volkKernelName = "")
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
    if(!volkKernelName.empty())
    {
        std::cout << "Machine:    " << getVolkMachineForFunc(volkKernelName, true) << std::endl;
    }
    std::cout << (genericMedianTime / vectorizedMedianTime) << "x faster" << std::endl;
}

static void benchmarkVectorizedOnly(
    const std::string& source,
    const std::string& target,
    const std::string& volkKernelName = "")
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
    if(!volkKernelName.empty())
    {
        std::cout << "Machine:    " << getVolkMachineForFunc(volkKernelName, true) << std::endl;
    }
}

int main(int, char**)
{
    try
    {
        srand(time(0));

        // TODO: portability
        const std::string modulePath = "./libvolkConverters.so";
        SoapySDR::loadModule(modulePath);

        // Places values in global variables
        volkLoadPreferences();

        std::cout << "SoapyVOLKConverters " << SoapySDR::getModuleVersion(modulePath) << std::endl;
        std::cout << "SoapySDR            " << SoapySDR::getLibVersion() << std::endl;
        std::cout << "VOLK                " << volk_version() << std::endl;

        std::cout << std::endl;
        std::cout << "Stats:" << std::endl;
        std::cout << " * Buffer size:  " << numElements << std::endl;
        std::cout << " * # iterations: " << numIterations << std::endl;
        std::cout << " * Scalar ratio: " << scalarRatio << std::endl;

        compareConverters(SOAPY_SDR_CS16, SOAPY_SDR_CF32, "volk_16ic_convert_32fc");
        compareConverters(SOAPY_SDR_S16, SOAPY_SDR_S8, "volk_16i_convert_8i");
        compareConverters(SOAPY_SDR_CF32, SOAPY_SDR_CS16, "volk_32fc_convert_16ic");
        compareConverters(SOAPY_SDR_S8, SOAPY_SDR_S16, "volk_8i_convert_16i");
        compareConverters(SOAPY_SDR_S16, SOAPY_SDR_F32, "volk_16i_s32f_convert_32f");
        compareConverters(SOAPY_SDR_F32, SOAPY_SDR_S16, "volk_32f_s32f_convert_32i");
        compareConverters(SOAPY_SDR_F32, SOAPY_SDR_S8, "volk_32f_s32f_convert_8i");
        compareConverters(SOAPY_SDR_S8, SOAPY_SDR_F32, "volk_8i_s32f_convert_32f");

        benchmarkVectorizedOnly(SOAPY_SDR_F32, SOAPY_SDR_F64, "volk_32f_convert_64f");
        benchmarkVectorizedOnly(SOAPY_SDR_F64, SOAPY_SDR_F32, "volk_64f_convert_32f");
        benchmarkVectorizedOnly(SOAPY_SDR_F32, SOAPY_SDR_S32, "volk_32f_s32f_convert_32i");
        benchmarkVectorizedOnly(SOAPY_SDR_S32, SOAPY_SDR_F32, "volk_32i_s32f_convert_32f");
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
