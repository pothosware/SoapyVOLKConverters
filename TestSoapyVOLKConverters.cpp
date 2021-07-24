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
    static std::uniform_int_distribution<int> dist(0, 127);
    return T(dist(gen));
}

template <typename T>
static EnableIfIntegral<T, T> getRandomValue()
{
    static std::uniform_int_distribution<T> dist(T(0), std::numeric_limits<T>::max());
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
static volk::vector<T> getRandomValues(size_t numElements)
{
    volk::vector<T> randomValues;
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
T median(const volk::vector<T>& inputs)
{
    volk::vector<T> sortedInputs(inputs);
    std::sort(sortedInputs.begin(), sortedInputs.end());

    return sortedInputs[sortedInputs.size() / 2];
}

template <typename T>
double medAbsDev(const volk::vector<T>& inputs)
{
    const T med = median(inputs);

    volk::vector<T> diffs;
    std::transform(
        inputs.begin(),
        inputs.end(),
        std::back_inserter(diffs),
        [&med](T val) {return std::abs(val - med); });

    return median(diffs);
}

template <typename T>
static EnableIfNotComplex<T, T> absDiff(const T& num0, const T& num1)
{
    return std::abs(num0 - num1);
}

template <typename T>
static EnableIfComplex<T, typename T::value_type> absDiff(const T& num0, const T& num1)
{
    return std::abs(std::abs(num0) - std::abs(num1));
}

template <typename T>
static void averageValues(
    const volk::vector<T>& vec0,
    const volk::vector<T>& vec1,
    T& medianOut,
    T& medAbsDevOut)
{
    volk::vector<T> diffs(vec0.size());
    for (size_t i = 0; i < vec0.size(); ++i)
    {
        diffs[i] = absDiff(vec0[i], vec1[i]);
    }

    medianOut = median(diffs);
    medAbsDevOut = medAbsDev(diffs);
}

template <typename T>
static void averageValues(
    const volk::vector<std::complex<T>>& vec0,
    const volk::vector<std::complex<T>>& vec1,
    T& medianOut,
    T& medAbsDevOut)
{
    volk::vector<T> diffs(vec0.size());
    for (size_t i = 0; i < vec0.size(); ++i)
    {
        diffs[i] = absDiff(vec0[i], vec1[i]);
    }

    medianOut = median(diffs);
    medAbsDevOut = medAbsDev(diffs);
}

template <typename T>
static inline EnableIfNotComplex<T, void> testOutputs(
    const volk::vector<T>& vec0,
    const volk::vector<T>& vec1)
{
    T median(0);
    T medAbsDev(0);
    averageValues(
        vec0,
        vec1,
        median,
        medAbsDev);

    std::cout << " * Average diff: " << double(median) << " +- " << double(medAbsDev) << std::endl;
}

template <typename T>
static EnableIfComplex<T, void> testOutputs(
    const volk::vector<T>& vec0,
    const volk::vector<T>& vec1)
{
    using ScalarT = typename T::value_type;

    ScalarT median(0);
    ScalarT medAbsDev(0);
    averageValues(
        vec0,
        vec1,
        median,
        medAbsDev);

    std::cout << " * Average complex diff: " << double(median) << " +- " << double(medAbsDev) << std::endl;
}

//
// Test code
//

struct TestConverters
{
    SoapySDR::ConverterRegistry::ConverterFunction convertType1ToType2{ nullptr };
    SoapySDR::ConverterRegistry::ConverterFunction convertType2ToType1{ nullptr };
};

bool getConvertFunctions(
    const std::string& type1,
    const std::string& type2,
    TestConverters& convertersOut)
{
    try
    {
        convertersOut.convertType1ToType2 = SoapySDR::ConverterRegistry::getFunction(
            type1,
            type2,
            SoapySDR::ConverterRegistry::VECTORIZED);

        convertersOut.convertType2ToType1 = SoapySDR::ConverterRegistry::getFunction(
            type2,
            type1,
            SoapySDR::ConverterRegistry::VECTORIZED);
    }
    catch (std::exception& ex)
    {
        std::cerr << " * Exception getting converters: " << ex.what() << std::endl;
        return false;
    }

    return true;
}

template <typename InType, typename OutType>
static bool testConverterLoopback(
    const std::string& type1,
    const std::string& type2,
    const double type1ToType2Scalar)
{
    static constexpr size_t numElements = 1024*8;

    std::cout << "-----" << std::endl;

    std::cout << "Testing " << type1 << " -> " << type2 << " (scaled x" << type1ToType2Scalar
              << ") -> " << type1 << " (scaled x" << (1.0 / type1ToType2Scalar) << ")..." << std::endl;

    TestConverters testConverters;
    if (!getConvertFunctions(type1, type2, testConverters)) return false;

    if (!testConverters.convertType1ToType2) return false;
    if (!testConverters.convertType2ToType1) return false;

    const volk::vector<InType> testValues = getRandomValues<InType>(numElements);
    volk::vector<OutType> convertedValues(numElements);
    volk::vector<InType> loopbackValues(numElements);

    testConverters.convertType1ToType2(
        testValues.data(),
        convertedValues.data(),
        numElements,
        type1ToType2Scalar);
    testConverters.convertType2ToType1(
        convertedValues.data(),
        loopbackValues.data(),
        numElements,
        (1.0 / type1ToType2Scalar));

    testOutputs(testValues, loopbackValues);

    return true;
}

//
// Main
//

int main(int, char**)
{
    if (!loadSoapyVOLK()) return EXIT_FAILURE;

    // Test scalars copied from ConverterPrimitives.hpp
    constexpr uint32_t S32FullScale = uint32_t(1 << 31);
    constexpr uint16_t S16FullScale = uint16_t(1 << 15);
    constexpr uint8_t   S8FullScale = uint8_t(1 << 7);

    constexpr double S8ToF32Scalar = 1.0 / S8FullScale;
    constexpr double S16ToF32Scalar = 1.0 / S16FullScale;
    constexpr double S32ToF32Scalar = 1.0 / S32FullScale;

    constexpr double F32ToS8Scalar = 1.0 / S8ToF32Scalar;
    constexpr double F32ToS16Scalar = 1.0 / S16ToF32Scalar;
    constexpr double F32ToS32Scalar = 1.0 / S32ToF32Scalar;

    testConverterLoopback<int8_t, float>(SOAPY_SDR_S8, SOAPY_SDR_F32, S8ToF32Scalar);
    testConverterLoopback<int16_t, float>(SOAPY_SDR_S16, SOAPY_SDR_F32, S16ToF32Scalar);
    testConverterLoopback<int32_t, float>(SOAPY_SDR_S32, SOAPY_SDR_F32, S32ToF32Scalar);
    testConverterLoopback<float, int8_t>(SOAPY_SDR_F32, SOAPY_SDR_S8, F32ToS8Scalar);
    testConverterLoopback<float, int16_t>(SOAPY_SDR_F32, SOAPY_SDR_S16, F32ToS16Scalar);
    testConverterLoopback<float, int32_t>(SOAPY_SDR_F32, SOAPY_SDR_S32, F32ToS32Scalar);
    testConverterLoopback<float, double>(SOAPY_SDR_F32, SOAPY_SDR_F64, 1.0);
    testConverterLoopback<double, float>(SOAPY_SDR_F64, SOAPY_SDR_F32, 1.0);
    testConverterLoopback<std::complex<int16_t>, std::complex<float>>(SOAPY_SDR_CS16, SOAPY_SDR_CF32, S16ToF32Scalar);
    testConverterLoopback<std::complex<float>, std::complex<int16_t>>(SOAPY_SDR_CF32, SOAPY_SDR_CS16, F32ToS16Scalar);

    // These converters don't support scaling and ignore the parameter
    testConverterLoopback<int8_t, int16_t>(SOAPY_SDR_S8, SOAPY_SDR_S16, 1.0);
    testConverterLoopback<int16_t, int8_t>(SOAPY_SDR_S16, SOAPY_SDR_S8, 1.0);

    std::cout << "-----" << std::endl;

    return EXIT_SUCCESS;
}