//
//  XDDSP_Types.h
//  XDDSP
//
//  Created by Adam Jackson on 25/5/2022.
//

#ifndef XDDSP_Types_h
#define XDDSP_Types_h


#include <vector>
#include <string>
#include <array>
#include <stdexcept>
#include <cmath>
#include <functional>









namespace XDDSP
{

// All samples processed by this library will be this type
#ifdef XDDSP2_SAMPLETYPE
typedef XDDSP2_SAMPLETYPE SampleType;
#else
typedef double SampleType;
#endif









// Helper class to initialise std::array with non-trivial constructors
template <std::size_t ... Is, typename T>
std::array<T, sizeof...(Is)> make_array_impl(const T& def, std::index_sequence<Is...>)
{
 return {{(static_cast<void>(Is), void(), def)...}};
}

template <std::size_t N, typename T>
std::array<T, N> make_array(const T& def)
{
 return make_array_impl(def, std::make_index_sequence<N>());
}










// Template function to construct an array of objects that take a Parameters object
// as the only constructor argument
class Parameters;

template <typename T, std::size_t ... Is>
std::array<T, sizeof...(Is)> make_component_array_impl(Parameters &p, std::index_sequence<Is...>)
{
 return {{(static_cast<void>(Is), void(), T(p))...}};
}

template <std::size_t N, typename T>
std::array<T, N> makeComponentArray(Parameters &p)
{
 return make_component_array_impl<T>(p, std::make_index_sequence<N>());
}










// Standard waveform function
typedef std::function<SampleType (SampleType)> WaveformFunction;










/*
inline void dsp_assert(bool condition)
{
#ifdef DEBUG
 if (!condition)
 {
  throw std::runtime_error("Assertion failed");
 }
#endif
}
*/
#ifdef DEBUG
#define dsp_assert(x) if (!(x)) throw std::runtime_error(#x " assertion failed")
#else
#define dsp_assert(x)
#endif










namespace ProcessQuality
{
enum
{
 LowQuality,
 MidQuality,
 HighQuality
};
}










}
#endif /* XDDSP_Types_h */
