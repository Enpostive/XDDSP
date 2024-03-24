/*
  ==============================================================================

    XDDSP_FIRImpulses.h
    Created: 24 Mar 2024 10:16:30am
    Author:  Adam Jackson

  ==============================================================================
*/

#ifndef XDDSP_FIRImpulses_h
#define XDDSP_FIRImpulses_h

#include "XDDSP_Types.h"









namespace XDDSP
{

namespace FIRImpulses
{










SampleType sinc(SampleType x)
{ return sin(M_PI*x)/(M_PI*x); }









class ImpulseBaseClass
{
protected:
 const int length;
 const int halfLength;
 
public:
 ImpulseBaseClass (int length) :
 length(length),
 halfLength(length/2)
 { }
};



class LowPass : public ImpulseBaseClass
{
 const SampleType f;

public:
 LowPass(int length, SampleType normalisedFrequency):
 ImpulseBaseClass(length),
 f(normalisedFrequency)
 {}
 
 SampleType operator()(int x)
 {
  x -= halfLength;
  return 2.*f*sinc(2.*f*x);
 }
};



class HighPass : public ImpulseBaseClass
{
 const SampleType f;

public:
 HighPass(int length, SampleType normalisedFrequency):
 ImpulseBaseClass(length),
 f(normalisedFrequency)
 {}
 
 SampleType operator()(int x)
 {
  SampleType dirac = 1. * (x == halfLength);
  x -= halfLength;
  return dirac - 2.*f*sinc(2.*f*x);
 }
};



class BandPass : public ImpulseBaseClass
{
 const SampleType f1, f2;

public:
 BandPass(int length, SampleType normFreqLow, SampleType normFreqHigh):
 ImpulseBaseClass(length),
 f1(normFreqHigh),
 f2(normFreqLow)
 {}
 
 SampleType operator()(SampleType x)
 {
  x -= halfLength;
  return 2.*f1*sinc(2.*f1*x) - 2.*f2*sinc(2.*f2*x);
 }
};










}










template <typename Impulse, typename T = SampleType>
void generateImpulseResponse(Impulse impulse, T* data, int length)
{
 for (int i = 0; i < length; ++i) data[i] = impulse(i);
}

template <typename Impulse, typename T, unsigned long length>
void generateImpulseResponse(Impulse impulse, std::array<T, length> &data)
{ generateImpulseResponse(impulse, data.data(), length); }

template <typename Impulse, typename T>
void generateImpulseResponse(Impulse impulse, std::vector<T> &data)
{ generateImpulseResponse(impulse, data.data(), data.size()); }










}










#endif // XDDSP_FIRImpulses_h
