//
//  XDDSP_Noise.h
//  XDDSPTestingHarness
//
//  Created by Adam Jackson on 31/5/2022.
//

#ifndef XDDSP_Noise_h
#define XDDSP_Noise_h

#include "XDDSP_Types.h"
#include "XDDSP_Classes.h"
#include "XDDSP_Inputs.h"
#include "XDDSP_Utilities.h"
#include <random>
#include <chrono>










namespace XDDSP
{










class RandomNumberBuffer
{
public:
 static constexpr PowerSize NoiseBufferSize {19};
 static int r;

private:
 static bool noiseBufferValid;
 static std::array<SampleType, NoiseBufferSize.size()> noiseBuffer;

public:
 RandomNumberBuffer()
 {
  if (!noiseBufferValid)
  {
   noiseBufferValid = true;
   std::mt19937 gnr; // Construct random number generator with default seed
   std::uniform_real_distribution<SampleType> rnd(-1., 1.);
   
   for (SampleType &s: noiseBuffer) s = rnd(gnr);
   
   auto timeValue = std::chrono::system_clock::now();
   std::chrono::duration<double> timeDuration = timeValue.time_since_epoch();
   std::mt19937::result_type seedValue =  static_cast<std::mt19937::result_type>(std::chrono::duration_cast<std::chrono::nanoseconds>(timeDuration).count());

   gnr.seed(seedValue);
   std::uniform_int_distribution<int> iRnd(0, NoiseBufferSize.size());
   r = iRnd(gnr);
  }
 }
 
 SampleType lookupRandomNumber(int index)
 {
  return noiseBuffer[index & NoiseBufferSize.mask()];
 }
 
 SampleType lookupNextRandomNumber()
 {
  SampleType result = lookupRandomNumber(r);
  r = (r + 1) & NoiseBufferSize.mask();
  return result;
 }
};









template <int ChannelCount = 1>
class NoiseGenerator : public Component<NoiseGenerator<ChannelCount>>
{
 // Private data members here
 RandomNumberBuffer noise;

public:
 // Specify your inputs as public members here
 // No Inputs
 
 // Specify your outputs like this
 Output<ChannelCount> noiseOut;
 
 // Include a definition for each input in the constructor
 NoiseGenerator(Parameters &p) :
 noiseOut(p)
 {
 }
 
 // This function is responsible for clearing the output buffers to a default state when
 // the component is disabled.
 void reset()
 {
  noiseOut.reset();
 }
 
 void stepProcess(int startPoint, int sampleCount)
 {
  for (int c = 0; c < ChannelCount; ++c)
  {
   for (int i = startPoint, s = sampleCount; s--; ++i)
   {
    noiseOut.buffer(c, i) = noise.lookupNextRandomNumber();
   }
  }
 }
};










// Increasing the SpectrumSize argument adds lower frequency components to the noise
template <int ChannelCount = 1, int SpectrumSize = 5>
class PinkNoiseGenerator : public Component<PinkNoiseGenerator<ChannelCount, SpectrumSize>>
{
 // Private data members here
 const PowerSize Size {SpectrumSize};
 RandomNumberBuffer noise;
 std::array<SampleType, ChannelCount> accum {0.};
 std::array<std::array<SampleType, SpectrumSize>, ChannelCount> gnr;
 int counter {0};
 int gIndex {0};
// const SampleType Atten {1./SpectrumSize};
 const SampleType Atten {static_cast<SampleType>(1./sqrt(SpectrumSize))};

public:
 // Specify your inputs as public members here
 // No inputs
 
 // Specify your outputs like this
 Output<ChannelCount> noiseOut;
 
 // Include a definition for each input in the constructor
 PinkNoiseGenerator(Parameters &p) :
 noiseOut(p)
 {
  accum.fill(0.);
  for (auto &g : gnr) g.fill(0.);
 }
 
 // This function is responsible for clearing the output buffers to a default state when
 // the component is disabled.
 void reset()
 {
  accum.fill(0.);
  for (auto &g : gnr) g.fill(0.);
  counter = 0;
  gIndex = 0;
  noiseOut.reset();
 }
 
 void stepProcess(int startPoint, int sampleCount)
 {
  for (int i = startPoint, s = sampleCount; s--; ++i)
  {
   counter = (counter + 1) & Size.mask();
   gIndex = lowestBitSet(counter);
   for (int c = 0; c < ChannelCount; ++c)
   {
    accum[c] -= gnr[c][gIndex];
    gnr[c][gIndex] = noise.lookupNextRandomNumber();
    accum[c] += gnr[c][gIndex];
    noiseOut.buffer(c, i) = accum[c]*Atten;
   }
  }
 }
};










template <typename SignalIn, int NoiseSpectrumSize = 5>
class AnalogNoiseSimulator : public Component<AnalogNoiseSimulator<SignalIn, NoiseSpectrumSize>>
{
 // Private data members here
public:
 static constexpr int Count = SignalIn::Count;
 
 // Specify your inputs as public members here
 SignalIn signalIn;
 
 PinkNoiseGenerator<Count, NoiseSpectrumSize> flickerNoise;
 NoiseGenerator<Count> shotNoise;
 NoiseGenerator<Count> jnNoise;
 
 SignalDelta<Connector<SignalIn, Count>> snAmp;
 
 SimpleGain<
 Connector<decltype(shotNoise), Count>,
 Connector<decltype(snAmp),Count>
 > shotNoiseModulator;
 
 SimpleGain<
 Connector<decltype(shotNoiseModulator.signalOut), Count>,
 ControlConstant<>
 > shotNoiseAtten;
 
 SimpleGain<
 Connector<decltype(jnNoise.noiseOut), Count>,
 Connector<SignalIn, Count>
 > jnNoiseModulator;
 
 SimpleGain<
 Sum<2, Count>,
 ControlConstant<>
 > whiteNoiseAtten;
 
 Sum<2, Count> noiseMix;
 
 SimpleGain<
 Connector<decltype(noiseMix), Count>,
 ControlConstant<>
 > noiseLevel;
 
 // Specify your outputs like this
 Connector<decltype(noiseLevel.signalOut), Count> signalOut;
 // Output<Count> signalOut;
 
 AnalogNoiseSimulator(Parameters &p, SignalIn _signalIn) :
 signalIn(_signalIn),
 flickerNoise(p),
 shotNoise(p),
 jnNoise(p),
 snAmp(p, {signalIn}),
 shotNoiseModulator(p, {shotNoise.noiseOut}, {snAmp.signalOut}),
 shotNoiseAtten(p, {shotNoiseModulator.signalOut}, {0.001}),
 jnNoiseModulator(p, {jnNoise.noiseOut}, {signalIn}),
 whiteNoiseAtten(p, {{shotNoiseAtten.signalOut}, {jnNoiseModulator.signalOut}}, {dB2Linear(-5.)}),
 noiseMix({flickerNoise.noiseOut, whiteNoiseAtten.signalOut}),
 noiseLevel(p, {noiseMix}, {dB2Linear(-80.)}),
 signalOut(noiseLevel.signalOut)
 {}
 
 void reset()
 {
  flickerNoise.reset();
  shotNoise.reset();
  jnNoise.reset();
  snAmp.reset();
  shotNoiseModulator.reset();
  shotNoiseAtten.reset();
  jnNoiseModulator.reset();
  whiteNoiseAtten.reset();
  noiseLevel.reset();
 }
 
 void stepProcess(int startPoint, int sampleCount)
 {
  flickerNoise.process(startPoint, sampleCount);
  shotNoise.process(startPoint, sampleCount);
  jnNoise.process(startPoint, sampleCount);
  snAmp.process(startPoint, sampleCount);
  shotNoiseModulator.process(startPoint, sampleCount);
  shotNoiseAtten.process(startPoint, sampleCount);
  jnNoiseModulator.process(startPoint, sampleCount);
  whiteNoiseAtten.process(startPoint, sampleCount);
  noiseLevel.process(startPoint, sampleCount);
 }
};










}



#endif /* XDDSP_Noise_h */
