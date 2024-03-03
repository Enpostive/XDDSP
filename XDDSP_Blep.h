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
 static SampleType lookupStep(double sn)
 {
  return lookup(BLEPTable, sn, -1.);
 }
 
 static SampleType lookupRamp(double sn)
 {
  return lookup(BLAMPTable, sn, 0.);
 }
};










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
