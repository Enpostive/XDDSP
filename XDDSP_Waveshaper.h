//
//  XDDSP_Waveshaper.h
//  XDDSP
//
//  Created by Adam Jackson on 26/5/2022.
//

#ifndef XDDSP_Waveshaper_h
#define XDDSP_Waveshaper_h










namespace XDDSP
{










template <typename SignalIn>
class Waveshaper : public Component<Waveshaper<SignalIn>>
{
 // Private data members here
 WaveformFunction func;
public:
 static constexpr int Count = SignalIn::Count;
 
 // Specify your inputs as public members here
 SignalIn signalIn;
 
 // Specify your outputs like this
 Output<Count> signalOut;
 
 // Include a definition for each input in the constructor
 Waveshaper(Parameters &p, SignalIn signalIn) :
 signalIn(signalIn),
 signalOut(p)
 {
  resetFunction();
 }
 
 void setFunction(WaveformFunction f)
 {
  func = f;
 }
 
 void resetFunction()
 {
  func = [](SampleType x) { return x; };
 }
 
 // This function is responsible for clearing the output buffers to a default state when
 // the component is disabled.
 void reset()
 {
  signalOut.reset();
 }
 
 void stepProcess(int startPoint, int sampleCount)
 {
  for (int c = 0; c < Count; ++c)
  {
   for (int i = startPoint, s = sampleCount; s--; ++i)
   {
    signalOut.buffer(c, i) = func(signalIn(c, i));
   }
  }
 }
};










/*
 WaveshapeLookupTable is a lookup table object that can be called like a WaveformFunction and thus used with the above Waveshaper object
 */
template <int LookupTableSize = 512>
class WaveshapeLookupTable
{
 std::array<SampleType, LookupTableSize> lookup;
 SampleType min;
 SampleType max;
 SampleType pointScale;

public:
 WaveshapeLookupTable()
 {
  setTable(-1., 1., [](SampleType x){ return x; });
 }

 void setTable(SampleType tableMinimum, SampleType tableMaximum, WaveformFunction f)
 {
  min = tableMinimum;
  max = tableMaximum;
  pointScale = static_cast<SampleType>(LookupTableSize)/(max - min);
  SampleType pointSize = (max - min)/static_cast<SampleType>(LookupTableSize);
  
  for (int i = 0; i < LookupTableSize; ++i)
  {
   SampleType x = min + pointSize*i;
   lookup[i] = f(x);
   x += pointScale;
  }
 }
 
 SampleType operator()(SampleType x)
 {
  SampleType t = (x - min)*pointScale;
  t = fastBoundary(t, 0., static_cast<SampleType>(LookupTableSize - 1));
  
  IntegerAndFraction tIF(t);

  SampleType step1 = lookup[tIF.intRep()];
  if (tIF.fracPart() == 0.) return step1;
  else
  {
   SampleType step2 = lookup[tIF.intRep() + 1];
   return LERP(tIF.fracPart(), step1, step2);
  }
 }
};










}

#endif /* XDDSP_Waveshaper_h */
