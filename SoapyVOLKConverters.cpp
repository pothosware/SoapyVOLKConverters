// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: GPL-3.0

/***********************************************************************
 * A Soapy module that adds type converters implemented in VOLK
 **********************************************************************/

#include <SoapySDR/ConverterRegistry.hpp>

#include <volk/volk.h>

#include <vector>

#define VOLK_CONVERTER(srcType, dstType, srcFormat, dstFormat, volkFunc) \
    static void func_ ## srcType ## _to_ ## dstType( \
        const void* srcBuff, \
        void* dstBuff, \
        const size_t numElems, \
        const double) \
    { \
        volkFunc( \
            reinterpret_cast<dstType*>(dstBuff), \
            reinterpret_cast<const srcType*>(srcBuff), \
            numElems); \
    } \
    static SoapySDR::ConverterRegistry register_ ## srcType ## _to_ ## dstType ( \
        srcFormat, \
        dstFormat, \
        SoapySDR::ConverterRegistry::VECTORIZED, \
        &func_ ## srcType ## _to_ ## dstType);

#define VOLK_CONVERTER_SCALED(srcType, dstType, srcFormat, dstFormat, volkFunc) \
    static void func_ ## srcType ## _to_ ## dstType( \
        const void* srcBuff, \
        void* dstBuff, \
        const size_t numElems, \
        const double scalar) \
    { \
        volkFunc( \
            reinterpret_cast<dstType*>(dstBuff), \
            reinterpret_cast<const srcType*>(srcBuff), \
            static_cast<float>(scalar), \
            numElems); \
    } \
    static SoapySDR::ConverterRegistry register_ ## srcType ## _to_ ## dstType ( \
        srcFormat, \
        dstFormat, \
        SoapySDR::ConverterRegistry::VECTORIZED, \
        &func_ ## srcType ## _to_ ## dstType);

VOLK_CONVERTER(lv_16sc_t, lv_32fc_t, SOAPY_SDR_CS16, SOAPY_SDR_CF32, volk_16ic_convert_32fc)
VOLK_CONVERTER(int16_t, int8_t, SOAPY_SDR_S16, SOAPY_SDR_S8, volk_16i_convert_8i)
VOLK_CONVERTER_SCALED(int16_t, float, SOAPY_SDR_S16, SOAPY_SDR_F32, volk_16i_s32f_convert_32f)
VOLK_CONVERTER(lv_32fc_t, lv_16sc_t, SOAPY_SDR_CF32, SOAPY_SDR_CS16, volk_32fc_convert_16ic)
VOLK_CONVERTER(float, double, SOAPY_SDR_F32, SOAPY_SDR_F64, volk_32f_convert_64f)
VOLK_CONVERTER_SCALED(float, int16_t, SOAPY_SDR_F32, SOAPY_SDR_S16, volk_32f_s32f_convert_16i)
VOLK_CONVERTER_SCALED(float, int32_t, SOAPY_SDR_F32, SOAPY_SDR_S32, volk_32f_s32f_convert_32i)
VOLK_CONVERTER_SCALED(float, int8_t, SOAPY_SDR_F32, SOAPY_SDR_S8, volk_32f_s32f_convert_8i)
VOLK_CONVERTER_SCALED(int32_t, float, SOAPY_SDR_S32, SOAPY_SDR_F32, volk_32i_s32f_convert_32f)
VOLK_CONVERTER(double, float, SOAPY_SDR_F64, SOAPY_SDR_F32, volk_64f_convert_32f)
VOLK_CONVERTER(int8_t, int16_t, SOAPY_SDR_S8, SOAPY_SDR_S16, volk_8i_convert_16i)
VOLK_CONVERTER_SCALED(int8_t, float, SOAPY_SDR_S8, SOAPY_SDR_F32, volk_8i_s32f_convert_32f)
