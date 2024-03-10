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
void fftDynamicSize(T *data, unsigned long n)
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
 
 
 T nRec = 1./static_cast<T>(n);
 for (i = 0; i < n; ++i) data[i] *= nRec;
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
void fftStaticSize(std::array<T, n> &data)
{
 fftDynamicSize(data.data(), data.size());
}

template <typename T>
void fftDynamicSize(std::vector<T> &data)
{
 fftDynamicSize(data.data(), data.size());
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
 return {data[index], data[n - index - 1]};
}









template <typename T>
void multiplyFFTs(T* input, T* m, unsigned long n)
{
 for (unsigned long p = 0; p < n/2; ++p)
 {
  const unsigned long p2 = n - p - 1;
  // Complex multipliction formula
  // x + yi = (a + bi)(c + di)
  //        = (ac - bd) + (ad + bc)i
  const T a = input[p];
  const T b = input[p2];
  const T c = m[p];
  const T d = m[p2];
  
  // x = ac - bd
  input[p] = a*c - b*d;
  // y = ad + bc
  input[p2] = a*d + b*c;
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









/*
template <typename SignalIn>
class FIRFilter : public Component<FIRFilter<SignalIn>>
{
public:
 static constexpr int Count = SignalIn::Count;

private:
 // Private data members here
 unsigned long fftSize = 0;
 unsigned long blockSize = 0;
 
 std::vector<std::vector<SampleType>> FFTKernels;
 std::array<std::vector<SampleType>, Count> procBuff;
 std::array<DynamicCircularBuffer<>, Count> outputBuffer;
 
 void computeBlockLength(unsigned long kernelLength)
 {
  PowerSize s;
  s.setToNextPowerTwo(kernelLength);
  fftSize = 8 * s.bits();
  blockSize = fftSize - kernelLength - 1;
 }
  
public:
 
 // Specify your inputs as public members here
 SignalIn signalIn;
 
 // Specify your outputs like this
 Output<Count> signalOut;
 
 // Include a definition for each input in the constructor
 FIRFilter(Parameters &p, SignalIn _signalIn) :
 signalIn(_signalIn),
 signalOut(p)
 {}
 
 // This function is responsible for clearing the output buffers to a default state when
 // the component is disabled.
 void reset()
 {
  signalOut.reset();
 }
 
 unsigned long getPropagationDelay()
 { return blockSize; }
 
 void resetKernels(unsigned long fft)
 {
  FFTKernels.clear();
  blockSize = 0;
 }
 
 template <typename T>
 void setKernel(int index, T* data, unsigned long length)
 {
  dsp_assert(index >= 0 && index < Count);
  
  if (index > 0) FFTKernels.resize(Count);
  else if (FFTKernels.size() == 0) FFTKernels.resize(1);
  
  FFTKernels[index].resize(length);
  std::copy(data, data + length, FFTKernels[index].begin());
 }
 
 
 
 // startProcess prepares the component for processing one block and returns the step
 // size. By default, it returns the entire sampleCount as one big step.
// int startProcess(int startPoint, int sampleCount)
// { }

 // stepProcess is called repeatedly with the start point incremented by step size
 void stepProcess(int startPoint, int sampleCount)
 {
  // If there are no kernels loaded, simply pass signal through
  if (FFTKernels.size() == 0)
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
   
   // Copy each input into it's process buffer and zero out the remaining length
   for (int c = 0; c < Count; ++c)
   {
    for (int i = startPoint, s = sampleCount; s--; ++i)
    {
     
    }
   }
  }
 }
 
 // finishProcess is called after the block has been processed
// void finishProcess()
// {}
};
*/
 
 
 
 
 
 
 
 
 
}

#endif /* FFT_h */
