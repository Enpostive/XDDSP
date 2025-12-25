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










/**
 * @brief A component which applies a waveshaping function to an input.
 * 
 * @tparam SignalIn Couples to an input signal. Can have as many channels as you like.
 */
template <typename SignalIn>
class Waveshaper : public Component<Waveshaper<SignalIn>>
{
 WaveformFunction func;
public:
 static constexpr int Count = SignalIn::Count;
 
 SignalIn signalIn;
 
 Output<Count> signalOut;
 
 Waveshaper(Parameters &p, SignalIn signalIn) :
 signalIn(signalIn),
 signalOut(p)
 {
  resetFunction();
 }
 
 /**
  * @brief Set the function object that will be used to transform each sample.
  * 
  * The callable object is called with the raw input sample as it's parameter and is expected to return a transformed sample.
  * 
  * @param f A callable object to transform the signal.
  */
 void setFunction(WaveformFunction f)
 {
  func = f;
 }
 
 /**
  * @brief Remove the current function and set an identity function in its place.
  * 
  */
 void resetFunction()
 {
  func = [](SampleType x) { return x; };
 }
 
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

 /**
  * @brief A callable function object which transforms the input signal using a lookup table.
  * 
  * This will be deprecated in favour of XDDSP::LookupTable. Use that instead of this object in any new code.
  * 
  * @tparam LookupTableSize the size of the lookup table.
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
