//
//  XDDSP.cpp
//  XDDSPTestingHarness
//
//  Created by Adam Jackson on 1/6/2022.
//

/*
 This file will not compile. It simply contains useful code templates which can be copy-pasted into the library to extend it
 */



/*
 Plugin DSP Component Template
 
 Here is a neat little template for a component to encapsulate an entire DSP network
*/
class PluginDSP : public Component<PluginDSP>
{
 // Private data members here
public:
 static constexpr int Count = 2;
 
 PluginInput<Count> signalIn;
 Output<Count> signalOut;
 
 PluginDSP(Parameters &p) :
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












