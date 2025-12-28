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










  /**
   * @brief An input only component which can come in handy when debugging your DSP Network.
   * 
   * @tparam SignalIn Couples to the signal to debug. May have as many channels as you like.
   */
template <typename SignalIn>
class DebugWatch : public Component<DebugWatch<SignalIn>>
{
public:
 static constexpr int Count = SignalIn::Count;
 
 /**
  * @brief Called with the channel number when a zero is detected.
  * 
  */
 std::function<void (int)> onZero;

 /**
  * @brief Called with the channel number when a nonzero is detected.
  * 
  */
 std::function<void (int)> onNonZero;

 /**
  * @brief Called with the channel number when a nan value is detected.
  * 
  */
 std::function<void (int)> onNAN;

 /**
  * @brief Called with the channel number when a denormal or subnormal number is detected.
  * 
  */
 std::function<void (int)> onDenormal;

 /**
  * @brief Called with the channel number when an infinite value is detected.
  * 
  */
 std::function<void (int)> onInfinite;
 
 SignalIn signalIn;
 
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










/**
 * @brief A probe suitable for measureing minimum, maximum and instantaneous values in a signal and reading them from non-DSP code.
 * 
 * This component is thread safe.
 * 
 * @tparam SignalIn Couples to the signal to measure. May have as many channels as you like.
 */
template <typename SignalIn>
class SignalProbe : public Component<SignalProbe<SignalIn>>
{
 std::array<SampleType, SignalIn::Count> maximumValue;
 std::array<SampleType, SignalIn::Count> minimumValue;
 std::array<SampleType, SignalIn::Count> instantaneousValue;
 
 std::mutex mtx;
public:
 static constexpr int Count = SignalIn::Count;
 
 SignalIn signalIn;
 
 SignalProbe(Parameters &p, SignalIn _signalIn) :
 signalIn(_signalIn)
 {
  reset();
 }
 
 /**
  * @brief Reset the minimum and maximum values.
  * 
  */
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
 
 /**
  * @brief Get the minimum value seen since the last reset.
  * 
  * @param channel The channel to check.
  * @return SampleType The minimum value.
  */
 SampleType getMinimumValue(int channel)
 {
  std::unique_lock lock(mtx);
  return minimumValue[channel];
 }
 
 /**
  * @brief Get the maximum value seen since the last reset.
  * 
  * @param channel The channel to check.
  * @return SampleType The maximum value.
  */
 SampleType getMaximumValue(int channel)
 {
  std::unique_lock lock(mtx);
  return maximumValue[channel];
 }
 
 /**
  * @brief Get the value with the highest magnitude seen since the last reset.
  * 
  * @param channel The channel to check.
  * @return SampleType The maximum value.
  */
 SampleType getAbsoluteMaximumValue(int channel)
 { 
  std::unique_lock lock(mtx);
  return fastMax(fabs(minimumValue[channel]), fabs(maximumValue[channel]));
 }
 
 /**
  * @brief Get the last value seen by the object
  * 
  * @param channel The channel to check.
  * @return SampleType The last value seen.
  */
 SampleType getInstantValue(int channel)
 {
  std::unique_lock lock(mtx);
  return instantaneousValue[channel];
 }
 
 /**
  * @brief Get the value with the highest magnitude seen since the last reset, then perform a reset straight away.
  * 
  * @param channel The channel to check.
  * @return SampleType The maximum value.
  */
 SampleType probe(int channel)
 {
  std::unique_lock lock(mtx);
  SampleType result = getAbsoluteMaximumValue(channel);
  minimumValue[channel] = maximumValue[channel] = 0.;
  return result;
 }
 
 /**
  * @brief Designed for getting the maximum value coming from a squared signal.
  * 
  * @param channel The channel to check.
  * @return SampleType The maximum value.
  */
 SampleType probeSqrt(int channel)
 {
  std::unique_lock lock(mtx);
  SampleType result = sqrt(maximumValue[channel]);
  minimumValue[channel] = maximumValue[channel] = 0.;
  return result;
 }
};










/**
  * @brief A component for outputing a signal which tracks the average level of the input signal.
  *
  * This component uses a circular buffer to take a rectangle window average, as opposed to using a One-Pole filter
  *
  * @tparam SignalIn Couples to an input to average out. This can have as many channels as you like.
  * @tparam SquareInputSignal Set this to 1 to cause the component to square every input value before taking the average.
 */
template <typename SignalIn, int SquareInputSignal = 0>
class SignalAverage : public Component<SignalAverage<SignalIn>>, public Parameters::ParameterListener
{
 Parameters &dspParam;
 
 std::array<DynamicCircularBuffer<>, SignalIn::Count> buffer;
 std::array<SampleType, SignalIn::Count> accum;
 int windowSize;
 SampleType recWindowSize;
 
 SampleType maxWindowSize;

public:
 static constexpr int Count = SignalIn::Count;
 
 SignalIn signalIn;
 
 Output<Count> signalOut;
 
 SignalAverage(Parameters &p, SignalIn _signalIn) :
 Parameters::ParameterListener(p),
 dspParam(p),
 signalIn(_signalIn),
 signalOut(p)
 {
  setWindowSize(1.);
  accum.fill(0.);
 }
 
 /**
  * @brief Set the maximum window size and allocate the required memory.
  * 
  * @param _maxWindowSize The desired maximum window size.
  */
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

 /**
  * @brief Set the window size to use without allocating extra memory.
  * 
  * Does bounds checking against the maximum window size. The accumulated average is recalculated.
  * 
  * @param _windowSize The desired window size.
  */
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
 
 void reset() override
 {
  accum.fill(0.);
  for (auto &b : buffer) b.reset(0.);
  signalOut.reset();
 }
 
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










/**
 * @brief A component which buffers samples from its input and makes them available in a thread safe manner
 * 
 * @tparam SignalIn Couples to the signal to monitor. Can have as many channels as you like.
 */
template <typename SignalIn>
class InterfaceBuffer : public Component<InterfaceBuffer<SignalIn>>
{
 std::array<DynamicCircularBuffer<>, SignalIn::Count> buffer;
 int bufferSize {32};
 std::mutex mux;
 
public:
 static constexpr int Count = SignalIn::Count;
 
 SignalIn signalIn;
 
 InterfaceBuffer(Parameters &p, SignalIn _signalIn) :
 signalIn(_signalIn)
 {}
 
 void reset()
 {
 }
 
 /**
  * @brief Set the size of the buffer which keeps input values.
  * 
  * @param size The new size of the buffer.
  */
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
 
 /**
  * @brief Extract the samples from the buffer of one channel into a vector.
  * 
  * This method overwrites the vector object which is passed in. It may allocate memory, but it does so before locking any mutex which means the actual extraction phase executes in a consistent manner.
  * 
  * @param channel The channel to extract.
  * @param vector The vector to write the samples into.
  */
 void extractChannel(int channel, std::vector<SampleType> &vector)
 {
  vector.resize(bufferSize);
  std::lock_guard lock(mux);
  for (int i = 0; i < bufferSize; ++i)
  {
   vector[i] = buffer[channel].tapOut(bufferSize - i - 1);
  }
 }
 
 /**
  * @brief Extract the sum of the samples from the buffer of every channel into a vector.
  * 
  * This method overwrites the vector object which is passed in. It may allocate memory, but it does so before locking any mutex which means the actual extraction phase executes in a consistent manner.
  * 
  * @param vector The vector to write the samples into.
  * @param scaleFactor The scaling to apply to the summed samples. Defaults to 1.
  */
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
 
 /**
  * @brief Extract a subset of the samples from the buffer of one channel into a vector.
  * 
  * This method overwrites the vector object which is passed in. It may allocate memory, but it does so before locking any mutex which means the actual extraction phase executes in a consistent manner.
  * 
  * @param channel The channel to extract.
  * @param vector The vector to write the samples into.
  * @param length The length of the buffer to read. It always starts from the most recent sample.
  */
 void partialExtractChannel(int channel, std::vector<SampleType> &vector, int length)
 {
  vector.resize(length);
  std::lock_guard lock(mux);
  for (int i = 0; i < length; ++i)
  {
   vector[i] = buffer[channel].tapOut(bufferSize - i - 1);
  }
 }
 
 /**
  * @brief Extract the sum of the samples from the buffer of every channel into a vector.
  * 
  * This method overwrites the vector object which is passed in. It may allocate memory, but it does so before locking any mutex which means the actual extraction phase executes in a consistent manner.
  * 
  * @param vector The vector to write the samples into.
  * @param length The length of the buffer to read. It always starts from the most recent sample.
  * @param scaleFactor The scaling to apply to the summed samples. Defaults to 1.
  */
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
