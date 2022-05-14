# Soapy SDR VOLK-based type converters

This Soapy module adds vectorized numeric conversion functions, providing more efficient conversions
in many cases than with stock SoapySDR. Once this module is installed, nothing more needs to be done;
Soapy modules that use SoapySDR's converter infrastructure will automatically default to these new
converters.

## Build Status

![Build Status](https://github.com/pothosware/SoapyVOLKConverters/actions/workflows/ci.yml/badge.svg)

## Dependencies

* C++14-compatible compiler
* VOLK - https://github.com/gnuradio/volk
* SoapySDR (0.7+) - https://github.com/pothosware/SoapySDR/wiki

## Licensing information

* GPLv3: http://www.gnu.org/licenses/gpl-3.0.html