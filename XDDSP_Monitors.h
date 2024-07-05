//
//  XDDSP_Monitors.h
//  XDDSPTestingHarness
//
//  Created by Adam Jackson on 3/6/2022.
//

#ifndef XDDSP_Monitors_h
#define XDDSP_Monitors_h

#include "XDDSP_Classes.h"
#include "XDDSP_CircularBuffer.h"
#include <mutex>










namespace XDDSP
{










template <typename SignalIn>
class DebugWatch : public Component<DebugWatch<SignalIn>>
{
 // Private data members here
public:
 static constexpr int Count = SignalIn::Count;
 
 std::function<void (int)> onZero;
 std::function<void (int)> onNonZero;
 std::function<void (int)> onNAN;
 std::function<void (int)> onDenormal;
 std::function<void (int)> onInfinite;
 
 // Specify your inputs as public members here
 SignalIn signalIn;
 
 // Specify your outputs like this
 // No outputs
 
 // Include a definition for each input in the constructor
 DebugWatch(Parameters &p, SignalIn _signalIn) :
 signalIn(_signalIn)
 {}
 
 void stepProcess(int startPoint, int sampleCount)
 {
  for (int c = 0; c < Count; ++c)
  {
   bool zero = false;
   bool nonZero = false;
   bool nan = false;
   bool denormal = false;
   bool infinite = false;
   for (int i = startPoint, s = sampleCount; s--; ++i)
   {
    SampleType sig = signalIn(c, i);
    switch (std::fpclassify(sig))
    {
     case FP_ZERO:
      zero = true;
      break;
      
     case FP_INFINITE:
      infinite = true;
      break;
      
     case FP_NAN:
      nan = true;
      break;
      
     case FP_SUBNORMAL:
      denormal = true;
      break;
      
     default:
      nonZero = true;
      break;
    }
   }
   
   if (onZero && zero) onZero(c);
   if (onNonZero && nonZero) onNonZero(c);
   if (onNAN && nan) onNAN(c);
   if (onDenormal && denormal) onDenormal(c);
   if (onInfinite && infinite) onInfinite(c);
  }
 }
};










template <typename SignalIn>
class SignalProbe : public Component<SignalProbe<SignalIn>>
{
 // Private data members here
 std::array<SampleType, SignalIn::Count> maximumValue;
 std::array<SampleType, SignalIn::Count> minimumValue;
 std::array<SampleType, SignalIn::Count> instantaneousValue;
 
 std::mutex mtx;
public:
 static constexpr int Count = SignalIn::Count;
 
 // Specify your inputs as public members here
 SignalIn signalIn;
 
 // Specify your outputs like this
 // No output
 
 // Include a definition for each input in the constructor
 SignalProbe(Parameters &p, SignalIn _signalIn) :
 signalIn(_signalIn)
 {
  reset();
 }
 
 void reset()
 {
  std::unique_lock lock(mtx);
  
  maximumValue.fill(0.);
  minimumValue.fill(0.);
  instantaneousValue.fill(0.);
 }
 
 void stepProcess(int startPoint, int sampleCount)
 {
  std::unique_lock lock(mtx);
  
  for (int c = 0; c < Count; ++c)
  {
   instantaneousValue[c] = signalIn(c, startPoint);
   for (int i = startPoint, s = sampleCount; s--; ++i)
   {
    maximumValue[c] = fastMax(maximumValue[c], signalIn(c, i));
    minimumValue[c] = fastMin(minimumValue[c], signalIn(c, i));
   }
  }
 }
 
 SampleType getMinimumValue(int channel)
 {
  std::unique_lock lock(mtx);
  return minimumValue[channel];
 }
 
 SampleType getMaximumValue(int channel)
 {
  std::unique_lock lock(mtx);
  return maximumValue[channel];
 }
 
 SampleType getAbsoluteMaximumValue(int channel)
 { 
  std::unique_lock lock(mtx);
  return fastMax(fabs(minimumValue[channel]), fabs(maximumValue[channel]));
 }
 
 SampleType getInstantValue(int channel)
 {
  std::unique_lock lock(mtx);
  return instantaneousValue[channel];
 }
 
 SampleType probe(int channel)
 {
  std::unique_lock lock(mtx);
  SampleType result = getAbsoluteMaximumValue(channel);
  minimumValue[channel] = maximumValue[channel] = 0.;
  return result;
 }
 
 SampleType probeSqrt(int channel)
 {
  std::unique_lock lock(mtx);
  SampleType result = sqrt(maximumValue[channel]);
  minimumValue[channel] = maximumValue[channel] = 0.;
  return result;
 }
};










// Set SquareInputSignal to 1 to have a squared signal average output, suitable for
// calculating RMS
template <typename SignalIn, int SquareInputSignal = 0>
class SignalAverage : public Component<SignalAverage<SignalIn>>, public Parameters::ParameterListener
{
 // Private data members here
 Parameters &dspParam;
 
 std::array<DynamicCircularBuffer<>, SignalIn::Count> buffer;
 std::array<SampleType, SignalIn::Count> accum;
 int windowSize;
 SampleType recWindowSize;
 
 SampleType maxWindowSize;

public:
 static constexpr int Count = SignalIn::Count;
 
 // Specify your inputs as public members here
 SignalIn signalIn;
 
 // Specify your outputs like this
 Output<Count> signalOut;
 
 // Include a definition for each input in the constructor
 SignalAverage(Parameters &p, SignalIn _signalIn) :
 Parameters::ParameterListener(p),
 dspParam(p),
 signalIn(_signalIn),
 signalOut(p)
 {
  setWindowSize(1.);
  accum.fill(0.);
 }
 
 void setMaximumWindowSize(SampleType _maxWindowSize)
 {
  if (_maxWindowSize <= 0.) return;
  maxWindowSize = _maxWindowSize;
  int iMax = static_cast<int>(maxWindowSize*dspParam.sampleRate());
  for (auto &b : buffer)
  {
   b.setMaximumLength(iMax);
  }
  reset();
 }
 
 virtual void updateSampleRate(double sr, double isr) override
 {
  int iMax = static_cast<int>(maxWindowSize*sr);
  for (auto &b : buffer)
  {
   b.setMaximumLength(iMax);
  }
  reset();
 }

 void setWindowSize(SampleType _windowSize)
 {
  if (_windowSize <= 0) return;
  _windowSize = fastMin(_windowSize, maxWindowSize);
  _windowSize *= dspParam.sampleRate();
  windowSize = static_cast<int>(_windowSize);
  recWindowSize = 1./static_cast<SampleType>(windowSize);
  for (int c = 0; c < Count; ++c)
  {
   accum[c] = 0.;
   for (int i = 0; i < windowSize; ++i)
   {
    accum[c] += buffer[c].tapOut(i);
   }
  }
 }
 
 // This function is responsible for clearing the output buffers to a default state when
 // the component is disabled.
 void reset() override
 {
  accum.fill(0.);
  for (auto &b : buffer) b.reset(0.);
  signalOut.reset();
 }
 
 // stepProcess is called repeatedly with the start point incremented by step size
 void stepProcess(int startPoint, int sampleCount) override
 {
  for (int c = 0; c < Count; ++c)
  {
   for (int i = startPoint, s = sampleCount; s--; ++i)
   {
    SampleType ss = signalIn(c, i);
    if (SquareInputSignal) ss *= ss;
    accum[c] += buffer[c].tapIn(ss);
    accum[c] -= buffer[c].tapOut(windowSize);
    signalOut.buffer(c, i) = accum[c]*recWindowSize;
   }
  }
 }
};










template <typename SignalIn>
class InterfaceBuffer : public Component<InterfaceBuffer<SignalIn>>
{
 // Private data members here
 std::array<DynamicCircularBuffer<>, SignalIn::Count> buffer;
 int bufferSize {32};
 std::mutex mux;
 
public:
 static constexpr int Count = SignalIn::Count;
 
 // Specify your inputs as public members here
 SignalIn signalIn;
 
 // Specify your outputs like this
 // No outputs
 
 // Include a definition for each input in the constructor
 InterfaceBuffer(Parameters &p, SignalIn _signalIn) :
 signalIn(_signalIn)
 {}
 
 // This function is responsible for clearing the output buffers to a default state when
 // the component is disabled.
 void reset()
 {
 }
 
 void setBufferSize(int size)
 {
  std::lock_guard lock(mux);
  bufferSize = size;
  for (auto& b : buffer)
  {
   b.setMaximumLength(size);
  }
 }
 
 void stepProcess(int startPoint, int sampleCount)
 {
  std::lock_guard lock(mux);
  for (int c = 0; c < Count; ++c)
  {
   for (int i = startPoint, s = sampleCount; s--; ++i)
   {
    buffer[c].tapIn(signalIn(c, i));
   }
  }
 }
 
 void extractChannel(int channel, std::vector<SampleType> &vector)
 {
  vector.resize(bufferSize);
  std::lock_guard lock(mux);
  for (int i = 0; i < bufferSize; ++i)
  {
   vector[i] = buffer[channel].tapOut(bufferSize - i - 1);
  }
 }
 
 void extractSumChannels(std::vector<SampleType> &vector, SampleType scaleFactor = 1.)
 {
  vector.resize(bufferSize);
  std::lock_guard lock(mux);
  for (int i = 0; i < bufferSize; ++i)
  {
   SampleType sum = 0.;
   for (int c = 0; c < Count; ++c)
   {
    sum += buffer[c].tapOut(bufferSize - i - 1);
   }
   vector[i] = sum*scaleFactor;
  }
 }
 
 void partialExtractChannel(int channel, std::vector<SampleType> &vector, int length)
 {
  vector.resize(length);
  std::lock_guard lock(mux);
  for (int i = 0; i < length; ++i)
  {
   vector[i] = buffer[channel].tapOut(bufferSize - i - 1);
  }
 }
 
 void partialExtractSumChannels(std::vector<SampleType> &vector,
                                int length,
                                SampleType scaleFactor = 1.)
 {
  vector.resize(length);
  std::lock_guard lock(mux);
  for (int i = 0; i < length; ++i)
  {
   SampleType sum = 0.;
   for (int c = 0; c < Count; ++c)
   {
    sum += buffer[c].tapOut(bufferSize - i - 1);
   }
   vector[i] = sum*scaleFactor;
  }
 }
};










}
#endif /* XDDSP_Monitors_h */
