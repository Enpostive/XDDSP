//
//  FFT.h
//  XDDSPLib
//
//  Created by Adam Jackson on 3/1/22.
//

#ifndef FFT_h
#define FFT_h

#include <cmath>
#include <array>
#include "XDDSP_Types.h"
#include "XDDSP_Parameters.h"




namespace XDDSP {


namespace FFTConstants {


static constexpr double sqrt2 = 1.414213562373095;
static constexpr double recSqrt2 = 1./sqrt2;

}


template <typename T>
void fftDynamicSize(T *data, unsigned long n, bool normalise = true)
{
 unsigned long i, j, k, i5, i6, i7, i8, i0, iD, i1, i2, i3, i4, n2, n4, n8;
 T t1, t2, t3, t4, t5, t6, a3, ss1, ss3, cc1, cc3, a, e;
 
 n4 = n - 1;
 
 //data shuffling
 for (i = 0, j = 0, n2 = n/2;
      i<n4;
      i++)
 {
  if (i < j)
  {
   t1 = data[j];
   data[j] = data[i];
   data[i] = t1;
  }
  k = n2;
  while (k <= j)
  {
   j -= k;
   k >>= 1;
  }
  j += k;
 }
 
 /*----------------------*/
 
 //length two butterflies
 i0 = 0;
 iD = 4;
 do
 {
  for (; i0 < n4; i0 += iD)
  {
   i1 = i0 + 1;
   t1 = data[i0];
   data[i0] = t1 + data[i1];
   data[i1] = t1 - data[i1];
  }
  iD <<= 1;
  i0 = iD - 2;
  iD <<= 1;
 } while (i0 < n4);
 
 /*----------------------*/
 //L shaped butterflies
 n2 = 2;
 for(k = n; k > 2; k >>= 1)
 {
  n2 <<= 1;
  n4 = n2>>2;
  n8 = n2>>3;
  e = 2*M_PI/(n2);
  i1 = 0;
  iD = n2<<1;
  do
  {
   for (; i1 < n; i1 += iD)
   {
    i2 = i1 + n4;
    i3 = i2 + n4;
    i4 = i3 + n4;
    t1 = data[i4] + data[i3];
    data[i4] -= data[i3];
    data[i3] = data[i1]-t1;
    data[i1] += t1;
    if (n4 != 1)
    {
     i0 = i1 + n8;
     i2 += n8;
     i3 += n8;
     i4 += n8;
     t1 = (data[i3] + data[i4])*FFTConstants::recSqrt2;
     t2 = (data[i3] - data[i4])*FFTConstants::recSqrt2;
     data[i4] = data[i2] - t1;
     data[i3] =- data[i2] - t1;
     data[i2] = data[i0] - t2;
     data[i0] += t2;
    }
   }
   iD <<= 1;
   i1 = iD - n2;
   iD <<= 1;
  } while (i1 < n);
  a = e;
  for (j = 2; j <= n8; j++)
  {
   a3 = 3*a;
   cc1 = std::cos(a);
   ss1 = std::sin(a);
   cc3 = std::cos(a3);
   ss3 = std::sin(a3);
   a = j*e;
   i = 0;
   iD = n2<<1;
   do
   {
    for (; i < n; i += iD)
    {
     i1 = i + j - 1;
     i2 = i1 + n4;
     i3 = i2 + n4;
     i4 = i3 + n4;
     i5 = i + n4 - j + 1;
     i6 = i5 + n4;
     i7 = i6 + n4;
     i8 = i7 + n4;
     t1 = data[i3]*cc1 + data[i7]*ss1;
     t2 = data[i7]*cc1 - data[i3]*ss1;
     t3 = data[i4]*cc3 + data[i8]*ss3;
     t4 = data[i8]*cc3 - data[i4]*ss3;
     t5 = t1 + t3;
     t6 = t2 + t4;
     t3 = t1 - t3;
     t4 = t2 - t4;
     t2 = data[i6] + t6;
     data[i3] = t6 - data[i6];
     data[i8] = t2;
     t2 = data[i2] - t3;
     data[i7] =- data[i2] - t3;
     data[i4] = t2;
     t1 = data[i1] + t5;
     data[i6] = data[i1] - t5;
     data[i1] = t1;
     t1 = data[i5] + t4;
     data[i5] -= t4;
     data[i2] = t1;
    }
    iD <<= 1;
    i = iD - n2;
    iD <<= 1;
   } while(i < n);
  }
 }
 
 if (normalise)
 {
  T nRec = 1./static_cast<T>(n);
  for (i = 0; i < n; ++i) data[i] *= nRec;
 }
}










template <typename T>
void ifftDynamicSize(T *data, unsigned long n)
{
 long i, j, k, i5, i6, i7, i8, i0, iD, i1, i2, i3, i4, n2, n4, n8, n1;
 T t1, t2, t3, t4, t5, a3, ss1, ss3, cc1, cc3, a, e;
 
 n1 = n - 1;
 n2 = n<<1;
 for(k = n; k > 2; k >>= 1)
 {
  iD = n2;
  n2 >>= 1;
  n4 = n2 >> 2;
  n8 = n2 >> 3;
  e = 2.*M_PI/(n2);
  i1 = 0;
  do
  {
   for (; i1 < n; i1 += iD)
   {
    i2 = i1 + n4;
    i3 = i2 + n4;
    i4 = i3 + n4;
    t1 = data[i1] - data[i3];
    data[i1] += data[i3];
    data[i2] *= 2.;
    data[i3] = t1 - 2.*data[i4];
    data[i4] = t1 + 2.*data[i4];
    if (n4 != 1)
    {
     i0 = i1 + n8;
     i2 += n8;
     i3 += n8;
     i4 += n8;
     t1 = (data[i2] - data[i0])*FFTConstants::recSqrt2;
     t2 = (data[i4] + data[i3])*FFTConstants::recSqrt2;
     data[i0] += data[i2];
     data[i2] = data[i4] - data[i3];
     data[i3] = 2.*(-t2 - t1);
     data[i4] = 2.*(-t2 + t1);
    }
   }
   iD <<= 1;
   i1 = iD - n2;
   iD <<= 1;
  } while (i1 < n1);
  a = e;
  for (j = 2; j <= n8; ++j)
  {
   a3 = 3.*a;
   cc1 = std::cos(a);
   ss1 = std::sin(a);
   cc3 = std::cos(a3);
   ss3 = std::sin(a3);
   a = j*e;
   i = 0;
   iD = n2<<1;
   do
   {
    for (; i < n; i += iD)
    {
     i1 = i + j - 1;
     i2 = i1 + n4;
     i3 = i2 + n4;
     i4 = i3 + n4;
     i5 = i + n4 - j + 1;
     i6 = i5 + n4;
     i7 = i6 + n4;
     i8 = i7 + n4;
     t1 = data[i1] - data[i6];
     data[i1] += data[i6];
     t2 = data[i5] - data[i2];
     data[i5] += data[i2];
     t3 = data[i8] + data[i3];
     data[i6] = data[i8] - data[i3];
     t4 = data[i4] + data[i7];
     data[i2] = data[i4] - data[i7];
     t5 = t1 - t4;
     t1 += t4;
     t4 = t2 - t3;
     t2 += t3;
     data[i3] = t5*cc1 + t4*ss1;
     data[i7] = -t4*cc1 + t5*ss1;
     data[i4] = t1*cc3 - t2*ss3;
     data[i8] = t2*cc3 + t1*ss3;
    }
    iD <<= 1;
    i = iD - n2;
    iD <<= 1;
   } while(i < n1);
  }
 }
 
 i0 = 0;
 iD = 4;
 do
 {
  for (; i0 < n1; i0 += iD)
  {
   i1 = i0 + 1;
   t1 = data[i0];
   data[i0] = t1 + data[i1];
   data[i1] = t1 - data[i1];
  }
  iD <<= 1;
  i0 = iD - 2;
  iD <<= 1;
 } while (i0 < n1);
 
 
 // Data shuffling
 for (i = 0, j = 0, n2 = n/2;
      i<n1;
      ++i)
 {
  if (i < j) std::swap(data[j], data[i]);
  k = n2;
  while (k <= j)
  {
   j -= k;
   k >>= 1;
  }
  j += k;
 }
}










template <typename T, unsigned long n>
void fftStaticSize(std::array<T, n> &data, bool normalise = true)
{
 fftDynamicSize(data.data(), data.size(), normalise);
}

template <typename T>
void fftDynamicSize(std::vector<T> &data, bool normalise = true)
{
 fftDynamicSize(data.data(), data.size(), normalise);
}










template <typename T, unsigned long n>
void ifftStaticSize(std::array<T, n> &data)
{
 ifftDynamicSize(data.data(), data.size());
}

template <typename T>
void ifftDynamicSize(std::vector<T> &data)
{
 ifftDynamicSize(data.data(), data.size());
}










template <typename T>
std::pair<T, T> getComplexSample(T* data, unsigned long index, unsigned long n)
{
 if (index == 0) return {data[0], 0.};
 if (index == n/2) return {data[n/2], 0.};
 return {data[index], data[n - index]};
}









template <typename T>
void multiplyFFTs(T* output, T* in1, T* in2, unsigned long n)
{
 output[0] = in1[0] * in2[0];
 output[n/2] = in1[n/2] * in2[n/2];
 for (unsigned long p = 1; p < n/2; ++p)
 {
  const unsigned long p2 = n - p;
  // Complex multipliction formula
  // x + yi = (a + bi)(c + di)
  //        = (ac - bd) + (ad + bc)i
  const T a = in1[p];
  const T b = in1[p2];
  const T c = in2[p];
  const T d = in2[p2];
  
  // x = ac - bd
  output[p] = std::fma(a, c, -b*d);
  // y = ad + bc
  output[p2] = std::fma(a, d, b*c);
 }
}










template <typename T>
void multiplyAndAddFFTs(T* output, T* in1, T* in2, unsigned long n)
{
 output[0] += in1[0] * in2[0];
 output[n/2] += in1[n/2] * in2[n/2];
 for (unsigned long p = 1; p < n/2; ++p)
 {
  const unsigned long p2 = n - p;
  const T a = in1[p];
  const T b = in1[p2];
  const T c = in2[p];
  const T d = in2[p2];
  
  // x += ac - bd
  output[p] = std::fma(a, c, output[p]);
  output[p] = std::fma(-b, d, output[p]);
  // y += ad + bc
  output[p2] = std::fma(a, d, output[p2]);
  output[p2] = std::fma(b, c, output[p2]);
 }
}










template <typename T>
T magnitudeAt(T* data, unsigned long index, unsigned long n)
{
 auto sample = getComplexSample(data, index, n);
 T magSqr = sample.first*sample.first + sample.second*sample.second;
 return sqrt(magSqr);
}









template <typename T>
void calculateMagnitudes(T* data, unsigned long n)
{
 int i;
 for (i = 0; i < n/2; ++i)
 {
  data[i] = magnitudeAt(data, i, n);
 }
 for (;i < n; ++i)
 {
  data[i] = 0.;
 }
}










/*
 template <typename T, unsigned long n>
 T autoCorrelateStaticSizeHalved(std::array<T, n> &data)
 {
 constexpr unsigned long nHalved = n/2;
 
 fftStaticSize(data);
 
 // Multiply by conjugate
 for (unsigned long i = 0; i < nHalved; ++i)
 {
 data[i] = data[i]*data[i] + data[data.size() - i - 1]*data[data.size() - i - 1];
 data[data.size() - i - 1] = 0.;
 }
 data[0] = data[1] = 0.;
 ifftStaticSize(data);
 
 T norm = data[0];
 if (norm > 0.) norm = 1./norm;
 for (unsigned long i = 0; i < nHalved; ++i)
 {
 data[i] = data[i]*norm;
 }
 
 unsigned long maxima = 1;
 constexpr T threshold = 0.8;
 while (maxima < nHalved && data[maxima] >= threshold) ++ maxima;
 while (maxima < nHalved && data[maxima] < threshold) ++maxima;
 while (maxima < nHalved && data[maxima - 1] <= data[maxima]) ++ maxima;
 if (data[maxima-2] > data[maxima]) --maxima;
 
 T fMaxima = -1;
 
 if (maxima < nHalved - 1)
 {
 T maxima1;
 XDDSP::IntersectionEstimator<T> intersect;
 intersect.setSampleValues(data[maxima - 2],
 data[maxima - 1],
 data[maxima],
 data[maxima + 1]);
 
 intersect.calculateStationaryPoints(fMaxima, maxima1);
 fMaxima = maxima + maxima1 - 2.;
 }
 
 return fMaxima;
 }
 */










namespace ConvolutionEngine
{


struct KernelContainer
{
 std::vector<std::vector<SampleType>> k;
 
 void setup(unsigned int kernelCount, unsigned int kernelSize)
 {
  k.resize(kernelCount);
  for (auto &kk: k) kk.resize(kernelSize, 0.);
 }
 
 unsigned int size() { return static_cast<unsigned int>(k.size()); }
 
 SampleType *getKernel(unsigned int index)
 {
  dsp_assert(index >= 0 && index < k.size());
  return k[index].data();
 }
};





class ConvolutionParameters
{
 PowerSize blockPowerSize;
 unsigned int _fftSize;
 unsigned int _segmentSize;
 
public:
 ConvolutionParameters()
 {
  setMaxBlockSize(64);
 }
 
 ConvolutionParameters(int maxBlockSize)
 {
  setMaxBlockSize(maxBlockSize);
 }
 
 void setMaxBlockSize(int maxBlockSize)
 {
  blockPowerSize.setToNextPowerTwo(maxBlockSize);
  _fftSize = 2*blockPowerSize.size();
  _segmentSize = _fftSize - blockPowerSize.size();
 }
 
 int blockSize() const { return blockPowerSize.size(); }
 
 unsigned int fftSize() const { return _fftSize; }
 
 unsigned int segmentSize() const {return _segmentSize; }
};





struct ImpulseResponse
{
 KernelContainer impulseKernels;
 unsigned int sampleCount;

 void setImpulseResponse(ConvolutionParameters &cp,
                         const SampleType *impulseSamples,
                         unsigned int size)
 {
  sampleCount = size;
  const unsigned int impulseKernCount = size / cp.segmentSize() + 1;
  impulseKernels.setup(impulseKernCount, cp.fftSize());
  
  unsigned int c = 0;
  const unsigned int copySize = cp.segmentSize();
  for (int i = 0; i < impulseKernCount; ++i)
  {
   impulseKernels.k[i].assign(cp.fftSize(), 0.);
   unsigned int cs = std::min(copySize, size - c);
   std::copy(impulseSamples + c, impulseSamples + c + cs, impulseKernels.getKernel(i));
   fftDynamicSize(impulseKernels.getKernel(i), cp.fftSize());
   c += copySize;
  }
 }
};




/*
class ConvolutionEngine
{
 ImpulseResponse *imp {nullptr};
 ConvolutionParameters &cp;
 KernelContainer inputKernels;
 
 std::vector<SampleType> inputBuffer;
 std::vector<SampleType> accumBuffer;
 std::vector<SampleType> outputPreBuffer;
 std::vector<SampleType> outputBuffer;
 std::vector<SampleType> overlapBuffer;
 
 unsigned int currentKernel;
 unsigned int inputDataPos;
 
 void addVector(SampleType* r, SampleType* a, SampleType* b, unsigned int count)
 {
  for (int i = 0; i < count; ++i) r[i] = a[i] + b[i];
 }
 
 void accumulateVector(SampleType* r, SampleType *a, unsigned int count)
 {
  addVector(r, r, a, count);
 }
 
public:
 ConvolutionEngine(ConvolutionParameters &p) : cp(p)
 {}
 
 void setImpulseResponse(ImpulseResponse &impulse)
 {
  imp = &impulse;
  const unsigned int impulseKernCount = imp->impulseKernels.size();
  const unsigned int inputKernCount =
  (cp.blockSize() > 128) ? impulseKernCount : 3*impulseKernCount;
  inputKernels.setup(inputKernCount, cp.fftSize());
  reset();
 }

 void reset()
 {
  inputBuffer.assign(cp.fftSize(), 0.);
  outputPreBuffer.assign(cp.fftSize(), 0.);
  accumBuffer.assign(cp.fftSize(), 0.);
  outputBuffer.assign(cp.fftSize(), 0.);
  overlapBuffer.assign(cp.fftSize(), 0.);
  for (auto &kk : inputKernels.k)
  {
   kk.assign(cp.fftSize(), 0.);
  }
  currentKernel = 0;
  inputDataPos = 0;
 }
 
 void processSamples(SampleType *input,
                     SampleType *output,
                     unsigned int sampleCount)
 {
  unsigned int numSamplesProcessed = 0;
  unsigned int indexStep = inputKernels.size() / imp->impulseKernels.size();
  
  while (numSamplesProcessed < sampleCount)
  {
   const bool firstInputData = (inputDataPos == 0);
   unsigned int numToProcess = std::min(sampleCount - numSamplesProcessed, cp.blockSize() - inputDataPos);
   std::copy(input + numSamplesProcessed,
             input + numSamplesProcessed + numToProcess,
             inputBuffer.begin() + inputDataPos);
   std::copy(inputBuffer.begin(),
             inputBuffer.end(),
             inputKernels.k[currentKernel].begin());
   fftDynamicSize(inputKernels.k[currentKernel], false);
   
   if (firstInputData)
   {
    accumBuffer.assign(cp.fftSize(), 0.);
    int index = currentKernel;
    for (int i = 1; i < imp->impulseKernels.size(); ++i)
    {
     index += indexStep;
     if (index > inputKernels.size()) index -= inputKernels.size();
     std::copy(inputKernels.k[index].begin(),
               inputKernels.k[index].end(),
               outputPreBuffer.begin());
     multiplyFFTs(outputPreBuffer.data(),
                  imp->impulseKernels.getKernel(i), cp.fftSize());
     accumulateVector(accumBuffer.data(), outputPreBuffer.data(), cp.fftSize());
    }
    
    std::copy(accumBuffer.begin(), accumBuffer.end(), outputPreBuffer.begin());
   }
   
   std::copy(outputPreBuffer.begin(), outputPreBuffer.end(), outputBuffer.begin());
   std::copy(inputKernels.k[currentKernel].begin(),
             inputKernels.k[currentKernel].end(),
             accumBuffer.begin());
   multiplyFFTs(accumBuffer.data(),
                imp->impulseKernels.getKernel(0),
                cp.fftSize());
   accumulateVector(outputBuffer.data(), accumBuffer.data(), cp.fftSize());
   ifftDynamicSize(outputBuffer);
   
   addVector(output + numSamplesProcessed,
             outputBuffer.data() + inputDataPos,
             overlapBuffer.data() + inputDataPos,
             numToProcess);
   
   inputDataPos += numToProcess;
   
   if (inputDataPos == cp.blockSize())
   {
    inputBuffer.assign(cp.fftSize(), 0.);
    inputDataPos = 0;
    accumulateVector(outputBuffer.data() + cp.blockSize(),
                     overlapBuffer.data() + cp.blockSize(),
                     cp.fftSize() - 2*cp.blockSize());
    for (int i = 0; i < (cp.fftSize() - cp.blockSize()); ++i)
    {
     overlapBuffer[i] = outputBuffer[i + cp.blockSize()];
    }
    currentKernel = (currentKernel > 0) ? currentKernel - 1 : inputKernels.size() - 1;
   }
   numSamplesProcessed += numToProcess;
  }
 }
};
*/




class ConvolutionEngine
{
 ImpulseResponse *imp {nullptr};
 ConvolutionParameters &convParam;
 
 std::vector<SampleType> inputBuffer;
 std::vector<SampleType> procBuffer;
 std::vector<SampleType> olapBuffer;
 unsigned int olapC {0};
 PowerSize olapSize;
 
 void addVector(SampleType* r, SampleType* a, SampleType* b, unsigned int count)
 {
  for (int i = 0; i < count; ++i) r[i] = a[i] + b[i];
 }
 
 void addVector(SampleType* r, SampleType *a, unsigned int count)
 {
  addVector(r, r, a, count);
 }
 
public:
 ConvolutionEngine(ConvolutionParameters &cp) :
 convParam(cp)
 {}
 
 void setImpulseResponse(ImpulseResponse &impulse)
 {
  imp = &impulse;
 }
 
 void reset()
 {
  if (imp)
  {
   procBuffer.resize(convParam.fftSize());
   inputBuffer.resize(convParam.fftSize());
   unsigned int overlapSize = convParam.segmentSize() + imp->sampleCount + convParam.fftSize();
   olapSize.setToNextPowerTwo(overlapSize);
   olapBuffer.assign(olapSize.size(), 0.);
   olapC = 0;
  }
 }
 
 void processSamples(SampleType *input,
                     SampleType *output,
                     unsigned int sampleCount)
 {
  if (imp)
  {
   {
    unsigned int i = 0;
    for (; i < sampleCount; ++i) inputBuffer[i] = input[i];
    for (; i < convParam.fftSize(); ++i) inputBuffer[i] = 0.;
   }
   
   fftDynamicSize(inputBuffer, false);
   
   for (int i = 0; i < imp->impulseKernels.size(); ++i)
   {
    multiplyFFTs(procBuffer.data(),
                 inputBuffer.data(),
                 imp->impulseKernels.k[i].data(),
                 convParam.fftSize());
    ifftDynamicSize(procBuffer);
    
    unsigned int j;
    unsigned int c = convParam.segmentSize()*i;
    unsigned int q = convParam.fftSize();
    for (j = 0; j < q; j++, ++c)
    {
     olapBuffer[(olapC + c) & olapSize.mask()] += procBuffer[j];
    }
   }
   
   for (int i = 0; i < sampleCount; ++i)
   {
    output[i] = olapBuffer[olapC];
    olapBuffer[olapC] = 0.;
    olapC = (olapC + 1) & olapSize.mask();
   }
  }
 }
};





}









template <typename SignalIn>
class ConvolutionFilter : public Component<ConvolutionFilter<SignalIn>>, public Parameters::ParameterListener
{
public:
 static constexpr int Count = SignalIn::Count;

private:
 // Private data members here
 bool initialised = false;
 
 struct ImpulseSample
 {
  bool set {false};
  SampleType* pointerToSample {nullptr};
  unsigned int length {0};
 };
 
 Parameters &dsp;
 ConvolutionEngine::ConvolutionParameters cp;
 
 std::array<ImpulseSample, Count> samples;
 std::vector<ConvolutionEngine::ImpulseResponse> imp;
 std::vector<ConvolutionEngine::ConvolutionEngine> eng;
 std::vector<SampleType> inputBuffer;
 
public:
 
 // Specify your inputs as public members here
 SignalIn signalIn;
 
 // Specify your outputs like this
 Output<Count> signalOut;
 
 // Include a definition for each input in the constructor
 ConvolutionFilter(Parameters &p, SignalIn _signalIn) :
 Parameters::ParameterListener(p),
 dsp(p),
 signalIn(_signalIn),
 signalOut(p)
 {
  resetConvolution();
  updateBufferSize(p.bufferSize());
  for (int i = 0; i < Count; ++i)
  {
   eng.emplace_back(cp);
  }
 }
 
 // This function is responsible for clearing the output buffers to a default state when
 // the component is disabled.
 void reset() override
 {
  for (auto &e : eng) e.reset();
  signalOut.reset();
 }
 
 void resetConvolution()
 {
  initialised = false;
  samples.fill(ImpulseSample());
  imp.clear();
  imp.assign(Count, ConvolutionEngine::ImpulseResponse());
 }
 
 template <typename T>
 void setImpulse(int index, T* data, unsigned int length)
 {
  dsp_assert(index >= 0 && index < Count);
  samples[index].set = data != nullptr;
  samples[index].pointerToSample = data;
  samples[index].length = length;
 }
 
 virtual void updateBufferSize(int bs) override
 {
  inputBuffer.resize(bs);
  cp.setMaxBlockSize(bs);
  initialiseConvolution();
 }

 bool isInitlialised() { return initialised; }

 void initialiseConvolution()
 {
  initialised = false;
  if (!samples[0].set) return;
  
  for (int i = 0; i < Count; ++i)
  {
   if (samples[i].set)
   {
    imp[i].setImpulseResponse(cp, samples[i].pointerToSample, samples[i].length);
    eng[i].setImpulseResponse(imp[i]);
   }
   else eng[i].setImpulseResponse(imp[0]);
  }
  
  initialised = true;
  for (auto &e : eng) e.reset();
 }
 
 // startProcess prepares the component for processing one block and returns the step
 // size. By default, it returns the entire sampleCount as one big step.
// int startProcess(int startPoint, int sampleCount)
// { }

 // stepProcess is called repeatedly with the start point incremented by step size
 void stepProcess(int startPoint, int sampleCount) override
 {
  // If there are no kernels loaded, simply pass signal through
  if (!initialised)
  {
   for (int c = 0; c < Count; ++c)
   {
    for (int i = startPoint, s = sampleCount; s--; ++i)
    {
     signalOut.buffer(c, i) = signalIn(c, i);
    }
   }
  }
  
  // otherwise process the inputs
  else
  {
   for (int c = 0; c < Count; ++c)
   {
    for (int i = startPoint, s = sampleCount; s--; ++i)
    {
     inputBuffer[i] = signalIn(c, i);
    }
    eng[c].processSamples(inputBuffer.data() + startPoint,
                          signalOut.buffer[c] + startPoint,
                          sampleCount);
   }
  }
 }
 
 // finishProcess is called after the block has been processed
// void finishProcess()
// {}
};

 
 
 
 
 
 
 
 
 
}

#endif /* FFT_h */
