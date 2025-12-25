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
#include "XDDSP_FFT.h"










namespace XDDSP
{










/**
 * @brief A component encapsulating a simple one-pole averaging filter, suitable for use as a simple lowpass filter, control smoother, RMS filter etc.
 * 
 * @tparam SignalIn Couples to the input signal. Can have as many channels as you like.
 */
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
 
 SignalIn signalIn;
 
 Output<Count> signalOut;
 
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
 
 /**
  * @brief Set the length of the averaging window in seconds.
  * 
  * @param seconds The length of the averaging window in seconds.
  */
 void setAveragingWindow(SampleType seconds)
 {
  parameter = seconds;
  updateFactor();
 }
};










/**
 * @brief A component encapsulating a simple static biquad filter.
 * 
 * This component exposes a coefficients object and a public interface object, enabling easy static configuration.
 * 
 * @tparam SignalIn Couples to the input signal. Can have as many channels as you like.
 */
template <typename SignalIn>
class StaticBiquad : public Component<StaticBiquad<SignalIn>>
{
 // Private data members here
 std::array<BiquadFilterKernel, SignalIn::Count> flt;
 
public:
 static constexpr int Count = SignalIn::Count;
 
 /**
  * @brief The exposed filter configuration object.
  * 
  */
 BiquadFilterCoefficients coeff;
 
 /**
  * @brief The exposed public interface object.
  * 
  */
 BiquadFilterPublicInterface interface;

 SignalIn signalIn;
 
 Output<Count> signalOut;
 
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










/**
 * @brief A component containing a dynamic biquad filter.
 * 
 * This filter hides its configuration object, instead opting to configure the filter based on input signals provided, eg. signals for frequency and quality.
 * 
 * @tparam SignalIn Couples to the input signal. Can have as many channels as you like.
 * @tparam FreqIn Couples to the signal providing the input frequency in Hz. Must have only one channel.
 * @tparam QIn Couples to the signal providing the quality factor. Must have only one channel.
 * @tparam GainIn Couples to the signal specifying the gain in dB. Must have only one channel.
 * @tparam StepSize Controls how often the component reconfigures the filter from the inputs. Default is every 16 samples.
 */
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
 
 SignalIn signalIn;
 FreqIn frequency;
 QIn qFactor;
 GainIn gain;

 static constexpr int Count = SignalIn::Count;
 
 Output<SignalIn::Count> signalOut;
 
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
 
 /**
  * @brief Set the mode of the filter. See BiquadFilterCoefficients for a list of modes.
  * 
  * @param mode The new mode of the filter.
  */
 void setFilterMode(int mode)
 { coeff.setFilterMode(mode); }
 
 /**
  * @brief Enable or disable a second biquad with the same settings in cascade mode.
  * 
  * @param cascade Set true to enable cascade, or false to disable.
  */
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










/**
 * @brief A component encapsulating a Linkwitz-Riley filter suitable for building phase-aligned crossover filters.
 * 
 * This component exposes two outputs, one signal with all the high frequency components and one with all the low frequency components.
 * 
 * @tparam SignalIn Couples to the input signal. Can have as many channels as you like.
 */
template <typename SignalIn>
class CrossoverFilter : public Component<CrossoverFilter<SignalIn>>
{
 std::array<LinkwitzRileyFilterKernel, SignalIn::Count> flt;
 
public:
 static constexpr int Count = SignalIn::Count;
 LinkwitzRileyFilterCoefficients coeff;
 
 SignalIn signalIn;
 
 Output<Count> lowPassOut;
 Output<Count> highPassOut;
 
 CrossoverFilter(Parameters &p, SignalIn signalIn) :
 coeff(p),
 signalIn(signalIn),
 lowPassOut(p),
 highPassOut(p)
 {}
 
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










/**
 * @brief A component encapsulating a Hilbert Transform using an FIR.
 * 
 * This hilbert transform component uses an FIR filter to perform the transform, making it optimal for low tap counts only. For higher tap counts and better quality output, see ConvolutionHilbertFilter
 * 
 * @tparam SignalIn Couples to the input signal. Can have as many channels as you like.
 * @tparam FIRTapCount Controls the size of the fir kernel used. **The tap count must be an odd number**. The default is 31.
 */
template <typename SignalIn, int FIRTapCount = 31>
class FIRHilbertTransform : public Component<FIRHilbertTransform<SignalIn>>
{
 static_assert(FIRTapCount % 2 == 1, "FIRHilbertTransform: Tap Count must be odd");
 
 // Private data members here
 std::array<SampleType, FIRTapCount> taps;
 std::array<DynamicCircularBuffer<>, SignalIn::Count> buffer;
 
 static constexpr SampleType alpha = 25.0/46.0;
public:
 static constexpr int DelayLength = FIRTapCount/2;
 static constexpr int Count = SignalIn::Count;
 
 SignalIn signalIn;
 
 Output<Count> inPhaseOut;
 Output<Count> quadratureOut;
 
 FIRHilbertTransform(Parameters &p, SignalIn _signalIn) :
 signalIn(_signalIn),
 inPhaseOut(p),
 quadratureOut(p)
 {
  for (auto &b: buffer)
  {
   b.setMaximumLength(FIRTapCount);
   b.reset();
  }
  
  taps.fill(0.0);
  for (int i = 0, x = -DelayLength; i < FIRTapCount; i += 2, x += 2)
  {
   if (x != 0) taps[i] = 2./(M_PI*static_cast<SampleType>(x));
  }
  
  applyWindowFunction(WindowFunction::CosineWindow(alpha), taps);
 }
 
 void reset()
 {
  for (auto &b: buffer) b.reset(0.);
  inPhaseOut.reset();
  quadratureOut.reset();
 }
 
 void stepProcess(int startPoint, int sampleCount)
 {
  for (int c = 0; c < Count; ++c)
  {
   for (int i = startPoint, s = sampleCount; s--; ++i)
   {
    SampleType x = 0.;
    buffer[c].tapIn(signalIn(c, i));
    
    for (int t = 0; t < FIRTapCount; t += 2)
    {
     x = std::fma(buffer[c].tapOut(t), taps[t], x);
    }
    
    inPhaseOut.buffer(c, i) = buffer[c].tapOut(DelayLength);
    quadratureOut.buffer(c, i) = x;
   }
  }
 }
};










/**
 * @brief A component encapsulating a Hilbert Transform using a convolution kernel.
 * 
 * This hilbert transform component uses a convolution kernel to perform the transform, making it optimal for large tap counts only. For smaller tap counts, see FIRHilbertTransform
 * 
 * @tparam SignalIn Couples to the input signal. Can have as many channels as you like.
 * @tparam FIRTapCount Controls the size of the convolution kernel used. **The tap count must be an odd number**. The default is 255.
 */
template <typename SignalIn, int FIRTapCount = 255>
class ConvolutionHilbertFilter : public Component<ConvolutionHilbertFilter<SignalIn>>
{
 static_assert(FIRTapCount % 2 == 1, "FIRHilbertTransform: Tap Count must be odd");
 
 ConvolutionFilter<SignalIn> filter;
 std::array<DynamicCircularBuffer<>, SignalIn::Count> buffer;


 // Private data members here
 std::array<SampleType, FIRTapCount> taps;
 static constexpr SampleType alpha = 25.0/46.0;
public:
 static constexpr int DelayLength = FIRTapCount/2;
 static constexpr int Count = SignalIn::Count;
 
 SignalIn &signalIn;
 
 Output<Count> inPhaseOut;
 Connector<decltype(filter.signalOut)> quadratureOut;
 
 ConvolutionHilbertFilter(Parameters &p, SignalIn _signalIn) :
 filter(p, _signalIn),
 signalIn(filter.signalIn),
 inPhaseOut(p),
 quadratureOut(filter.signalOut)
 {
  for (auto &b: buffer)
  {
   b.setMaximumLength(FIRTapCount);
   b.reset(0.);
  }

  taps.fill(0.0);
  for (int i = 0, x = -DelayLength; i < FIRTapCount; i += 2, x += 2)
  {
   if (x != 0) taps[i] = 2./(M_PI*static_cast<SampleType>(x));
  }
  
  applyWindowFunction(WindowFunction::CosineWindow(alpha), taps);
  
  filter.setImpulse(0, taps.data(), FIRTapCount);
  filter.setFFTHint(FIRTapCount);
  filter.initialiseConvolution();
 }

 void reset()
 {
  filter.reset();
  inPhaseOut.reset();
 }
 
 void stepProcess(int startPoint, int sampleCount)
 {
  filter.process(startPoint, sampleCount);
  
  for (int c = 0; c < Count; ++c)
  {
   for (int i = startPoint, s = sampleCount; s--; ++i)
   {
    buffer[c].tapIn(signalIn(c, i));
    inPhaseOut.buffer(c, i) = buffer[c].tapOut(DelayLength);
   }
  }
 }
};

 
 
 
 
 
 
 
 
 
/**
 * @brief A component encapsulating a hilbert approximator using an IIR filter.
 * 
 * This code was taken from another source.
 * TODO: Figure out where this code came from!
 * 
 * @tparam SignalIn Couples to the input signal. Can have as many channels as you like.
 */
template <typename SignalIn>
class IIRHilbertApproximator : public Component<IIRHilbertApproximator<SignalIn>>
{
 // Private data members here
 std::array<std::array<SampleType, 33>, SignalIn::Count> h;
 
public:
 static constexpr int Count = SignalIn::Count;
 
 SignalIn signalIn;
 
 Output<Count> quadratureOut;
 Output<Count> inPhaseOut;
 
 IIRHilbertApproximator(Parameters &p, SignalIn _signalIn) :
 signalIn(_signalIn),
 quadratureOut(p),
 inPhaseOut(p)
 {
  for (auto &hx : h) hx.fill(0.);
 }
 
 void reset()
 {
  quadratureOut.reset();
  inPhaseOut.reset();
  for (auto &hx : h) hx.fill(0.);
 }
 
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
};

 
 
 
 
 
 
 
 
 
}


#endif /* XDDSP_Filters_h */
