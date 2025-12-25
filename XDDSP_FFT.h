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
#include <thread>
#include "XDDSP_Types.h"
#include "XDDSP_Parameters.h"
#include "XDDSP_Functions.h"




namespace XDDSP {


namespace FFTConstants {


static constexpr double sqrt2 = 1.414213562373095;
static constexpr double recSqrt2 = 1./sqrt2;

}


/**
 * @brief Compute an FFT inside an arbitrary buffer.
 * 
 * The input samples are transformed in place from the time domain to the frequency domain. The output consists of complex numbers, with the real parts being in the first half of the array and the imaginary parts running backwards in the second half. The function XDDSP::getComplexSample is provided to fetch complex numbers from the resulting array and convert them to std::complex<>
 * 
 * @tparam T The sample type
 * @param data The data to transform. The data is overwritten by the transformed data.
 * @param n The size of the buffer. Must be a power of 2.
 * @param normalise If true, the transformed data is normalised at the end.
 */
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










/**
 * @brief Transform an FFT back into the time domain.
 * 
 * The input samples are transformed in place from the frequency domain to the time domain. The input consists of complex numbers, with the real parts being in the first half of the array and the imaginary parts running backwards in the second half.
 * 
 * @tparam T The sample type.
 * @param data The data to transfer.
 * @param n The size of the buffer, must be a power of 2.
 */
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










/**
 * @brief Compute an FFT on data in a std::array
 * 
 * The input samples are transformed in place from the time domain to the frequency domain. The output consists of complex numbers, with the real parts being in the first half of the array and the imaginary parts running backwards in the second half. The function XDDSP::getComplexSample is provided to fetch complex numbers from the resulting array and convert them to std::complex<>
 * 
 * @tparam T The sample type, inferred from the input.
 * @tparam n The size of the array, inferred from the input.
 * @param data The samples to transform.
 * @param normalise If true, the resulting FFT is normalised.
 */
template <typename T, unsigned long n>
void fftStaticSize(std::array<T, n> &data, bool normalise = true)
{
 fftDynamicSize(data.data(), data.size(), normalise);
}

/**
 * @brief Compute an FFT on data in a std::vector
 * 
 * The input samples are transformed in place from the time domain to the frequency domain. The output consists of complex numbers, with the real parts being in the first half of the array and the imaginary parts running backwards in the second half. The function XDDSP::getComplexSample is provided to fetch complex numbers from the resulting array and convert them to std::complex<>
 * 
 * @tparam T The sample type, inferred from the input.
 * @param data The vector containing the data.
 * @param normalise If true, the resulting FFT is normalised.
 */
template <typename T>
void fftDynamicSize(std::vector<T> &data, bool normalise = true)
{
 fftDynamicSize(data.data(), data.size(), normalise);
}









/**
 * @brief Perform an inverse FFT on samples in a std::array.
 * 
 * The input samples are transformed in place from the frequency domain to the time domain. The input consists of complex numbers, with the real parts being in the first half of the array and the imaginary parts running backwards in the second half.
 * 
 * @tparam T The sample type.
 * @tparam n The size of the buffer, must be a power of 2.
 * @param data The data to transfer.
 */
template <typename T, unsigned long n>
void ifftStaticSize(std::array<T, n> &data)
{
 ifftDynamicSize(data.data(), data.size());
}

/**
 * @brief 
 * 
 * The input samples are transformed in place from the frequency domain to the time domain. The input consists of complex numbers, with the real parts being in the first half of the array and the imaginary parts running backwards in the second half.
 * 
 * @tparam T The sample type.
 * @param data The data to transfer.
 */
template <typename T>
void ifftDynamicSize(std::vector<T> &data)
{
 ifftDynamicSize(data.data(), data.size());
}










/**
 * @brief Extract a complex sample from an FFT result.
 * 
 * @tparam T Sample type inferred from input parameters.
 * @param data A pointer to the sample data.
 * @param index The index of the complex number sought.
 * @param n The length of the actual array.
 * @return std::pair<T, T> The complex number returned.
 */
template <typename T>
std::pair<T, T> getComplexSample(T* data, unsigned long index, unsigned long n)
{
 if (index == 0) return {data[0], 0.};
 if (index == n/2) return {data[n/2], 0.};
 return {data[index], data[n - index]};
}










/**
 * @brief Perform a complex multiplication of two FFTs
 * 
 * @tparam T The sample type, inferred from the input.
 * @param output A pointer to a suitable output buffer.
 * @param in1 A pointer to one FFT input.
 * @param in2 A pointer to the other FFT inpit.
 * @param n The size of the output buffer, which must be the same as the two input buffers.
 */
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










/**
 * @brief Multiply two FFTs together and add the result to another FFT result.
 * 
 * @tparam T The sample type, inferred from the input.
 * @param output A pointer to the output buffer which contains the FFT to be added to.
 * @param in1 A pointer to one FFT input.
 * @param in2 A pointer to the other FFT inpit.
 * @param n The size of the output buffer, which must be the same as the two input buffers.
 */
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










/**
 * @brief Calculate the magnitude of one complex number in the FFT result.
 * 
 * @tparam T The sample type.
 * @param data The pointer to the data buffer.
 * @param index The index into the buffer.
 * @param n The length of the buffer.
 * @return T The calculated magnitude.
 */
template <typename T>
T magnitudeAt(T* data, unsigned long index, unsigned long n)
{
 auto sample = getComplexSample(data, index, n);
 T magSqr = sample.first*sample.first + sample.second*sample.second;
 return sqrt(magSqr);
}










/**
 * @brief Calculate every magnitude in an FFT. The FFT is overwritten with magnitudes in the first half of the buffer, and zeros in the second half.
 * 
 * @tparam T The sample type.
 * @param data The pointer to the buffer.
 * @param n The size of the buffer.
 */
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











/**
 * @brief Do autocorrelation on the input data. The input data is destroyed.
 * 
 * @tparam T The sample type, inferred from the input.
 * @param data The pointer to the data.
 * @param n The length of the data.
 * @return T The autocorrelation result.
 */
template <typename T>
T autoCorrelateDynamicSizeHalved(T *data, unsigned long n)
{
 unsigned long nHalved = n/2;
 
 fftDynamicSize(data, n);
 
 // Multiply by conjugate
 for (unsigned long i = 0; i < nHalved; ++i)
 {
  data[i] = data[i]*data[i] + data[n - i - 1]*data[n - i - 1];
  data[n - i - 1] = 0.;
 }
 data[0] = data[1] = 0.;
 ifftDynamicSize(data, n);
 
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
  XDDSP::IntersectionEstimator intersect;
  intersect.setSampleValues(data[maxima - 2],
                            data[maxima - 1],
                            data[maxima],
                            data[maxima + 1]);
  
  intersect.calculateStationaryPoints(fMaxima, maxima1);
  fMaxima = maxima + maxima1 - 2.;
 }
 
 return fMaxima;
}


/**
 * @brief Do autocorrelation on the input data in a std::array. The input data is destroyed.
 * 
 * @tparam T The sample type, inferred from the input.
 * @tparam n The length of the data.
 * @param data The pointer to the data.
 * @return T The autocorrelation result.
 */
template <typename T, unsigned long n>
T autoCorrelateStaticSizeHalved(std::array<T, n> &data)
{
 return autoCorrelateDynamicSizeHalved(data.data(), n);
}

/**
 * @brief Do autocorrelation on the input data in a std::vector. The input data is destroyed.
 * 
 * @tparam T The sample type, inferred from the input.
 * @param data The pointer to the data.
 * @return T The autocorrelation result.
 */
template <typename T>
T autoCorrelateDynamicSizeHalved(std::vector<T> &data)
{
 return autoCorrelateDynamicSizeHalved(data.data(), data.size());
}


/**
 * @brief A class which pre-allocates a processing buffer to perform autocorrelation.
 * 
 * @tparam SampleLength The size of the buffer to allocate.
 */
template <unsigned long SampleLength>
class AutoCorrelator
{
 Parameters &dspParam;
 std::array<SampleType, 2*SampleLength> buffer;
 
public:
/**
 * @brief Construct a new Auto Correlator object
 * 
 * @param p A parameters object.
 */
 AutoCorrelator(Parameters &p) : dspParam(p)
 {}
 
 /**
  * @brief Do autocorrelation on the input data. The input data is destroyed.
  * 
  * @param data The pointer to the data.
  * @param length The length of the data.
  * @return SampleType The autocorrelation result.
  */
 SampleType autoCorrelate(SampleType *data, unsigned long length)
 {
  if (length > SampleLength) length = SampleLength;

  std::copy(data, data + length, buffer.begin());
  std::fill(buffer.begin() + length, buffer.end(), 0.);
  applyWindowFunction(WindowFunction::Gauss(length, 0.3), buffer.data(), length);
  SampleType a = autoCorrelateStaticSizeHalved(buffer);
  if (a > 0) a = dspParam.sampleRate()/a;
  return a;
 }
 
 /**
  * @brief Do autocorrelation on the input data in a std::array.
  * 
  * @tparam n The length of the data.
  * @param data The pointer to the data.
  * @return SampleType The autocorrelation result.
  */
 template <unsigned long n>
 SampleType autoCorrelate(std::array<SampleType, n> &data)
 { return autoCorrelate(data.data(), n); }
 
 /**
 * @brief Do autocorrelation on the input data in a std::vector.
 * 
 * @param data The pointer to the data.
 * @return SampleType The autocorrelation result.
 */
SampleType autoCorrelate(std::vector<SampleType> &data)
 { return autoCorrelate(data.data(), data.size()); }
};










/**
 * @brief A class with a buffer stored in an internal std::vector used for doing antocorrelation,
 * 
 */
class DynamicAutoCorrelator
{
 Parameters &dspParam;
 unsigned long sampleLength {0};
 std::vector<SampleType> buffer;
 
public:
/**
 * @brief Construct a new Dynamic Auto Correlator object
 * 
 * @param p Parameters object
 */
 DynamicAutoCorrelator(Parameters &p) : dspParam(p)
 {}
 
 /**
  * @brief Set the size of the buffer.
  * 
  * @param bufferSize The new buffer size.
  */
 void setBufferSize(unsigned long bufferSize)
 {
  sampleLength = bufferSize;
  buffer.resize(2*bufferSize, 0.);
 }
 
 /**
  * @brief Do autocorrelation on the input data. The input data is destroyed.
  * 
  * @param data The pointer to the data.
  * @param length The length of the data.
  * @return SampleType The autocorrelation result.
  */
 SampleType autoCorrelate(SampleType *data, unsigned long length)
 {
  if (length > sampleLength) length = sampleLength;


  std::copy(data, data + length, buffer.begin());
  std::fill(buffer.begin() + length, buffer.end(), 0.);
  applyWindowFunction(WindowFunction::Gauss(length, 0.3), buffer.data(), length);
  SampleType a = autoCorrelateDynamicSizeHalved(buffer);
  if (a > 0) a = dspParam.sampleRate()/a;
  return a;
 }
 
 /**
  * @brief Do autocorrelation on the input data in a std::array.
  * 
  * @tparam n The length of the data.
  * @param data The pointer to the data.
  * @return SampleType The autocorrelation result.
  */
 template <unsigned long n>
 SampleType autoCorrelate(std::array<SampleType, n> &data)
 { return autoCorrelate(data.data(), n); }
 
 /**
  * @brief Do autocorrelation on the input data in a std::vector.
  * 
  * @param data The pointer to the data.
  * @return SampleType The autocorrelation result.
  */
 SampleType autoCorrelate(std::vector<SampleType> &data)
 { return autoCorrelate(data.data(), data.size()); }
};










namespace ConvolutionEngine
{

/**
 * @brief An internal container for storing convolution kernels.
 * 
 */
struct KernelContainer
{
 std::vector<std::vector<SampleType>> k;
 
 /// Set up the container
 void setup(unsigned int kernelCount, unsigned int kernelSize)
 {
  k.resize(kernelCount);
  for (auto &kk: k) kk.resize(kernelSize, 0.);
 }
 
 /// Get the size of the container
 unsigned int size() { return static_cast<unsigned int>(k.size()); }
 
 /// Return a pointer to convolution kernel data.
 SampleType *get(unsigned int index)
 {
  dsp_assert(index >= 0 && index < k.size());
  return k[index].data();
 }
};





/**
 * @brief An internal class for managing the parameters of the convolution engine.
 * 
 */
class ConvolutionParameters
{
 int iBS {256};
 int dBS {256};
  
 int iFS;
 int dFS;
 
 bool dP {false};
 
public:
 ConvolutionParameters()
 {
  setParameters(iBS, dBS);
 }
 
 // Set the buffer size and FFT size hint
 void setParameters(int bufferSize, int fftHint)
 {
  iBS = PowerSize::nextPowerTwoMinusOne(bufferSize) + 1;
  dBS = PowerSize::nextPowerTwoMinusOne(fftHint) + 1;
  dBS = std::max(iBS, dBS);
  
  dP = iBS != dBS;
  iFS = 2*iBS;
  dFS = 2*dBS;
 }
 
 int inputFFTSize() const { return iFS; }
 int deferredFFTSize() const { return dFS; }
 int inputSize() const { return iBS; }
 int deferredSize() const { return dBS; }
 bool deferredProcessing() const { return dP; }
};





/**
 * @brief The data structure for storing impulse response data.
 * 
 */
struct ImpulseResponse
{
 KernelContainer inputKernels;
 KernelContainer deferredKernels;
 unsigned int sampleCount;

 // Load an impulse response into the container, with consideration for the convolution parameters.
 void setImpulseResponse(ConvolutionParameters &cp,
                         const SampleType *impulseSamples,
                         unsigned int size)
 {
  if (size < cp.deferredFFTSize()) cp.setParameters(cp.inputSize(), size/2);
  sampleCount = size;
  unsigned int inputKernCount = (size/cp.inputSize() + 1);
  if (cp.deferredProcessing() && cp.deferredSize()/cp.inputSize() < inputKernCount)
  {
   inputKernCount = cp.deferredSize()/cp.inputSize();
  }
  computeKernels(inputKernels,
                 inputKernCount,
                 cp.inputFFTSize(),
                 cp.inputSize(),
                 size,
                 0,
                 impulseSamples);

  if (cp.deferredProcessing())
  {
   const unsigned int deferredKernCount = size / cp.deferredSize();
   computeKernels(deferredKernels,
                  deferredKernCount,
                  cp.deferredFFTSize(),
                  cp.deferredSize(),
                  size,
                  1,
                  impulseSamples);
  }
  else deferredKernels.setup(0, 0);
 }
 
private:
 void computeKernels(KernelContainer &k,
                     unsigned int count,
                     unsigned int fftSize,
                     unsigned int segmentSize,
                     unsigned int totalSize,
                     unsigned int startPoint,
                     const SampleType *impulseSamples)
 {
  k.setup(count, fftSize);
  
  unsigned int c = startPoint*segmentSize;
  for (int i = 0; i < count; ++i)
  {
   k.k[i].assign(fftSize, 0.);
   unsigned int start = std::min(c, totalSize - 1);
   unsigned int cs = std::min(c + segmentSize, totalSize - 1);
   if (start < cs) std::copy(impulseSamples + start, impulseSamples + cs, k.get(i));
   fftDynamicSize(k.get(i), fftSize);
   c += segmentSize;
  }
 }
};





/**
 * @brief An internal class which encapsulates a convolution engine for one signal.
 * 
 * @tparam ConnectorChannelCount The expected channel count of the input signal.
 */
template <int ConnectorChannelCount>
class ConvolutionEngine
{
 ImpulseResponse *imp {nullptr};
 ConvolutionParameters &cp;
 
 bool startDeferredProcess {false};
 bool killDeferredProcess {false};
 std::mutex dmux;
 std::mutex amux;
 std::condition_variable deferredProcessTrigger;
 int procBufferInUse {0};
 std::array<std::vector<SampleType>, 2> deferProc;
 std::thread deferredProcThread;

 std::vector<SampleType> inputBuffer;
 std::vector<SampleType> deferBuffer;
 std::vector<SampleType> procBuffer;
 std::vector<SampleType> olapBuffer;
 unsigned int olapC {0};
 unsigned int deferC {0};
 unsigned int deferOlapC {0};
 PowerSize olapSize;
 
 void multiplyAndAccumulate(SampleType *input,
                            SampleType *irKernel,
                            SampleType *proc,
                            unsigned int fftSize,
                            unsigned int offset)
 {
  multiplyFFTs(proc, input, irKernel, fftSize);
  ifftDynamicSize(proc, fftSize);
  unsigned int c = offset;
  {
   std::unique_lock lock(amux);
   for (unsigned int j = 0; j < fftSize; j++, ++c)
   {
    olapBuffer[c & olapSize.mask()] += proc[j];
   }
  }
 }
 
 void doConvolution()
 {
  const unsigned int fftSize = cp.inputFFTSize();
  const unsigned int segmentSize = cp.inputSize();
  fftDynamicSize(inputBuffer.data(), fftSize, false);
  
  for (int i = 0; i < imp->inputKernels.size(); ++i)
  {
   multiplyAndAccumulate(inputBuffer.data(),
                         imp->inputKernels.get(i),
                         procBuffer.data(),
                         fftSize,
                         olapC + segmentSize*i);
  }
 }
 
 void doDeferredConvolution(unsigned int offset)
 {
  {
   std::unique_lock lock(dmux);
   
   std::fill(deferProc[procBufferInUse].begin() + cp.deferredSize(),
             deferProc[procBufferInUse].end(),
             0.);
   fftDynamicSize(deferProc[procBufferInUse].data(), cp.deferredFFTSize(), false);
   
   multiplyAndAccumulate(deferProc[procBufferInUse].data(),
                         imp->deferredKernels.get(0),
                         deferBuffer.data(),
                         cp.deferredFFTSize(),
                         olapC + offset);
   deferOlapC = olapC + offset;
   procBufferInUse = 1 - procBufferInUse;
   startDeferredProcess = true;
  }
  deferredProcessTrigger.notify_one();
 }
 
 void deferredProcessor()
 {
  std::unique_lock lock(dmux);
  while (!killDeferredProcess)
  {
   deferredProcessTrigger.wait(lock, [&]() { return killDeferredProcess || startDeferredProcess; });

   if (startDeferredProcess)
   {
    int pbu = 1 - procBufferInUse;
    startDeferredProcess = false;
    for (unsigned int i = 1; i < imp->deferredKernels.size(); ++i)
    {
     multiplyAndAccumulate(deferProc[pbu].data(),
                           imp->deferredKernels.get(i),
                           deferBuffer.data(),
                           cp.deferredFFTSize(),
                           deferOlapC + cp.deferredSize()*i);
    }
   }
  }
 }
  
public:
 PConnector<ConnectorChannelCount> signalIn;
 
 /**
  * @brief Construct a new Convolution Engine object.
  * 
  * The convolution engine starts a thread upon construction. The thread waits on an internal structure for data to process.
  * 
  * @tparam Source The source of the connection, inferred from the parameter.
  * @param cp The convolution parameters object.
  * @param c The coupler to take input from.
  */
 template<typename Source>
 ConvolutionEngine(ConvolutionParameters &cp, Coupler<Source, ConnectorChannelCount> &c) :
 cp(cp),
 deferredProcThread([&]() {deferredProcessor();}),
 signalIn(c)
 {}
 
 /**
  * @brief Construct a copy of a Convolution Engine object.
  * 
  * The convolution engine starts a thread upon construction. The thread waits on an internal structure for data to process.
  * 
  * @param rhs The other object to copy from.
  */
 ConvolutionEngine(ConvolutionEngine &&rhs) :
 cp(rhs.cp),
 deferredProcThread([&]() {deferredProcessor();}),
 signalIn(rhs.signalIn)
 {}
 
 /**
  * @brief Stop the processing thread, then destroy the Convolution Engine object.
  * 
  * Currently, this object waits for the processing thread to finish with no time out.
  * TODO: Add a timeout for the process thread wait.
  */
 ~ConvolutionEngine()
 {
  {
   std::unique_lock lock(dmux);
   killDeferredProcess = true;
  }
  deferredProcessTrigger.notify_one();
  if (deferredProcThread.joinable()) deferredProcThread.join();
 }
 
 /**
  * @brief Set the Impulse Response object.
  * 
  * @param impulse 
  */
 void setImpulseResponse(ImpulseResponse &impulse)
 {
  std::unique_lock lock(dmux);
  imp = &impulse;
 }
 
 /**
  * @brief Initialise the convolution engine.
  * 
  * Initialises and resets the convolution engine. This needs to be called whenever any of the convolution parameters are changed.
  */
 void initialise()
 {
  {
   std::unique_lock lock(dmux);
   procBuffer.resize(cp.deferredFFTSize());
   inputBuffer.resize(cp.inputFFTSize());
   deferBuffer.resize(cp.deferredFFTSize());
   deferProc[0].resize(cp.deferredFFTSize());
   deferProc[1].resize(cp.deferredFFTSize());
   if (imp)
   {
    unsigned int overlapSize = cp.deferredSize() + imp->sampleCount + cp.deferredFFTSize();
    olapSize.setToNextPowerTwo(overlapSize);
    olapBuffer.resize(olapSize.size());
   }
  }

  reset();
 }
 
 /**
  * @brief Reset the convolution engine.
  * 
  * Performs a quick reset. Call this to clear the internal buffers and reset the convolution process, without doing all the extra work needed when convolution parameters are changed.
  */
 void reset()
 {
  std::unique_lock lock(dmux);
  deferC = 0;
  procBufferInUse = 0;
  if (imp)
  {
   olapBuffer.assign(olapSize.size(), 0.);
   olapC = 0;
  }
 }
 
 /**
  * @brief Process some samples from the input.
  * 
  * @param channel Which channel to process.
  * @param startPoint The start point in the channel.
  * @param output A pointer to an output buffer.
  * @param sampleCount How many samples to process.
  */
 void processSamples(int channel,
                     int startPoint,
                     SampleType *output,
                     unsigned int sampleCount)
 {
  if (imp)
  {
   {
    unsigned int i = 0;
    for (; i < sampleCount; ++i) inputBuffer[i] = signalIn(channel, i + startPoint);
    for (; i < cp.inputFFTSize(); ++i) inputBuffer[i] = 0.;
   }
   
   if (cp.deferredProcessing())
   {
    unsigned int i = 0;
    for (; i < sampleCount && deferC < cp.deferredSize(); ++i, ++deferC)
    {
     deferProc[procBufferInUse][deferC] = inputBuffer[i];
    }
    if (deferC == cp.deferredSize())
    {
     deferC = 0;
     doDeferredConvolution(i);
     for (; i < sampleCount && deferC < cp.deferredSize(); ++i, ++deferC)
     {
      deferProc[procBufferInUse][deferC] = inputBuffer[i];
     }
    }
   }
   
   doConvolution();
   
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









/**
 * @brief A component for performing convolution on an input signal.
 * 
 * This component can handle mono or multi-channel impulse responses. Upon construction, the convolution engine starts multiple threads to perform background processing on signal data. These threads are automatically stopped upon destruction of the component, however they currently wait on the stopping thread indefinitely, so a stuck thread might result in a hang.
 * 
 * @tparam SignalIn Couples to the input signal. Can have as many channels as you like.
 */
template <typename SignalIn>
class ConvolutionFilter : public Component<ConvolutionFilter<SignalIn>>, public Parameters::ParameterListener
{
public:
 static constexpr int Count = SignalIn::Count;

private:
 bool initialised = false;
 
 struct ImpulseSample
 {
  bool set {false};
  SampleType* pointerToSample {nullptr};
  unsigned int length {0};
 };
 
 int selectedFFTSize {256};
 Parameters &dsp;
 ConvolutionEngine::ConvolutionParameters cp;
 
 std::array<ImpulseSample, Count> samples;
 std::vector<ConvolutionEngine::ImpulseResponse> imp;
 std::vector<ConvolutionEngine::ConvolutionEngine<Count>> eng;
 
 std::mutex mtx;
 
public:
 
 SignalIn signalIn;
 
 Output<Count> signalOut;
 
 // Upon construction, the convolution engine starts multiple threads to perform background processing on signal data. These threads are automatically stopped upon destruction of the component, however they currently wait on the stopping thread indefinitely, so a stuck thread might result in a hang.
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
   eng.emplace_back(cp, signalIn);
  }
 }
 
 void reset() override
 {
  std::lock_guard<std::mutex> lock(mtx);
  for (auto &e : eng) e.reset();
  signalOut.reset();
 }
 
 /**
  * @brief Clear the convolution engines, unload the impulse response samples and bypass the component.
  * 
  */
 void resetConvolution()
 {
  std::lock_guard<std::mutex> lock(mtx);
  initialised = false;
  samples.fill(ImpulseSample());
  imp.clear();
  imp.assign(Count, ConvolutionEngine::ImpulseResponse());
 }
 
 /**
  * @brief Set the impulse response data for one channel.
  * 
  * Set the impulse response samples with this method first, then call ConvolutionFilter::initialiseConvolution to load the impulse response samples into the convolution engine. You must load a sample into index 0 at a minimum otherwise initialisation will fail. Each index corresponds to a channel (ie. left/right/other). If any indexes are left empty, that channel is loaded with the sample in index 0 at initialisation.
  * 
  * @param index The index to load this sample into. There is one index for each channel.
  * @param data A pointer to the sample data.
  * @param length The length of the sample data.
  */
 void setImpulse(int index, SampleType* data, unsigned int length)
 {
  dsp_assert(index >= 0 && index < Count);
  samples[index].set = data != nullptr;
  samples[index].pointerToSample = data;
  samples[index].length = length;
 }
 
 virtual void updateBufferSize(int bs) override
 {
  initialiseConvolution();
 }

 /**
  * @brief Returns whether the convolution engine is fully initialised.
  * 
  * When the engine is fully initialised then convolution happens, otherwise this component will feed its input straight into its output.
  * 
  * @return true If it is initialised.
  * @return false If it is not initialised.
  */
 bool isInitlialised() const { return initialised; }
 
 /**
  * @brief Set a hint for the FFT size to be used by the convolution engine.
  * 
  * The convolution engine uses FFT to speed up the convolution algorithm. Some systems may perform better with smaller or larger FFT size chunks. This method provides a way to set the FFT size in order to find the optimal size. Calling this will cause the engine to be reinitialised immediately.
  * 
  * @param hint A hint for the convolution engine about what might be the optimal size for the FFT. This can be any number, and the convolution engine might ignore the value. It is internally rounded to a power of 2.
  */
 void setFFTHint(unsigned int hint)
 {
  selectedFFTSize = hint;
  initialiseConvolution();
 }
 
 /**
  * @brief Return the current size of the FFT chunk being used by the convolution engine.
  * 
  * @return int The current size of the FFT chunk being used by the convolution engine. It may or may not be equal to the FFT hint provided.
  */
 int getFFTSize() const { return cp.deferredFFTSize(); }

 /**
  * @brief Prepare for convolution.
  * 
  * Call this method after setting all convolution parameters and loading the impulse response data. This must be called again if new impulse response data is loaded. It is automatically called whenever the buffer size or fft hints are changed.
  */
 void initialiseConvolution()
 {
  std::lock_guard<std::mutex> lock(mtx);
  cp.setParameters(dsp.bufferSize(), selectedFFTSize);
  
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
  
  for (auto &e: eng) e.initialise();
  
  initialised = true;
 }
 
 void stepProcess(int startPoint, int sampleCount) override
 {
  std::lock_guard<std::mutex> lock(mtx);
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
    eng[c].processSamples(c,
                          startPoint,
                          signalOut.buffer[c] + startPoint,
                          sampleCount);
   }
  }
 }
};

 
 
 
 
 
 
 
 
 
}

#endif /* FFT_h */
