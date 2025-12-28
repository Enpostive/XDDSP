//
//  XDDSP_Oscillators.h
//  XDDSP
//
//  Created by Adam Jackson on 27/5/2022.
//

#ifndef XDDSP_Oscillators_h
#define XDDSP_Oscillators_h

#include "XDDSP_Types.h"
#include "XDDSP_Blep.h"










namespace XDDSP
{










 /**
  * @brief A basic multi-channel oscillator which uses a callable object to generate a waveform.
  * 
  * This object calls FuncOscillator::func with a value between 0 and 1. Upon construction, the func object is pointed to a basic sine wave function.
  * 
  * This object supports basic phase modulation. There is no anti-aliasing.
  * 
  * @tparam FrequencyIn Couples to a frequency input. The frequency is in Hz. This can have as many channels as you like.
  * @tparam PhaseModIn Couples to a phase modulation input. It must have the same number of channels as FrequencyIn.
  */
template <
typename FrequencyIn,
typename PhaseModIn
>
class FuncOscillator : public Component<FuncOscillator<FrequencyIn, PhaseModIn>>
{
 static_assert(FrequencyIn::Count == PhaseModIn::Count || PhaseModIn::Count == 1, "FrequencyIn and PhaseModIn count mismatch");
public:
 static constexpr int Count = FrequencyIn::Count;
 
private:
 Parameters &dspParam;
 
 std::array<SampleType, Count> phase;
public:
 
 /**
  * @brief This is where you put the callable object which will produce the waveform samples.
  * 
  */
 WaveformFunction func;
 
 FrequencyIn frequencyIn;
 PhaseModIn phaseModIn;
 
 Output<Count> signalOut;
 
 FuncOscillator(Parameters &p, FrequencyIn _frequencyIn, PhaseModIn _phaseModIn) :
 dspParam(p),
 frequencyIn(_frequencyIn),
 phaseModIn(_phaseModIn),
 signalOut(p)
 {
  func = [](SampleType i) { return sin(2.*M_PI*i); };
 }
 
 void reset()
 {
  phase.fill(0.);
  signalOut.reset();
 }
 
 /**
  * @brief Set the phase of the oscillator on one channel.
  * 
  * @param channel The channel index of the oscillator to set.
  * @param _phase The new phase of the oscialltor.
  */
 void setPhase(int channel, SampleType _phase)
 {
  phase[channel] = _phase - floor(_phase);
 }
 
 /**
  * @brief Set the phase of every oscillator.
  * 
  * @param _phase The new phase of every oscillator.
  */
 void setPhase(SampleType _phase)
 {
  for (auto &p : phase) p = _phase - floor(_phase);
 }
 
 void stepProcess(int startPoint, int sampleCount)
 {
  SampleType mod;
  
  for (int i = startPoint, s = sampleCount; s--; ++i)
  {
   if (Count == 1) mod = phaseModIn(i);
   for (int c = 0; c < Count; ++c)
   {
    if (Count > 1) mod = phaseModIn(c, i);
    SampleType p = phase[c] + mod;
    p -= floor(p);
    signalOut.buffer(c, i) = func(p);
    phase[c] += fastBoundary(frequencyIn(c, i)*dspParam.sampleInterval(), 0., 0.5);
    phase[c] -= floor(phase[c]);
   }
  }
 }
};










/**
 * @brief A multi-channel band-limited sawtooth oscillator.
 * 
 * @tparam FrequencyIn Couples to a frequency in Hz. This can have as many channels as you like.
 */
template <typename FrequencyIn>
class BandLimitedSawOscillator : public Component<BandLimitedSawOscillator<FrequencyIn>>
{
public:
 static constexpr int Count = FrequencyIn::Count;
 
private:
 Parameters &dspParam;
 
 std::array<SampleType, Count> phase;
 std::array<BLEPGenerator, Count> blep;
public:

 FrequencyIn frequencyIn;
 
 Output<Count> signalOut;
 
 BandLimitedSawOscillator(Parameters &p, FrequencyIn _frequencyIn) :
 dspParam(p),
 frequencyIn(_frequencyIn),
 signalOut(p)
 {
  phase.fill(0.);
 }
 
 void reset()
 {
  phase.fill(0.);
  for (auto &b : blep) b.reset();
  signalOut.reset();
 }
 
 /**
  * @brief Set the phase of the oscillator on one channel.
  * 
  * @param channel The channel index of the oscillator to set.
  * @param _phase The new phase of the oscialltor.
  */
 void setPhase(int channel, SampleType _phase)
 {
  phase[channel] = _phase - floor(_phase);
 }
 
 /**
  * @brief Set the phase of every oscillator.
  * 
  * @param _phase The new phase of every oscillator.
  */
 void setPhase(SampleType _phase)
 {
  for (auto &p : phase) p = _phase - floor(_phase);
 }
 
 void stepProcess(int startPoint, int sampleCount)
 {
  for (int c = 0; c < Count; ++c)
  {
   for (int i = startPoint, s = sampleCount; s--; ++i)
   {
    SampleType ppStep = fastBoundary(frequencyIn(c, i)*dspParam.sampleInterval(), 0., 0.5);
    if(phase[c] < ppStep) blep[c].applyBLEP(2., phase[c]/ppStep);
    signalOut.buffer(c, i) = 1. - 2*phase[c] + 4*ppStep + blep[c].getNextBLEPSample();
    phase[c] += ppStep;
    phase[c] -= floor(phase[c]);
   }
  }
 }
};










/**
 * @brief A multi-channel band-limited square wave oscillator with PWM input.
 * 
 * @tparam FrequencyIn Couples to a frequency in Hz. This can have as many channels as you like.
 * @tparam PulseWidthIn Couples to a pulse-width input between 0 and 1. Must have the same number of channels as FrequencyIn.
 */
template <typename FrequencyIn, typename PulseWidthIn>
class BandLimitedSquareOscillator : public Component<BandLimitedSquareOscillator<FrequencyIn, PulseWidthIn>>
{
public:
 static constexpr int Count = FrequencyIn::Count;
 
private:
 Parameters &dspParam;
 
 std::array<SampleType, Count> phase;
 std::array<int, Count> prevState;
 std::array<BLEPGenerator, Count> blep;
public:
 
 FrequencyIn frequencyIn;
 PulseWidthIn pulseWidthIn;
 
 Output<Count> signalOut;
 
 BandLimitedSquareOscillator(Parameters &p, FrequencyIn _frequencyIn, PulseWidthIn _pulseWidthIn) :
 dspParam(p),
 frequencyIn(_frequencyIn),
 pulseWidthIn(_pulseWidthIn),
 signalOut(p)
 {
  phase.fill(0.);
  prevState.fill(0);
 }
 
 void reset()
 {
  phase.fill(0.);
  prevState.fill(0);
  for (auto &b : blep) b.reset();
  signalOut.reset();
 }
 
 /**
  * @brief Set the phase of the oscillator on one channel.
  * 
  * @param channel The channel index of the oscillator to set.
  * @param _phase The new phase of the oscialltor.
  */
 void setPhase(int channel, SampleType _phase)
 {
  phase[channel] = _phase - floor(_phase);
 }
 
 /**
  * @brief Set the phase of every oscillator.
  * 
  * @param _phase The new phase of every oscillator.
  */
 void setPhase(SampleType _phase)
 {
  for (auto &p : phase) p = _phase - floor(_phase);
 }
 
 void stepProcess(int startPoint, int sampleCount)
 {
  SampleType ppStep = frequencyIn(startPoint)*dspParam.sampleInterval();
  SampleType pwi = pulseWidthIn(startPoint);
  SampleType epwi = boundary(pwi, ppStep, static_cast<SampleType>(1.) - ppStep);
  for (int c = 0; c < Count; ++c)
  {
   if (Count > 1)
   {
    ppStep = fastBoundary(frequencyIn(c, startPoint)*dspParam.sampleInterval(), 0., 0.5);
    pwi = pulseWidthIn(c, startPoint);
    epwi = boundary(pwi, ppStep, static_cast<SampleType>(1.) - ppStep);
   }
   for (int i = startPoint, s = sampleCount; s--; ++i)
   {
    ppStep = fastBoundary(frequencyIn(c, i)*dspParam.sampleInterval(), 0., 0.5);
    
    SampleType epwi = boundary(pwi, ppStep, static_cast<SampleType>(1.) - ppStep);
    SampleType fracState = epwi - phase[c];
    int state = signum(fracState);
    if (state != 0 && prevState[c] != 0 && state != prevState[c])
    {
     SampleType fracPart = (state == 1)*phase[c]/ppStep ;
     fracPart += (state == -1)*-1.*fracState/ppStep;
     blep[c].applyBLEP(state*2., fracPart);
    }
    signalOut.buffer(c, i) = static_cast<SampleType>(state) + blep[c].getNextBLEPSample();
    prevState[c] = state;

    phase[c] += ppStep;
    phase[c] -= floor(phase[c]);
   }
  }
 }
};










/**
 * @brief A multi-channel band-limited triangle wave oscillator.
 * 
 * @tparam FrequencyIn Couples to a frequency in Hz. This can have as many channels as you like.
 */
template <typename FrequencyIn>
class BandLimitedTriangleOscillator : public Component<BandLimitedTriangleOscillator<FrequencyIn>>
{
public:
 static constexpr int Count = FrequencyIn::Count;
 
private:
 Parameters &dspParam;
 
 std::array<SampleType, Count> phase;
 std::array<BLEPGenerator, Count> blep;
 
 SampleType tri(SampleType phase)
 {
  return ((phase > 0.5) ? 3. - 4.*phase : -1. + 4*phase);
 }
 
public:
 
 FrequencyIn frequencyIn;
 
 Output<Count> signalOut;
 
 BandLimitedTriangleOscillator(Parameters &p, FrequencyIn _frequencyIn) :
 dspParam(p),
 frequencyIn(_frequencyIn),
 signalOut(p)
 {
  phase.fill(0.);
 }
 
 void reset()
 {
  phase.fill(0.);
  signalOut.reset();
 }
 
 /**
  * @brief Set the phase of the oscillator on one channel.
  * 
  * @param channel The channel index of the oscillator to set.
  * @param _phase The new phase of the oscialltor.
  */
 void setPhase(int channel, SampleType _phase)
 {
  phase[channel] = _phase - floor(_phase);
 }
 
 /**
  * @brief Set the phase of every oscillator.
  * 
  * @param _phase The new phase of every oscillator.
  */
 void setPhase(SampleType _phase)
 {
  for (auto &p : phase) p = _phase - floor(_phase);
 }
 
 void stepProcess(int startPoint, int sampleCount)
 {
  for (int c = 0; c < Count; ++c)
  {
   for (int i = startPoint, s = sampleCount; s--; ++i)
   {
    SampleType ppStep = fastBoundary(frequencyIn(c, i)*dspParam.sampleInterval(), 0., 0.5);
    SampleType phase2 = phase[c] - 0.5;
    
    if (phase[c] < ppStep) blep[c].applyBLAMP(4.*ppStep, phase[c]/ppStep);
    if (phase2 > 0. && phase2 < ppStep) blep[c].applyBLAMP(-4.*ppStep, phase2/ppStep);
    signalOut.buffer(c, i) = tri(phase[c]) + blep[c].getNextBLEPSample();
    
    phase[c] += ppStep;
    phase[c] -= floor(phase[c]);
   }
  }
 }
};










/*
template <typename FrequencyIn>
class TemplateOscillator : public Component<TemplateOscillator<FrequencyIn>>
{
 // Private data members here
public:
 static constexpr int Count = FrequencyIn::Count;
 
private:
 // Private data members here
 Parameters &dspParam;
 
 std::array<SampleType, Count> phase;
// std::array<BLEPGenerator, Count> blep;
public:
 
 // Specify your inputs as public members here
 FrequencyIn frequencyIn;
 
 // Specify your outputs like this
 Output<Count> signalOut;
 
 // Include a definition for each input in the constructor
 TemplateOscillator(Parameters &p, FrequencyIn _frequencyIn) :
 dspParam(p),
 frequencyIn(_frequencyIn),
 signalOut(p)
 {
  phase.fill(0.);
 }
 
 // This function is responsible for clearing the output buffers to a default state when
 // the component is disabled.
 void reset()
 {
  phase.fill(0.);
//  for (auto &b : blep) b.reset();
  signalOut.reset();
 }
 
 void setPhase(int channel, SampleType _phase)
 {
  phase[channel] = _phase - floor(_phase);
 }
 
 void setPhase(SampleType _phase)
 {
  for (auto &p : phase) p = _phase - floor(_phase);
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
    SampleType ppStep = fastBoundary(frequencyIn(c, i)*dspParam.sampleInterval(), 0., 0.5);
    
//    if(phase[c] < ppStep) blep[c].applyBLEP(2., phase[c]/ppStep);
    signalOut.buffer(c, i) = //TODO: Type DSP here;
    
    phase[c] += ppStep;
    phase[c] -= floor(phase[c]);
   }
  }
 }
};

*/








}

#endif /* XDDSP_Oscillators_h */
