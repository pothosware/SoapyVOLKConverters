// Copyright (c) 2019-2021 Nicholas Corgan
// SPDX-License-Identifier: GPL-3.0

#include <SoapySDR/ConverterRegistry.hpp>
#include <SoapySDR/Formats.hpp>
#include <SoapySDR/Modules.hpp>

#include <volk/volk_alloc.hh>

#include <cmath>
#include <complex>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

using SoapySDR::ConverterRegistry;

//
// Utility (TODO: common file brought in here and in benchmark)
//

template <typename T>
struct IsComplex : std::false_type {};

template <typename T>
struct IsComplex<std::complex<T>> : std::true_type {};

template <typename T, typename Ret>
using EnableIfByte = typename std::enable_if_t<(sizeof(T) == 1), Ret>;

template <typename T, typename Ret>
using EnableIfIntegral = typename std::enable_if_t<std::is_integral_v<T> && !IsComplex<T>::value && (sizeof(T) > 1), Ret>;

template <typename T, typename Ret>
using EnableIfFloatingPoint = typename std::enable_if_t<std::is_floating_point_v<T> && !IsComplex<T>::value, Ret>;

template <typename T, typename Ret>
using EnableIfNotComplex = typename std::enable_if_t<!IsComplex<T>::value, Ret>;

template <typename T, typename Ret>
using EnableIfComplex = typename std::enable_if_t<IsComplex<T>::value, Ret>;

static std::random_device rd;
static std::mt19937 gen(rd());

template <typename T>
static EnableIfByte<T, T> getRandomValue()
{
    static std::uniform_int_distribution<int> dist(0, 50);
    return T(dist(gen));
}

template <typename T>
static EnableIfIntegral<T, T> getRandomValue()
{
    static std::uniform_int_distribution<T> dist(T(0), T(50));
    return dist(gen);
}

template <typename T>
static EnableIfFloatingPoint<T, T> getRandomValue()
{
    static std::uniform_real_distribution<T> dist(T(0.0), T(1.0));
    return dist(gen);
}

template <typename T>
static EnableIfComplex<T, T> getRandomValue()
{
    using ScalarType = typename T::value_type;
    return T(getRandomValue<ScalarType>(), getRandomValue<ScalarType>());
}

template <typename T>
static std::vector<T> getRandomValues(size_t numElements)
{
    std::vector<T> randomValues;
    for (size_t i = 0; i < numElements; ++i) randomValues.emplace_back(getRandomValue<T>());

    return randomValues;
}

static bool loadSoapyVOLK()
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

template <typename T>
T median(const std::vector<T>& inputs)
{
    std::vector<T> sortedInputs(inputs);
    std::sort(sortedInputs.begin(), sortedInputs.end());

    return sortedInputs[sortedInputs.size() / 2];
}

template <typename T>
static EnableIfNotComplex<T, T> absDiff(const T& num0, const T& num1)
{
    return std::abs(num0 - num1);
}

template <typename T>
static EnableIfComplex<T, T> absDiff(const T& num0, const T& num1)
{
    return std::abs(std::abs(num0) - std::abs(num1));
}

template <typename T>
static EnableIfNotComplex<T, T> medianDiff(
    const std::vector<T>& vec0,
    const std::vector<T>& vec1)
{
    std::vector<T> diffs(vec0.size());

    for (size_t i = 0; i < vec0.size(); ++i)
    {
        diffs[i] = std::abs(vec0[i] - vec1[i]);
    }

    return median(diffs);
}

template <typename T>
static EnableIfComplex<T, typename T::value_type> medianDiff(
    const std::vector<T>& vec0,
    const std::vector<T>& vec1)
{
    std::vector<T::value_type> diffs(vec0.size());

    for (size_t i = 0; i < vec0.size(); ++i)
    {
        diffs[i] = std::abs(std::abs(vec0[i]) - std::abs(vec1[i]));
    }

    return median(diffs);
}

template <typename T>
static inline EnableIfNotComplex<T, void> testOutputs(
    const std::vector<T>& genericConverterOutputs,
    const std::vector<T>& vectorizedConverterOutputs)
{
    std::cout << " * Average diff: " << double(medianDiff(genericConverterOutputs, vectorizedConverterOutputs)) << std::endl;
}

template <typename T>
static inline EnableIfComplex<T, void> testOutputs(
    const std::vector<T>& genericConverterOutputs,
    const std::vector<T>& vectorizedConverterOutputs)
{
    std::cout << " * Average complex diff: " << double(medianDiff(genericConverterOutputs, vectorizedConverterOutputs)) << std::endl;
}

//
// Test code
//

template <typename InType, typename OutType>
static bool testConverterLoopback(
    const std::string& source,
    const std::string& target,
    const double scalar)
{
    static constexpr size_t numElements = 1024;

    std::cout << "Testing " << source << " -> " << target << " (scaled x" << scalar << ")..." << std::endl;

    const std::vector<InType> testValues = getRandomValues<InType>(numElements);
    std::vector<OutType> genericConverterOutputs(numElements);
    std::vector<OutType> vectorizedConverterOutputs(numElements);

    SoapySDR::ConverterRegistry::ConverterFunction genericFunc = nullptr;
    SoapySDR::ConverterRegistry::ConverterFunction vectorizedFunc = nullptr;

    try
    {
        genericFunc = SoapySDR::ConverterRegistry::getFunction(
            source,
            target,
            SoapySDR::ConverterRegistry::GENERIC);
    }
    catch(std::exception & ex)
    {
        std::cerr << " * Exception getting generic converter: " << ex.what() << std::endl;
    }

    try
    {
        vectorizedFunc = SoapySDR::ConverterRegistry::getFunction(
            source,
            target,
            SoapySDR::ConverterRegistry::VECTORIZED);
    }
    catch(std::exception& ex)
    {
        std::cerr << " * Exception getting vectorized converter: " << ex.what() << std::endl;
        return false;
    }

    if (genericFunc)
    {
        std::cout << " * Running generic converter..." << std::endl;
        genericFunc(testValues.data(), genericConverterOutputs.data(), numElements, scalar);
    }

    std::cout << " * Running vectorized converter..." << std::endl;
    vectorizedFunc(testValues.data(), vectorizedConverterOutputs.data(), numElements, scalar);

    if(genericFunc) testOutputs(genericConverterOutputs, vectorizedConverterOutputs);

    return true;
}

//
// Main
//

int main(int, char**)
{
    if (!loadSoapyVOLK()) return EXIT_FAILURE;

    // These converters don't support scaling and ignore the parameter
    testConverterLoopback<std::complex<int16_t>, std::complex<float>>(SOAPY_SDR_CS16, SOAPY_SDR_CF32, 1.0);
    testConverterLoopback<int16_t, int8_t>(SOAPY_SDR_S16, SOAPY_SDR_S8, 1.0);
    testConverterLoopback<std::complex<float>, std::complex<int16_t>>(SOAPY_SDR_CF32, SOAPY_SDR_CS16, 1.0);
    testConverterLoopback<float, double>(SOAPY_SDR_F32, SOAPY_SDR_F64, 1.0);
    testConverterLoopback<double, float>(SOAPY_SDR_F64, SOAPY_SDR_F32, 1.0);
    testConverterLoopback<int8_t, int16_t>(SOAPY_SDR_S8, SOAPY_SDR_S16, 1.0);

    // Tests with scaling
    testConverterLoopback<int8_t, float>(SOAPY_SDR_S8, SOAPY_SDR_F32, 1.0);
    testConverterLoopback<int16_t, float>(SOAPY_SDR_S16, SOAPY_SDR_F32, 1.0);
    testConverterLoopback<int32_t, float>(SOAPY_SDR_S32, SOAPY_SDR_F32, 1.0);
    testConverterLoopback<float, int8_t>(SOAPY_SDR_F32, SOAPY_SDR_S8, 1.0);
    testConverterLoopback<float, int16_t>(SOAPY_SDR_F32, SOAPY_SDR_S16, 1.0);
    testConverterLoopback<float, int32_t>(SOAPY_SDR_F32, SOAPY_SDR_S32, 1.0);

    return EXIT_SUCCESS;
}