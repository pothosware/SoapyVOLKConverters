// Copyright (c) 2019-2021 Nicholas Corgan
// SPDX-License-Identifier: GPL-3.0

/***********************************************************************
 * A Soapy module that adds type converters implemented in VOLK
 **********************************************************************/

#include <SoapySDR/ConverterRegistry.hpp>

#include <volk/volk_alloc.hh>
#include <volk/volk.h>

//
// int8_t
//

static SoapySDR::ConverterRegistry registerS8ToS16(
    SOAPY_SDR_S8,
    SOAPY_SDR_S16,
    SoapySDR::ConverterRegistry::VECTORIZED,
    [](const void* srcBuff, void* dstBuff, const size_t numElems, const double)
    {
        volk_8i_convert_16i(
            reinterpret_cast<int16_t*>(dstBuff),
            reinterpret_cast<const int8_t*>(srcBuff),
            static_cast<unsigned int>(numElems));
    });

static SoapySDR::ConverterRegistry registerS8ToF32(
    SOAPY_SDR_S8,
    SOAPY_SDR_F32,
    SoapySDR::ConverterRegistry::VECTORIZED,
    [](const void* srcBuff, void* dstBuff, const size_t numElems, const double scalar)
    {
        volk_8i_s32f_convert_32f(
            reinterpret_cast<float*>(dstBuff),
            reinterpret_cast<const int8_t*>(srcBuff),
            static_cast<float>(1.0 / scalar),
            static_cast<unsigned int>(numElems));
    });

static SoapySDR::ConverterRegistry registerS8ToF64(
    SOAPY_SDR_S8,
    SOAPY_SDR_F64,
    SoapySDR::ConverterRegistry::VECTORIZED,
    [](const void* srcBuff, void* dstBuff, const size_t numElems, const double scalar)
    {
        volk::vector<float> intermediate(numElems);
        volk_8i_s32f_convert_32f(
            intermediate.data(),
            reinterpret_cast<const int8_t*>(srcBuff),
            static_cast<float>(1.0 / scalar),
            static_cast<unsigned int>(numElems));

        volk_32f_convert_64f(
            reinterpret_cast<double*>(dstBuff),
            intermediate.data(),
            static_cast<unsigned int>(numElems));
    });

//
// int16_t
//

static SoapySDR::ConverterRegistry registerS16ToS8(
    SOAPY_SDR_S16,
    SOAPY_SDR_S8,
    SoapySDR::ConverterRegistry::VECTORIZED,
    [](const void* srcBuff, void* dstBuff, const size_t numElems, const double)
    {
        volk_16i_convert_8i(
            reinterpret_cast<int8_t*>(dstBuff),
            reinterpret_cast<const int16_t*>(srcBuff),
            static_cast<unsigned int>(numElems));
    });

static SoapySDR::ConverterRegistry registerS16ToF32(
    SOAPY_SDR_S16,
    SOAPY_SDR_F32,
    SoapySDR::ConverterRegistry::VECTORIZED,
    [](const void* srcBuff, void* dstBuff, const size_t numElems, const double scalar)
    {
        volk_16i_s32f_convert_32f(
            reinterpret_cast<float*>(dstBuff),
            reinterpret_cast<const int16_t*>(srcBuff),
            static_cast<float>(1.0 / scalar),
            static_cast<unsigned int>(numElems));
    });

static SoapySDR::ConverterRegistry registerS16ToF64(
    SOAPY_SDR_S16,
    SOAPY_SDR_F64,
    SoapySDR::ConverterRegistry::VECTORIZED,
    [](const void* srcBuff, void* dstBuff, const size_t numElems, const double scalar)
    {
        volk::vector<float> intermediate(numElems);
        volk_16i_s32f_convert_32f(
            intermediate.data(),
            reinterpret_cast<const int16_t*>(srcBuff),
            static_cast<float>(1.0 / scalar),
            static_cast<unsigned int>(numElems));

        volk_32f_convert_64f(
            reinterpret_cast<double*>(dstBuff),
            intermediate.data(),
            static_cast<unsigned int>(numElems));
    });

//
// int32_t
//

static SoapySDR::ConverterRegistry registerS32ToF32(
    SOAPY_SDR_S32,
    SOAPY_SDR_F32,
    SoapySDR::ConverterRegistry::VECTORIZED,
    [](const void* srcBuff, void* dstBuff, const size_t numElems, const double scalar)
    {
        volk_32i_s32f_convert_32f(
            reinterpret_cast<float*>(dstBuff),
            reinterpret_cast<const int32_t*>(srcBuff),
            static_cast<float>(1.0 / scalar),
            static_cast<unsigned int>(numElems));
    });

static SoapySDR::ConverterRegistry registerS32ToF64(
    SOAPY_SDR_S32,
    SOAPY_SDR_F64,
    SoapySDR::ConverterRegistry::VECTORIZED,
    [](const void* srcBuff, void* dstBuff, const size_t numElems, const double scalar)
    {
        volk::vector<float> intermediate(numElems);
        volk_32i_s32f_convert_32f(
            intermediate.data(),
            reinterpret_cast<const int32_t*>(srcBuff),
            static_cast<float>(1.0 / scalar),
            static_cast<unsigned int>(numElems));

        volk_32f_convert_64f(
            reinterpret_cast<double*>(dstBuff),
            intermediate.data(),
            static_cast<unsigned int>(numElems));
    });

//
// float
//

static SoapySDR::ConverterRegistry registerF32ToS8(
    SOAPY_SDR_F32,
    SOAPY_SDR_S8,
    SoapySDR::ConverterRegistry::VECTORIZED,
    [](const void* srcBuff, void* dstBuff, const size_t numElems, const double scalar)
    {
        volk_32f_s32f_convert_8i(
            reinterpret_cast<int8_t*>(dstBuff),
            reinterpret_cast<const float*>(srcBuff),
            static_cast<float>(scalar),
            static_cast<unsigned int>(numElems));
    });

static SoapySDR::ConverterRegistry registerF32ToS16(
    SOAPY_SDR_F32,
    SOAPY_SDR_S16,
    SoapySDR::ConverterRegistry::VECTORIZED,
    [](const void* srcBuff, void* dstBuff, const size_t numElems, const double scalar)
    {
        volk_32f_s32f_convert_16i(
            reinterpret_cast<int16_t*>(dstBuff),
            reinterpret_cast<const float*>(srcBuff),
            static_cast<float>(scalar),
            static_cast<unsigned int>(numElems));
    });

static SoapySDR::ConverterRegistry registerF32ToS32(
    SOAPY_SDR_F32,
    SOAPY_SDR_S32,
    SoapySDR::ConverterRegistry::VECTORIZED,
    [](const void* srcBuff, void* dstBuff, const size_t numElems, const double scalar)
    {
        volk_32f_s32f_convert_32i(
            reinterpret_cast<int32_t*>(dstBuff),
            reinterpret_cast<const float*>(srcBuff),
            static_cast<float>(scalar),
            static_cast<unsigned int>(numElems));
    });

static SoapySDR::ConverterRegistry registerF32ToF32(
    SOAPY_SDR_F32,
    SOAPY_SDR_F32,
    SoapySDR::ConverterRegistry::VECTORIZED,
    [](const void* srcBuff, void* dstBuff, const size_t numElems, const double scalar)
    {
        volk_32f_s32f_multiply_32f(
            reinterpret_cast<float*>(dstBuff),
            reinterpret_cast<const float*>(srcBuff),
            static_cast<float>(scalar),
            static_cast<unsigned int>(numElems));
    });

static SoapySDR::ConverterRegistry registerF32ToF64(
    SOAPY_SDR_F32,
    SOAPY_SDR_F64,
    SoapySDR::ConverterRegistry::VECTORIZED,
    [](const void* srcBuff, void* dstBuff, const size_t numElems, const double scalar)
    {
        volk::vector<float> scaled(numElems);
        volk_32f_s32f_multiply_32f(
            scaled.data(),
            (const float*)srcBuff,
            static_cast<float>(scalar),
            static_cast<unsigned int>(numElems));

        volk_32f_convert_64f(
            (double*)dstBuff,
            scaled.data(),
            static_cast<unsigned int>(numElems));
    });

//
// double
//

static SoapySDR::ConverterRegistry registerF64ToS8(
    SOAPY_SDR_F64,
    SOAPY_SDR_S8,
    SoapySDR::ConverterRegistry::VECTORIZED,
    [](const void* srcBuff, void* dstBuff, const size_t numElems, const double scalar)
    {
        volk::vector<float> intermediate(numElems);
        volk_64f_convert_32f(
            intermediate.data(),
            reinterpret_cast<const double*>(srcBuff),
            static_cast<unsigned int>(numElems));

        volk_32f_s32f_convert_8i(
            reinterpret_cast<int8_t*>(dstBuff),
            intermediate.data(),
            static_cast<float>(scalar),
            static_cast<unsigned int>(numElems));
    });

static SoapySDR::ConverterRegistry registerF64ToS16(
    SOAPY_SDR_F64,
    SOAPY_SDR_S16,
    SoapySDR::ConverterRegistry::VECTORIZED,
    [](const void* srcBuff, void* dstBuff, const size_t numElems, const double scalar)
    {
        volk::vector<float> intermediate(numElems);
        volk_64f_convert_32f(
            intermediate.data(),
            reinterpret_cast<const double*>(srcBuff),
            static_cast<unsigned int>(numElems));

        volk_32f_s32f_convert_16i(
            reinterpret_cast<int16_t*>(dstBuff),
            intermediate.data(),
            static_cast<float>(scalar),
            static_cast<unsigned int>(numElems));
    });

static SoapySDR::ConverterRegistry registerF64ToS32(
    SOAPY_SDR_F64,
    SOAPY_SDR_S32,
    SoapySDR::ConverterRegistry::VECTORIZED,
    [](const void* srcBuff, void* dstBuff, const size_t numElems, const double scalar)
    {
        volk::vector<float> intermediate(numElems);
        volk_64f_convert_32f(
            intermediate.data(),
            reinterpret_cast<const double*>(srcBuff),
            static_cast<unsigned int>(numElems));

        volk_32f_s32f_convert_32i(
            reinterpret_cast<int32_t*>(dstBuff),
            intermediate.data(),
            static_cast<float>(scalar),
            static_cast<unsigned int>(numElems));
    });

static SoapySDR::ConverterRegistry registerF64ToF32(
    SOAPY_SDR_F64,
    SOAPY_SDR_F32,
    SoapySDR::ConverterRegistry::VECTORIZED,
    [](const void* srcBuff, void* dstBuff, const size_t numElems, const double scalar)
    {
        volk::vector<float> unscaled(numElems);
        volk_64f_convert_32f(
            unscaled.data(),
            (const double*)srcBuff,
            static_cast<unsigned int>(numElems));

        volk_32f_s32f_multiply_32f(
            (float*)dstBuff,
            unscaled.data(),
            static_cast<float>(scalar),
            static_cast<unsigned int>(numElems));
    });

//
// std::complex<int16_t>
//

static void convertCS16ToCF32(const void* srcBuff, void* dstBuff, const size_t numElems, const double scalar)
{
    volk::vector<lv_32fc_t> unscaled(numElems);
    volk_16ic_convert_32fc(
        unscaled.data(),
        (const lv_16sc_t*)srcBuff,
        static_cast<unsigned int>(numElems));

    const lv_32fc_t complexScalar(scalar);
    volk_32fc_s32fc_multiply_32fc(
        (lv_32fc_t*)dstBuff,
        unscaled.data(),
        complexScalar,
        static_cast<unsigned int>(numElems));
}

static SoapySDR::ConverterRegistry registerCS16ToCF32(
    SOAPY_SDR_CS16,
    SOAPY_SDR_CF32,
    SoapySDR::ConverterRegistry::VECTORIZED,
    &convertCS16ToCF32);

static SoapySDR::ConverterRegistry registerCS16ToCF64(
    SOAPY_SDR_CS16,
    SOAPY_SDR_CF64,
    SoapySDR::ConverterRegistry::VECTORIZED,
    [](const void* srcBuff, void* dstBuff, const size_t numElems, const double scalar)
    {
        volk::vector<float> intermediate(numElems * 2);
        convertCS16ToCF32(srcBuff, intermediate.data(), numElems, scalar);

        volk_32f_convert_64f(
            reinterpret_cast<double*>(dstBuff),
            intermediate.data(),
            static_cast<unsigned int>(numElems * 2));
    });

//
// std::complex<int8_t>
//

static SoapySDR::ConverterRegistry registerCS8ToCS16(
    SOAPY_SDR_CS8,
    SOAPY_SDR_CS16,
    SoapySDR::ConverterRegistry::VECTORIZED,
    [](const void* srcBuff, void* dstBuff, const size_t numElems, const double)
    {
        volk_8i_convert_16i(
            reinterpret_cast<int16_t*>(dstBuff),
            reinterpret_cast<const int8_t*>(srcBuff),
            static_cast<unsigned int>(numElems * 2));
    });

//
// std::complex<int16_t>
//

static SoapySDR::ConverterRegistry registerCS16ToCS8(
    SOAPY_SDR_CS16,
    SOAPY_SDR_CS8,
    SoapySDR::ConverterRegistry::VECTORIZED,
    [](const void* srcBuff, void* dstBuff, const size_t numElems, const double)
    {
        volk_16i_convert_8i(
            reinterpret_cast<int8_t*>(dstBuff),
            reinterpret_cast<const int16_t*>(srcBuff),
            static_cast<unsigned int>(numElems * 2));
    });

//
// std::complex<float>
//

static void convertCF32ToCS16(const void* srcBuff, void* dstBuff, const size_t numElems, const double scalar)
{
    const lv_32fc_t complexScalar(scalar);
    volk::vector<lv_32fc_t> scaled(numElems);
    volk_32fc_s32fc_multiply_32fc(
        scaled.data(),
        (const lv_32fc_t*)srcBuff,
        complexScalar,
        static_cast<unsigned int>(numElems));

    volk_32fc_convert_16ic(
        (lv_16sc_t*)dstBuff,
        scaled.data(),
        static_cast<unsigned int>(numElems));
}

static SoapySDR::ConverterRegistry registerCF32ToCS16(
    SOAPY_SDR_CF32,
    SOAPY_SDR_CS16,
    SoapySDR::ConverterRegistry::VECTORIZED,
    &convertCF32ToCS16);

//
// std::complex<double>
//

static SoapySDR::ConverterRegistry registerCF64toCS16(
    SOAPY_SDR_CF64,
    SOAPY_SDR_CS16,
    SoapySDR::ConverterRegistry::VECTORIZED,
    [](const void* srcBuff, void* dstBuff, const size_t numElems, const double scalar)
    {
        volk::vector<float> intermediate(numElems * 2);
        volk_64f_convert_32f(
            intermediate.data(),
            reinterpret_cast<const double*>(srcBuff),
            static_cast<unsigned int>(numElems * 2));

        convertCF32ToCS16(intermediate.data(), dstBuff, numElems, scalar);
    });