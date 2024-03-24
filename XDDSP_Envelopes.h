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
    envOut(c, i) = state[c];
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
    envOut(c, i) = state[c].env;
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
 }
 
 // This function is responsible for clearing the output buffers to a default state when
 // the component is disabled.
 void reset()
 {
  signalOut.reset();
 }
 
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
 {
  setThresholdAndKnee(db, knee);
 }
 
 void setKnee(SampleType db)
 {
  setThresholdAndKnee(threshold, db);
 }
 
 SampleType getThreshold() const
 { return threshold; }
 
 SampleType getKnee() const
 { return knee; }
 
 void setRatioAbove(SampleType r)
 { ratioAbove = (r == 0.) ? 0. : (1.0 / r); }

 SampleType getRatioAbove() const
 {
  return (ratioAbove == 0.) ? 0. : 1./ratioAbove;
 }
 
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
  
  for (int c = 0; c < Count; ++c)
  {
   for (int i = startPoint, s = sampleCount; s--; ++i)
   {
    signalOut.buffer(c, i) = computeGainCurve(signalIn(c, i));
   }
  }
 }
 
 // finishProcess is called after the block has been processed
// void finishProcess()
// {}
};

 
 
 
 
 
 
 
 
 
}

#endif /* XDDSP_Envelopes_h */
