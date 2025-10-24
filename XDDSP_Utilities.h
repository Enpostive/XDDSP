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










template <typename SignalIn, int SignalCount>
class MixDown : public Component<MixDown<SignalIn, SignalCount>>
{
 // Private data members here
public:
 static constexpr int ChannelCount = SignalIn::Count;
 
 // Specify your inputs as public members here
 std::array<SignalIn, SignalCount> signalsIn;
 
 // Specify your outputs like this
 Output<ChannelCount> signalOut;
 
 // Include a definition for each input in the constructor
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










template <typename SignalIn, typename RectifyLevelIn>
class Rectifier : public Component<Rectifier<SignalIn, RectifyLevelIn>>
{
 static_assert(RectifyLevelIn::Count == 1, "Rectifier expects a single channel control source");
 // Private data members here
public:
 static constexpr int Count = SignalIn::Count;

 // Specify your inputs as public members here
 SignalIn signalIn;
 RectifyLevelIn rectifyLevelIn;
 
 // Specify your outputs like this
 Output<SignalIn::Count> signalOut;
 
 // Include a definition for each input in the constructor
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










template <typename SignalIn>
class SignalDelta : public Component<SignalDelta<SignalIn>>
{
 // Private data members here
 Parameters &dspParam;
 std::array<SampleType, SignalIn::Count> h;
 
public:
 static constexpr int Count = SignalIn::Count;
 
 // Specify your inputs as public members here
 SignalIn signalIn;
 
 // Specify your outputs like this
 Output<Count> signalOut;
 
 // Include a definition for each input in the constructor
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










template <typename SignalIn, typename MinimumIn, typename MaximumIn>
class Clipper : public Component<Clipper<SignalIn, MinimumIn, MaximumIn>>
{
 static_assert(MinimumIn::Count == 1 && MaximumIn::Count == 1, "Clipper expects control signals with single channels");
 // Private data members here
public:
 static constexpr int Count = SignalIn::Count;
 
 // Specify your inputs as public members here
 SignalIn signalIn;
 MinimumIn minimumIn;
 MaximumIn maximumIn;
 
 // Specify your outputs like this
 Output<Count> signalOut;
 
 // Include a definition for each input in the constructor
 Clipper(Parameters &p,
         SignalIn _signalIn,
         MinimumIn _minimumIn,
         MaximumIn _maximumIn) :
 signalIn(_signalIn),
 minimumIn(_minimumIn),
 maximumIn(_maximumIn),
 signalOut(p)
 {}
 
 // This function is responsible for clearing the output buffers to a default state when
 // the component is disabled.
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










template <typename SignalIn, int InputCount>
class Maximum : public Component<Maximum<SignalIn, InputCount>>
{
 // Private data members here
public:
 static constexpr int Count = SignalIn::Count;
 
 // Specify your inputs as public members here
 std::array<SignalIn, InputCount> signalIn;
 
 // Specify your outputs like this
 Output<Count> signalOut;
 
 // Include a definition for each input in the constructor
 Maximum(Parameters &p, const std::array<SignalIn, InputCount> &_signalIn) :
 signalIn(_signalIn),
 signalOut(p)
 {}
 
 // This function is responsible for clearing the output buffers to a default state when
 // the component is disabled.
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









class TimeSignal : public Component<TimeSignal>
{
 // Private data members here
 Parameters &dspParam;
 uint64_t sampleTime {0};
 SampleType scalePPQ {1.};
 SampleType scaleSeconds {1.};
 bool sync {false};
 
public:
 static constexpr int Count = 1;
 
 // Specify your inputs as public members here
 // This class has no inputs
 
 // Specify your outputs like this
 Output<Count> timeSamples;
 Output<Count> timePPQ;
 Output<Count> timeSeconds;
 
 // Include a definition for each input in the constructor
 TimeSignal(Parameters &p) :
 dspParam(p),
 timeSamples(p),
 timePPQ(p),
 timeSeconds(p)
 {}
 
 // This function is responsible for clearing the output buffers to a default state when
 // the component is disabled.
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
 
 void setScalePPQ(SampleType _scale)
 {
  scalePPQ = _scale;
 }
 
 void setScaleSeconds(SampleType _scale)
 {
  scaleSeconds = _scale;
 }
 
 void setSync(bool _sync)
 { sync = _sync; }
};










template <typename StartIn, typename EndIn, typename SpeedIn, int StepSize = INT_MAX>
class Counter : public Component<Counter<StartIn, EndIn, SpeedIn, StepSize>>
{
 static_assert(StartIn::Count == EndIn::Count && StartIn::Count == SpeedIn::Count, "Counter expects all control signals to have the same channel count");
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
 Counter(Parameters &p, StartIn _startIn, EndIn _endIn, SpeedIn _speedIn) :
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
 
 void setCounter(SampleType counter)
 { _counter.fill(counter); }
 
 void setCounter(int channel, SampleType counter)
 { _counter[channel] = counter; }
 
 // startProcess prepares the component for processing one block and returns the step
 // size. By default, it returns the entire sampleCount as one big step.
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
 
 void setCounter(SampleType counter)
 { _counter.fill(counter); }
 
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










template <typename TopIn, typename BottomIn, typename SwitchIn>
class TopBottomSwitch : public Component<TopBottomSwitch<TopIn, BottomIn, SwitchIn>>
{
 static_assert(TopIn::Count == BottomIn::Count && TopIn::Count == SwitchIn::Count, "TopBottomSwitch expects all inputs to have the same number of channels");
 // Private data members here
public:
 static constexpr int Count = TopIn::Count;
 
 // Specify your inputs as public members here
 TopIn topIn;
 BottomIn bottomIn;
 SwitchIn switchIn;
 
 // Specify your outputs like this
 Output<Count> signalOut;
 
 // Include a definition for each input in the constructor
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
