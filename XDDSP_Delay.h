//
//  XDDSP_Delay.h
//  XDDSP
//
//  Created by Adam Jackson on 26/5/2022.
//

#ifndef XDDSP_Delay_h
#define XDDSP_Delay_h

#include "XDDSP_CircularBuffer.h"










namespace XDDSP
{









// Delay times are in samples and are rounded to the nearest sample without interpolation
template <
typename SignalIn,
typename DelayTimeIn,
typename BufferType = DynamicCircularBuffer<>
>
class LowQualityDelay :
public Component<LowQualityDelay<SignalIn, DelayTimeIn, BufferType>>
{
 static_assert(DelayTimeIn::Count == 1, "DelayTimeIn is expected to have just one channel");
 
 // Private data members here
 std::array<BufferType, SignalIn::Count> buffer;
 
public:
 static constexpr int Count = SignalIn::Count;
 
 // Specify your inputs as public members here
 SignalIn signalIn;
 DelayTimeIn delayTimeIn;
 
 // Specify your outputs like this
 Output<Count> signalOut;
 
 // Include a definition for each input in the constructor
 LowQualityDelay(Parameters &p, SignalIn signalIn, DelayTimeIn delayTimeIn) :
 signalIn(signalIn),
 delayTimeIn(delayTimeIn),
 signalOut(p)
 {}
 
 void reset()
 {
  for (auto& b : buffer) b.reset();
  signalOut.reset();
 }
 
 void setMaximumDelayTime(uint32_t maxDelay)
 {
  for (auto& b : buffer) b.setMaximumLength(maxDelay);
 }
 
 void stepProcess(int startPoint, int sampleCount)
 {
  for (int i = startPoint, s = sampleCount; s--; ++i)
  {
   uint32_t delayTime = fastBoundary(delayTimeIn(0, i), 1., buffer[0].getSize());
   for (int c = 0; c < Count; ++c)
   {
    buffer[c].tapIn(signalIn(c, i));
    signalOut.buffer(c, i) = buffer[c].tapOut(delayTime);
   }
  }
 }
};










// Delay times are in samples and are rounded to the nearest sample without interpolation
template <
typename SignalIn,
typename DelayTimeIn,
typename BufferType = DynamicCircularBuffer<>
>
class MultiTapDelay :
public Component<MultiTapDelay<SignalIn, DelayTimeIn, BufferType>>
{
 // Private data members here
 std::array<BufferType, SignalIn::Count> buffer;
 
public:
 static constexpr int CountChannels = SignalIn::Count;
 static constexpr int CountTaps = DelayTimeIn::Count;
 
 // Specify your inputs as public members here
 SignalIn signalIn;
 DelayTimeIn delayTimeIn;
 
 // Specify your outputs like this
 std::array<Output<CountChannels>, CountTaps> tapOut;
 
 // Include a definition for each input in the constructor
 MultiTapDelay(Parameters &p, SignalIn signalIn, DelayTimeIn delayTimeIn) :
 signalIn(signalIn),
 delayTimeIn(delayTimeIn),
 tapOut(make_array<CountTaps>(Output<CountChannels>(p)))
 {}
 
 void reset()
 {
  for (auto& b : buffer) b.reset();
  for (auto& t : tapOut) t.reset();
 }
 
 void setMaximumDelayTime(uint32_t maxDelay)
 {
  for (auto& b : buffer) b.setMaximumLength(maxDelay);
 }
 
 void stepProcess(int startPoint, int sampleCount)
 {
  for (int c = 0; c < CountChannels; ++c)
  {
   for (int i = startPoint, s = sampleCount; s--; ++i)
   {
    buffer[c].tapIn(signalIn(c, i));
    for (int t = 0; t < CountTaps; ++t)
    {
     uint32_t delayTime = fastBoundary(delayTimeIn(t, i), 1., buffer[c].getSize());
     tapOut[t].buffer(c, i) = buffer[c].tapOut(delayTime);
    }
   }
  }
 }
};










// Delay times are in samples. Linear interpolation is used between samples
template <
typename SignalIn,
typename DelayTimeIn,
typename BufferType = DynamicCircularBuffer<>
>
class MediumQualityDelay :
public Component<MediumQualityDelay<SignalIn, DelayTimeIn, BufferType>>
{
 static_assert(DelayTimeIn::Count == 1, "MediumQualityDelay expects a delay time input with a single channel");
 
 // Private data members here
 std::array<BufferType, SignalIn::Count> buffer;
 
public:
 static constexpr int Count = SignalIn::Count;
 
 // Specify your inputs as public members here
 SignalIn signalIn;
 DelayTimeIn delayTimeIn;
 
 // Specify your outputs like this
 Output<Count> signalOut;
 
 // Include a definition for each input in the constructor
 MediumQualityDelay(Parameters &p, SignalIn signalIn, DelayTimeIn delayTimeIn) :
 signalIn(signalIn),
 delayTimeIn(delayTimeIn),
 signalOut(p)
 {}
 
 void reset()
 {
  for (auto& b : buffer) b.reset();
  signalOut.reset();
 }
 
 void setMaximumDelayTime(uint32_t maxDelay)
 {
  for (auto& b : buffer) b.setMaximumLength(maxDelay);
 }
 
 void stepProcess(int startPoint, int sampleCount)
 {
  for (int i = startPoint, s = sampleCount; s--; ++i)
  {
   const SampleType fDelay = fastBoundary(delayTimeIn(0, i), 1., buffer[0].getSize());
   IntegerAndFraction iaf(fDelay);

   for (int c = 0; c < Count; ++c)
   {
    buffer[c].tapIn(signalIn(c, i));
    SampleType x0 = buffer[c].tapOut(iaf.intRep());
    SampleType x1 = buffer[c].tapOut(iaf.intRep() + 1);
    signalOut.buffer(c, i) = LERP(iaf.fracPart(), x0, x1);
   }
  }
 }
};










// Delay times are in samples. Hermite interpolation is used between samples
template <
typename SignalIn,
typename DelayTimeIn,
typename BufferType = DynamicCircularBuffer<>
>
class HighQualityDelay :
public Component<HighQualityDelay<SignalIn, DelayTimeIn, BufferType>>
{
 static_assert(DelayTimeIn::Count == 1, "HighQualityDelay expects a delay time input with a single channel");
 
 // Private data members here
 std::array<BufferType, SignalIn::Count> buffer;
 
public:
 static constexpr int Count = SignalIn::Count;
 
 // Specify your inputs as public members here
 SignalIn signalIn;
 DelayTimeIn delayTimeIn;
 
 // Specify your outputs like this
 Output<Count> signalOut;
 
 // Include a definition for each input in the constructor
 HighQualityDelay(Parameters &p, SignalIn signalIn, DelayTimeIn delayTimeIn) :
 signalIn(signalIn),
 delayTimeIn(delayTimeIn),
 signalOut(p)
 {}
 
 void reset()
 {
  for (auto& b : buffer) b.reset();
  signalOut.reset();
 }
 
 void setMaximumDelayTime(uint32_t maxDelay)
 {
  for (auto& b : buffer) b.setMaximumLength(maxDelay);
 }
 
 void stepProcess(int startPoint, int sampleCount)
 {
  for (int i = startPoint, s = sampleCount; s--; ++i)
  {
   const SampleType fDelay = fastBoundary(delayTimeIn(0, i), 2., buffer[0].getSize());
   IntegerAndFraction iaf(fDelay);

   for (int c = 0; c < Count; ++c)
   {
    buffer[c].tapIn(signalIn(c, i));
    SampleType xm1 = buffer[c].tapOut(iaf.intRep() - 1);
    SampleType x0 = buffer[c].tapOut(iaf.intRep());
    SampleType x1 = buffer[c].tapOut(iaf.intRep() + 1);
    SampleType x2 = buffer[c].tapOut(iaf.intRep() + 2);
    signalOut.buffer(c, i) = hermite(iaf.fracPart(), xm1, x0, x1, x2);
   }
  }
 }
};










}

#endif /* XDDSP_Delay_h */
