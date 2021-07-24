// Copyright (c) 2021 Nicholas Corgan
// SPDX-License-Identifier: GPL-3.0

#pragma once

#include <volk/volk_alloc.hh>

#include <cmath>
#include <complex>
#include <random>
#include <stdexcept>
#include <type_traits>

namespace TestUtility
{
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

    bool loadSoapyVOLK();

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
}