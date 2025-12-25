//
//  XDDSP_Utilities.h
//  XDDSP
//
//  Created by Adam Jackson on 26/5/2022.
//

#ifndef XDDSP_Utilities_h
#define XDDSP_Utilities_h










namespace XDDSP
{










/**
 * @brief A component which couples an array of inputs and sums them into a single output.
 * 
 * @tparam SignalIn Couples to one input. The class of coupler selected is duplicated in an array. The coupler can have as many channels as you like.
 * @tparam SignalCount The number of input signals to be mixed.
 */
template <typename SignalIn, int SignalCount>
class MixDown : public Component<MixDown<SignalIn, SignalCount>>
{
public:
 static constexpr int ChannelCount = SignalIn::Count;
 
 std::array<SignalIn, SignalCount> signalsIn;
 
 Output<ChannelCount> signalOut;
 
 MixDown(Parameters &p, const std::array<SignalIn, SignalCount> &_signalsIn) :
 signalsIn(_signalsIn),
 signalOut(p)
 {}
 
 void reset()
 {
  signalOut.reset();
 }
 
 void stepProcess(int startPoint, int sampleCount)
 {
  for (int i = startPoint, s = sampleCount; s--; ++i)
  {
   for (int c = 0; c < ChannelCount; ++c)
   {
    SampleType sum = 0.;
    for (int s = 0; s < SignalCount; ++s)
    {
     sum += signalsIn[s](c, i);
    }
    signalOut.buffer(c, i) = sum;
   }
  }
 }
};










/**
 * @brief A component which applies a gain signal to an input.
 * 
 * @tparam SignalIn The signal to amplify. Can have as many channels as you like.
 * @tparam GainIn The linear gain signal to apply. Must either have one channel or the same number of channels as SignalIn.
 */
template <typename SignalIn, typename GainIn>
class SimpleGain : public Component<SimpleGain<SignalIn, GainIn>>
{
 static_assert(GainIn::Count == 1 || GainIn::Count == SignalIn::Count, "GainIn type has invalid channel count. Channel count must either be 1 or be equal to the channel count of SignalIn");
 
 static constexpr bool MultiGains = (GainIn::Count == SignalIn::Count);
public:
 SignalIn signalIn;
 GainIn gainIn;
 
 Output<SignalIn::Count> signalOut;
 
 SimpleGain(Parameters &p, SignalIn _signalIn, GainIn _gainIn) :
 signalIn(_signalIn),
 gainIn(_gainIn),
 signalOut(p)
 {}
 
 void reset()
 {
  signalOut.reset();
 }
 
 void stepProcess(int startPoint, int sampleCount)
 {
  for (int c = 0; c < SignalIn::Count; ++c)
  {
   for (int i = startPoint, s = sampleCount; s--; ++i)
   {
    signalOut.buffer(c, i) = signalIn(c, i)*(MultiGains ? gainIn(c, i) : gainIn(i));
   }
  }
 }
};










/**
 * @brief A component which rectifies a signal.
 * 
 * @tparam SignalIn The signal to rectify.
 * @tparam RectifyLevelIn A signal to specify the level which is considered "zero". Anything below this signal is rectified about the signal.
 */
template <typename SignalIn, typename RectifyLevelIn>
class Rectifier : public Component<Rectifier<SignalIn, RectifyLevelIn>>
{
 static_assert(RectifyLevelIn::Count == 1, "Rectifier expects a single channel control source");
public:
 static constexpr int Count = SignalIn::Count;

 SignalIn signalIn;
 RectifyLevelIn rectifyLevelIn;
 
 Output<SignalIn::Count> signalOut;
 
 Rectifier(Parameters &p,
           SignalIn _signalIn,
           RectifyLevelIn _rectifyLevelIn) :
 signalIn(_signalIn),
 rectifyLevelIn(_rectifyLevelIn),
 signalOut(p)
 {}
 
 void reset()
 {
  signalOut.reset();
 }
 
 void stepProcess(int startPoint, int sampleCount)
 {
  for (int c = 0; c < Count; ++c)
  {
   for (int i = startPoint, s = sampleCount; s--; ++i)
   {
    signalOut.buffer(c, i) = fabs(signalIn(c, i) - rectifyLevelIn(i)) + rectifyLevelIn(i);
   }
  }
 }
};










/**
 * @brief A component which takes an input signal and outputs a delta signal.
 * 
 * @tparam SignalIn Couples to the input signal. Can have as many channels as you like.
 */
template <typename SignalIn>
class SignalDelta : public Component<SignalDelta<SignalIn>>
{
 Parameters &dspParam;
 std::array<SampleType, SignalIn::Count> h;
 
public:
 static constexpr int Count = SignalIn::Count;
 
 SignalIn signalIn;
 
 Output<Count> signalOut;
 
 SignalDelta(Parameters &p, SignalIn _signalIn) :
 dspParam(p),
 signalIn(_signalIn),
 signalOut(p)
 {
  h.fill(0.);
 }
 
 void reset()
 {
  h.fill(0.);
  signalOut.reset();
 }
 
 void stepProcess(int startPoint, int sampleCount)
 {
  for (int c = 0; c < Count; ++c)
  {
   for (int i = startPoint, s = sampleCount; s--; ++i)
   {
    signalOut.buffer(c, i) = (signalIn(c, i) - h[c])*dspParam.sampleRate();
    h[c] = signalIn(c, i);
   }
  }
 }
};










/**
 * @brief A component which constrains a signal between two other signals.
 * 
 * @tparam SignalIn Couples to an input signal.
 * @tparam MinimumIn Couples to a signal which specifies the maximum level allowed.
 * @tparam MaximumIn Couples to a signal which specifies the minimum level allowed.
 */
template <typename SignalIn, typename MinimumIn, typename MaximumIn>
class Clipper : public Component<Clipper<SignalIn, MinimumIn, MaximumIn>>
{
 static_assert(MinimumIn::Count == 1 && MaximumIn::Count == 1, "Clipper expects control signals with single channels");
public:
 static constexpr int Count = SignalIn::Count;
 
 SignalIn signalIn;
 MinimumIn minimumIn;
 MaximumIn maximumIn;
 
 Output<Count> signalOut;
 
 Clipper(Parameters &p,
         SignalIn _signalIn,
         MinimumIn _minimumIn,
         MaximumIn _maximumIn) :
 signalIn(_signalIn),
 minimumIn(_minimumIn),
 maximumIn(_maximumIn),
 signalOut(p)
 {}
 
 void reset()
 {
  signalOut.reset();
 }
 
 void stepProcess(int startPoint, int sampleCount)
 {
  for (int c = 0; c < Count; ++c)
  {
   for (int i = startPoint, s = sampleCount; s--; ++i)
   {
    signalOut.buffer(c, i) = fastBoundary(signalIn(c, i), minimumIn(i), maximumIn(i));
   }
  }
 }
};










/**
 * @brief A component which takes multiple signals and outputs the signal which has the highest level.
 * 
 * @tparam SignalIn Couples to the inputs. Can have as many channels as you like.
 * @tparam InputCount The number of inputs to take.
 */
template <typename SignalIn, int InputCount>
class Maximum : public Component<Maximum<SignalIn, InputCount>>
{
public:
 static constexpr int Count = SignalIn::Count;
 
 std::array<SignalIn, InputCount> signalIn;
 
 Output<Count> signalOut;
 
 Maximum(Parameters &p, const std::array<SignalIn, InputCount> &_signalIn) :
 signalIn(_signalIn),
 signalOut(p)
 {}
 
 void reset()
 {
  signalOut.reset();
 }
 
 void stepProcess(int startPoint, int sampleCount)
 {
  for (int c = 0; c < Count; ++c)
  {
   for (int i = startPoint, s = sampleCount; s--; ++i)
   {
    SampleType max = signalIn[0](c, i);
    for (int s = 1; s < InputCount; ++s)
    {
     max = fastMax(max, signalIn[s](c, i));
    }
    signalOut.buffer(c, i) = max;
   }
  }
 }
};









/**
 * @brief A component which outputs three time signals.
 * 
 */
class TimeSignal : public Component<TimeSignal>
{
 Parameters &dspParam;
 uint64_t sampleTime {0};
 SampleType scalePPQ {1.};
 SampleType scaleSeconds {1.};
 bool sync {false};
 
public:
 static constexpr int Count = 1;
 
 Output<Count> timeSamples;
 Output<Count> timePPQ;
 Output<Count> timeSeconds;
 
 TimeSignal(Parameters &p) :
 dspParam(p),
 timeSamples(p),
 timePPQ(p),
 timeSeconds(p)
 {}
 
 void reset()
 {
  timeSamples.reset();
  timePPQ.reset();
  timeSeconds.reset();
 }

 void stepProcess(int startPoint, int sampleCount)
 {
  double tempo;
  double ppq;
  double seconds;
  bool playing = dspParam.getTransportInformation(tempo, ppq, seconds);
  
  SampleType beatsPerSecond = scalePPQ*tempo/60.;
  SampleType beatsPerSample = dspParam.sampleRate()*beatsPerSecond;
  SampleType secondsPerSample = scaleSeconds*dspParam.sampleInterval();
  
  if (sync && playing)
  {
   // If the transport is playing, syncronise the sample counter to the host
   sampleTime = seconds/secondsPerSample;
  }
  else
  {
   // If the transport is not playing, syncronise the ppq and seconds to the counter
   seconds = sampleTime*secondsPerSample;
   ppq = seconds*beatsPerSecond;
  }
  
  for (int i = startPoint, s = sampleCount; s--; ++i)
  {
   timeSamples.buffer(i) = sampleTime;
   timePPQ.buffer(i) = ppq;
   timeSeconds.buffer(i) = seconds;
   
   ++sampleTime;
   ppq += beatsPerSample;
   seconds += secondsPerSample;
  }
 }
 
 /**
  * @brief Set a scale factor to use on the PPQ signal.
  * 
  * @param _scale The scale factor.
  */
 void setScalePPQ(SampleType _scale)
 {
  scalePPQ = _scale;
 }
 
 /**
  * @brief Set a scale factor to use on the seconds signal.
  * 
  * @param _scale The scale factor.
  */
 void setScaleSeconds(SampleType _scale)
 {
  scaleSeconds = _scale;
 }
 
 /**
  * @brief Set whether the component syncronises with the MIDI information provided in the Parameters object.
  * 
  * @param _sync 
  */
 void setSync(bool _sync)
 { sync = _sync; }
};










/**
 * @brief A counter object.
 * 
 * This counter counts up or down (depnding on the value for StepSize). When the counter reaches one of the boundaries, it stops counting.
 * 
 * @tparam StartIn Couples to a signal which specifies the start point of the counter. Can have as many channels as you like.
 * @tparam EndIn Couples to a signal which specifies the end point of the counter. Must have the same number of channels as StartIn.
 * @tparam SpeedIn Couples to a signal which specifies the speed of the counter. Must have the same number of channels as StartIn.
 * @tparam StepSize Selects how often the component updates the counter speed, the default is INT_MAX.
 */
template <typename StartIn, typename EndIn, typename SpeedIn, int StepSize = INT_MAX>
class Counter : public Component<Counter<StartIn, EndIn, SpeedIn, StepSize>>
{
 static_assert(StartIn::Count == EndIn::Count && StartIn::Count == SpeedIn::Count, "Counter expects all control signals to have the same channel count");
 std::array<SampleType, StartIn::Count> _counter;
public:
 static constexpr int Count = StartIn::Count;
 
 const std::array<SampleType, StartIn::Count> &currentCount {_counter};
 
 StartIn startIn;
 EndIn endIn;
 SpeedIn speedIn;
 
 Output<Count> counterOut;
 
 Counter(Parameters &p, StartIn _startIn, EndIn _endIn, SpeedIn _speedIn) :
 startIn(_startIn),
 endIn(_endIn),
 speedIn(_speedIn),
 counterOut(p)
 {
  _counter.fill(0.);
 }
 
 void reset()
 {
  _counter.fill(0.);
  counterOut.reset();
 }
 
 /**
  * @brief Set the counter value on all channels.
  * 
  * @param counter The new counter value.
  */
 void setCounter(SampleType counter)
 { _counter.fill(counter); }
 
 /**
  * @brief Set the counter value on a single channel.
  * 
  * @param channel The counter to set.
  * @param counter The new counter value.
  */
 void setCounter(int channel, SampleType counter)
 { _counter[channel] = counter; }
 
 int startProcess(int startPoint, int sampleCount)
 { return std::min(sampleCount, StepSize); }

 void stepProcess(int startPoint, int sampleCount)
 {
  for (int c = 0; c < Count; ++c)
  {
   SampleType speed = speedIn(c, 0);
   for (int i = startPoint, s = sampleCount; s--; ++i)
   {
    _counter[c] += speed;
    if (_counter[c] < startIn(c, i)) _counter[c] = startIn(c, i);
    if (_counter[c] > endIn(c, i)) _counter[c] = endIn(c, i);
    counterOut.buffer(c, i) = _counter[c];
   }
  }
 }
};










/**
 * @brief A counter object.
 * 
 * This counter counts up or down (depnding on the value for StepSize). When the counter reaches one of the boundaries, it loops back to the other boundary.
 * 
 * @tparam StartIn Couples to a signal which specifies the start point of the counter. Can have as many channels as you like.
 * @tparam EndIn Couples to a signal which specifies the end point of the counter. Must have the same number of channels as StartIn.
 * @tparam SpeedIn Couples to a signal which specifies the speed of the counter. Must have the same number of channels as StartIn.
 */
template <typename StartIn, typename EndIn, typename SpeedIn>
class LoopCounter : public Component<LoopCounter<StartIn, EndIn, SpeedIn>>
{
 static_assert(StartIn::Count == EndIn::Count && StartIn::Count == SpeedIn::Count, "LoopSignal expects all control signals to have the same channel count");
 // Private data members here
 std::array<SampleType, StartIn::Count> _counter;
public:
 static constexpr int Count = StartIn::Count;
 
 // Read only access to internal counter
 const std::array<SampleType, StartIn::Count> &currentCount {_counter};
 
 // Specify your inputs as public members here
 StartIn startIn;
 EndIn endIn;
 SpeedIn speedIn;
 
 // Specify your outputs like this
 Output<Count> counterOut;
 
 // Include a definition for each input in the constructor
 LoopCounter(Parameters &p, StartIn _startIn, EndIn _endIn, SpeedIn _speedIn) :
 startIn(_startIn),
 endIn(_endIn),
 speedIn(_speedIn),
 counterOut(p)
 {
  _counter.fill(0.);
 }
 
 // This function is responsible for clearing the output buffers to a default state when
 // the component is disabled.
 void reset()
 {
  _counter.fill(0.);
  counterOut.reset();
 }
 
 /**
  * @brief Set the counter value on all channels.
  * 
  * @param counter The new counter value.
  */
 void setCounter(SampleType counter)
 { _counter.fill(counter); }
 
 /**
  * @brief Set the counter value on a single channel.
  * 
  * @param channel The counter to set.
  * @param counter The new counter value.
  */
 void setCounter(int channel, SampleType counter)
 { _counter[channel] = counter; }
 
 void stepProcess(int startPoint, int sampleCount)
 {
  for (int c = 0; c < Count; ++c)
  {
   for (int i = startPoint, s = sampleCount; s--; ++i)
   {
    _counter[c] += speedIn(c, i);
//    _counter[c] += (endIn(c, i) - startIn(c, i))*(_counter[c] <= startIn(c, i));
//    _counter[c] -= (endIn(c, i) - startIn(c, i))*(_counter[c] >= endIn(c, i));
    if (_counter[c] <= startIn(c, i)) _counter[c] += (endIn(c, i) - startIn(c, i));
    if (_counter[c] >= endIn(c, i)) _counter[c] -= (endIn(c, i) - startIn(c, i));
    counterOut.buffer(c, i) = _counter[c];
   }
  }
 }
};










/**
 * @brief Selects between two different signals depending on the sign of a third.
 * 
 * @tparam TopIn Couples to an input which will be put out if SwitchIn is above 0.
 * @tparam BottomIn Couples to an input which will be put out if SwitchIn is equal to or below 0.
 * @tparam SwitchIn Couples to the input which selects either TopIn or ButtomIn.
 */
template <typename TopIn, typename BottomIn, typename SwitchIn>
class TopBottomSwitch : public Component<TopBottomSwitch<TopIn, BottomIn, SwitchIn>>
{
 static_assert(TopIn::Count == BottomIn::Count && TopIn::Count == SwitchIn::Count, "TopBottomSwitch expects all inputs to have the same number of channels");
public:
 static constexpr int Count = TopIn::Count;
 
 TopIn topIn;
 BottomIn bottomIn;
 SwitchIn switchIn;
 
 Output<Count> signalOut;
 
 TopBottomSwitch(Parameters &p,
                 TopIn _topIn,
                 BottomIn _bottomIn,
                 SwitchIn _switchIn) :
 topIn(_topIn),
 bottomIn(_bottomIn),
 switchIn(_switchIn),
 signalOut(p)
 {}
 
 void reset()
 {
  signalOut.reset();
 }
 
 void stepProcess(int startPoint, int sampleCount)
 {
  for (int c = 0; c < Count; ++c)
  {
   for (int i = startPoint, s = sampleCount; s--; ++i)
   {
    signalOut.buffer(c, i) =
    topIn(c, i)*(switchIn(c, i) > 0.) +
    bottomIn(c, i)*(switchIn(c, i) <= 0.);
   }
  }
 }
};










}

#endif /* XDDSP_Utilities_h */
