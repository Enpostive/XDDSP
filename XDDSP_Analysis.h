/*
  ==============================================================================

    XDDSP_Analysis.h
    Created: 23 Aug 2023 7:54:13pm
    Author:  Adam Jackson

  ==============================================================================
*/

#ifndef XDDSP_Analysis_h
#define XDDSP_Analysis_h

#include "XDDSP_Types.h"
#include "XDDSP_Functions.h"
#include "XDDSP_Parameters.h"










namespace XDDSP
{










template <typename SignalIn, unsigned long ReserveSize = 32768>
class LUFSBlockCollector : public Component<LUFSBlockCollector<SignalIn>>, public Parameters::ParameterListener
{
 static inline SampleType LUFSDB(SampleType x)
 { return -0.691 + 10*log10(x); }

 mutable std::mutex mux;

 std::vector<SampleType> blocks;
 mutable std::vector<SampleType> blockRecord;
 DynamicCircularBuffer<> buffer;
 SampleType accum {0.};
 SampleType recipBlockLength;
 unsigned int count {0};
 unsigned int blockInterval {0};
 unsigned int blockLength {0};

public:
 static constexpr int Count = SignalIn::Count;
 
 // Specify your inputs as public members here
 SignalIn signalIn;
 
 // Specify your outputs like this
 // This component has no outputs
 
 // Include a definition for each input in the constructor
 LUFSBlockCollector(Parameters &p, SignalIn _signalIn) :
 Parameters::ParameterListener(p),
 signalIn(_signalIn)
 {
  blocks.reserve(ReserveSize);
  blockRecord.reserve(ReserveSize);
  updateSampleRate(p.sampleRate(), p.sampleInterval());
 }
 
 void updateSampleRate(SampleType sr, SampleType isr) override
 {
  blockInterval = 0.1*sr;
  blockLength = 4*blockInterval;
  recipBlockLength = 1./blockLength;
  buffer.setMaximumLength(blockLength);
 }
 
 // This function is responsible for clearing the output buffers to a default state when
 // the component is disabled.
 void reset() override
 {
  accum = 0.;
  count = 0;
  buffer.reset(0.);
 }
 
 int getBlockCount() const
 { return blocks.size(); }
 
 SampleType getLastBlock() const
 {
  std::lock_guard lock(mux);
  if (blocks.size() > 0) return LUFSDB(blocks.back());
  return 0.;
 }
 
 SampleType integrateBlocks() const
 {
  {
   std::lock_guard lock(mux);
   blockRecord = blocks;
  }

  int acceptedBlocks = 0;
  SampleType accum = 0;
  
  for (SampleType i : blockRecord)
  {
   if (LUFSDB(i) > -70.)
   {
    accum += i;
    ++acceptedBlocks;
   }
  }
  
  SampleType thresh;
  if (acceptedBlocks > 0)
   thresh = LUFSDB(accum/acceptedBlocks) - 10.;
  else
   thresh = -70.;
  acceptedBlocks = 0;
  accum = 0.;
  
  for (SampleType i : blockRecord)
  {
   if (LUFSDB(i) > thresh)
   {
    accum += i;
    ++acceptedBlocks;
   }
  }
  
  if (acceptedBlocks > 0)
   thresh = LUFSDB(accum/acceptedBlocks);
  else
   thresh = LUFSDB(0.);
  return thresh;
 }
 
 // startProcess prepares the component for processing one block and returns the step
 // size. By default, it returns the entire sampleCount as one big step.
 // int startProcess(int startPoint, int sampleCount)
 // { return std::min(sampleCount, StepSize); }
 
 // stepProcess is called repeatedly with the start point incremented by step size
 void stepProcess(int startPoint, int sampleCount) override
 {
  std::lock_guard lock(mux);
  for (int i = startPoint, s = sampleCount; s--; ++i)
  {
   SampleType sqrAccum = 0.;
   for (int c = 0; c < Count; ++c)
   {
    SampleType sqr = signalIn(c, i);
    sqr *= sqr;
    sqrAccum += sqr;
   }
   accum += buffer.tapIn(sqrAccum);
   accum -= buffer.tapOut(blockLength);
   
   ++count;
   if (count >= blockInterval)
   {
    count = 0;
    blocks.push_back(accum*recipBlockLength);
   }
  }
 }
 
 // finishProcess is called after the block has been processed
 // void finishProcess()
 // {}
};










}










#endif // XDDSP_Analysis_h
