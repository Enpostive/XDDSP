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









// StepSize can be increased if StartIn and EndIn are constants
template <typename StartIn, typename EndIn, int StepSize = 16>
class Ramp : public Component<Ramp<StartIn, EndIn>, StepSize>
{
 static_assert(StartIn::Count == EndIn::Count, "Ramp requires both control signals to have the same number of channels");
 
 // Private data members here
 int rampTime;
 int rampLength;
 
public:
 static constexpr int Count = StartIn::Count;
 
 // Specify your inputs as public members here
 StartIn startIn;
 EndIn endIn;
 
 // Specify your outputs like this
 Output<Count> rampOut;
 
 // Include a definition for each input in the constructor
 Ramp(Parameters &p, StartIn _startIn, EndIn _endIn) :
 startIn(_startIn),
 endIn(_endIn),
 rampOut(p)
 {}
 
 // This function is responsible for clearing the output buffers to a default state when
 // the component is disabled.
 void reset()
 {
  rampOut.reset();
 }
 
 // stepProcess is called repeatedly with the start point incremented by step size
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
  If time < 0 then ramp counts down time samples until the ramp starts
  If 0 <= time <= length then the ramp starts from time and counts up to length
  If time > length then the ramp stops at the end and holds
  */
 void setRampTime(int time, int length)
 {
  rampTime = time;
  rampLength = length;
 }
};










template <int SignalCount = 1>
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
   start = ramp;
   end = target;
   delta = (end - start)/static_cast<SampleType>(length);
   rampLength = length;
   rampTime = -abs(time);
  }
  
  void setTo(SampleType target)
  {
   ramp = end = target;
   rampTime = rampLength = 0;
  }
  
  SampleType doStep()
  {
   if (rampTime < 0)
   {
    ramp = start;
    ++rampTime;
   }
   else if (rampTime < rampLength)
   {
    ramp = start + static_cast<SampleType>(rampTime)*delta;
    ++rampTime;
   }
   else
   {
    ramp = end;
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
 
 // Include a definition for each input in the constructor
 RampTo(Parameters &p) :
 rampOut(p)
 {}
 
 // This function is responsible for clearing the output buffers to a default state when
 // the component is disabled.
 void reset()
 {
  rampOut.reset();
 }
 
 void setControl(int channel, SampleType target)
 {
  dsp_assert(channel >= 0 && channel < Count);
  ramps[channel].setTo(target);
 }
 
 void setRamp(int channel, int time, int length, SampleType target)
 {
  dsp_assert(channel >= 0 && channel < Count);
  ramps[channel].set(target, time, length);
 }
 
 // stepProcess is called repeatedly with the start point incremented by step size
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
 
 // Private data members here
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
 
 // Specify your inputs as public members here
 AttackTimeSamples attackTimeSamples;
 DecayTimeSamples decayTimeSamples;
 SustainLevel sustainLevel;
 ReleaseTimeSamples releaseTimeSamples;
 
 // Specify your outputs like this
 Output<Count> envOut;
 
 // Include a definition for each input in the constructor
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
 
 // This function is responsible for clearing the output buffers to a default state when
 // the component is disabled.
 void reset()
 {
  envReset();
  envOut.reset();
 }
 
 // startProcess prepares the component for processing one block and returns the step
 // size. By default, it returns the entire sampleCount as one big step.
 // int startProcess(int startPoint, int sampleCount)
 // { return std::min(sampleCount, StepSize); }
 
 // stepProcess is called repeatedly with the start point incremented by step size
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
 
 void triggerEnvelope()
 {
  state = STATE_Attack;
  stateEnv = env;
  stateTime = 0;
 }
 
 void releaseEnvelope()
 {
  if (state != STATE_Inactive)
  {
   state = STATE_Release;
   stateTime = 0;
  }
 }
 
 bool envelopeActive()
 { return state != STATE_Inactive; }
};









template <
typename SignalIn,
typename RampIn,
typename RampOut,
int StepSize = IntegerMaximum
>
class Trapezoid : public Component<Trapezoid<SignalIn, RampIn, RampOut, StepSize>, StepSize>
{
 static_assert(RampIn::Count == RampOut::Count, "Trapezoid expects RampIn and RampOut to have same channel count");
 static_assert(RampIn::Count == SignalIn::Count || RampIn::Count == 1, "Trapezoid expects one channel for controls, or one channel per signal input");
 
 // Private data members here
public:
 static constexpr int Count = SignalIn::Count;
 
 // Specify your inputs as public members here
 SignalIn signalIn;
 RampIn rampIn;
 RampOut rampOut;
 
 // Specify your outputs like this
 Output<Count> envOut;
 
 // Include a definition for each input in the constructor
 Trapezoid(Parameters &p, SignalIn _signalIn, RampIn _rampIn, RampOut _rampOut) :
 signalIn(_signalIn),
 rampIn(_rampIn),
 rampOut(_rampOut),
 envOut(p)
 {}
 
 // This function is responsible for clearing the output buffers to a default state when
 // the component is disabled.
 void reset()
 {
  envOut.reset();
 }
 
 // stepProcess is called repeatedly with the start point incremented by step size
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
    SampleType x = signalIn(c, i);
    SampleType ri = recRampIn*fastBoundary(x, 0., rIn);
    SampleType ro = recRampOut*fastBoundary(1. - x, 0., rOut);
    envOut.buffer(c, i) = ri*ro;
   }
  }
 }
};









template <typename PositionIn, typename PiecewiseData>
class PiecewiseEnvelopeSampler : public Component<PiecewiseEnvelopeSampler<PositionIn, PiecewiseData>>
{
 // Private data members here
 PiecewiseData *envData {nullptr};
 
public:
 static constexpr int Count = PositionIn::Count;
 
 // Specify your inputs as public members here
 PositionIn positionIn;
 
 // Specify your outputs like this
 Output<Count> envOut;
 
 // Include a definition for each input in the constructor
 PiecewiseEnvelopeSampler(Parameters &p, PositionIn _positionIn) :
 positionIn(_positionIn),
 envOut(p)
 {}
 
 void connect(PiecewiseData &data)
 { envData = &data; }
 
 void disconnect()
 { envData = nullptr; }
 
 // This function is responsible for clearing the output buffers to a default state when
 // the component is disabled.
 void reset()
 {
  envOut.reset();
 }
  
 // stepProcess is called repeatedly with the start point incremented by step size
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










template <typename PiecewiseData>
class PiecewiseEnvelope : public Component<PiecewiseEnvelope<PiecewiseData>>
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
 PiecewiseData *envData {nullptr};
 SampleType position {0.};
 int activeMode {ActiveModeReleased};
 SampleType loopEndPosition {0.};
 SampleType loopReturnDelta {0.};
 SampleType loopSustainPosition {0.};

public:
 static constexpr int Count = 1;
 
 // No Inputs
 
 // Specify your outputs like this
 Output<Count> envOut;
 
 // Include a definition for each input in the constructor
 PiecewiseEnvelope(Parameters &p) :
 dspParam(p),
 envOut(p)
 {}
 
 void connect(PiecewiseData &data)
 { envData = &data; }
 
 void disconnect()
 { envData = nullptr; }
 
 // This function is responsible for clearing the output buffers to a default state when
 // the component is disabled.
 void reset()
 {
  activeMode = ActiveModeInactive;
  position = 0.;
  envOut.reset();
 }
 
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
 
 void releaseEnvelope()
 {
  activeMode = ActiveModeReleased;
 }
 
 bool envelopeActive()
 {
  return activeMode != ActiveModeInactive;
 }
 
 SampleType currentPosition()
 { return position; }
  
 // stepProcess is called repeatedly with the start point incremented by step size
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
 
 // finishProcess is called after the block has been processed
 // void finishProcess()
 // {}
};










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
 
 // Specify your inputs as public members here
 SignalIn signalIn;
 RiseIn riseIn;
 FallIn fallIn;
 
 // Specify your outputs like this
 Output<Count> envOut;
 
 // Include a definition for each input in the constructor
 ExponentialEnvelopeFollower(Parameters &p, SignalIn _signalIn, RiseIn _riseIn, FallIn _fallIn) :
 signalIn(_signalIn),
 riseIn(_riseIn),
 fallIn(_fallIn),
 envOut(p)
 {}
 
 // This function is responsible for clearing the output buffers to a default state when
 // the component is disabled.
 void reset()
 {
  state.fill(0.);
  envOut.reset();
 }
 
 // startProcess prepares the component for processing one block and returns the step
 // size. By default, it returns the entire sampleCount as one big step.
// int startProcess(int startPoint, int sampleCount)
// { return std::min(sampleCount, StepSize); }

 // stepProcess is called repeatedly with the start point incremented by step size
 void stepProcess(int startPoint, int sampleCount)
 {
  // Attack and release times are measured in samples
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
 
 // finishProcess is called after the block has been processed
// void finishProcess()
// {}
};

 
 
 
 
 
 
 
 
 
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

 // Specify your inputs as public members here
 SignalIn signalIn;
 RiseIn riseIn;
 FallIn fallIn;

 // Specify your outputs like this
 Output<Count> envOut;
 
 // Include a definition for each input in the constructor
 LinearEnvelopeFollower(Parameters &p, SignalIn _signalIn, RiseIn _riseIn, FallIn _fallIn) :
 signalIn(_signalIn),
 riseIn(_riseIn),
 fallIn(_fallIn),
 envOut(p)
 {}
 
 // This function is responsible for clearing the output buffers to a default state when
 // the component is disabled.
 void reset()
 {
  state.fill(State());
  envOut.reset();
 }
 
 // startProcess prepares the component for processing one block and returns the step
 // size. By default, it returns the entire sampleCount as one big step.
// int startProcess(int startPoint, int sampleCount)
// { return std::min(sampleCount, StepSize); }

 // stepProcess is called repeatedly with the start point incremented by step size
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
 
 // finishProcess is called after the block has been processed
// void finishProcess()
// {}
};

 
 
 
 
 
 
 
 
 
template <typename SignalIn>
class DynamicsProcessingGainSignal : public Component<DynamicsProcessingGainSignal<SignalIn>>
{
 // Private data members here
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
 
 // Specify your inputs as public members here
 SignalIn signalIn;
 
 // Specify your outputs like this
 Output<Count> signalOut;
 
 // Include a definition for each input in the constructor
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
 
 // This function is responsible for clearing the output buffers to a default state when
 // the component is disabled.
 void reset()
 { signalOut.reset(); }
 
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
 
 void setThreshold(SampleType db)
 { setThresholdAndKnee(db, knee); }
 
 void setKnee(SampleType db)
 { setThresholdAndKnee(threshold, db); }
 
 SampleType getThreshold() const
 { return threshold; }
 
 SampleType getKnee() const
 { return knee; }
 
 void setRatioAbove(SampleType r)
 { ratioAbove = (r == 0.) ? 0. : (1.0 / r); }

 SampleType getRatioAbove() const
 { return (ratioAbove == 0.) ? 0. : 1./ratioAbove; }
 
 void setRatioBelow(SampleType r)
 { ratioBelow = 1.0 / r; }
 
 SampleType getRatioBelow() const
 { return (ratioBelow == 0.) ? 0. : 1./ratioBelow; }
 
 void setLimit()
 { ratioAbove = 0.; }
 
 void setMakeup(SampleType db)
 { makeupLinear = dB2Linear(makeup = db); }
 
 SampleType getMakeup() const
 { return makeup; }
 
 void setMaxGain(SampleType db)
 { maxGainLinear = dB2Linear(maxGain = db); }
 
 SampleType getMaxGain() const
 { return maxGain; }
 
 void setChannelLink(SampleType link)
 { channelLink = fastBoundary(link, 0., 1.); }
 
 SampleType getChannelLink() const
 { return channelLink; }
 
 SampleType computeGainCurve(SampleType e)
 {
  SampleType ga, gb, d, c, s;
  e = std::max(e, 0.0000001);

  d = pk ? fastBoundary((e - lThresh)*recipDThresh, 0., 1.) : 1.*(e > lThresh);
  s = lThresh - lThresh*d + threshLinear*d;
  c = 1. - d + ratioAbove*d;
  ga = c - (s*c - s)/e;
  
  d = pk ? fastBoundary((hThresh - e)*recipDThresh, 0., 1.) : 1.*(e < lThresh);
  s = hThresh - hThresh*d + threshLinear*d;
  c = 1. - d + ratioBelow*d;
  gb = std::max(c - (s*c - s)/e, 0.);
  
  return fastBoundary(ga*gb*makeupLinear, 0., maxGainLinear);
 }
 
 // startProcess prepares the component for processing one block and returns the step
 // size. By default, it returns the entire sampleCount as one big step.
// int startProcess(int startPoint, int sampleCount)
// { return std::min(sampleCount, StepSize); }

 // stepProcess is called repeatedly with the start point incremented by step size
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
 
 // finishProcess is called after the block has been processed
// void finishProcess()
// {}
};

 
 
 
 
 
 
 
 
/*
 A component for finding a piecewise envelope that will fit over the input signal. The input signal must be an audio signal with no DC offset. The best case is for the signal to contain as little low frequency content as possible, however low frequency content can easily be handled as long as the buffer length is appropriate and you don't mind a little latency!
*/
template <typename SignalIn>
class PiecewiseEnvelopeFinder : public Component<PiecewiseEnvelopeFinder<SignalIn>>, public Parameters::ParameterListener
{
public:
 static constexpr int Count = SignalIn::Count;

 struct Maxima
 {
  SampleType amp;
  int time;
  
  operator bool() const
  { return time != -1; }
  
  void invalidate()
  { time = -1; }
 };

private:
 
 struct MaximaRegion
 {
  SampleType amp {0.};
  int time {-1}; // -1 here invalidates all following regions
  int prevZeroCrossing {-1};
  int endOfRegion {-1}; // -1 here indicates that this region is still growing
  
  MaximaRegion() {}
  
  MaximaRegion(SampleType a, int t, int prev, int end = -1)
  : amp(a), time(t), prevZeroCrossing(prev), endOfRegion(end)
  {}
  
  Maxima maximaAtTop() const
  {
   dsp_assert(time != -1);
   return {amp, time};
  }
  
  Maxima maximaAtZeroCrossing() const
  {
   dsp_assert(time != -1);
   return {amp, prevZeroCrossing};
  }
  
  Maxima maximaAtEndOfRegion() const
  {
   dsp_assert(time != -1);
   return {amp, endOfRegion};
  }
  
  int length() const
  { return endOfRegion - prevZeroCrossing; }
  
  operator bool() const
  { return time != -1; }
  
  void invalidate()
  { time = -1; }
 };
 
 Parameters &dspParam;


 // Audio buffers
 std::array<DynamicCircularBuffer<>, Count> buffer;
 
 // Maxima buffers
 std::array<DynamicCircularBuffer<Maxima>, Count> envMaxima;
 int currentEnvMaximaBufferSize {512};
 std::array<DynamicCircularBuffer<MaximaRegion>, Count> regions;
 
 // Local parameters
 SampleType clumpingFrequency {200.};
 SampleType zeroThreshold;
 SampleType risingSlopeMultiplier {0.5};
 SampleType fallingSlopeMultiplier {0.125};
 SampleType bigJumpFraction {1./4.};
 SampleType maximumRegionLengthSeconds {0.1};

 // Processing variables
 int clumpingLength;
 int lengthBufferSamples;
 int lengthRegionBuffer;
 std::array<int, Count> lastRegionProcessed;
 SampleType clumpingSlope;
 SampleType fallingSlope;
 SampleType risingSlope;
 int maximumRegionSize;
 int envSamplePoint;

 Maxima& maxima(int c, int i)
 { return envMaxima[c].tapOut(i); }
 
 MaximaRegion& region(int c, int i)
 { return regions[c].tapOut(i); }

 void initEnvelopeMaximaBuffer()
 {
  for (auto &m: envMaxima) m.setMaximumLength(currentEnvMaximaBufferSize);
  resetEnvelopeMaximaBuffer();
 }
 
 void resetEnvelopeMaximaBuffer()
 {
  for (int c = 0; c < Count; ++c)
  {
   envMaxima[c].reset(Maxima());
   for (int i = currentEnvMaximaBufferSize; i > 0; --i)
   {
    envMaxima[c].tapIn({zeroThreshold, i*clumpingLength});
   }
  }
 }
 
 void initAndResetRegions()
 {
  lengthRegionBuffer = lengthBufferSamples;
  for (int c = 0; c < Count; ++c)
  {
   regions[c].setMaximumLength(lengthRegionBuffer);
   // Insert one record invalidating all prior records
   regions[c].tapIn({zeroThreshold, -1, -1, -1});
  }
 }

 void advanceMaximaBuffers(int advanceAmount)
 {
  for (auto &m: envMaxima)
  {
   int i = 0;
   bool stillInBuffer = true;
   while (i < currentEnvMaximaBufferSize && stillInBuffer)
   {
    // Before we incrememt, do the test to see if were still in the buffer
    stillInBuffer = (m.tapOut(i).time < lengthBufferSamples);
    // We still want to increment the first maxima that has left the buffer
    m.tapOut(i).time += advanceAmount;
    // and then we invalidate the next maxima
    ++i;
   }
   if (i < currentEnvMaximaBufferSize) m.tapOut(i).invalidate();
  }
  
  for (auto &m: regions)
  {
   int i = 0;
   // We only advance the maxima records up to the length of the buffer
   // Any records that come after that are invalidated instead
   while (m.tapOut(i).time != -1)
   {
    MaximaRegion &r = m.tapOut(i);
    r.time += advanceAmount;
    r.prevZeroCrossing += advanceAmount;
    r.endOfRegion += advanceAmount;
    if (r.endOfRegion >= lengthBufferSamples) r.invalidate();
    else ++i;
   }
  }
 }
 
 void insertMaximaAtEnd(int c, Maxima m)
 {
  if (m.amp < zeroThreshold) m.amp = zeroThreshold;
  envMaxima[c].tapIn(m);
 }

 void insertEnvelopeMaximaInSlotBefore(int c, Maxima m, int index = 0)
 {
  // Inserts a maxima at the slot before the index
  // Insert it at the end normally
  envMaxima[c].tapIn(m);
  // Then use swaps to bubble it down the buffer into the correct place
  for (int i = 0; i <= index; ++i)
  {
   std::swap(maxima(c, i), maxima(c, i + 1));
  }
 }
 
 int findZeroCrossingBefore(int c, int n)
 {
  SampleType sgn = signum(buffer[c].tapOut(n));
  while (++n < lengthBufferSamples &&
         sgn == signum(buffer[c].tapOut(n)));
  --n;
  return n;
 }
 
 int findZeroCrossingAfter(int c, int n)
 {
  SampleType sgn = signum(buffer[c].tapOut(n));
  while (n > 0 &&
         sgn == signum(buffer[c].tapOut(--n)));
  return n;
 }
 
 std::pair<SampleType, int> findMaximumAmplitudeInBuffer(int c, int start, int end)
 {
  SampleType max = fabs(buffer[c].tapOut(start));
  int time = start;
  for (int i = start + 1; i < end; ++i)
  {
   SampleType s = fabs(buffer[c].tapOut(i));
   if (s > max)
   {
    max = s;
    time = i;
   }
  }
  
  return {max, time};
 }
 
 std::pair<bool, SampleType> validateArbitrarySegment(const Maxima &start,
                                                      const Maxima &end,
                                                      int c)
 {
  SampleType max = fabs(buffer[c].tapOut(end.time));
  bool valid = end.amp > fabs(buffer[c].tapOut(end.time));
  for (int i = end.time + 1; i <= start.time; ++i)
  {
   const SampleType s = fabs(buffer[c].tapOut(i));
   const SampleType e = interpolatePointBetweenArbitraryMaxima(start, end, i);
   max = std::max(s, max);
   if (e <= s) valid = false;
  }
  
  return {valid, max};
 }
 
 bool quickValidateArbitrarySegment(const Maxima &start,
                                    const Maxima &end,
                                    int c)
 {
  auto [valid, max] = validateArbitrarySegment(start, end, c);
  return valid;
 }
 
 SampleType timeToReachAmplitudeAtSlope(SampleType deltaAmp, SampleType slope)
 {
  return deltaAmp/slope;
 }
 
 SampleType slopeBetweenArbitraryMaxima(const Maxima &start, const Maxima &end)
 {
  if (start.time == end.time) return 0.;
  SampleType da = end.amp - start.amp;
  SampleType dt = start.time - end.time;
  return da/dt;
 }
 
 SampleType slopeFromMaxima(const Maxima &m, int c, int index)
 {
  return slopeBetweenArbitraryMaxima(maxima(c, index), m);
 }
 
 SampleType slopeOnEnvelopeEndingAtIndex(int c, int index)
 {
  return slopeBetweenArbitraryMaxima(maxima(c, index), maxima(c, index + 1));
 }
 
 SampleType interpolatePointAlongSlopeFromArbitraryMaxima(const Maxima &m, SampleType slope, int t)
 {
  SampleType df = m.time - t;
  SampleType x = m.amp + slope*df;
  return x;
 }
 
 SampleType interpolatePointBetweenArbitraryMaxima(const Maxima &start,
                                                   const Maxima &end,
                                                   int t)
 {
  SampleType slope = slopeBetweenArbitraryMaxima(start, end);
  return interpolatePointAlongSlopeFromArbitraryMaxima(end, slope, t);
 }
 
 SampleType interpolatePointFromMaxima(const Maxima &test, int c, int index, int t)
 {
  SampleType slope = slopeFromMaxima(test, c, index);
  return interpolatePointAlongSlopeFromArbitraryMaxima(maxima(c, index), slope, t);
 }
 
 void sanitiseSegmentRecursive(int c, const Maxima &start, const Maxima &end, int endIndex, int recursion = 5)
 {
  if (recursion == 0) return;
  if (start.time == end.time) return;
  int maxTime = start.time - 1;
  SampleType s = fabs(buffer[c].tapOut(maxTime));
  SampleType maxSlope = slopeBetweenArbitraryMaxima(start, {s, maxTime});
  bool valid = s <= interpolatePointBetweenArbitraryMaxima(start, end, maxTime);
  for (int i = start.time - 2; i > end.time; --i)
  {
   s = fabs(buffer[c].tapOut(i));
   SampleType slope = slopeBetweenArbitraryMaxima(start, {s, i});
   if (slope > maxSlope)
   {
    maxSlope = slope;
    maxTime = i;
   }
   if (s > interpolatePointBetweenArbitraryMaxima(start, end, i))
   {
    valid = false;
   }
  }
  
  if (!valid)
  {
   s = fabs(buffer[c].tapOut(maxTime));
   Maxima n = {s, maxTime};
   sanitiseSegmentRecursive(c, start, n, endIndex, recursion - 1);
   insertEnvelopeMaximaInSlotBefore(c, n, endIndex);
   sanitiseSegmentRecursive(c, n, end, endIndex, recursion - 1);
  }
 }
 
 void sanitiseSegment(int c, int startIndex, int endIndex)
 {
  Maxima start = maxima(c, startIndex);
  Maxima end = maxima(c, endIndex);
  sanitiseSegmentRecursive(c, start, end, endIndex);
 }
 
 void calculateClumpingTimes(double sr, double isr)
 {
  clumpingLength = static_cast<int>(sr/clumpingFrequency);
  clumpingSlope = 0.5*clumpingFrequency*M_PI*isr;
 }
 
 int findNextRegionBoundary(int c, int t)
 {
  int zx = findZeroCrossingAfter(c, t);
  int ml = t - maximumRegionSize;
  return std::max(zx, ml);
 }
 
 void findNewRegions(int c)
 {
  int t = region(c, 0).endOfRegion;
  if (t == -1) t = maxima(c, 0).time + 1;
  int nt = findNextRegionBoundary(c, t);
  
  while (nt > 0)
  {
   // There is a zero crossing, let's create a new region for it then look for another
   auto [max, time] = findMaximumAmplitudeInBuffer(c, nt, t);
   regions[c].tapIn(MaximaRegion(max, time, t, nt));
   ++lastRegionProcessed[c];
   t = nt;
   nt = findNextRegionBoundary(c, t);
  }
 }
 
 SampleType slopeFromArbitraryMaximaToRegion(int c, const Maxima &m, int index)
 {
  MaximaRegion &mr = region(c, index);
  dsp_assert(mr);
  dsp_assert(mr.endOfRegion < m.time);
  SampleType slopeToPrev = slopeBetweenArbitraryMaxima(m, mr.maximaAtZeroCrossing());
  SampleType slopeToNext = slopeBetweenArbitraryMaxima(m, mr.maximaAtEndOfRegion());
  if (mr.prevZeroCrossing > m.time) return slopeToNext;
  return (region(c, index).amp > m.amp ? slopeToPrev : slopeToNext);
 }
 
 std::tuple<int, SampleType, SampleType> findNextMaximaCandidates(int c)
 {
  Maxima &lastMaxima = maxima(c, 0);
  int index = lastRegionProcessed[c] - 1;
  dsp_assert(region(c, index));
  int maxSlopeIndex = index;
  int maxIndex = index;
  SampleType maxAmp = region(c, index).amp;
  SampleType maxSlope = slopeFromArbitraryMaximaToRegion(c, lastMaxima, index);
  if (region(c, index).amp > lastMaxima.amp &&
      region(c, index).prevZeroCrossing == lastMaxima.time)
  {
   // We have found ourselves butted up against a higher region. This is a special case where the last
   // maxima gets moved up instead of adding another maxima
   // So we should straight away return the first region we find and detect this case in the main loop
   return {maxSlopeIndex, maxSlope, maxAmp};
  }
  if (lastRegionProcessed[c] == 2 && region(c, 1).length() >= clumpingLength)
  {
   // There are 2 regions and the first one is longer than the clumping length
   // This is a special case
   index = 0;
   SampleType slope = slopeFromArbitraryMaximaToRegion(c, lastMaxima, index);
   if (slope > maxSlope)
   {
    maxSlope = slope;
    maxSlopeIndex = index;
   }
   if (region(c, index).amp > maxAmp)
   {
    maxIndex = index;
    maxAmp = region(c, index).amp;
   }
  }
  else
  {
   --index;
   while (index >= 0 && lastMaxima.time - region(c, index).prevZeroCrossing < clumpingLength)
   {
    SampleType slope = slopeFromArbitraryMaximaToRegion(c, lastMaxima, index);
    if (slope > maxSlope)
    {
     maxSlope = slope;
     maxSlopeIndex = index;
    }
    if (region(c, index).amp > maxAmp)
    {
     maxIndex = index;
     maxAmp = region(c, index).amp;
    }
    --index;
   }
  }

  return {maxSlopeIndex, maxSlope, maxAmp};
 }
 
 
 
 
 
public:
 // Const accessors to allow for visualisation and read-only access to the maxima points
 const std::array<DynamicCircularBuffer<>, Count> &audioBuffer {buffer};
 const std::array<DynamicCircularBuffer<Maxima>, Count> &maximaBuffer {envMaxima};
 const std::array<DynamicCircularBuffer<MaximaRegion>, Count> &regionBuffer {regions};
 const SampleType &currentClumpingFrequency {clumpingFrequency};
 const SampleType &ZeroThreshold {zeroThreshold};
 const int &envelopePropagationDelay {envSamplePoint};
  
 // Specify your inputs as public members here
 SignalIn signalIn;
 
 // Outputs
 Output<Count> envOut;
 
 // Include a definition for each input in the constructor
 PiecewiseEnvelopeFinder(Parameters &p, SignalIn _signalIn) :
 Parameters::ParameterListener(p),
 dspParam(p),
 zeroThreshold(dB2Linear(-80.)),
 signalIn(_signalIn),
 envOut(p)
 {
  updateSampleRate(dspParam.sampleRate(), dspParam.sampleInterval());
  initEnvelopeMaximaBuffer();
  lastRegionProcessed.fill(0);
 }
 
 SampleType getEnvelopeAtTime(int c, int t)
 {
  int i = 0;
  while (i < currentEnvMaximaBufferSize &&
         maxima(c, i) &&
         maxima(c, i).time < t) ++i;
  if (i == currentEnvMaximaBufferSize || !maxima(c, i)) return maxima(c, i - 1).amp;
  if (i == 0 && maxima(c, 0).time >= t) return maxima(c, 0).amp;
  return interpolatePointFromMaxima(maxima(c, i), c, i - 1, t);
 }
 
 void updateSampleRate(double sr, double isr) override
 {
  maximumRegionSize = static_cast<int>(ceil(maximumRegionLengthSeconds*sr));
  envSamplePoint = 2.*maximumRegionSize;
  int calculatedBufferSize = 3*maximumRegionSize;
  for (auto &b: buffer)
  {
   b.setMaximumLength(calculatedBufferSize);
   b.reset(0.);
  }
  lengthBufferSamples = buffer[0].getSize();
  calculateClumpingTimes(sr, isr);
  resetEnvelopeMaximaBuffer();
  initAndResetRegions();
 }
 
 // Set the maximum region length to contain at least a full cycle of the selected
 // clumping frequency, or longer if you don't mind a bit of latency or you
 // plan on changing the clumping frequency to a lower value without reset.
 void setMaximumLengthOfRegion(SampleType seconds)
 {
  maximumRegionLengthSeconds = seconds;
  updateSampleRate(dspParam.sampleRate(), dspParam.sampleInterval());
 }
 
 void setClumpingFrequency(SampleType hz)
 {
  hz = std::max(10., std::min(1000., hz));
  clumpingFrequency = hz;
  calculateClumpingTimes(dspParam.sampleRate(), dspParam.sampleInterval());
 }
 
 void setRisingSlopeMultiplier(SampleType m)
 { risingSlopeMultiplier = boundary(m, SampleType(0.001), SampleType(1.)); }
 
 void setFallingSlopeMultiplier(SampleType m)
 { fallingSlopeMultiplier = boundary(m, SampleType(0.001), SampleType(1.)); }
 
 void setBigJumpDetectionThreshold(SampleType frac)
 { bigJumpFraction = boundary(frac, SampleType(0.), SampleType(1.)); }
 
 void setEnvelopeMaximaBufferSize(int size)
 {
  currentEnvMaximaBufferSize = size;
  initEnvelopeMaximaBuffer();
 }
 
 void setZeroThreshold(SampleType zero)
 { zeroThreshold = zero; }
 
 void setZeroThresholdDb(SampleType db)
 { setZeroThreshold(dB2Linear(db)); }
 
 // This function is responsible for clearing the output buffers to a default state when
 // the component is disabled.
 void reset() override
 {
  setMaximumLengthOfRegion(maximumRegionLengthSeconds);
  resetEnvelopeMaximaBuffer();
  for (auto &b : buffer)
  {
   b.reset(0.);
  }
  lastRegionProcessed.fill(0);
  envOut.reset();
 }
 
 int startProcess(int startPoint, int sampleCount) override
 { return std::min(sampleCount, clumpingLength); }

 // stepProcess is called repeatedly with the start point incremented by step size
 void stepProcess(int startPoint, int sampleCount) override
 {
  fallingSlope = clumpingSlope*fallingSlopeMultiplier;
  risingSlope = clumpingSlope*risingSlopeMultiplier;
  advanceMaximaBuffers(sampleCount);
  for (int c = 0; c < Count; ++c)
  {
   // Fill the audio buffer up to the current sample
   for (int i = startPoint, s = sampleCount; s--; ++i) buffer[c].tapIn(signalIn(c, i));

   findNewRegions(c);
   
   while (lastRegionProcessed[c] > 0 &&
          region(c, lastRegionProcessed[c]).length() == maximumRegionSize)
   {
    Maxima &m = maxima(c, 0);
    MaximaRegion &mr = region(c, lastRegionProcessed[c]);
    dsp_assert(m.time >= mr.prevZeroCrossing);
    if (m.time == mr.prevZeroCrossing) m.amp = mr.amp;
    else if (m.time > mr.prevZeroCrossing) insertMaximaAtEnd(c, mr.maximaAtZeroCrossing());
    --lastRegionProcessed[c];
   }

   while (maxima(c, 0).time > clumpingLength && lastRegionProcessed[c] > 1)
   {
    Maxima *lastMaxima = &maxima(c, 0);
    auto [__msi, __ms, __ma] = findNextMaximaCandidates(c);
    int maxSlopeIndex = __msi;
    SampleType maxSlope = __ms;
    SampleType maxAmp = __ma;
    
    MaximaRegion &maxSlopeRegion = region(c, maxSlopeIndex);
    
    if (maxSlopeRegion.amp > lastMaxima->amp &&
        maxSlopeRegion.prevZeroCrossing == lastMaxima->time)
    {
     // We have found ourselves butted up against a higher region. This is a special case where the last
     // maxima gets moved up instead of adding another maxima
     // Here is where we detect this case in the main loop
     *lastMaxima = maxSlopeRegion.maximaAtZeroCrossing();
     lastMaxima = &maxima(c, 1);
    }
    else if (maxSlope > 0.)
    {
     insertMaximaAtEnd(c, maxSlopeRegion.maximaAtZeroCrossing());
     lastRegionProcessed[c] = maxSlopeIndex + 1;
    }
    else
    {
     insertMaximaAtEnd(c, maxSlopeRegion.maximaAtEndOfRegion());
     lastRegionProcessed[c] = maxSlopeIndex;
    }
    
    Maxima &newMaxima = maxima(c, 0);
    SampleType slope = slopeBetweenArbitraryMaxima(*lastMaxima, newMaxima);
    if (lastMaxima->amp < maxAmp*bigJumpFraction)
    {
     int dt = static_cast<int>(floor((maxAmp - maxSlopeRegion.amp)/clumpingSlope));
     // We have moved the maxima to a new location and we don't know what region
     // it's landed in. So we need to figure that out
     newMaxima.amp = maxAmp;
     newMaxima.time -= dt;
     int t = 0;
     while (region(c, t) &&
            region(c, t).endOfRegion < newMaxima.time) ++t;
     if (t > 0 && region(c, t).endOfRegion > newMaxima.time) --t;
     dsp_assert(t >= 0);
     dsp_assert((t > 0) ? region(c, t - 1).endOfRegion < newMaxima.time : true);
     lastRegionProcessed[c] = t;
     slope = slopeBetweenArbitraryMaxima(*lastMaxima, newMaxima);
     if (slope < clumpingSlope)
     {
      // Not steep enough, carefully move the last maxima so that no excursions occur
      SampleType lowestDesiredLevel = ((lastMaxima->time - newMaxima.time < clumpingLength) ?
                                       lastMaxima->amp :
                                       zeroThreshold);
      t = newMaxima.time;
      Maxima m;
      bool valid;
      do
      {
       ++t;
       m = {interpolatePointAlongSlopeFromArbitraryMaxima(newMaxima, clumpingSlope, t), t};
       valid = quickValidateArbitrarySegment(*lastMaxima, m, c);
      } while (m.amp > lowestDesiredLevel &&
               valid);
      --t;
      m = {interpolatePointAlongSlopeFromArbitraryMaxima(newMaxima, clumpingSlope, t), t};
      insertEnvelopeMaximaInSlotBefore(c, m);
     }
     else
     {
      // Too steep, move the last maxima up to soften the slope, no need to worry
      // about excursions
      lastMaxima->amp = interpolatePointAlongSlopeFromArbitraryMaxima(newMaxima, clumpingSlope, lastMaxima->time);
     }
    }
    else if (slope > risingSlope)
    {
     int i = 1;
     do
     {
      lastMaxima = &maxima(c, i);
      SampleType q = interpolatePointAlongSlopeFromArbitraryMaxima(newMaxima, risingSlope, lastMaxima->time);
      if (lastMaxima->amp < q) lastMaxima->amp = q;
      ++i;
     } while (lastMaxima->time - newMaxima.time < clumpingLength);
    }
    else if (slope < -fallingSlope)
    {
     newMaxima.amp = interpolatePointAlongSlopeFromArbitraryMaxima(*lastMaxima, -fallingSlope, newMaxima.time);
    }
   }
   int i = startPoint;
   int s = sampleCount;
   int t = envSamplePoint + sampleCount;
   while (s--)
   {
    SampleType env = getEnvelopeAtTime(c, t);
    envOut.buffer(c, i) = env;
    ++i;
    --t;
   }
  }
 }
};

 
 
 
 
 
 
 
 
 
}

#endif /* XDDSP_Envelopes_h */
