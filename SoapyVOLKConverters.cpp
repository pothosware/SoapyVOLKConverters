// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: GPL-3.0

/***********************************************************************
 * A Soapy module that adds type converters implemented in VOLK
 **********************************************************************/

#include <SoapySDR/ConverterRegistry.hpp>

#include <volk/volk.h>

#include <vector>

/*
 * S16 -> S8
 */
static void volkS16S8(const void* srcBuff, void* dstBuff, const size_t numElems, const double /*scaler*/)
{
    volk_16i_convert_8i(
        reinterpret_cast<int8_t*>(dstBuff),
        reinterpret_cast<const int16_t*>(srcBuff),
        numElems);
}

/*
 * S16 -> F32
 */
static void volkS16F32(const void* srcBuff, void* dstBuff, const size_t numElems, const double scaler)
{
    volk_16i_s32f_convert_32f(
        reinterpret_cast<float*>(dstBuff),
        reinterpret_cast<const int16_t*>(srcBuff),
        static_cast<float>(scaler),
        numElems);
}

/*
 * F32 -> F64
 */
static void volkF32F64(const void* srcBuff, void* dstBuff, const size_t numElems, const double /*scaler*/)
{
    volk_32f_convert_64f(
        reinterpret_cast<double*>(dstBuff),
        reinterpret_cast<const float*>(srcBuff),
        numElems);
}

/*
 * F32 -> S16
 */
static void volkF32S16(const void* srcBuff, void* dstBuff, const size_t numElems, const double scaler)
{
    volk_32f_s32f_convert_16i(
        reinterpret_cast<int16_t*>(dstBuff),
        reinterpret_cast<const float*>(srcBuff),
        static_cast<float>(scaler),
        numElems);
}

/*
 * CS16 -> CF32
 */
static void volkCS16CF32(const void* srcBuff, void* dstBuff, const size_t numElems, const double /*scaler*/)
{
    volk_16ic_convert_32fc(
        reinterpret_cast<lv_32fc_t*>(dstBuff),
        reinterpret_cast<const lv_16sc_t*>(srcBuff),
        numElems);
}

/*
 * CF32 -> CS16
 */
static void volkCF32CS16(const void* srcBuff, void* dstBuff, const size_t numElems, const double /*scaler*/)
{
    volk_32fc_convert_16ic(
        reinterpret_cast<lv_16sc_t*>(dstBuff),
        reinterpret_cast<const lv_32fc_t*>(srcBuff),
        numElems);
}

static const std::vector<SoapySDR::ConverterRegistry> VolkConverters =
{
    SoapySDR::ConverterRegistry(SOAPY_SDR_S16, SOAPY_SDR_S8, SoapySDR::ConverterRegistry::VECTORIZED, &volkS16S8),
    SoapySDR::ConverterRegistry(SOAPY_SDR_S16, SOAPY_SDR_F32, SoapySDR::ConverterRegistry::VECTORIZED, &volkS16F32),
    SoapySDR::ConverterRegistry(SOAPY_SDR_F32, SOAPY_SDR_S16, SoapySDR::ConverterRegistry::VECTORIZED, &volkF32S16),
    SoapySDR::ConverterRegistry(SOAPY_SDR_F32, SOAPY_SDR_F64, SoapySDR::ConverterRegistry::VECTORIZED, &volkF32F64),
    SoapySDR::ConverterRegistry(SOAPY_SDR_CS16, SOAPY_SDR_CF32, SoapySDR::ConverterRegistry::VECTORIZED, &volkCS16CF32),
    SoapySDR::ConverterRegistry(SOAPY_SDR_CF32, SOAPY_SDR_CS16, SoapySDR::ConverterRegistry::VECTORIZED, &volkCF32CS16),
};
