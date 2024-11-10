//
//  XDDSP.h
//  XDDSP
//
//  Created by Adam Jackson on 24/5/2022.
//

#ifndef XDDSP_h
#define XDDSP_h

#include "XDDSP_Types.h"
#include "XDDSP_Functions.h"
#include "XDDSP_Parameters.h"
#include "XDDSP_Classes.h"
#include "XDDSP_Inputs.h"
#include "XDDSP_Utilities.h"
#include "XDDSP_Monitors.h"
#include "XDDSP_Noise.h"
#include "XDDSP_Mixing.h"
#include "XDDSP_Filters.h"
#include "XDDSP_Delay.h"
#include "XDDSP_Waveshaper.h"
#include "XDDSP_Oscillators.h"
#include "XDDSP_PiecewiseEnvelopeData.h"
#include "XDDSP_Envelopes.h"
#include "XDDSP_FIRImpulses.h"
#include "XDDSP_WindowFunctions.h"
#include "XDDSP_FFT.h"
#include "XDDSP_Analysis.h"
#include "XDDSP_Polyphony.h"










/*
 This component definition template will come in handy

// Input types are specified as template arguments
// Input types go first, because they never have defaults
// Don't forget to copy the input type names into the Component template arguments
template <typename SignalIn>
class NewComponent : public Component<NewComponent<SignalIn>>
{
 // Private data members here
public:
 static constexpr int Count = SignalIn::Count;
 
 // Specify your inputs as public members here
 SignalIn signalIn;
 
 // Specify your outputs like this
 Output<Count> signalOut;
 
 // Include a definition for each input in the constructor
 NewComponent(Parameters &p, SignalIn _signalIn) :
 signalIn(_signalIn),
 signalOut(p)
 {}
 
 // This function is responsible for clearing the output buffers to a default state when
 // the component is disabled.
 void reset()
 {
  signalOut.reset();
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
    // DSP work done here
   }
  }
 }
 
 // triggerProcess is called once if 'samplesToNextTrigger' reaches zero
 // components can use 'setNextTrigger' to set a trigger point
// void triggerprocess(int triggerPoint)
// {}

 // finishProcess is called after the block has been processed
// void finishProcess()
// {}
};

 
 
 
 
 
 
 
 
 
 This definition template is for new components meant to be used with SummingArray
 
 // Input types can only be PCoupler<Output>, ControlConstant or SmoothControlContant
template <int ChannelCount>
class SummingComponent : public Component<SummingComponent<ChannelCount>>
{
 // Private data members here
public:
 static constexpr int Count = ChannelCount;
 
 // Inputs can only be certain types
 PCoupler<Output<ChannelCount>> pCoupleInput;
 ControlConstant<ChannelCount> constInput;
 
 // DON'T CHANGE THIS DEFINITION OR THE ARRAY WILL BREAK!
 Output<Count> signalOut;
 // Coupler<Output<Count>> signalOut;
 // Don't forget to fix the reset() method if the coupler is used
 
 // The only parameter allowed in the constructor is Parameters
 SummingComponent(Parameters &p) :
 signalOut(p)
 {}
 
 // This function is responsible for clearing the output buffers to a default state when
 // the component is disabled.
 void reset()
 {
  signalOut.reset();
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
    // DSP work done here
   }
  }
 }
 
 // finishProcess is called after the block has been processed
 // void finishProcess()
 // {}
};
 
 
 
 
 
 
 
 
 
 
 */














#endif /* XDDSP_h */
