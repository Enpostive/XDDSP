//
//  XDDSP_Envelopes.h
//  XDDSPTestingHarness
//
//  Created by Adam Jackson on 28/5/2022.
//

#ifndef XDDSP_Envelopes_h
#define XDDSP_Envelopes_h


#include "XDDSP_Classes.h"
#include "XDDSP_PiecewiseEnvelopeData.h"










namespace XDDSP
{









/**
 * @brief A component for generating ramp signals.
 * 
 * @tparam StartIn Couples to the signal specifying the start point of the ramp.
 * @tparam EndIn Couples to the signal specifying the end point of the ramp.
 * @tparam StepSize An optional parameter to change the step size. The ramp is updated at the beginning of each step. Default 16, or change it to be MAX_INT if you are using constant start and end signals.
 */
template <typename StartIn, typename EndIn, int StepSize = 16>
class Ramp : public Component<Ramp<StartIn, EndIn>, StepSize>
{
 static_assert(StartIn::Count == EndIn::Count, "Ramp requires both control signals to have the same number of channels");
 
 int rampTime;
 int rampLength;
 
public:
 static constexpr int Count = StartIn::Count;
 
 // Specify your inputs as public members here
 StartIn startIn;
 EndIn endIn;
 
 // Specify your outputs like this
 Output<Count> rampOut;
 
 Ramp(Parameters &p, StartIn _startIn, EndIn _endIn) :
 startIn(_startIn),
 endIn(_endIn),
 rampOut(p)
 {}
 
 void reset()
 {
  rampOut.reset();
 }
 
 void stepProcess(int startPoint, int sampleCount)
 {
  for (int c = 0; c < Count; ++c)
  {
   int time = rampTime;
   int length = rampLength;
   SampleType end = endIn(c, startPoint);
   SampleType start = startIn(c, startPoint);
   SampleType delta = (end - start)/static_cast<SampleType>(length);
   for (int i = startPoint, s = sampleCount; s--; ++i)
   {
    if (time < 0)
    {
     rampOut.buffer(c, i) = start;
     ++time;
    }
    else if (time < length)
    {
     rampOut.buffer(c, i) = start + static_cast<SampleType>(time)*delta;
     ++time;
    }
    else
    {
     rampOut.buffer(c, i) = end;
    }
   }
  }
  if (rampTime < rampLength) rampTime += sampleCount;
 }
 
 /*
  */

  /**
   * @brief Set the current ramp time.
   * 
   * Use this method to trigger ramps to start. If time < 0 then Ramp counts down samples until the ramp starts. If 0 <= time <= length then Ramp starts from time and counts up to length. If time > length then Ramp stops at the end and holds.
   * 
   * @param time The time parameter.
   * @param length The length parameter.
   */
 void setRampTime(int time, int length)
 {
  rampTime = time;
  rampLength = length;
 }
};










/**
 * @brief A component for ramping from a current value to a new value.
 * 
 * This is designed so that it can be swapped in place of a ControlConstant in most cases (no modifier support)
 *
 * @tparam SignalCount The number of channels to make available.
 * @tparam DefaultRamp The length of a ramp in samples if no ramp time is specified.
 */
template <int SignalCount = 1, int DefaultRamp = 0>
class RampTo : public Component<RampTo<SignalCount>>
{
 // Private data members here
 struct RampKernel
 {
  int rampTime {0};
  int rampLength {0};
  SampleType start {0.};
  SampleType end {0.};
  SampleType delta {0.};
  SampleType ramp {0.};
  
  void reset() { *this = RampKernel(); }
  
  void set(SampleType target, int time, int length)
  {
   if (length == 0)
   {
    ramp = end = target;
    rampTime = rampLength = 0;
   }
   else
   {
    start = ramp;
    end = target;
    delta = (end - start)/static_cast<SampleType>(length);
    rampLength = length;
    rampTime = -abs(time);
   }
  }
  
  void setTo(SampleType target)
  {
   set(target, 0, DefaultRamp);
  }
  
  SampleType doStep()
  {
   if (rampTime >= rampLength)
   {
    ramp = end;
   }
   else if (rampTime < 0)
   {
    ramp = start;
    ++rampTime;
   }
   else
   {
    ramp = start + static_cast<SampleType>(rampTime)*delta;
    ++rampTime;
   }
   
   return ramp;
  }
 };
 
 std::array<RampKernel, SignalCount> ramps;
 
public:
 static constexpr int Count = SignalCount;
 
 // Specify your inputs as public members here
 // No inputs
 
 // Specify your outputs like this
 Output<Count> rampOut;
 
 RampTo(Parameters &p) :
 rampOut(p)
 {}
 
 void reset()
 {
  rampOut.reset();
 }
 
 /**
  * @brief Start a default ramp to the target value.
  * 
  * @param channel The channel to start ramping.
  * @param target The target value for the ramp to end.
  */
 void setControl(int channel, SampleType target)
 {
  dsp_assert(channel >= 0 && channel < Count);
  ramps[channel].setTo(target);
 }
 
 /**
  * @brief Start a default ramp to the target value on all channels.
  * 
  * @param target The target value for the ramp to end.
  */
 void setControl(SampleType target)
 {
  for (auto &r: ramps) r.setTo(target);
 }
 
 /**
  * @brief Get the target value for the ramp on one channel.
  * 
  * @param channel The channel to check.
  * @return SampleType The target value for the ramp.
  */
 SampleType getControl(int channel)
 {
  dsp_assert(channel >= 0 && channel < Count);
  return ramps[channel].end;
 }
 
 /**
  * @brief Get the target value for the ramp on channel 0.
  * 
  * @return SampleType The target value for the ramp.
  */
 SampleType getControl()
 { return getControl(0); }

 /**
  * @brief Start a ramp on one channel.
  * 
  * @param channel The channel to ramp.
  * @param time The time to wait before starting the ramp.
  * @param length The length of the ramp.
  * @param target The target value of the ramp.
  */
 void setRamp(int channel, int time, int length, SampleType target)
 {
  dsp_assert(channel >= 0 && channel < Count);
  ramps[channel].set(target, time, length);
 }
 
 /**
  * @brief Start a ramp on all channel.
  * 
  * @param time The time to wait before starting the ramp.
  * @param length The length of the ramp.
  * @param target The target value of the ramp.
  */
 void setRamp(int time, int length, SampleType target)
 {
  for (auto &r: ramps) r.set(target, time, length);
 }
 
 void stepProcess(int startPoint, int sampleCount)
 {
  for (int c = 0; c < Count; ++c)
  {
   for (int i = startPoint, s = sampleCount; s--; ++i)
   {
    rampOut.buffer(c, i) = ramps[c].doStep();
   }
  }
 }
};










/**
 * @brief A component for generating ADSR envelopes.
 * 
 * @tparam AttackTimeSamples Couples to the signal specifying the attack time in samples. Only one channel is allowed.
 * @tparam DecayTimeSamples Couples to the signal specifying the decay time in samples. Only one channel is allowed.
 * @tparam SustainLevel Couples to the signal to use for the sustain level. Only one channel is allowed.
 * @tparam ReleaseTimeSamples Couples to the release time in samples. Only one channel is allowed.
 * @tparam StepSize The step size to use if the attack, delay or release samples are dynamic. Only one channel is allowed.
 */
template <
typename AttackTimeSamples,
typename DecayTimeSamples,
typename SustainLevel,
typename ReleaseTimeSamples,
int StepSize = IntegerMaximum
>
class ADSRGenerator : public Component<ADSRGenerator<
AttackTimeSamples,
DecayTimeSamples,
SustainLevel,
ReleaseTimeSamples,
StepSize>, StepSize>
{
 static_assert(AttackTimeSamples::Count == 1 &&
               DecayTimeSamples::Count == 1 &&
               SustainLevel::Count == 1 &&
               ReleaseTimeSamples::Count == 1, "ADSRGenerator only accepts inputs with one channel");
 
 enum
 {
  STATE_Inactive = 0,
  STATE_Attack,
  STATE_Decay,
  STATE_Sustain,
  STATE_Release
 };
 
 Parameters &dsp;
 
 SampleType env {0.};
 int state {STATE_Inactive};
 int stateTime {0};
 SampleType stateEnv {0.};
 SampleType delta {0.};
 
 void envReset()
 {
  env = 0.;
  state = STATE_Inactive;
  stateTime = 0;
  stateEnv = 0.;
  delta = 0.;
 }
 
public:
 static constexpr int Count = 1;
 
 AttackTimeSamples attackTimeSamples;
 DecayTimeSamples decayTimeSamples;
 SustainLevel sustainLevel;
 ReleaseTimeSamples releaseTimeSamples;
 
 Output<Count> envOut;
 
 ADSRGenerator(Parameters &p,
               AttackTimeSamples _attackTimeSamples,
               DecayTimeSamples _decayTimeSamples,
               SustainLevel _sustainLevel,
               ReleaseTimeSamples _releaseTimeSamples) :
 dsp(p),
 attackTimeSamples(_attackTimeSamples),
 decayTimeSamples(_decayTimeSamples),
 sustainLevel(_sustainLevel),
 releaseTimeSamples(_releaseTimeSamples),
 envOut(p)
 {}
 
 void reset()
 {
  envReset();
  envOut.reset();
 }
 
 void stepProcess(int startPoint, int sampleCount)
 {
  for (int c = 0; c < Count; ++c)
  {
   int i = startPoint;
   int s = sampleCount;
   SampleType r;
   
   switch (state)
   {
    case STATE_Release:
     r = releaseTimeSamples(i);
     if (r <= stateTime) r = stateTime + 1;
     delta = (-env/(r - static_cast<SampleType>(stateTime)));
     while (env > 0.)
     {
      if (!s--) goto blockEnd;
      envOut.buffer(i) = env;
      env += delta;
      ++stateTime;
      ++i;
     }
     envReset();
     
    case STATE_Inactive:
     while (s--)
     {
      envOut.buffer(i) = 0.;
      ++i;
     }
     break;
     
    case STATE_Attack:
     r = attackTimeSamples(i);
     if (r <= stateTime) r = stateTime + 1;
     delta = (1. - env)/(r - static_cast<SampleType>(stateTime));
     while (env < 1.)
     {
      if (!s--) goto blockEnd;
      envOut.buffer(i) = env;
      env += delta;
      ++i;
      ++stateTime;
     }
     state = STATE_Decay;
     env = 1.;
     stateTime = 0;
     
    case STATE_Decay:
     r = decayTimeSamples(i);
     if (r <= stateTime) r = stateTime + 1;
     delta = (env - sustainLevel(i))/(r - static_cast<SampleType>(stateTime));
     while (env > sustainLevel(i))
     {
      if (!s--) goto blockEnd;
      envOut.buffer(i) = env;
      env -= delta;
      ++i;
      ++stateTime;
     }
     state = STATE_Sustain;
     
    case STATE_Sustain:
     env = sustainLevel(i);
     while (s--)
     {
      envOut.buffer(i) = env;
      ++i;
     }
     
    blockEnd:
     break;
   }
  }
 }
 
 /**
  * @brief Triggers the envelope.
  * 
  */
 void triggerEnvelope()
 {
  state = STATE_Attack;
  stateEnv = env;
  stateTime = 0;
 }
 
 /**
  * @brief Releases the envelope.
  * 
  */
 void releaseEnvelope()
 {
  if (state != STATE_Inactive)
  {
   state = STATE_Release;
   stateTime = 0;
  }
 }
 
 /**
  * @brief Returns true if the envelope is active.
  * 
  * @return true Envelope is active.
  * @return false Envelope is inactive.
  */
 bool envelopeActive()
 { return state != STATE_Inactive; }
};










/**
 * @brief This component outputs an Attack-Release envelope controlled by the time input.
 * 
 * The time signal controls the envelope. The signal should rise between 0 and 1 for the intended duration of the envelope. The RampIn and RampOut signals control the fraction of the total time that each ramp lasts for.
 * 
 * @tparam TimeIn Couples to the time input. The envelope becomes active when the time signal is between 0 and 1.
 * @tparam RampIn Couples to the signal which controls ramp in.
 * @tparam RampOut Couples to the signal which controls ramp out.
 * @tparam StepSize Controls how often the ramp signal is updated. Leave at the default for static signals.
 */
template <
typename TimeIn,
typename RampIn,
typename RampOut,
int StepSize = IntegerMaximum
>
class Trapezoid : public Component<Trapezoid<TimeIn, RampIn, RampOut, StepSize>, StepSize>
{
 static_assert(RampIn::Count == RampOut::Count, "Trapezoid expects RampIn and RampOut to have same channel count");
 static_assert(RampIn::Count == TimeIn::Count || RampIn::Count == 1, "Trapezoid expects one channel for controls, or one channel per signal input");
 
 public:
 static constexpr int Count = TimeIn::Count;
 
 TimeIn timeIn;
 RampIn rampIn;
 RampOut rampOut;
 
 Output<Count> envOut;
 
 Trapezoid(Parameters &p, TimeIn _timeIn, RampIn _rampIn, RampOut _rampOut) :
 timeIn(_timeIn),
 rampIn(_rampIn),
 rampOut(_rampOut),
 envOut(p)
 {}
 
 void reset()
 {
  envOut.reset();
 }
 
 void stepProcess(int startPoint, int sampleCount)
 {
  SampleType rIn = rampIn(startPoint);
  SampleType rOut = rampOut(startPoint);
  SampleType recRampIn = 1./rIn;
  SampleType recRampOut = 1./rOut;
  for (int c = 0; c < Count; ++c)
  {
   if (RampIn::Count > 1)
   {
    rIn = rampIn(c, startPoint);
    rOut = rampOut(c, startPoint);
    recRampIn = 1./rIn;
    recRampOut = 1./rOut;
   }
   for (int i = startPoint, s = sampleCount; s--; ++i)
   {
    SampleType x = timeIn(c, i);
    SampleType ri = recRampIn*fastBoundary(x, 0., rIn);
    SampleType ro = recRampOut*fastBoundary(1. - x, 0., rOut);
    envOut.buffer(c, i) = ri*ro;
   }
  }
 }
};










/**
 * @brief Samples a piecewise envelope, using an input to control what part of the envelope is sampled.
 * 
 * The module connects to an XDDSP::PiecewiseEnvelopeData to get the information for the envelope.
 * 
 * @tparam PositionIn Couples to a position input.
 */
template <typename PositionIn>
class PiecewiseEnvelopeSampler : public Component<PiecewiseEnvelopeSampler<PositionIn>>
{
 PiecewiseEnvelopeData *envData {nullptr};
 
public:
 static constexpr int Count = PositionIn::Count;
 
 PositionIn positionIn;
 
 Output<Count> envOut;
 
 PiecewiseEnvelopeSampler(Parameters &p, PositionIn _positionIn) :
 positionIn(_positionIn),
 envOut(p)
 {}
 
 /**
  * @brief Connect a XDDSP::PiecewiseEnvelopeData object
  * 
  * @param data The XDDSP::PiecewiseEnvelopeData to connect to.
  */
 void connect(PiecewiseEnvelopeData &data)
 { envData = &data; }
 
 /**
  * @brief Disconnect the PiecewiseData object.
  * 
  */
 void disconnect()
 { envData = nullptr; }
 
 void reset()
 {
  envOut.reset();
 }
  
 void stepProcess(int startPoint, int sampleCount)
 {
  if (envData == nullptr) return;
  for (int c = 0; c < Count; ++c)
  {
   for (int i = startPoint, s = sampleCount; s--; ++i)
   {
    envOut.buffer(c, i) = envData->resolveRandomPoint(positionIn(c, i));
   }
  }
 }
};










/**
 * @brief Run a piecewise envelope.
 * 
 * The module connects to an XDDSP::PiecewiseEnvelopeData object and plays back the envelope.
 * 
 */
class PiecewiseEnvelope : public Component<PiecewiseEnvelope>
{
 enum
 {
  ActiveModeInactive,
  ActiveModeTriggered,
  ActiveModeReleased,
  ActiveModeSustain,
  ActiveModeSustainHold,
  ActiveModeLoop
 };
 
 // Private data members here
 Parameters &dspParam;
 PiecewiseEnvelopeData *envData {nullptr};
 SampleType position {0.};
 int activeMode {ActiveModeReleased};
 SampleType loopEndPosition {0.};
 SampleType loopReturnDelta {0.};
 SampleType loopSustainPosition {0.};

public:
 static constexpr int Count = 1;
 
 Output<Count> envOut;
 
 PiecewiseEnvelope(Parameters &p) :
 dspParam(p),
 envOut(p)
 {}
 
 /**
  * @brief Connect a XDDSP::PiecewiseEnvelopeData object
  * 
  * @param data The XDDSP::PiecewiseEnvelopeData to connect to.
  */
 void connect(PiecewiseEnvelopeData &data)
 { envData = &data; }
 
 /**
  * @brief Disconnect the PiecewiseData object.
  * 
  */
 void disconnect()
 { envData = nullptr; }
 
 void reset()
 {
  activeMode = ActiveModeInactive;
  position = 0.;
  envOut.reset();
 }
 
 /**
  * @brief Start the envelope.
  * 
  */
 void triggerEnvelope()
 {
  if (envData)
  {
   activeMode = ActiveModeTriggered;
   if (envData->isLoopSustainPoint())
   {
    activeMode = ActiveModeSustain;
    loopSustainPosition = envData->getLoopStartTime();
   }
   else if (envData->getLoopStartPoint() > -1)
   {
    activeMode = ActiveModeLoop;
    loopEndPosition = envData->getLoopEndTime();
    loopReturnDelta = loopEndPosition - envData->getLoopStartTime();
   }
  }
  else activeMode = ActiveModeInactive;
  position = 0.;
 }
 
 /**
  * @brief Release the envelope
  * 
  */
 void releaseEnvelope()
 {
  activeMode = ActiveModeReleased;
 }
 
 /**
  * @brief Returns true if the envelope is active.
  * 
  * @return true Envelope is active.
  * @return false Envelope is inactive.
  */
 bool envelopeActive()
 {
  return activeMode != ActiveModeInactive;
 }
 
 /**
  * @brief Return the current playback position of the envelope.
  * 
  * @return SampleType The current position of the envelope.
  */
 SampleType currentPosition()
 { return position; }
  
 void stepProcess(int startPoint, int sampleCount)
 {
  if (envData == nullptr) return;
  int i = startPoint;
  int s = sampleCount;
  switch (activeMode)
  {
   case ActiveModeTriggered:
   case ActiveModeReleased:
    while (position < envData->getEnvelopeLength())
    {
     if (!s--) goto blockEnd;
     envOut.buffer(0, i) = envData->resolveRandomPoint(position);
     position += dspParam.sampleInterval();
     ++i;
    }
    activeMode = ActiveModeInactive;
    
   case ActiveModeInactive:
    for (; (s--); ++i)
    {
     envOut.buffer(0, i) = envData->resolveRandomPoint(position);
     position += dspParam.sampleInterval();
    }
    break;
    
   case ActiveModeSustain:
    while (position < loopSustainPosition)
    {
     if (!s--) goto blockEnd;
     envOut.buffer(0, i) = envData->resolveRandomPoint(position);
     position += dspParam.sampleInterval();
     ++i;
    }
   
    activeMode = ActiveModeSustainHold;
    position = loopSustainPosition;

   case ActiveModeSustainHold:
    for (; (s--); ++i)
    {
     envOut.buffer(0, i) = envData->resolveRandomPoint(position);
    }
    break;

   case ActiveModeLoop:
    for (; (s--); ++i)
    {
     envOut.buffer(0, i) = envData->resolveRandomPoint(position);
     position += dspParam.sampleInterval();
     if (position > loopEndPosition) position -= loopReturnDelta;
    }
    
   blockEnd:
    break;
  }
 }
};










/**
 * @brief Creates an exponential envelope signal from an input signal.
 * 
 * Attack and release times are expected in samples.
 * 
 * @tparam SignalIn Couples to the input signal.
 * @tparam RiseIn Couples to the signal controlling the rise time.
 * @tparam FallIn Couples to the signal controlling the fall time.
 * @tparam StepSize Control how often the rise/fall times are updated. Leave at default for static signals.
 */
template <
typename SignalIn,
typename RiseIn,
typename FallIn,
int StepSize = INT_MAX>
class ExponentialEnvelopeFollower : public Component<ExponentialEnvelopeFollower<SignalIn, RiseIn, FallIn, StepSize>>
{
public:
 static constexpr int Count = SignalIn::Count;
 static_assert(RiseIn::Count == FallIn::Count, "AttackIn and ReleaseIn channel counts must be equal");
 static_assert(RiseIn::Count == 1 || RiseIn::Count == SignalIn::Count, "AttackIn channel count must be equal to SignalIn count or 1");
 static constexpr int ControlCount = RiseIn::Count;
 
private:
 std::array<SampleType, Count> state;
 
public:
 
 SignalIn signalIn;
 RiseIn riseIn;
 FallIn fallIn;
 
 Output<Count> envOut;
 
 ExponentialEnvelopeFollower(Parameters &p, SignalIn _signalIn, RiseIn _riseIn, FallIn _fallIn) :
 signalIn(_signalIn),
 riseIn(_riseIn),
 fallIn(_fallIn),
 envOut(p)
 {}
 
 void reset()
 {
  state.fill(0.);
  envOut.reset();
 }
 
 void stepProcess(int startPoint, int sampleCount)
 {
  SampleType riseCoeff = (riseIn(0, startPoint) > 2) ? expCoef(riseIn(0, startPoint)) : 0.;
  SampleType fallCoeff = (fallIn(0, startPoint) > 2) ? expCoef(fallIn(0, startPoint)) : 0.;
  
  for (int c = 0; c < Count; ++c)
  {
   if (ControlCount > 1 && c > 0)
   {
    riseCoeff = (riseIn(c, 0) > startPoint) ? expCoef(riseIn(c, startPoint)) : 0.;
    fallCoeff = (fallIn(c, 0) > startPoint) ? expCoef(fallIn(c, startPoint)) : 0.;
   }
   
   for (int i = startPoint, s = sampleCount; s--; ++i)
   {
    const SampleType t = signalIn(c, i);
    const SampleType coeff = (t > state[c])*riseCoeff + (t < state[c])*fallCoeff;
    expTrack(state[c], t, coeff);
    envOut.buffer(c, i) = state[c];
   }
  }
 }
};

 
 
 
 
 
 
 
 

/**
 * @brief Creates a linear envelope signal from an input signal.
 * 
 * Attack and release times are expected in samples.
 * 
 * @tparam SignalIn Couples to the input signal.
 * @tparam RiseIn Couples to the signal controlling the rise time.
 * @tparam FallIn Couples to the signal controlling the fall time.
 * @tparam StepSize Control how often the rise/fall times are updated. Leave at default for static signals.
 */
template <
typename SignalIn,
typename RiseIn,
typename FallIn,
int StepSize = INT_MAX>
class LinearEnvelopeFollower : public Component<LinearEnvelopeFollower<SignalIn, RiseIn, FallIn, StepSize>>
{
public:
 static constexpr int Count = SignalIn::Count;
 static_assert(RiseIn::Count == FallIn::Count, "AttackIn and ReleaseIn channel counts must be equal");
 static_assert(RiseIn::Count == 1 || RiseIn::Count == SignalIn::Count, "AttackIn channel count must be equal to SignalIn count or 1");
 static constexpr int ControlCount = RiseIn::Count;
 
private:
 struct State
 {
  SampleType target {0.};
  SampleType coeff {0.};
  SampleType env {0.};
 };
 
 std::array<State, Count> state;
 
public:
 SampleType flux {0.00001};

 SignalIn signalIn;
 RiseIn riseIn;
 FallIn fallIn;

 Output<Count> envOut;
 
 LinearEnvelopeFollower(Parameters &p, SignalIn _signalIn, RiseIn _riseIn, FallIn _fallIn) :
 signalIn(_signalIn),
 riseIn(_riseIn),
 fallIn(_fallIn),
 envOut(p)
 {}
 
 void reset()
 {
  state.fill(State());
  envOut.reset();
 }
 
 void stepProcess(int startPoint, int sampleCount)
 {
  SampleType riseTime = fastMax(riseIn(0, startPoint), 1.);
  SampleType fallTime = fastMax(fallIn(0, startPoint), 1.);
  
  for (int c = 0; c < Count; ++c)
  {
   if (ControlCount > 1 && c > 0)
   {
    riseTime = fastMax(riseIn(c, startPoint), 1.);
    fallTime = fastMax(fallIn(c, startPoint), 1.);
   }
   
   for (int i = startPoint, s = sampleCount; s--; ++i)
   {
    SampleType x = signalIn(c, i);
    SampleType t = x - state[c].target;
    if (fabs(t) > flux)
    {
     state[c].target = x;
     t = x - state[c].env;
     SampleType rampTime = (t > 0)*riseTime + (t <= 0)*fallTime;
     state[c].coeff = t / rampTime;
    }
    
    t = state[c].target - state[c].env;
    state[c].env += fastBoundary(state[c].coeff, fastMin(t, 0.), fastMax(t, 0.));
    envOut.buffer(c, i) = state[c].env;
   }
  }
 }
};

 
 
 
 
 
 
 
 

/**
 * @brief Takes an envelope signal as input and produces a gain signal for dynamics control.
 * 
 * This component produces a gain signal in response to an envelope signal, according to the specified dynamics settings.
 * 
 * @tparam SignalIn Couples with the input signal.
 */
template <typename SignalIn>
class DynamicsProcessingGainSignal : public Component<DynamicsProcessingGainSignal<SignalIn>>
{
 SampleType threshold;
 SampleType knee;
 SampleType ratioAbove;
 SampleType ratioBelow;
 SampleType makeup;
 SampleType makeupLinear;
 
 SampleType threshLinear;
 SampleType lThresh;
 SampleType hThresh;
 SampleType recipDThresh;
 
 SampleType channelLink;
 
 bool pk;
 
 SampleType maxGainLinear;
 SampleType maxGain;
 
public:
 static constexpr int Count = SignalIn::Count;
 
 SignalIn signalIn;
 
 Output<Count> signalOut;
 
 DynamicsProcessingGainSignal(Parameters &p, SignalIn _signalIn) :
 signalIn(_signalIn),
 signalOut(p)
 {
  setThresholdAndKnee(-12., 0.);
  setRatioAbove(2.);
  setRatioBelow(1.);
  setMakeup(0.);
  setMaxGain(36);
  setChannelLink(1.);
 }
 
 void reset()
 { signalOut.reset(); }
 
 /**
  * @brief Set the threshold and knee settings together.
  * 
  * @param threshDB New threshold.
  * @param kneeDB New knee.
  */
 void setThresholdAndKnee(SampleType threshDB, SampleType kneeDB)
 {
  if (kneeDB < 0.) kneeDB = 0.;
//  if (kneeDB > threshDB) kneeDB = threshDB;
  
  threshold = threshDB;
  knee = kneeDB;
  
  threshLinear = dB2Linear(threshold);
  lThresh = threshLinear / dB2Linear(knee);
  hThresh = 2.0 * threshLinear - lThresh;
  pk = (hThresh != lThresh);
  recipDThresh = (hThresh == lThresh) ? 0. : (1. / (hThresh - lThresh));
 }
 
 /**
  * @brief Set just the threshold setting.
  * 
  * @param db New threshold.
  */
 void setThreshold(SampleType db)
 { setThresholdAndKnee(db, knee); }
 
 /**
  * @brief Set just the knee setting.
  * 
  * @param db New knee.
  */
 void setKnee(SampleType db)
 { setThresholdAndKnee(threshold, db); }
 
 /**
  * @brief Get the threshold setting.
  * 
  * @return SampleType Current threhold setting in decibels.
  */
 SampleType getThreshold() const
 { return threshold; }
 
 /**
  * @brief Get the knee setting.
  * 
  * @return SampleType Current knee setting in decibels.
  */
 SampleType getKnee() const
 { return knee; }
 
 /**
  * @brief Set the ratio to apply to the gain setting when the envelope goes above the threshold.
  * 
  * @param r New ratio
  */
 void setRatioAbove(SampleType r)
 { ratioAbove = (r == 0.) ? 0. : (1.0 / r); }

 /**
  * @brief Get the ratio being applied to the gain setting when the envelope goes above the threshold.
  * 
  * @return SampleType The current ratio setting.
  */
 SampleType getRatioAbove() const
 { return (ratioAbove == 0.) ? 0. : 1./ratioAbove; }
 
 /**
  * @brief Set the ratio to apply to the gain setting when the envelope goes below the threshold.
  * 
  * @param r New ratio
  */
 void setRatioBelow(SampleType r)
 { ratioBelow = 1.0 / r; }
 
 /**
  * @brief Get the ratio being applied to the gain setting when the envelope goes below the threshold.
  * 
  * @return SampleType The current ratio setting.
  */
 SampleType getRatioBelow() const
 { return (ratioBelow == 0.) ? 0. : 1./ratioBelow; }
 
 /**
  * @brief Configure the dynamics processor as a limiter.
  * 
  */
 void setLimit()
 { ratioAbove = 0.; }
 
 /**
  * @brief Set the makeup gain in dB.
  * 
  * @param db The makeup gain to be added to the gain signal.
  */
 void setMakeup(SampleType db)
 { makeupLinear = dB2Linear(makeup = db); }
 
 /**
  * @brief Get the makeup gain in dB.
  * 
  * @return SampleType The current makeup gain.
  */
 SampleType getMakeup() const
 { return makeup; }
 
 /**
  * @brief Set a limit on the maximum amount of gain allowed to be applied.
  * 
  * @param db Maximum gain in dB.
  */
 void setMaxGain(SampleType db)
 { maxGainLinear = dB2Linear(maxGain = db); }
 
 /**
  * @brief Get the current limit on the maximum amount of gain allowed to be applied.
  * 
  * @return SampleType Maximm gain in dB.
  */
 SampleType getMaxGain() const
 { return maxGain; }
 
 /**
  * @brief Set the channel link factor.
  * 
  * @param link A number between 0 and 1 where 0 is full stereo and 1 is completely linked.
  */
 void setChannelLink(SampleType link)
 { channelLink = fastBoundary(link, 0., 1.); }
 
 /**
  * @brief Get the current setting of the channel link factor.
  * 
  * @return SampleType The current channel link factor.
  */
 SampleType getChannelLink() const
 { return channelLink; }
 
 /**
  * @brief Caclulate the gain curve for a given envelope input.
  * 
  * This code is exposed so that interface widgets can graph the gain curve accurately using the same code used to do the actual gain reduction.
  * 
  * @param e Envelope input to calculate.
  * @return SampleType Resulting gain value.
  */
 SampleType computeGainCurve(SampleType e)
 {
  SampleType ga, gb, d, c, s;
  e = std::max(e, static_cast<SampleType>(0.0000001));

  d = pk ? fastBoundary((e - lThresh)*recipDThresh, 0., 1.) : 1.*(e > lThresh);
  s = lThresh - lThresh*d + threshLinear*d;
  c = 1. - d + ratioAbove*d;
  ga = c - (s*c - s)/e;
  
  d = pk ? fastBoundary((hThresh - e)*recipDThresh, 0., 1.) : 1.*(e < lThresh);
  s = hThresh - hThresh*d + threshLinear*d;
  c = 1. - d + ratioBelow*d;
  gb = std::max(c - (s*c - s)/e, static_cast<SampleType>(0.));
  
  return fastBoundary(ga*gb*makeupLinear, 0., maxGainLinear);
 }
 
 void stepProcess(int startPoint, int sampleCount)
 {
  const MixingLaws::MixWeights mix = MixingLaws::LinearFadeLaw::getWeights(channelLink);
  const SampleType cm = std::get<0>(mix);
  const SampleType lm = std::get<1>(mix);
  
  for (int i = startPoint, s = sampleCount; s--; ++i)
  {
   SampleType link = 0.;
   for (int c = 0; c < Count; ++c) link = fastMax(signalIn(c, i), link);
   for (int c = 0; c < Count; ++c) signalOut.buffer(c, i) = computeGainCurve(cm*signalIn(c, i) + lm*link);
  }
 }
};

 
 
 
 
 
 
 
 

}

#endif /* XDDSP_Envelopes_h */
