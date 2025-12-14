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









/**
 * @brief A simple delay component with no iterpolation.
 * 
 * @tparam SignalIn Couples to a signal to be delayed. The signal can have as many channels as you like.
 * @tparam DelayTimeIn Couples to the delay time input. Only one channel is allowed. The delay time is measured in samples and bounds checking is performed.
 * @tparam BufferType Either CircularBuffer, DynamicCircularBuffer or ModulusCircularBuffer, depending on your requirements.
 */
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
 
 // The input signal
 SignalIn signalIn;

 // The delay time signal
 DelayTimeIn delayTimeIn;
 
 // The delayed signal output.
 Output<Count> signalOut;
 
 LowQualityDelay(Parameters &p, SignalIn signalIn, DelayTimeIn delayTimeIn) :
 signalIn(signalIn),
 delayTimeIn(delayTimeIn),
 signalOut(p)
 {}
 
 void reset()
 {
  for (auto& b : buffer) b.reset(0.);
  signalOut.reset();
 }
 
 /**
  * @brief Set the maximum delay time on the underlying buffer objects.
  * 
  * If the component was compiled using the CircularBuffer class, this call is ignored.
  * 
  * @param maxDelay The new maximum delay time.
  */
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










/**
 * @brief A simple delay with multiple taps
 * 
 * @tparam SignalIn Couples to a signal to be delayed. The signal can have as many channels as you like.
 * @tparam DelayTimeIn Couples to the delay time input. An output is created for each channel in this coupler. The delay time is measured in samples and bounds checking is performed.
 * @tparam BufferType Either CircularBuffer, DynamicCircularBuffer or ModulusCircularBuffer, depending on your requirements.
 */
template <
typename SignalIn,
typename DelayTimeIn,
typename BufferType = DynamicCircularBuffer<>
>
class MultiTapDelay :
public Component<MultiTapDelay<SignalIn, DelayTimeIn, BufferType>>
{
 std::array<BufferType, SignalIn::Count> buffer;
 
public:
 static constexpr int CountChannels = SignalIn::Count;
 static constexpr int CountTaps = DelayTimeIn::Count;
 
 // The input signal to be delayed.
 SignalIn signalIn;

 // The delay time signal.
 DelayTimeIn delayTimeIn;
 
 // The delayed signal outputs.
 std::array<Output<CountChannels>, CountTaps> tapOut;
 
 MultiTapDelay(Parameters &p, SignalIn signalIn, DelayTimeIn delayTimeIn) :
 signalIn(signalIn),
 delayTimeIn(delayTimeIn),
 tapOut(make_array<CountTaps>(Output<CountChannels>(p)))
 {}
 
 void reset()
 {
  for (auto& b : buffer) b.reset(0.);
  for (auto& t : tapOut) t.reset();
 }
 
 /**
  * @brief Set the maximum delay time on the underlying buffer objects.
  * 
  * If the component was compiled using the CircularBuffer class, this call is ignored.
  * 
  * @param maxDelay The new maximum delay time.
  */
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










/**
 * @brief A simple delay component with linear interpolation.
 * 
 * @tparam SignalIn Couples to a signal to be delayed. The signal can have as many channels as you like.
 * @tparam DelayTimeIn Couples to the delay time input. Only one channel is allowed. The delay time is measured in samples and bounds checking is performed.
 * @tparam BufferType Either CircularBuffer, DynamicCircularBuffer or ModulusCircularBuffer, depending on your requirements.
 */
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
 
 // The input signal to be delayed.
 SignalIn signalIn;
 
 // The delay time signal.
 DelayTimeIn delayTimeIn;
 
 // The delayed signal output.
 Output<Count> signalOut;
 
 MediumQualityDelay(Parameters &p, SignalIn signalIn, DelayTimeIn delayTimeIn) :
 signalIn(signalIn),
 delayTimeIn(delayTimeIn),
 signalOut(p)
 {}
 
 void reset()
 {
  for (auto& b : buffer) b.reset(0.);
  signalOut.reset();
 }
 
 /**
  * @brief Set the maximum delay time on the underlying buffer objects.
  * 
  * If the component was compiled using the CircularBuffer class, this call is ignored.
  * 
  * @param maxDelay The new maximum delay time.
  */
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










/**
 * @brief A simple delay component with hermite interpolation.
 * 
 * @tparam SignalIn Couples to a signal to be delayed. The signal can have as many channels as you like.
 * @tparam DelayTimeIn Couples to the delay time input. Only one channel is allowed. The delay time is measured in samples and bounds checking is performed.
 * @tparam BufferType Either CircularBuffer, DynamicCircularBuffer or ModulusCircularBuffer, depending on your requirements.
 */
template <
typename SignalIn,
typename DelayTimeIn,
typename BufferType = DynamicCircularBuffer<>
>
class HighQualityDelay :
public Component<HighQualityDelay<SignalIn, DelayTimeIn, BufferType>>
{
 static_assert(DelayTimeIn::Count == 1, "HighQualityDelay expects a delay time input with a single channel");
 
 std::array<BufferType, SignalIn::Count> buffer;
 
public:
 static constexpr int Count = SignalIn::Count;
 
 // The input signal to be delayed
 SignalIn signalIn;
 
 // The delay time signal.
 DelayTimeIn delayTimeIn;
 
 // The delayed signal output.
 Output<Count> signalOut;
 
 HighQualityDelay(Parameters &p, SignalIn signalIn, DelayTimeIn delayTimeIn) :
 signalIn(signalIn),
 delayTimeIn(delayTimeIn),
 signalOut(p)
 {}
 
 void reset()
 {
  for (auto& b : buffer) b.reset(0.);
  signalOut.reset();
 }
 
 /**
  * @brief Set the maximum delay time on the underlying buffer objects.
  * 
  * If the component was compiled using the CircularBuffer class, this call is ignored.
  * 
  * @param maxDelay The new maximum delay time.
  */
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
