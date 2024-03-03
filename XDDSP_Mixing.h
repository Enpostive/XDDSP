//
//  XDDSP_Mixing.h
//  XDDSPTestingHarness
//
//  Created by Adam Jackson on 29/5/2022.
//

#ifndef XDDSP_Mixing_h
#define XDDSP_Mixing_h


#include "XDDSP_Types.h"









namespace XDDSP
{










// Mixing Laws
// Use these classes for Crossfader and Panner components
// Expected inputs are between 0 and 1

namespace MixingLaws
{
typedef std::tuple<SampleType, SampleType> MixWeights;

struct LinearFadeLaw
{
 static MixWeights getWeights(SampleType p)
 {
  p = fastBoundary(p, 0., 1.);
  return {1. - p, p};
 }
};

struct EqualPowerLaw
{
 static MixWeights getWeights(SampleType p)
 {
  p = fastBoundary(p, 0., 1.);
  return {cos(p*M_PI*0.5), sin(p*M_PI*0.5)};
 }
};


struct FullMiddleLaw
{
 static MixWeights getWeights(SampleType p)
 {
  //p = 1. - 2*(1. - p);
  p = std::fma(-2., 1. - p, 1);
  return {fastBoundary(1. - p, 0., 1.), fastBoundary(1. + p, 0., 1.)};
 }
};
}










template <
typename ASignalIn,
typename BSignalIn,
typename CrossfadeIn,
typename MixLaw,
int StepSize = 16
>
class Crossfader :
public Component<Crossfader<ASignalIn, BSignalIn, CrossfadeIn, MixLaw, StepSize>, StepSize>
{
 static_assert(ASignalIn::Count == BSignalIn::Count, "Crossfader expects inputs to have same channel count");
 static_assert(ASignalIn::Count == CrossfadeIn::Count || CrossfadeIn::Count == 1, "Crossfader expects control signal to have one channel or same channel count as inputs");
 // Private data members here
 
public:
 static constexpr int Count = ASignalIn::Count;
 
 // Specify your inputs as public members here
 ASignalIn aSignalIn;
 BSignalIn bSignalIn;
 CrossfadeIn crossfadeIn;
 
 // Specify your outputs like this
 Output<Count> signalOut;
 
 // Include a definition for each input in the constructor
 Crossfader(Parameters &p,
            ASignalIn _aSignalIn,
            BSignalIn _bSignalIn,
            CrossfadeIn _crossfadeIn) :
 aSignalIn(_aSignalIn),
 bSignalIn(_bSignalIn),
 crossfadeIn(_crossfadeIn),
 signalOut(p)
 {}
 
 // This function is responsible for clearing the output buffers to a default state when
 // the component is disabled.
 void reset()
 {
  signalOut.reset();
 }
 
 // stepProcess is called repeatedly with the start point incremented by step size
 void stepProcess(int startPoint, int sampleCount)
 {
  MixingLaws::MixWeights w = MixLaw::getWeights(crossfadeIn(startPoint));
  for (int c = 0; c < Count; ++c)
  {
   if (CrossfadeIn::Count > 1)
   {
    w = MixLaw::getWeights(crossfadeIn(c, startPoint));
   }
   for (int i = startPoint, s = sampleCount; s--; ++i)
   {
    //    signalOut.buffer(c, i) = aSignalIn(c, i)*std::get<0>(w) + bSignalIn(c, i)*std::get<1>(w);
    signalOut.buffer(c, i) = std::fma(aSignalIn(c, i),
                               std::get<0>(w),
                               bSignalIn(c, i) * std::get<1>(w));
   }
  }
 }
};










template <
typename SignalIn,
typename PanIn,
typename MixLaw,
int StepSize = 16
>
class Panner : public Component<Panner<SignalIn, PanIn, MixLaw, StepSize>, StepSize>
{
 static_assert(SignalIn::Count == PanIn::Count || PanIn::Count == 1, "Panner expects control signal to have one channel or same channel count as input");
 
 // Private data members here
public:
 static constexpr int Count = SignalIn::Count;
 
 // Specify your inputs as public members here
 SignalIn signalIn;
 PanIn panIn;
 
 // Specify your outputs like this
 Output<Count> aSignalOut;
 Output<Count> bSignalOut;
 
 // Include a definition for each input in the constructor
 Panner(Parameters &p, SignalIn _signalIn, PanIn _panIn) :
 signalIn(_signalIn),
 panIn(_panIn),
 aSignalOut(p),
 bSignalOut(p)
 {}
 
 // This function is responsible for clearing the output buffers to a default state when
 // the component is disabled.
 void reset()
 {
  aSignalOut.reset();
  bSignalOut.reset();
 }
 
 // stepProcess is called repeatedly with the start point incremented by step size
 void stepProcess(int startPoint, int sampleCount)
 {
  MixingLaws::MixWeights w = MixLaw::getWeights(panIn(startPoint));
  for (int c = 0; c < Count; ++c)
  {
   if (PanIn::Count > 1)
   {
    w = MixLaw::getWeights(panIn(c, startPoint));
   }
   for (int i = startPoint, s = sampleCount; s--; ++i)
   {
    aSignalOut.buffer(c, i) = std::get<0>(w)*signalIn(c, i);
    bSignalOut.buffer(c, i) = std::get<1>(w)*signalIn(c, i);
   }
  }
 }
};










template <
typename SignalIn,
typename PanIn,
typename MixLaw = MixingLaws::EqualPowerLaw,
int StepSize = 16>
class StereoPanner : public Component<StereoPanner<SignalIn, PanIn, MixLaw, StepSize>, StepSize>
{
 static_assert(SignalIn::Count == 2 && PanIn::Count == 1, "StereoPanner expects a specific configuration of inputs: 2 channels on SignalIn and 1 channel on PanIn");
 
 // Private data members here
 
 const SampleType middleLevel;
public:
 static constexpr int Count = 2;
 
 // Specify your inputs as public members here
 SignalIn signalIn;
 PanIn panIn;
 
 // Specify your outputs like this
 Output<Count> signalOut;
 
 // Include a definition for each input in the constructor
 StereoPanner(Parameters &p, SignalIn _signalIn, PanIn _panIn) :
 middleLevel(1./std::get<0>(MixLaw::getWeights(0.5))),
 signalIn(_signalIn),
 panIn(_panIn),
 signalOut(p)
 {}
 
 // This function is responsible for clearing the output buffers to a default state when
 // the component is disabled.
 void reset()
 {
  signalOut.reset();
 }
 
 // stepProcess is called repeatedly with the start point incremented by step size
 void stepProcess(int startPoint, int sampleCount)
 {
  MixingLaws::MixWeights w = MixLaw::getWeights(panIn(sampleCount));
  std::get<0>(w) *= middleLevel;
  std::get<1>(w) *= middleLevel;
  
  for (int i = startPoint, s = sampleCount; s--; ++i)
  {
   signalOut.buffer(0, i) = signalIn(0, i)*std::get<0>(w);
   signalOut.buffer(1, i) = signalIn(1, i)*std::get<1>(w);
  }
 }
};










template <
typename SignalIn,
typename GainIn,
typename PanIn,
typename MixLaw = MixingLaws::EqualPowerLaw,
int StepSize = IntegerMaximum>
class MonoToStereoMixBus : Component<MonoToStereoMixBus<SignalIn, GainIn, PanIn, MixLaw, StepSize>, StepSize>
{
 static_assert(SignalIn::Count == 1,
               "MonoToStereoMixBus only accepts mono signal inputs");
 static_assert(GainIn::Count == 1 && PanIn::Count == 1,
               "MonoToStereoMixBus only accepts single channel control inputs");
 
 const SampleType middleLevel;
 
public:
 struct MixCoupler
 {
  PConnector<1> signalIn;
  PConnector<1> gainIn;
  PConnector<1> panIn;
 };
 
 static constexpr int Count = 2;
 
 std::vector<MixCoupler> connections;
 Output<2> stereoOut;
 
 MonoToStereoMixBus(Parameters &p) :
 middleLevel(1./std::get<0>(MixLaw::getWeights(0.5))),
 stereoOut(p)
 {
 }
 
 void reset()
 {
  stereoOut.reset();
 }
 
 // startProcess prepares the component for processing one block and returns the step
 // size. By default, it returns the entire sampleCount as one big step.
 // int startProcess(int startPoint, int sampleCount)
 // { return std::min(sampleCount, StepSize); }
 
 // stepProcess is called repeatedly with the start point incremented by step size
 void stepProcess(int startPoint, int sampleCount)
 {
  stereoOut.reset();
  for (MixCoupler &m: connections)
  {
   MixingLaws::MixWeights w = MixLaw::getWeights(m.panIn(sampleCount));
   std::get<0>(w) *= middleLevel;
   std::get<1>(w) *= middleLevel;
   for (int i = startPoint, s = sampleCount; s--; ++i)
   {
    SampleType g = m.signalIn(i)*m.gainIn(i);
    stereoOut.buffer(0, i) += g*std::get<0>(w);
    stereoOut.buffer(1, i) += g*std::get<1>(w);
   }
  }
 }
};










template <
typename SignalIn,
typename GainIn,
typename PanIn,
typename MixLaw = MixingLaws::EqualPowerLaw,
int StepSize = IntegerMaximum>
class StereoToStereoMixBus : Component<StereoToStereoMixBus<SignalIn, GainIn, PanIn, MixLaw, StepSize>, StepSize>
{
 static_assert(SignalIn::Count == 2,
               "StereoToStereoMixBus only accepts stereo signal inputs");
 static_assert(GainIn::Count == 1 && PanIn::Count == 1,
               "StereoToStereoMixBus only accepts single channel control inputs");
 
 const SampleType middleLevel;
 
public:
 struct MixCoupler
 {
  PConnector<2> signalIn;
  PConnector<1> gainIn;
  PConnector<1> panIn;
 };
 
 static constexpr int Count = 2;
 
 std::vector<MixCoupler> connections;
 Output<2> stereoOut;
 
 StereoToStereoMixBus(Parameters &p) :
 middleLevel(1./std::get<0>(MixLaw::getWeights(0.5))),
 stereoOut(p)
 {
 }
 
 void reset()
 {
  stereoOut.reset();
 }
 
 // startProcess prepares the component for processing one block and returns the step
 // size. By default, it returns the entire sampleCount as one big step.
 // int startProcess(int startPoint, int sampleCount)
 // { return std::min(sampleCount, StepSize); }
 
 // stepProcess is called repeatedly with the start point incremented by step size
 void stepProcess(int startPoint, int sampleCount)
 {
  stereoOut.reset();
  for (MixCoupler &m: connections)
  {
   MixingLaws::MixWeights w = MixLaw::getWeights(m.panIn(sampleCount));
   std::get<0>(w) *= middleLevel;
   std::get<1>(w) *= middleLevel;
   for (int i = startPoint, s = sampleCount; s--; ++i)
   {
    SampleType g = m.gainIn(i);
    stereoOut.buffer(0, i) += g*std::get<0>(w)*m.signalIn(0, i);
    stereoOut.buffer(1, i) += g*std::get<1>(w)*m.signalIn(1, i);
   }
  }
 }
};










}

#endif /* XDDSP_Mixing_h */
