//
//  XDDSP_Filters.h
//  XDDSP
//
//  Created by Adam Jackson on 26/5/2022.
//

#ifndef XDDSP_Filters_h
#define XDDSP_Filters_h

#include "XDDSP_BiquadKernel.h"
#include "XDDSP_LinkwitzRileyKernel.h"
#include "XDDSP_WindowFunctions.h"










namespace XDDSP
{










template <typename SignalIn>
class OnePoleAveragingFilter : public Component<OnePoleAveragingFilter<SignalIn>>, public Parameters::ParameterListener
{
 // Private data members here
 Parameters &dspParam;
 std::array<SampleType, SignalIn::Count> value;
 SampleType factor {0.}; // No averaging by default
 SampleType parameter {5.};
 
 void updateFactor()
 {
  factor = expCoef(parameter*dspParam.sampleRate());
 }
 
public:
 static constexpr int Count = SignalIn::Count;
 
 // Specify your inputs as public members here
 SignalIn signalIn;
 
 // Specify your outputs like this
 Output<Count> signalOut;
 
 // Include a definition for each input in the constructor
 OnePoleAveragingFilter(Parameters &p, SignalIn signalIn) :
 Parameters::ParameterListener(p),
 dspParam(p),
 signalIn(signalIn),
 signalOut(p)
 {
  reset();
  updateFactor();
 }
 
 virtual void updateSampleRate(double sr, double isr) override
 {
  updateFactor();
 }

 void reset()
 {
  value.fill(0.);
  signalOut.reset();
 }
 
 void stepProcess(int startPoint, int sampleCount)
 {
  for (int c = 0; c < Count; ++c)
  {
   for (int i = startPoint, s = sampleCount; s--; ++i)
   {
    expTrack(value[c], signalIn(c, i), factor);
    signalOut.buffer(c, i) = value[c];
   }
  }
 }
 
 void setAveragingWindow(SampleType seconds)
 {
  parameter = seconds;
  updateFactor();
 }
};










template <typename SignalIn>
class StaticBiquad : public Component<StaticBiquad<SignalIn>>
{
 // Private data members here
 std::array<BiquadFilterKernel, SignalIn::Count> flt;
 
public:
 static constexpr int Count = SignalIn::Count;
 
 BiquadFilterCoefficients coeff;
 BiquadFilterPublicInterface interface;

 // Specify your inputs as public members here
 SignalIn signalIn;
 
 // Specify your outputs like this
 Output<Count> signalOut;
 
 // Include a definition for each input in the constructor
 StaticBiquad(Parameters &p, SignalIn signalIn) :
 coeff(p),
 interface(coeff),
 signalIn(signalIn),
 signalOut(p)
 {
  reset();
 }
 
 void reset()
 {
  for (auto &f : flt) f.reset();
  signalOut.reset();
 }
 
 void stepProcess(int startPoint, int sampleCount)
 {
  for (int c = 0; c < Count; ++c)
  {
   for (int i = startPoint, s = sampleCount; s--; ++i)
   {
    signalOut.buffer(c, i) = flt[c].process(coeff, signalIn(c, i));
   }
  }
 }
};










template <
typename SignalIn,
typename FreqIn,
typename QIn,
typename GainIn,
int StepSize = 16
>
class DynamicBiquad : public Component<DynamicBiquad<SignalIn, FreqIn, QIn, GainIn, StepSize>, StepSize>
{
 static_assert(FreqIn::Count == 1, "DynamicBiquad expects a single channel for frequency");
 static_assert(QIn::Count == 1, "DynamicBiquad expects a single channel for Q factor");
 static_assert(GainIn::Count == 1, "DynamicBiquad expects a single channel for gain");
 // Private data members here
 Parameters &dspParam;

 std::array<BiquadFilterKernel, SignalIn::Count> flt;
 BiquadFilterCoefficients coeff;
public:
 BiquadFilterPublicInterface interface;
 
 // Specify your inputs as public members here
 SignalIn signalIn;
 FreqIn frequency;
 QIn qFactor;
 GainIn gain;

 static constexpr int Count = SignalIn::Count;
 
 // Specify your outputs like this
 Output<SignalIn::Count> signalOut;
 
 // Include a definition for each input in the constructor
 DynamicBiquad(Parameters &p,
               SignalIn signalIn,
               FreqIn frequency,
               QIn qFactor,
               GainIn gain) :
 dspParam(p),
 coeff(p),
 interface(coeff),
 signalIn(signalIn),
 frequency(frequency),
 qFactor(qFactor),
 gain(gain),
 signalOut(p)
 {}
 
 void reset()
 {
  signalOut.reset();
  for (auto &f : flt) f.reset();
 }
 
 void setFilterMode(int mode)
 { coeff.setFilterMode(mode); }
 
 void setFilterCascade(bool cascade)
 { coeff.setCascade(cascade); }
 
 void stepProcess(int startPoint, int sampleCount)
 {
  coeff.setAllFilterParams(frequency(0, startPoint),
                           qFactor(0, startPoint),
                           gain(0, startPoint));
  
  for (int c = 0; c < Count; ++c)
  {
   for (int i = startPoint, s = sampleCount; s--; ++i)
   {
    signalOut.buffer(c, i) = flt[c].process(coeff, signalIn(c, i));
   }
  }
 }
};










template <typename SignalIn>
class CrossoverFilter : public Component<CrossoverFilter<SignalIn>>
{
 // Private data members here
 std::array<LinkwitzRileyFilterKernel, SignalIn::Count> flt;
 
public:
 static constexpr int Count = SignalIn::Count;
 LinkwitzRileyFilterCoefficients coeff;
 
 // Specify your inputs as public members here
 SignalIn signalIn;
 
 // Specify your outputs like this
 Output<Count> lowPassOut;
 Output<Count> highPassOut;
 
 // Include a definition for each input in the constructor
 CrossoverFilter(Parameters &p, SignalIn signalIn) :
 coeff(p),
 signalIn(signalIn),
 lowPassOut(p),
 highPassOut(p)
 {}
 
 // This function is responsible for clearing the output buffers to a default state when
 // the component is disabled.
 void reset()
 {
  lowPassOut.reset();
  highPassOut.reset();
  for (auto &f : flt) f.reset();
 }
 
 void stepProcess(int startPoint, int sampleCount)
 {
  for (int c = 0; c < Count; ++c)
  {
   for (int i = startPoint, s = sampleCount; s--; ++i)
   {
    flt[c].process(coeff, lowPassOut.buffer(c, i), highPassOut.buffer(c, i), signalIn(c, i));
   }
  }
 }
};










template <typename SignalIn, int FIRTapCount = 31>
class FIRHilbertTransform : public Component<FIRHilbertTransform<SignalIn>>
{
 static_assert(FIRTapCount % 2 == 1, "FIRHilbertTransform: Tap Count must be odd");
 
 // Private data members here
 std::array<SampleType, FIRTapCount> taps;
 std::array<DynamicCircularBuffer<>, SignalIn::Count> inPhaseDelay;
 std::array<DynamicCircularBuffer<>, SignalIn::Count> buffer;
 
 static constexpr SampleType alpha = 25.0/46.0;
public:
 static constexpr int DelayLength = FIRTapCount/2;
 static constexpr int Count = SignalIn::Count;
 
 // Specify your inputs as public members here
 SignalIn signalIn;
 
 // Specify your outputs like this
 Output<Count> inPhaseOut;
 Output<Count> quadratureOut;
 
 // Include a definition for each input in the constructor
 FIRHilbertTransform(Parameters &p, SignalIn _signalIn) :
 signalIn(_signalIn),
 inPhaseOut(p),
 quadratureOut(p)
 {
  for (auto &d: inPhaseDelay)
  {
   d.setMaximumLength(DelayLength);
   d.reset();
  }
  
  for (auto &b: buffer)
  {
   b.setMaximumLength(FIRTapCount);
   b.reset();
  }
  
  taps.fill(0.0);
  for (int i = 0, x = -DelayLength; i < FIRTapCount; i += 2, x += 2)
  {
   if (x != 0) taps[i] = 1./static_cast<SampleType>(x);
  }
  
  applyWindowFunction(WindowFunction::CosineWindow(alpha), taps);
 }
 
 // This function is responsible for clearing the output buffers to a default state when
 // the component is disabled.
 void reset()
 {
  for (auto &d: inPhaseDelay) d.reset();
  for (auto &b: buffer) b.reset();
  inPhaseOut.reset();
  quadratureOut.reset();
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
    SampleType x = 0.;
    buffer[c].tapIn(signalIn(c, i));
    inPhaseDelay[c].tapIn(signalIn(c, i));
    
    for (int t = 0; t < FIRTapCount; ++t)
    {
     x += buffer[c].tapOut(t)*taps[t];
    }
    
    inPhaseOut.buffer(c, i) = inPhaseDelay[c].tapOut(DelayLength);
    quadratureOut.buffer(c, i) = x;
   }
  }
 }
 
 // finishProcess is called after the block has been processed
// void finishProcess()
// {}
};

 
 
 
 
 
 
 
 
 
template <typename SignalIn>
class IIRHilbertApproximator : public Component<IIRHilbertApproximator<SignalIn>>
{
 // Private data members here
 std::array<std::array<SampleType, 33>, SignalIn::Count> h;
 
public:
 static constexpr int Count = SignalIn::Count;
 
 // Specify your inputs as public members here
 SignalIn signalIn;
 
 // Specify your outputs like this
 Output<Count> quadratureOut;
 Output<Count> inPhaseOut;
 
 // Include a definition for each input in the constructor
 IIRHilbertApproximator(Parameters &p, SignalIn _signalIn) :
 signalIn(_signalIn),
 quadratureOut(p),
 inPhaseOut(p)
 {
  for (auto &hx : h) hx.fill(0.);
 }
 
 // This function is responsible for clearing the output buffers to a default state when
 // the component is disabled.
 void reset()
 {
  quadratureOut.reset();
  inPhaseOut.reset();
  for (auto &hx : h) hx.fill(0.);
 }
 
 // startProcess prepares the component for processing one block and returns the step
 // size. By default, it returns the entire sampleCount as one big step.
// int startProcess(int startPoint, int sampleCount)
// { return std::min(sampleCount, StepSize); }

 // stepProcess is called repeatedly with the start point incremented by step size
 void stepProcess(int startPoint, int sampleCount)
 {
  SampleType xa, xb;
  for (int c = 0; c < Count; ++c)
  {
   for (int i = startPoint, s = sampleCount; s--; ++i)
   {
    xa = h[c][1] - 0.999533593f*signalIn(c, i);  h[c][1] = h[c][0];
    h[c][0] = signalIn(c, i) + 0.999533593f*xa;
    xb = h[c][3] - 0.997023120f*xa;  h[c][3] = h[c][2];
    h[c][2] = xa + 0.997023120f*xb;
    xa = h[c][5] - 0.991184054f*xb;  h[c][5] = h[c][4];
    h[c][4] = xb + 0.991184054f*xa;
    xb = h[c][7] - 0.975597057f*xa;  h[c][7] = h[c][6];
    h[c][6] = xa + 0.975597057f*xb;
    xa = h[c][9] - 0.933889435f*xb;  h[c][9] = h[c][8];
    h[c][8] = xb + 0.933889435f*xa;
    xb = h[c][11] - 0.827559364f*xa;  h[c][11] = h[c][10];
    h[c][10] = xa + 0.827559364f*xb;
    xa = h[c][13] - 0.590957946f*xb;  h[c][13] = h[c][12];
    h[c][12] = xb + 0.590957946f*xa;
    xb = h[c][15] - 0.219852059f*xa;  h[c][15] = h[c][14];
    h[c][14] = xa + 0.219852059f*xb;
    quadratureOut.buffer(c, i) = h[c][32]; h[c][32] = xb;

    // out2 filter chain: 8 allpasses
    xa = h[c][17] - 0.998478404f*signalIn(c, i);  h[c][17] = h[c][16];
    h[c][16] = signalIn(c, i) + 0.998478404f*xa;
    xb = h[c][19] - 0.994786059f*xa;  h[c][19] = h[c][18];
    h[c][18] = xa + 0.994786059f*xb;
    xa = h[c][21] - 0.985287169f*xb;  h[c][21] = h[c][20];
    h[c][20] = xb + 0.985287169f*xa;
    xb = h[c][23] - 0.959716311f*xa;  h[c][23] = h[c][22];
    h[c][22] = xa + 0.959716311f*xb;
    xa = h[c][25] - 0.892466594f*xb;  h[c][25] = h[c][24];
    h[c][24] = xb + 0.892466594f*xa;
    xb = h[c][27] - 0.729672406f*xa;  h[c][27] = h[c][26];
    h[c][26] = xa + 0.729672406f*xb;
    xa = h[c][29] - 0.413200818f*xb;  h[c][29] = h[c][28];
    h[c][28] = xb + 0.413200818f*xa;
    inPhaseOut.buffer(c, i) = h[c][31] - 0.061990080f*xa; h[c][31] = h[c][30];
    h[c][30] = xa + 0.061990080f*inPhaseOut.buffer(c, i);
   }
  }
 }
 
 // finishProcess is called after the block has been processed
// void finishProcess()
// {}
};

 
 
 
 
 
 
 
 
 
}


#endif /* XDDSP_Filters_h */
