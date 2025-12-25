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










/**
 * @brief A class for accessing a global lookup table made of random numbers.
 * 
 * This buffer contains 512k of high quality procedurally generated noise, which is good for about 11 seconds at a 44.1kHz sample rate. The noise is generated using the mersenne twister mt19937 with the default random number seed, guaranteeing noise that is deterministic, making it great for predictable and repeatable randomness when required, while being big enough that no discernable pattern will stand out to the average listener.
 */
class RandomNumberBuffer
{
public:
 static constexpr PowerSize NoiseBufferSize {19};
 static int r;

private:
 static bool noiseBufferValid;
 static std::array<SampleType, NoiseBufferSize.size()> noiseBuffer;

public:
 /**
  * @brief Construct a new RandomNumberBuffer object.
  * 
  * This method checks whether the global buffer has been generated and if not this method then proceeds to generate it. Then it chooses a random index into the buffer based on the current system time.
  */
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
 
 /**
  * @brief Look up a random number from the buffer. This method allows for deterministic look up of random numbers for any purpose.
  * 
  * @param index The index into the buffer. The look up is bounded to the size of the buffer by a modulo operation.
  * @return SampleType The random sample at that index.
  */
 SampleType lookupRandomNumber(int index)
 {
  return noiseBuffer[index & NoiseBufferSize.mask()];
 }
 
 /**
  * @brief Get the random number in the buffer at the current random index then increment the index.
  * 
  * @return SampleType The next random number.
  */
 SampleType lookupNextRandomNumber()
 {
  SampleType result = lookupRandomNumber(r);
  r = (r + 1) & NoiseBufferSize.mask();
  return result;
 }
};










/**
 * @brief A component which outputs white noise.
 * 
 * @tparam ChannelCount The number of output channels to make available.
 */
template <int ChannelCount = 1>
class NoiseGenerator : public Component<NoiseGenerator<ChannelCount>>
{
 RandomNumberBuffer noise;

public:
 Output<ChannelCount> noiseOut;
 
 NoiseGenerator(Parameters &p) :
 noiseOut(p)
 {
 }
 
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










/**
 * @brief A component which outputs pink noise.
 * 
 * @tparam ChannelCount The number of channels to make available.
 * @tparam SpectrumSize The depth of the generator algorithm. Increasing the spectrum size adds lower frequency components to the noise and increases computation overhead. Default is 5.
 */
template <int ChannelCount = 1, int SpectrumSize = 5>
class PinkNoiseGenerator : public Component<PinkNoiseGenerator<ChannelCount, SpectrumSize>>
{
 const PowerSize Size {SpectrumSize};
 RandomNumberBuffer noise;
 std::array<SampleType, ChannelCount> accum {0.};
 std::array<std::array<SampleType, SpectrumSize>, ChannelCount> gnr;
 int counter {0};
 int gIndex {0};
 const SampleType Atten {static_cast<SampleType>(1./sqrt(SpectrumSize))};

public:
 Output<ChannelCount> noiseOut;
 
 PinkNoiseGenerator(Parameters &p) :
 noiseOut(p)
 {
  accum.fill(0.);
  for (auto &g : gnr) g.fill(0.);
 }
 
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










/**
 * @brief A component for adding convincing analog noise to a signal.
 * 
 * This component doesn't directly add noise to the input signal. This component generates base line pink noise to simulate flicker noise, then adds shot noise and junction noise modulated by the input signal. The output can be mixed back with the original input to add a convincing analog flavour. Read the documentation on AnalogNoiseSimulator::shotNoiseAtten, AnalogNoiseSimulator::whiteNoiseAtten and AnalogNoiseSimulator::noiseLevel for configuration options.
 * 
 * @tparam SignalIn Couples to an input to generate noise for. This can have as many channels as you like.
 * @tparam NoiseSpectrumSize The depth of the generator algorithm. Increasing the spectrum size adds lower frequency components to the noise and increases computation overhead. Values over 8 start to see diminishing returns as the frequencies added are not audible. Default is 5.
 */
template <typename SignalIn, int NoiseSpectrumSize = 5>
class AnalogNoiseSimulator : public Component<AnalogNoiseSimulator<SignalIn, NoiseSpectrumSize>>
{
public:
 static constexpr int Count = SignalIn::Count;
 
 SignalIn signalIn;
 
 PinkNoiseGenerator<Count, NoiseSpectrumSize> flickerNoise;
 NoiseGenerator<Count> shotNoise;
 NoiseGenerator<Count> jnNoise;
 
 SignalDelta<Connector<SignalIn>> snAmp;
 
 SimpleGain<
 Connector<decltype(shotNoise)>,
 Connector<decltype(snAmp)>
 > shotNoiseModulator;
 
 /**
  * @brief Modify the gainIn control on this to customise the flavour of the noise. Default 0.001.
  * 
  */
 SimpleGain<
 Connector<decltype(shotNoiseModulator.signalOut)>,
 ControlConstant<>
 > shotNoiseAtten;
 
 SimpleGain<
 Connector<decltype(jnNoise.noiseOut)>,
 Connector<SignalIn>
 > jnNoiseModulator;
 
 /**
  * @brief Modify the gainIn control on this to customise the flavour of the noise. Default 0.56234133 (-5 dB)
  * 
  */
 SimpleGain<
 Sum<2, Count>,
 ControlConstant<>
 > whiteNoiseAtten;
 
 Sum<2, Count> noiseMix;
 
 /**
  * @brief You can change the gainIn from the default of 0.0001 (-80dB) on this one to simulate hotter or colder electronics.
  * 
  */
 SimpleGain<
 Connector<decltype(noiseMix)>,
 ControlConstant<>
 > noiseLevel;
 
 Connector<decltype(noiseLevel.signalOut)> signalOut;
 
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
