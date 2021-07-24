// Copyright (c) 2019-2021 Nicholas Corgan
// SPDX-License-Identifier: GPL-3.0

#include "TestUtility.hpp"

#include <SoapySDR/ConverterRegistry.hpp>
#include <SoapySDR/Formats.hpp>

#include <volk/volk_alloc.hh>

#include <cstdint>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>

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

template <typename T>
TestUtility::EnableIfNotComplex<T, void> testOutputs(
    const volk::vector<T>& vec0,
    const volk::vector<T>& vec1)
{
    T median(0);
    T medAbsDev(0);
    TestUtility::averageValues(
        vec0,
        vec1,
        median,
        medAbsDev);

    std::cout << " * Average diff: " << double(median) << " +- " << double(medAbsDev) << std::endl;
}

template <typename T>
TestUtility::EnableIfComplex<T, void> testOutputs(
    const volk::vector<T>& vec0,
    const volk::vector<T>& vec1)
{
    using ScalarT = typename T::value_type;

    ScalarT median(0);
    ScalarT medAbsDev(0);
    TestUtility::averageValues(
        vec0,
        vec1,
        median,
        medAbsDev);

    std::cout << " * Average complex diff: " << double(median) << " +- " << double(medAbsDev) << std::endl;
}

template <typename InType, typename OutType>
bool testConverterLoopback(
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

    const volk::vector<InType> testValues = TestUtility::getRandomValues<InType>(numElements);
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
    if (!TestUtility::loadSoapyVOLK()) return EXIT_FAILURE;

    // int8_t
    testConverterLoopback<int8_t, int16_t>(
        SOAPY_SDR_S8,
        SOAPY_SDR_S16,
        1.0); // No scaling
    testConverterLoopback<int8_t, float>(
        SOAPY_SDR_S8,
        SOAPY_SDR_F32,
        TestUtility::S8ToF32Scalar);
    testConverterLoopback<int8_t, double>(
        SOAPY_SDR_S8,
        SOAPY_SDR_F64,
        TestUtility::S8ToF32Scalar);

    // int16_t
    testConverterLoopback<int16_t, int8_t>(
        SOAPY_SDR_S16,
        SOAPY_SDR_S8,
        1.0); // No scaling
    testConverterLoopback<int16_t, float>(
        SOAPY_SDR_S16,
        SOAPY_SDR_F32,
        TestUtility::S16ToF32Scalar);

    // int32_t
    testConverterLoopback<int32_t, float>(
        SOAPY_SDR_S32,
        SOAPY_SDR_F32,
        TestUtility::S32ToF32Scalar);

    // float
    testConverterLoopback<float, int8_t>(
        SOAPY_SDR_F32,
        SOAPY_SDR_S8,
        TestUtility::F32ToS8Scalar);
    testConverterLoopback<float, int16_t>(
        SOAPY_SDR_F32,
        SOAPY_SDR_S16,
        TestUtility::F32ToS16Scalar);
    testConverterLoopback<float, int32_t>(
        SOAPY_SDR_F32,
        SOAPY_SDR_S32,
        TestUtility::F32ToS32Scalar);
    testConverterLoopback<float, float>(
        SOAPY_SDR_F32,
        SOAPY_SDR_F32,
        10.0);
    testConverterLoopback<float, double>(
        SOAPY_SDR_F32,
        SOAPY_SDR_F64,
        10.0);

    // double
    testConverterLoopback<double, int8_t>(
        SOAPY_SDR_F64,
        SOAPY_SDR_S8,
        TestUtility::F32ToS8Scalar);
    testConverterLoopback<double, int16_t>(
        SOAPY_SDR_F64,
        SOAPY_SDR_S16,
        TestUtility::F32ToS16Scalar);
    testConverterLoopback<double, int32_t>(
        SOAPY_SDR_F64,
        SOAPY_SDR_S32,
        TestUtility::F32ToS32Scalar);
    testConverterLoopback<double, float>(
        SOAPY_SDR_F64,
        SOAPY_SDR_F32,
        10.0);

    // std::complex<int8_t>
    testConverterLoopback<std::complex<int8_t>, std::complex<int16_t>>(
        SOAPY_SDR_CS8,
        SOAPY_SDR_CS16,
        1.0); // No scaling
    testConverterLoopback<std::complex<int8_t>, std::complex<float>>(
        SOAPY_SDR_CS8,
        SOAPY_SDR_CF32,
        TestUtility::S8ToF32Scalar);
    testConverterLoopback<std::complex<int8_t>, std::complex<double>>(
        SOAPY_SDR_CS8,
        SOAPY_SDR_CF64,
        TestUtility::S8ToF32Scalar);

    // std::complex<int16_t>
    testConverterLoopback<std::complex<int16_t>, std::complex<int8_t>>(
        SOAPY_SDR_CS16,
        SOAPY_SDR_CS8,
        1.0); // No scaling
    testConverterLoopback<std::complex<int16_t>, std::complex<float>>(
        SOAPY_SDR_CS16,
        SOAPY_SDR_CF32,
        TestUtility::S16ToF32Scalar);
    testConverterLoopback<std::complex<int16_t>, std::complex<double>>(
        SOAPY_SDR_CS16,
        SOAPY_SDR_CF64,
        TestUtility::S16ToF32Scalar);

    // std::complex<int32_t>
    testConverterLoopback<std::complex<int32_t>, std::complex<float>>(
        SOAPY_SDR_CS32,
        SOAPY_SDR_CF32,
        TestUtility::S32ToF32Scalar);

    // std::complex<float>
    testConverterLoopback<std::complex<float>, std::complex<int8_t>>(
        SOAPY_SDR_CF32,
        SOAPY_SDR_CS8,
        TestUtility::F32ToS8Scalar);
    testConverterLoopback<std::complex<float>, std::complex<int16_t>>(
        SOAPY_SDR_CF32,
        SOAPY_SDR_CS16,
        TestUtility::F32ToS16Scalar);
    testConverterLoopback<std::complex<float>, std::complex<int32_t>>(
        SOAPY_SDR_CF32,
        SOAPY_SDR_CS32,
        TestUtility::F32ToS32Scalar);
    testConverterLoopback<std::complex<float>, std::complex<float>>(
        SOAPY_SDR_CF32,
        SOAPY_SDR_CF32,
        10.0);
    testConverterLoopback<std::complex<float>, std::complex<double>>(
        SOAPY_SDR_CF32,
        SOAPY_SDR_CF64,
        10.0);

    // std::complex<double>
    testConverterLoopback<std::complex<double>, std::complex<int8_t>>(
        SOAPY_SDR_CF64,
        SOAPY_SDR_CS8,
        TestUtility::F32ToS8Scalar);
    testConverterLoopback<std::complex<double>, std::complex<int16_t>>(
        SOAPY_SDR_CF64,
        SOAPY_SDR_CS16,
        TestUtility::F32ToS16Scalar);
    testConverterLoopback<std::complex<double>, std::complex<int32_t>>(
        SOAPY_SDR_CF64,
        SOAPY_SDR_CS32,
        TestUtility::F32ToS32Scalar);
    testConverterLoopback<std::complex<double>, std::complex<float>>(
        SOAPY_SDR_CF64,
        SOAPY_SDR_CF32,
        10.0);

    std::cout << "-----" << std::endl;

    return EXIT_SUCCESS;
}