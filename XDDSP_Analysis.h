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










  /**
   * @brief Measures RMS levels and reports the overall loudness as described in the LUFS standard.
   *        This component only does the block splitting and RMS measurement parts of the standard. A k-Weighted filter is required to be placed before this component in the signal chain to obtain an actual dBLUFS measurement.
   * 
   * @tparam SignalIn Couples to the input signal.
   * @tparam ReserveSize The size to reserve for the first memory allocation.
   */
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
 
 /**
  * @brief The input signal
  * 
  */
 SignalIn signalIn;
 
 /**
  * @brief Construct a new LUFSBlockCollector object
  * 
  * @param p The Parameters object
  * @param _signalIn A coupler to connect the input signal. The coupler is copy constructed into the object.
  */
 LUFSBlockCollector(Parameters &p, SignalIn _signalIn) :
 Parameters::ParameterListener(p),
 signalIn(_signalIn)
 {
  blocks.reserve(ReserveSize);
  blockRecord.reserve(ReserveSize);
  updateSampleRate(p.sampleRate(), p.sampleInterval());
 }
 
 virtual void updateSampleRate(double sr, double isr) override
 {
  blockInterval = 0.1*sr;
  blockLength = 4*blockInterval;
  recipBlockLength = 1./blockLength;
  buffer.setMaximumLength(blockLength);
 }
 
 void reset() override
 {
  accum = 0.;
  count = 0;
  buffer.reset(0.);
  blocks.clear();
 }
 
 /**
  * @brief Get the number of blocks recorded by the component.
  * 
  * @return int The number of blocks.
  */
 int getBlockCount() const
 { return blocks.size(); }
 
 /**
  * @brief Calculate the RMS of the last block recorded.
  * 
  * @return SampleType The RMS in decibels.
  */
 SampleType getLastBlock() const
 {
  std::lock_guard lock(mux);
  if (blocks.size() > 0) return LUFSDB(blocks.back());
  return 0.;
 }
 
 /**
  * @brief Perform the LUFS calculations and report the perceived loudness of the input so far.
  * 
  * @return SampleType The perceived loudness of the input in decibels.
  */
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
};










}










#endif // XDDSP_Analysis_h
