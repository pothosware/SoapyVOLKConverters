// Copyright (c) 2019-2021 Nicholas Corgan
// SPDX-License-Identifier: GPL-3.0

#include "TestUtility.hpp"

#include <SoapySDR/ConverterRegistry.hpp>
#include <SoapySDR/Formats.hpp>
#include <SoapySDR/Version.hpp>

#include <volk/constants.h>
#include <volk/volk.h>
#include <volk/volk_prefs.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

static constexpr size_t numElements = 16384;
static constexpr size_t numIterations = 10000;

//
// Utility functions
//

static size_t numVolkArchPrefs = 0;
static volk_arch_pref* volkArchPrefs = nullptr;
static volk_arch_pref* volkArchPrefsEnd = nullptr;

// This should only be called once, as volk_load_preferences resets the
// global struct each time, even if it's loaded with the same file.
void volkLoadPreferences()
{
    numVolkArchPrefs = volk_load_preferences(&volkArchPrefs);
    volkArchPrefsEnd = volkArchPrefs + numVolkArchPrefs;
}

// Assumes preferences are loaded
std::string getVolkMachineForFunc(const std::string& kernel)
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
        return "";
    }

    return std::string(prefsIter->impl_a);
}

template <typename InType, typename OutType>
void benchmarkConverter(
    const std::string& source,
    const std::string& target,
    SoapySDR::ConverterRegistry::FunctionPriority priority,
    double scalar,
    double* pMedian,
    double* pMedAbsDev)
{
    using Microseconds = std::chrono::duration<double, std::micro>;

    auto converterFunc = SoapySDR::ConverterRegistry::getFunction(
                             source,
                             target,
                             priority);

    volk::vector<double> times;
    times.reserve(numIterations);

    const auto input = TestUtility::getRandomValues<InType>(numElements);
    volk::vector<OutType> output(numElements);

    for(size_t i = 0; i < numIterations; ++i)
    {
        auto startTime = std::chrono::system_clock::now();

        converterFunc(
            input.data(),
            output.data(),
            numElements,
            scalar);

        auto endTime = std::chrono::system_clock::now();
        Microseconds iterationTime = endTime-startTime;
        times.emplace_back(iterationTime.count());
    }

    (*pMedian) = TestUtility::median(times);
    (*pMedAbsDev) = TestUtility::medAbsDev(times);
}

template <typename InType, typename OutType>
void compareConverters(
    const std::string& source,
    const std::string& target,
    double scalar,
    const std::string& volkKernelName)
{
    double genericMedianTime, genericMedAbsDevTime;
    double vectorizedMedianTime, vectorizedMedAbsDevTime;

    std::cout << std::endl << source << " -> " << target << " (scaled x" << scalar << ")" << std::endl;

    try
    {
        benchmarkConverter<InType, OutType>(
            source,
            target,
            SoapySDR::ConverterRegistry::GENERIC,
            scalar,
            &genericMedianTime,
            &genericMedAbsDevTime);
        benchmarkConverter<InType, OutType>(
            source,
            target,
            SoapySDR::ConverterRegistry::VECTORIZED,
            scalar,
            &vectorizedMedianTime,
            &vectorizedMedAbsDevTime);
        std::cout << "Generic:    " << genericMedianTime << "us +- " << genericMedAbsDevTime << "us" << std::endl;
        std::cout << "Vectorized: " << vectorizedMedianTime << "us +- " << vectorizedMedAbsDevTime << "us" << std::endl;
        std::cout << "Machine:    " << getVolkMachineForFunc(volkKernelName) << std::endl;
        std::cout << (genericMedianTime / vectorizedMedianTime) << "x faster" << std::endl;
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Benchmark failed with exception: " << ex.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "Unknown exception caught." << std::endl;
    }
}

template <typename InType, typename OutType>
void benchmarkVectorizedOnly(
    const std::string& source,
    const std::string& target,
    double scalar,
    const std::string& volkKernelName)
{
    double medianTime, medAbsDevTime;

    std::cout << std::endl << source << " -> " << target << " (scaled x" << scalar << ")" << std::endl;

    try
    {
        benchmarkConverter<InType, OutType>(
            source,
            target,
            SoapySDR::ConverterRegistry::VECTORIZED,
            scalar,
            &medianTime,
            &medAbsDevTime);
        std::cout << "Vectorized: " << medianTime << "us +- " << medAbsDevTime << "us" << std::endl;
        std::cout << "Machine:    " << getVolkMachineForFunc(volkKernelName) << std::endl;
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Benchmark failed with exception: " << ex.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "Unknown exception caught." << std::endl;
    }
}

int main(int, char**)
{
    try
    {
        if (!TestUtility::loadSoapyVOLK()) return EXIT_FAILURE;

        // Places values in global variables
        volkLoadPreferences();

        //std::cout << "SoapyVOLKConverters " << SoapySDR::getModuleVersion(modulePath) << std::endl;
        std::cout << "SoapySDR            " << SoapySDR::getLibVersion() << std::endl;
        std::cout << "VOLK                " << volk_version() << std::endl;

        std::cout << std::endl;
        std::cout << "Stats:" << std::endl;
        std::cout << " * Buffer size:  " << numElements << std::endl;
        std::cout << " * # iterations: " << numIterations << std::endl;

        // int8_t
        compareConverters<int8_t, int16_t>(
            SOAPY_SDR_S8,
            SOAPY_SDR_S16,
            1.0, // No scaling
            "volk_16i_convert_8i");
        compareConverters<int8_t, float>(
            SOAPY_SDR_S8,
            SOAPY_SDR_F32,
            TestUtility::S8ToF32Scalar,
            "volk_8i_s32f_convert_32f");
        benchmarkVectorizedOnly<int8_t, double>(
            SOAPY_SDR_S8,
            SOAPY_SDR_F64,
            TestUtility::S8ToF32Scalar,
            "volk_8i_s32f_convert_32f");

        // int16_t
        compareConverters<int16_t, int8_t>(
            SOAPY_SDR_S16,
            SOAPY_SDR_S8,
            1.0, // No scaling
            "volk_16i_convert_8i");
        compareConverters<int16_t, float>(
            SOAPY_SDR_S16,
            SOAPY_SDR_F32,
            TestUtility::S16ToF32Scalar,
            "volk_16i_s32f_convert_32f");
        benchmarkVectorizedOnly<int16_t, double>(
            SOAPY_SDR_S16,
            SOAPY_SDR_F64,
            TestUtility::S16ToF32Scalar,
            "volk_16i_s32f_convert_32f");

        // int32_t
        benchmarkVectorizedOnly<int32_t, float>(
            SOAPY_SDR_S32,
            SOAPY_SDR_F32,
            TestUtility::S32ToF32Scalar,
            "volk_32i_s32f_convert_32f");
        benchmarkVectorizedOnly<int32_t, double>(
            SOAPY_SDR_S32,
            SOAPY_SDR_F64,
            TestUtility::S32ToF32Scalar,
            "volk_32i_s32f_convert_32f");

        // float
        compareConverters<float, int8_t>(
            SOAPY_SDR_F32,
            SOAPY_SDR_S8,
            TestUtility::F32ToS8Scalar,
            "volk_32f_s32f_convert_8i");
        compareConverters<float, int16_t>(
            SOAPY_SDR_F32,
            SOAPY_SDR_S16,
            TestUtility::F32ToS16Scalar,
            "volk_32f_s32f_convert_16i");
        benchmarkVectorizedOnly<float, int32_t>(
            SOAPY_SDR_F32,
            SOAPY_SDR_S32,
            TestUtility::F32ToS32Scalar,
            "volk_32f_s32f_convert_32i");
        compareConverters<float, float>(
            SOAPY_SDR_F32,
            SOAPY_SDR_F32,
            10.0,
            "volk_32f_s32f_multiply_32f");
        benchmarkVectorizedOnly<float, double>(
            SOAPY_SDR_F32,
            SOAPY_SDR_F64,
            1.0,
            "volk_32f_convert_64f");

        // double
        benchmarkVectorizedOnly<double, int8_t>(
            SOAPY_SDR_F64,
            SOAPY_SDR_S8,
            TestUtility::F32ToS8Scalar,
            "volk_32f_s32f_convert_8i");
        benchmarkVectorizedOnly<double, int16_t>(
            SOAPY_SDR_F64,
            SOAPY_SDR_S16,
            TestUtility::F32ToS16Scalar,
            "volk_32f_s32f_convert_16i");
        benchmarkVectorizedOnly<double, int32_t>(
            SOAPY_SDR_F64,
            SOAPY_SDR_S32,
            TestUtility::F32ToS32Scalar,
            "volk_32f_s32f_convert_32i");
        benchmarkVectorizedOnly<double, float>(
            SOAPY_SDR_F64,
            SOAPY_SDR_F32,
            10.0,
            "volk_32f_s32f_multiply_32f");

        // std::complex<int8_t>
        compareConverters<std::complex<int8_t>, std::complex<int16_t>>(
            SOAPY_SDR_CS8,
            SOAPY_SDR_CS16,
            1.0, // No scaling
            "volk_16i_convert_8i");
        compareConverters<std::complex<int8_t>, std::complex<float>>(
            SOAPY_SDR_CS8,
            SOAPY_SDR_CF32,
            TestUtility::S8ToF32Scalar,
            "volk_8i_s32f_convert_32f");
        benchmarkVectorizedOnly<std::complex<int8_t>, std::complex<double>>(
            SOAPY_SDR_CS8,
            SOAPY_SDR_CF64,
            TestUtility::S8ToF32Scalar,
            "volk_8i_s32f_convert_32f");

        // std::complex<int16_t>
        compareConverters<std::complex<int16_t>, std::complex<int8_t>>(
            SOAPY_SDR_CS16,
            SOAPY_SDR_CS8,
            1.0, // No scaling
            "volk_16i_convert_8i");
        compareConverters<std::complex<int16_t>, std::complex<float>>(
            SOAPY_SDR_CS16,
            SOAPY_SDR_CF32,
            TestUtility::S16ToF32Scalar,
            "volk_16i_s32f_convert_32f");
        benchmarkVectorizedOnly<std::complex<int16_t>, std::complex<double>>(
            SOAPY_SDR_CS16,
            SOAPY_SDR_CF64,
            TestUtility::S16ToF32Scalar,
            "volk_16i_s32f_convert_32f");

        // std::complex<int32_t>
        benchmarkVectorizedOnly<std::complex<int32_t>, std::complex<float>>(
            SOAPY_SDR_CS32,
            SOAPY_SDR_CF32,
            TestUtility::S32ToF32Scalar,
            "volk_32i_s32f_convert_32f");
        benchmarkVectorizedOnly<std::complex<int32_t>, std::complex<double>>(
            SOAPY_SDR_CS32,
            SOAPY_SDR_CF64,
            TestUtility::S32ToF32Scalar,
            "volk_32i_s32f_convert_32f");

        // std::complex<float>
        compareConverters<std::complex<float>, std::complex<int8_t>>(
            SOAPY_SDR_CF32,
            SOAPY_SDR_CS8,
            TestUtility::F32ToS8Scalar,
            "volk_32f_s32f_convert_8i");
        compareConverters<std::complex<float>, std::complex<int16_t>>(
            SOAPY_SDR_CF32,
            SOAPY_SDR_CS16,
            TestUtility::F32ToS16Scalar,
            "volk_32f_s32f_convert_16i");
        benchmarkVectorizedOnly<std::complex<float>, std::complex<int32_t>>(
            SOAPY_SDR_CF32,
            SOAPY_SDR_CS32,
            TestUtility::F32ToS32Scalar,
            "volk_32f_s32f_convert_32i");
        compareConverters<std::complex<float>, std::complex<float>>(
            SOAPY_SDR_CF32,
            SOAPY_SDR_CF32,
            10.0,
            "volk_32f_s32f_multiply_32f");
        benchmarkVectorizedOnly<std::complex<float>, std::complex<double>>(
            SOAPY_SDR_CF32,
            SOAPY_SDR_CF64,
            1.0,
            "volk_32f_convert_64f");

        // std::complex<double>
        benchmarkVectorizedOnly<std::complex<double>, std::complex<int8_t>>(
            SOAPY_SDR_CF64,
            SOAPY_SDR_CS8,
            TestUtility::F32ToS8Scalar,
            "volk_32f_s32f_convert_8i");
        benchmarkVectorizedOnly<std::complex<double>, std::complex<int16_t>>(
            SOAPY_SDR_CF64,
            SOAPY_SDR_CS16,
            TestUtility::F32ToS16Scalar,
            "volk_32f_s32f_convert_16i");
        benchmarkVectorizedOnly<std::complex<double>, std::complex<int32_t>>(
            SOAPY_SDR_CF64,
            SOAPY_SDR_CS32,
            TestUtility::F32ToS32Scalar,
            "volk_32f_s32f_convert_32i");
        benchmarkVectorizedOnly<std::complex<double>, std::complex<float>>(
            SOAPY_SDR_CF64,
            SOAPY_SDR_CF32,
            10.0,
            "volk_32f_s32f_multiply_32f");
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
