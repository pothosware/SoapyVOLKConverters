// Copyright (c) 2019-2021 Nicholas Corgan
// SPDX-License-Identifier: GPL-3.0

/***********************************************************************
 * A Soapy module that adds type converters implemented in VOLK
 **********************************************************************/

#include <SoapySDR/ConverterRegistry.hpp>
#include <SoapySDR/Logger.hpp>

#include <volk/volk.h>
#include <volk/volk_alloc.hh>
#include <volk/volk_prefs.h>

//
// Initialization
//

struct ModuleInit
{
    ModuleInit()
    {
        constexpr size_t VOLKPathSize = 512;
        char path[VOLKPathSize] = {0};

        volk_get_config_path(path, true);
        if(path[0] == 0)
        {
            SoapySDR::log(
                SOAPY_SDR_WARNING,
                "SoapyVOLKConverters: no VOLK config file found. Run volk_profile for best performance.");
        }
    }
};

static const ModuleInit Init;

//
// Common code
//

static void convertS8ToF64(const void* srcBuff, void* dstBuff, const size_t numElems, const double scalar)
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
}

static void convertS16ToF64(const void* srcBuff, void* dstBuff, const size_t numElems, const double scalar)
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
}

static void convertS32ToF64(const void* srcBuff, void* dstBuff, const size_t numElems, const double scalar)
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
}

static void convertF32ToF64(const void* srcBuff, void* dstBuff, const size_t numElems, const double scalar)
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
}

static void convertF64ToS8(const void* srcBuff, void* dstBuff, const size_t numElems, const double scalar)
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
}

static void convertF64ToS16(const void* srcBuff, void* dstBuff, const size_t numElems, const double scalar)
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
}

static void convertF64ToS32(const void* srcBuff, void* dstBuff, const size_t numElems, const double scalar)
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
}

static void convertF64ToF32(const void* srcBuff, void* dstBuff, const size_t numElems, const double scalar)
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
}

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
    &convertS8ToF64);

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
    &convertS16ToF64);

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
    &convertS32ToF64);

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
    &convertF32ToF64);

//
// double
//

static SoapySDR::ConverterRegistry registerF64ToS8(
    SOAPY_SDR_F64,
    SOAPY_SDR_S8,
    SoapySDR::ConverterRegistry::VECTORIZED,
    &convertF64ToS8);

static SoapySDR::ConverterRegistry registerF64ToS16(
    SOAPY_SDR_F64,
    SOAPY_SDR_S16,
    SoapySDR::ConverterRegistry::VECTORIZED,
    &convertF64ToS16);

static SoapySDR::ConverterRegistry registerF64ToS32(
    SOAPY_SDR_F64,
    SOAPY_SDR_S32,
    SoapySDR::ConverterRegistry::VECTORIZED,
    &convertF64ToS32);

static SoapySDR::ConverterRegistry registerF64ToF32(
    SOAPY_SDR_F64,
    SOAPY_SDR_F32,
    SoapySDR::ConverterRegistry::VECTORIZED,
    &convertF64ToF32);

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

static SoapySDR::ConverterRegistry registerCS8ToCF32(
    SOAPY_SDR_CS8,
    SOAPY_SDR_CF32,
    SoapySDR::ConverterRegistry::VECTORIZED,
    [](const void* srcBuff, void* dstBuff, const size_t numElems, const double scalar)
    {
        volk_8i_s32f_convert_32f(
            reinterpret_cast<float*>(dstBuff),
            reinterpret_cast<const int8_t*>(srcBuff),
            static_cast<float>(1.0 / scalar),
            static_cast<unsigned int>(numElems * 2));
    });

static SoapySDR::ConverterRegistry registerCS8ToCF64(
    SOAPY_SDR_CS8,
    SOAPY_SDR_CF64,
    SoapySDR::ConverterRegistry::VECTORIZED,
    [](const void* srcBuff, void* dstBuff, const size_t numElems, const double scalar)
    {
        convertS8ToF64(srcBuff, dstBuff, (numElems * 2), scalar);
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

static SoapySDR::ConverterRegistry registerCS16ToCF32(
    SOAPY_SDR_CS16,
    SOAPY_SDR_CF32,
    SoapySDR::ConverterRegistry::VECTORIZED,
    [](const void* srcBuff, void* dstBuff, const size_t numElems, const double scalar)
    {
        volk_16i_s32f_convert_32f(
            reinterpret_cast<float*>(dstBuff),
            reinterpret_cast<const int16_t*>(srcBuff),
            static_cast<float>(1.0 / scalar),
            static_cast<unsigned int>(numElems * 2));
    });

static SoapySDR::ConverterRegistry registerCS16ToCF64(
    SOAPY_SDR_CS16,
    SOAPY_SDR_CF64,
    SoapySDR::ConverterRegistry::VECTORIZED,
    [](const void* srcBuff, void* dstBuff, const size_t numElems, const double scalar)
    {
        convertS16ToF64(srcBuff, dstBuff, (numElems * 2), scalar);
    });

//
// std::complex<int32_t>
//

static SoapySDR::ConverterRegistry registerCS32ToCF32(
    SOAPY_SDR_CS32,
    SOAPY_SDR_CF32,
    SoapySDR::ConverterRegistry::VECTORIZED,
    [](const void* srcBuff, void* dstBuff, const size_t numElems, const double scalar)
    {
        volk_32i_s32f_convert_32f(
            reinterpret_cast<float*>(dstBuff),
            reinterpret_cast<const int32_t*>(srcBuff),
            static_cast<float>(1.0 / scalar),
            static_cast<unsigned int>(numElems * 2));
    });

static SoapySDR::ConverterRegistry registerCS32ToCF64(
    SOAPY_SDR_CS32,
    SOAPY_SDR_CF64,
    SoapySDR::ConverterRegistry::VECTORIZED,
    [](const void* srcBuff, void* dstBuff, const size_t numElems, const double scalar)
    {
        convertS32ToF64(srcBuff, dstBuff, (numElems * 2), scalar);
    });

//
// std::complex<float>
//

static SoapySDR::ConverterRegistry registerCF32ToCS8(
    SOAPY_SDR_CF32,
    SOAPY_SDR_CS8,
    SoapySDR::ConverterRegistry::VECTORIZED,
    [](const void* srcBuff, void* dstBuff, const size_t numElems, const double scalar)
    {
        volk_32f_s32f_convert_8i(
            reinterpret_cast<int8_t*>(dstBuff),
            reinterpret_cast<const float*>(srcBuff),
            static_cast<float>(scalar),
            static_cast<unsigned int>(numElems * 2));
    });

static SoapySDR::ConverterRegistry registerCF32ToCS16(
    SOAPY_SDR_CF32,
    SOAPY_SDR_CS16,
    SoapySDR::ConverterRegistry::VECTORIZED,
    [](const void* srcBuff, void* dstBuff, const size_t numElems, const double scalar)
    {
        volk_32f_s32f_convert_16i(
            reinterpret_cast<int16_t*>(dstBuff),
            reinterpret_cast<const float*>(srcBuff),
            static_cast<float>(scalar),
            static_cast<unsigned int>(numElems * 2));
    });

static SoapySDR::ConverterRegistry registerCF32ToCS32(
    SOAPY_SDR_CF32,
    SOAPY_SDR_CS32,
    SoapySDR::ConverterRegistry::VECTORIZED,
    [](const void* srcBuff, void* dstBuff, const size_t numElems, const double scalar)
    {
        volk_32f_s32f_convert_32i(
            reinterpret_cast<int32_t*>(dstBuff),
            reinterpret_cast<const float*>(srcBuff),
            static_cast<float>(scalar),
            static_cast<unsigned int>(numElems * 2));
    });

static SoapySDR::ConverterRegistry registerCF32ToCF32(
    SOAPY_SDR_CF32,
    SOAPY_SDR_CF32,
    SoapySDR::ConverterRegistry::VECTORIZED,
    [](const void* srcBuff, void* dstBuff, const size_t numElems, const double scalar)
    {
        volk_32f_s32f_multiply_32f(
            reinterpret_cast<float*>(dstBuff),
            reinterpret_cast<const float*>(srcBuff),
            static_cast<float>(scalar),
            static_cast<unsigned int>(numElems * 2));
    });

static SoapySDR::ConverterRegistry registerCF32ToCF64(
    SOAPY_SDR_CF32,
    SOAPY_SDR_CF64,
    SoapySDR::ConverterRegistry::VECTORIZED,
    [](const void* srcBuff, void* dstBuff, const size_t numElems, const double scalar)
    {
        convertF32ToF64(srcBuff, dstBuff, (numElems * 2), scalar);
    });

//
// std::complex<double>
//

static SoapySDR::ConverterRegistry registerCF64ToCS8(
    SOAPY_SDR_CF64,
    SOAPY_SDR_CS8,
    SoapySDR::ConverterRegistry::VECTORIZED,
    [](const void* srcBuff, void* dstBuff, const size_t numElems, const double scalar)
    {
        convertF64ToS8(srcBuff, dstBuff, (numElems * 2), scalar);
    });

static SoapySDR::ConverterRegistry registerCF64ToCS16(
    SOAPY_SDR_CF64,
    SOAPY_SDR_CS16,
    SoapySDR::ConverterRegistry::VECTORIZED,
    [](const void* srcBuff, void* dstBuff, const size_t numElems, const double scalar)
    {
        convertF64ToS16(srcBuff, dstBuff, (numElems * 2), scalar);
    });

static SoapySDR::ConverterRegistry registerCF64ToCS32(
    SOAPY_SDR_CF64,
    SOAPY_SDR_CS32,
    SoapySDR::ConverterRegistry::VECTORIZED,
    [](const void* srcBuff, void* dstBuff, const size_t numElems, const double scalar)
    {
        convertF64ToS32(srcBuff, dstBuff, (numElems * 2), scalar);
    });

static SoapySDR::ConverterRegistry registerCF64ToF32(
    SOAPY_SDR_CF64,
    SOAPY_SDR_CF32,
    SoapySDR::ConverterRegistry::VECTORIZED,
    [](const void* srcBuff, void* dstBuff, const size_t numElems, const double scalar)
    {
        convertF64ToF32(srcBuff, dstBuff, (numElems * 2), scalar);
    });
