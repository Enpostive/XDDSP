/*
  ==============================================================================

    XDDSP_Blep.h
    Created: 9 Apr 2023 8:35:07pm
    Author:  Adam Jackson

  ==============================================================================
*/

#ifndef XDDSP_Blep_h
#define XDDSP_Blep_h

#include "XDDSP_Functions.h"










namespace XDDSP
{










  /**
   * @brief A class encapsulating the logic to perform lookups in the Band-Limited stEP and Band-Limited rAMP tables.
   * 
   * Only hermite interpolation is supported.
   */
class BLEPLookup
{
public:
 static constexpr PowerSize BLEPSize {4};
 
private:
 static constexpr double TABLEBOUNDARY = static_cast<double>(BLEPSize.size());
 static constexpr int oversample = 4;
 
 static double BLEPTable[66];
 static double BLAMPTable[66];
 
 static SampleType lookup(double *table, double sn, double stepm1)
 {
  double t = sn;
  if (t < 0. || t >= TABLEBOUNDARY) return 0.;
  //  t += BLEPMidPoint;
  t *= oversample;
  
  IntegerAndFraction tI(t);
  double step1 = table[tI.intRep()];
  double step2 = table[tI.intRep()+1];
  double step3 = table[tI.intRep()+2];
  
  if (tI.intRep()) stepm1 = table[tI.intRep()-1];
  
  return hermite(tI.fracPart(), stepm1, step1, step2, step3);
 }
 
public:
 /**
  * @brief Return a band-limited step sample.
  * 
  * @param sn Time since the step.
  * @return SampleType The sample.
  */
 static SampleType lookupStep(double sn)
 {
  return lookup(BLEPTable, sn, -1.);
 }
 
 /**
  * @brief Return a band-limited ramp sample.
  * 
  * @param sn Time since the ramp started.
  * @return SampleType The sample.
  */
 static SampleType lookupRamp(double sn)
 {
  return lookup(BLAMPTable, sn, 0.);
 }
};










/**
 * @brief Objects of this class can be used to trigger and buffer the samples for band limited steps and ramps. These are useful for synthesizing band-limited oscillators and band-limited distortion algorithms.
 * 
 */
class BLEPGenerator
{
 static constexpr unsigned int BLEPSize = BLEPLookup::BLEPSize.size();
 static constexpr unsigned int BLEPMask = BLEPLookup::BLEPSize.mask();
 
 std::array<SampleType, BLEPSize> blepBuffer;
 int blepc;
 
public:
 BLEPGenerator()
 {
  reset();
 }
 
 void reset()
 {
  blepBuffer.fill(0);
  blepc = 0;
 }
 
 /**
  * @brief Generate a BLEP and add it into the BLEP buffer.
  * 
  * @param gain The magnitude of the BLEP.
  * @param bc The time since the BLEP started, in samples. This is expected to be between 0 and 1.
  */
 void applyBLEP(SampleType gain, SampleType bc)
 {
  int cc = blepc;
  for (int j = 0; j < BLEPSize; ++j)
  {
   blepBuffer[cc] += gain*BLEPLookup::lookupStep(bc);
   cc = (cc + 1) & BLEPMask;
   bc++;
  }
 }
 
 /**
  * @brief Generate a BLAMP and add it into the BLEP buffer.
  * 
  * @param gain The magnitude of the BLAMP.
  * @param bc The time since the BLAMP started, in samples. This is expected to be between 0 and 1.
  */
 void applyBLAMP(SampleType gain, SampleType bc)
 {
  int cc = blepc;
  for (int j = 0; j < BLEPSize; ++j)
  {
   blepBuffer[cc] += gain*BLEPLookup::lookupRamp(bc);
   cc = (cc + 1) & BLEPMask;
   bc++;
  }
 }
 
 /**
  * @brief Get the next sample in the BLEP buffer and advance the buffer once.
  * 
  * @return SampleType The next BLEP buffer sample.
  */
 SampleType getNextBLEPSample()
 {
  SampleType t = blepBuffer[blepc];
  blepBuffer[blepc] = 0.;
  blepc = (blepc + 1) & BLEPMask;
  return t;
 }
};









}









#endif // XDDSP_Blep_h
