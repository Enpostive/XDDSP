//
//  XDDSP_Functions.h
//  XDDSP
//
//  Created by Adam Jackson on 25/5/2022.
//

#ifndef XDDSP_Functions_h
#define XDDSP_Functions_h

#include "XDDSP_Types.h"










namespace XDDSP
{










/**
 * @brief Clamp a number between a lower and a higher boundary
 * 
 * If high and low are reversed, this function will always return the value given in low.
 * 
 * @tparam T The data type, inferred from the parameters.
 * @param x The value to clamp.
 * @param low The lower boundary value.
 * @param high The higher boundary value.
 * @return T The clamped value.
 */
template <typename T>
inline T boundary(T x, T low, T high)
{
 return std::max(low, std::min(high, x));
}

/**
 * @brief Constrain a number to a maximum magnitude.
 * 
 * @tparam T The data type, inferred from the parameters.
 * @param x The number to constrain.
 * @param limit The maximum magnitude. Must be positive.
 * @return T The constrained value.
 */
template <typename T>
inline T clip(T x, T limit)
{
 return boundary(x, -limit, limit);
}

/**
 * @brief A placeholder method to encapsulate the fastest possible method of computing the maximum between two samples.
 * 
 * Currently uses std::max
 * 
 * @param a A value to compare.
 * @param b A value to compare.
 * @return SampleType The larger value.
 */
inline SampleType fastMax(SampleType a, SampleType b)
{
/* a -= b;
 a += fabs(a);
 a *= 0.5;
 a += b;
 return a;*/
 return std::max(a, b);
}

/**
 * @brief A placeholder method to encapsulate the fastest possible method of computing the smaller of two samples.
 * 
 * Currently uses std::min
 * 
 * @param a A value to compare.
 * @param b A value to compare.
 * @return SampleType The smaller value.
 */
inline SampleType fastMin(SampleType a, SampleType b)
{
/* a = b - a;
 a += fabs(a);
 a *= 0.5;
 a = b - a;
 return a;*/
 return std::min(a, b);
}

/**
 * @brief A placeholder method to encapsulate the fastest possible method of clamping a sample between between two samples.
 * 
 * Currently uses std::max and std::min. If min and max are reversed, this function will always return the value given in min.
 * 
 * @param x A value to compare.
 * @param min The lower boundary value.
 * @param max The higher boundary value.
 * @return SampleType The larger value.
 */
inline SampleType fastBoundary(SampleType x, SampleType min, SampleType max)
{
/* SampleType x1 = fabs(x - min);
 SampleType x2 = fabs(x - max);
 x = x1 + (min + max);
 x -= x2;
 x *= 0.5;
 return x;*/
 return boundary(x, min, max);
}

/**
 * @brief A placeholder method to encapsulate the fastest possible method of clipping a sample to a maximum magnitude.
 * 
 * Currently uses std::max and std::min.

 * @param x The number to constrain.
 * @param limit The maximum magnitude. Must be positive.
 * @return T The constrained value.
 */
nline SampleType fastClip(SampleType x, SampleType limit)
{
 return fastBoundary(x, -limit, limit);
}

/**
 * @brief Convert a linear sample (measurement, gain etc.) to a dB sample.
 * 
 * @param l The linear sample to convert.
 * @return SampleType The converted sample.
 */
inline SampleType linear2dB(SampleType l)
{
 return log(l) / 0.115129254649702;
}

/**
 * @brief Convert a dB sample (measurement, gain etc.) to a linear sample.
 * 
 * @param dB The sample in decibels to convert.
 * @return SampleType The converted sample.
 */
inline SampleType dB2Linear(SampleType dB)
{
 return exp(dB * 0.115129254649702);
}

/**
 * @brief Perform a linear interpolation between two samples.
 * 
 * @param fracPos The amount to interpolate between 0 and 1.
 * @param x0 Sample at position 0.
 * @param x1 Sample at position 1.
 * @return SampleType The interpolated sample.
 */
inline SampleType LERP(SampleType fracPos, SampleType x0, SampleType x1)
{
 // return x0 + (x1 - x0)*fracPos;
 return std::fma(x1 - x0, fracPos, x0);
}

/**
 * @brief Hermite interpolation
 * 
 * Performs a cubic interpolation of a curve described by 4 equidistant samples.
 * 
 * Thank you to Laurent de Soras.
 * 
 * Taken from https://www.musicdsp.org/en/latest/Other/93-hermite-interpollation.html
 * 
 * @param fracPos The amount to interpolate between 0 and 1.
 * @param xm1 Sample at position -1.
 * @param x0 Sample at position 0.
 * @param x1 Sample at position 1.
 * @param x2 Sample at position 2.
 * @return SampleType The interpolated Sample.
 */
inline SampleType hermite(SampleType fracPos, SampleType xm1, SampleType x0, SampleType x1, SampleType x2)
{
 const SampleType c = 0.5*(x1 - xm1);
 const SampleType v = x0 - x1;
 const SampleType w = c + v;
 const SampleType a = w + v + 0.5*(x2 - x0);
 const SampleType b_neg = w + a;
 
 return ((((a*fracPos) - b_neg)*fracPos + c)*fracPos + x0);
}

/**
 * @brief Maps a linear control to an exponential curve between a minimum and a maximum.
 * 
 * @param min The smallest allowed output.
 * @param max The largest allowed output.
 * @param input The input between 0 and 1.
 * @param exp The curve factor. Values larger than 0 curve up early, values at 0 are linear and values smaller than 0 curve up late.
 * @return SampleType The curve mapped value.
 */
inline SampleType exponentialCurve(SampleType min,
                                 SampleType max,
                                 SampleType input,
                                 SampleType exp)
{
 return min + (max - min)*pow(input, exp);
}

/**
 * @brief Maps a linear control to an exponential curve described by a minimum and a delta value.
 * 
 * @param min The smallest allowed output.
 * @param delta The distance fromt he smallest output to the largest output.
 * @param input The input between 0 and 1.
 * @param exp The curve factor. Values larger than 0 curve up early, values at 0 are linear and values smaller than 0 curve up late.
 * @return SampleType The curve mapped value.
 */
inline SampleType exponentialDeltaCurve(SampleType min,
                                        SampleType delta,
                                        SampleType input,
                                        SampleType exp)
{
 return min + (delta)*pow(input, exp);
}


/**
 * @brief Maps an exponential curve value back to a linear value.
 * 
 * @param min The smallest allowed output.
 * @param delta The distance fromt he smallest output to the largest output.
 * @param output The value on the curve between the minimum and the maximum.
 * @param exp The curve factor. Values larger than 0 curve up early, values at 0 are linear and values smaller than 0 curve up late.
 * @return SampleType A linear value between 0 and 1.
 */
inline SampleType inverseExponentialDeltaCurve(SampleType min,
                                               SampleType delta,
                                               SampleType output,
                                               SampleType exp)
{
 return pow((output - min)/delta, 1./exp);
}

/**
 * @brief Calculate an exponential decay coefficient that drops to 1% within a certain number of samples.
 * 
 * @param samples The number of samples to decay by.
 * @return SampleType The resulting coefficient.
 */
inline SampleType expCoef(SampleType samples)
{
 return exp(-4.605170185988091 / samples);
}

/**
 * @brief A method to calulcate the sign of any number type.
 * 
 * @tparam T The number type.
 * @param x The value to test.
 * @return constexpr T -1, 0 or 1, depending on the sign of x
 */
template <typename T>
inline constexpr T signum(T x)
{
 return (static_cast<T>(0) < x) - (x < static_cast<T>(0));
}

/**
 * @brief A quick method which takes a variable passed by reference and decays it towards the target at the given factor.
 * 
 * @param value A variable passed by reference to decay.
 * @param target The target value for the variable.
 * @param factor The decay coefficient, probably calculated with XDDSP::expCoef.
 */
inline void expTrack(SampleType &value, SampleType target, SampleType factor)
{
 value = std::fma(value - target, factor, target);
}

constexpr int ABeforeMiddleC = 69;

/**
 * @brief Calculate the ratio between two equal tempered notes.
 * 
 * @param st The difference betweent the notes in semitones.
 * @return SampleType The ratio between the notes.
 */
inline SampleType semitoneRatio(SampleType st)
{
 constexpr SampleType oneOverTwelve = 1./12.;
 return pow(2, st*oneOverTwelve);
}

/**
 * @brief Return the index of the lowest bit set in a 32-bit unsigned integer.
 * 
 * @param word The 32-bit unsigned integer to check.
 * @return int The index of the lowest bit set. Returns 0 for no bits set. 1 for the lowest significant bit and 32 for the highest significant bit.
 */
inline int lowestBitSet(uint32_t word)
{
 static const int BitPositionLookup[32] = // hash table
 {
  0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8,
  31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
 };
 return BitPositionLookup[((uint32_t)((word & -word) * 0x077CB531U)) >> 27];
}








// Some static function definitions that are quite handy

int recursiveBinarySearch(int start, int end, std::function<bool (int)> f);










/**
 * @brief A class encapsulating some common min-max functionality.
 * 
 * This class contains some common functions that come in handy where a signal, parameter or control has a minimum and a maximum value.
 * 
 * This class has two different modes: Min-Max mode and Top-Bottom mode.
 * 
 * In Min-Max mode, the minimum and maximum value only describe a region where a signal is valid. If they are put in the wrong order, this class will swap them around.
 * 
 * In Top-Bottom mode, the minimum and maximum value describe a literal value at the top and bottom of a control range, allowing the range to be inverted if required. An example of this might be the radius of a turn, where higher numbers result in slower turns, so it might make sence to put higher numbers at the bottom of the control range.
 * 
 * @tparam TopBottom Anything non-zero enables Top-Bottom mode.
 */
template <int TopBottom = 0>
class MinMax
{
 SampleType _min {0.};
 SampleType _max {1.};
 SampleType _delta {_max - _min};
 
public:
 /**
  * @brief Construct a new default Min Max object.
  * 
  * The default settings for minimin and maximum are 0 and 1 respectively.
  */
 MinMax() {}
 
 /**
  * @brief Construct a new Min Max object.
  * 
  * @param min The minimum value.
  * @param max The maximum value.
  */
 MinMax(SampleType min, SampleType max)
 {
  setMinMax(min, max);
 }
 
 /**
  * @brief Construct a copy of another Min Max object.
  * 
  * @param rhs The other object to copy.
  */
 MinMax(const MinMax& rhs) :
 _min(rhs._min),
 _max(rhs._max),
 _delta(rhs._delta)
 {}
 
 /**
  * @brief Set the minimum and maximum values.
  * 
  * @param min The minimum value.
  * @param max The maximum value.
  */
 void setMinMax(SampleType min, SampleType max)
 {
  if ((TopBottom == 0) && (min > max)) std::swap(min, max);
  _min = min;
  _max = max;
  _delta = _max - _min;
 }
 
 /**
  * @brief Set just the minimum value.
  * 
  * @param min The minimum value.
  */
 void setMin(SampleType min)
 { setMinMax(min, _max); }
 
 /**
  * @brief Set just the maximum value.
  * 
  * @param max The maximum value.
  */
 void setMax(SampleType max)
 { setMinMax(_min, max); }
 
 /**
  * @brief Get the minimum value.
  * 
  * @return SampleType The minimum value.
  */
 SampleType min()
 { return _min; }
 
 /**
  * @brief Get the maximum value.
  * 
  * @return SampleType The maximum value.
  */
 SampleType max()
 { return _max; }
 
 /**
  * @brief Get the difference between minimum and maximum.
  * 
  * In Top-Bottom mode, this number is allowed to be negative.
  * 
  * @return SampleType The current delta between minimum and maximum.
  */
 SampleType delta()
 { return _delta; }
 
 /**
  * @brief Clamp a value between minimum and maximum values.
  * 
  * Regardless of what mode it is in, the object will always correctly clamp between the smallest and the largest values in the object.
  * 
  * @param input Some value to clamp.
  * @return SampleType The clamped value.
  */
 SampleType fastBoundary(SampleType input)
 {
  if (TopBottom == 0) return ::XDDSP::fastBoundary(input, _min, _max);
  return (_min < _max) ?
  ::XDDSP::fastBoundary(input, _min, _max) :
  ::XDDSP::fastBoundary(input, _max, _min);
 }
 
 /**
  * @brief Perform a linear interpolation between the minimum and the maximum.
  * 
  * @param input A fraction between 0 and 1.
  * @return SampleType The value interpolated between minimum and maximum.
  */
 SampleType LERP(SampleType input)
 { return ::XDDSP::LERP(input, _min, _max); }
 
 /**
  * @brief Perform the opposite of an interpolation.
  * 
  * @param input A value between minimum and maximum.
  * @return SampleType A normalised value between 0 and 1.
  */
 SampleType normalise(SampleType input)
 { return (input - _min)/_delta; }
 
 /**
  * @brief Interpolate between minimum and maximum, with a bias curve described by the exponent.
  * 
  * @param input A fraction between 0 and 1.
  * @param exponent The curve factor. Values larger than 0 curve up early, values at 0 are linear and values smaller than 0 curve up late.
  * @return SampleType A value between minimum and maximum corresponding to the input and curve.
  */
 SampleType expCurve(SampleType input, SampleType exponent)
 { return exponentialDeltaCurve(_min, _delta, input, exponent); }
 
 /**
  * @brief Perform the inverse operation of expCurve.
  * 
  * @param input A value between minimum and maximum.
  * @param exponent The curve factor. Values larger than 0 curve up early, values at 0 are linear and values smaller than 0 curve up late.
  * @return SampleType A value between 0 and 1 such that, when entered into expCurve, returns the same value as the input.
  */
 SampleType invCurve(SampleType input, SampleType exponent)
 { return inverseExponentialDeltaCurve(_min, _delta, input, exponent); }
};









/**
 * @brief To be deprecated in favour of adding this functionality to MinMax
 * 
 */
class LogarithmicScale
{
 SampleType _min {0.};
 SampleType _max {1.};
 SampleType _delta {_max - _min};
 
public:
 LogarithmicScale()
 {}
 
 LogarithmicScale(SampleType min, SampleType max) :
 _min(std::log10(min)),
 _max(std::log10(max)),
 _delta(_max - _min)
 {}
 
 LogarithmicScale(const LogarithmicScale &rhs) :
 _min(rhs._min),
 _max(rhs._max),
 _delta(rhs._delta)
 {}
 
 SampleType getPlotRatio(SampleType x)
 { return (std::log10(x) - _min)/_delta; }
 
 SampleType pickPoint(SampleType x)
 { return pow(10., LERP(x, _min, _max)); }
};










/**
 * @brief A class containing commonly used functionality in relation to powers of two
 * 
 * Unless otherwise noted, all the defaults referred to in the documentation for this class refer to the default construction with 8 bits.
 * 
 * It has constexpr methods which allow it to be used inside template arguments in some cases.
 */
class PowerSize
{
 uint32_t b;
 uint32_t s;
 uint32_t m;
public:
/**
 * @brief Construct a default PowerSize object using 8 bits.
 * 
 */
 constexpr PowerSize():
 b(8),
 s(1 << b),
 m(s - 1)
 {}
 
 /**
  * @brief Construct a PowerSize object with a number of bits between 0 and 32.
  * 
  */
 constexpr PowerSize(uint32_t bits):
 b(bits),
 s(1 << bits),
 m(s - 1)
 {}
 
 /**
  * @brief Returns the number of bits the object was constructed with.
  * 
  * Default = 8.
  * 
  * @return uint32_t The number of bits the object was constructed with.
  */
 constexpr uint32_t bits() const
 { return b; }
 
 /**
  * @brief Returns the power of two corresponding to the number of bits.
  * 
  * Default = 256.
  * 
  * @return uint32_t The power of two corresponding to the number of bits.
  */
 constexpr uint32_t size() const
 { return s; }
 
 /**
  * @brief Returns a number with all the bits set to 1, useful for masking.
  * 
  * Default = 255.
  * 
  * @return uint32_t A number with all the bits set to 1, useful for masking.
  */
 constexpr uint32_t mask() const
 { return m; }
 
 /**
  * @brief Reset the object with a different number of bits.
  * 
  * @param bits Number of bits to use.
  */
 void setBits(int bits)
 {
  dsp_assert(bits >= 0 && bits < 33);
  b = bits;
  s = 1 << b;
  m = s - 1;
 }
 
 /**
  * @brief Reset the object using the next power of two higher than the target value.
  * 
  * @param rs The target value.
  */
 void setToNextPowerTwo(uint32_t rs)
 {
  static const int MultiplyDeBruijnBitPosition[32] =
  {
   0, 9, 1, 10, 13, 21, 2, 29, 11, 14, 16, 18, 22, 25, 3, 30,
   8, 12, 20, 28, 15, 17, 24, 7, 19, 27, 23, 6, 26, 5, 4, 31
  };

  m = nextPowerTwoMinusOne(rs);
  b = MultiplyDeBruijnBitPosition[(uint32_t)(m * 0x07C4ACDDU) >> 27];
  s = m + 1;
 }
 
 /**
  * @brief Make a mask by finding the next power of two higher than the target value and subtracting one.
  * 
  * @param rs The target value.
  * @return uint32_t The next power of two subtract one.
  */
 static uint32_t nextPowerTwoMinusOne(uint32_t rs)
 {
  rs = rs - 1;
  rs |= rs >> 1;
  rs |= rs >> 2;
  rs |= rs >> 4;
  rs |= rs >> 8;
  rs |= rs >> 16;
  return rs;
 }
 
 /**
  * @brief Construct and return a PowerSize object using the PowerSize::setToNextPowerTwo setter method on a new object.
  * 
  * @param rs A target value.
  * @return PowerSize A PowerSize object made with enough bits to fit the target value.
  */
 static PowerSize fromNextPowerTwo(uint32_t rs)
 {
  PowerSize r;
  r.setToNextPowerTwo(rs);
  return r;
 }
};










/**
 * @brief A class encapsulating the best algorithm for splitting a sample into its integer and fraction components.
 * 
 * @tparam IntType The integer type used by the class.
 */
template <typename IntType = int>
class IntegerAndFraction
{
 SampleType iP;
 SampleType fP;
 IntType i;
public:
 /**
  * @brief Construct the IntegerAndFraction object and calculate the integer and fraction parts of the provided sample.
  * 
  */
 constexpr IntegerAndFraction(SampleType whole) :
 iP(trunc(whole)),
 fP(whole - iP),
 i(static_cast<IntType>(iP))
 {
  dsp_assert(!isnan(whole));
 }
 
 /**
  * @brief Return the integer part as a floating point type.
  * 
  * @return SampleType The integer part.
  */
 constexpr SampleType intPart() const
 { return iP; }
 
 /**
  * @brief Return the fraction part.
  * 
  * @return SampleType The fraction part.
  */
 constexpr SampleType fracPart() const
 { return fP; }
 
 /**
  * @brief Return the integer part in an integer form.
  * 
  * @return IntType The integer part.
  */
 constexpr IntType intRep() const
 { return i; }
};










// TODO: Document LinearEstimator or deprecate it.
class LinearEstimator
{
 SampleType s1;
 SampleType s2;
 SampleType d1;
 SampleType d2;
 SampleType direction;
 
public:
 constexpr LinearEstimator(SampleType x0, SampleType x1, SampleType t = 0.) :
 s1(t - x0),
 s2(x1 - t),
 d1(signum(s1)),
 d2(signum(s2)),
 direction(signum(d1 + d2))
 {}
 
 constexpr SampleType x() const
 { return (s1 + s2 == 0.) ? 0. : s1/(s1 + s2); }
 
 constexpr bool isIntersection() const
 { return direction != 0.; }
 
 constexpr SampleType intersectionDirection() const
 { return direction; }
};










/**
 * @brief A class for estimating the intersection of a cubic waveform and flat line fixed at some value.
 * 
 */
class IntersectionEstimator
{
 static constexpr double c1_6 = 1./6.;
 static constexpr double c3_2 = 3./2.;
 static constexpr double c5_2 = 5./2.;
 static constexpr double c1_3 = 1./3.;
 static constexpr double c11_6 = 11./6.;
 
 double a, b, c, d, e, mu;
 
 double f(double x)
 {
  double xx = x * x;
  return a*xx*x + b*xx + c*x + e;
 }
 
public:
 /**
  * @brief Set the samples that define the cubic waveform. 
  * 
  * The object is expecting an intersection to lie between the samples xm1 and x1. No bounds checking or sanity checking is performed.
  * 
  * @param xm2 
  * @param xm1 
  * @param x1 
  * @param x2 
  */
 void setSampleValues(SampleType xm2,
                      SampleType xm1,
                      SampleType x1,
                      SampleType x2)
 {
  a = -c1_6*xm2 + 0.5*xm1 - 0.5*x1 + c1_6*x2;
  b = xm2 - c5_2*xm1 + 2*x1 - 0.5*x2;
  c = -c11_6*xm2 + 3*xm1 - c3_2*x1 + c1_3*x2;
  e = xm2;
 }
 
 /**
  * @brief Do the actual intersection estimation.
  * 
  * @param p The constant where we want to solve for the intersection.
  * @param epsilon The allowable error in the estimation.
  */
 void estimateIntersection(double p, double epsilon = 0.001)
 {
  double _a = 1.;
  double _b = 2.;
  double fa = f(_a) - p;
  double fb = f(_b) - p;
  double _c = 1.5;
  double fc;
  
  if ((fa < 0. && fb > 0.) || (fa > 0. && fb < 0.))
  {
   for (int i = 0; i < 8; ++i)
   {
    _c = 0.5*(_a+_b);
    fc = f(_c) - p;
    if (std::signbit(fa) == std::signbit(fc))
    {
     _a = _c;
     fa = fc;
    }
    else
    {
     _b = _c;
    }
   }
  }
  
  
  double dd = _c;
  
  double err = (_b - _a) / 2.;
  double ddd;
  int looplimit = 4;
  
  while (looplimit && fabs(err) > epsilon && f(dd) != 0.)
  {
   --looplimit;
   mu = 3.*a*dd*dd + 2.*b*dd + c;
   if (mu != 0) err = (f(dd) - p)/mu;
   else err = 0.;
   ddd = dd + err;
   //   ddd = dd - (a*dd*dd*dd + b*dd*dd + c*dd + e - p)/mu;
   dd = ddd;
  }
  
  if (dd <= 1. || dd >= 2.)
  {
   dd = 1.5;
  }
  
  d = dd - 1.;
 }
 
 // TODO: Document calculateStationaryPoints or deprecate it.
 bool calculateStationaryPoints(SampleType &minimum, SampleType &maximum)
 {
  const double dis = b*b - 3.*a*c;
  if (dis < 0.) return false;
  
  const double quadVertex = calculateInflectionPoint();
  if (dis == 0.)
  {
   minimum = quadVertex;
   maximum = quadVertex;
   return true;
  }
  
  const double sqrtDis = std::sqrt(dis)/(3.*a);
  minimum = quadVertex - sqrtDis;
  maximum = quadVertex + sqrtDis;
  
  if (f(minimum) > f(maximum)) std::swap(minimum, maximum);
  return true;
 }
 
 // TODO: Document calculateInflectionPoint or deprecate it.
 SampleType calculateInflectionPoint()
 {
  return -b/(3.*a);
 }
 
 /**
  * @brief Get the location and slote of the estimated intersection.
  * 
  * @param frac A variable passed by reference to return the distance between xm1 and x1 of the intersection.
  * @param slope The slope of the cubic function at the intersection.
  */
 void getIntersectionValues(SampleType &frac, SampleType &slope)
 {
  frac = d;
  slope = mu;
 }
};










/**
 * @brief A callable object which creates a lookup table from a function.
 * 
 * This class exposes a MinMax object to describe the boundaries where the lookup table is calculated.
 * 
 * To use a LookupTable object: First use the default constructor, then set the minimum and maximum in the LookupTable::boundaries property, then use LookupTable::calculateTable to populate the lookup table with values.
 * 
 * @tparam Size The size of the table. Depending on the quality mode, padding may be added to the table.
 * @tparam Quality The quality of the interpolation performed. See XDDSP::ProcessQuality
 */
template <int Size, int Quality = ProcessQuality::MidQuality>
class LookupTable
{
 static_assert(Quality == ProcessQuality::LowQuality ||
               Quality == ProcessQuality::MidQuality ||
               Quality == ProcessQuality::HighQuality,
               "Invalid quality specifier");

 static constexpr int Padding()
 {
  return
  (Quality == ProcessQuality::LowQuality) ? 1 :
  ((Quality == ProcessQuality::MidQuality) ? 2 : 4);
 }
 
 static constexpr int Offset()
 {
  return
  (Quality == ProcessQuality::LowQuality) ? 0 :
  ((Quality == ProcessQuality::MidQuality) ? 0 : 1);
 }

 static constexpr SampleType RecSize = 1./Size;
 
 std::array<SampleType, Size + Padding()> table;
 
public:
 /**
  * @brief Set the boundaries here before calculating the table.
  */
 MinMax<> boundaries;

 /**
  * @brief Construct a new LookupTable object
  * 
  */
 LookupTable()
 {
  table.fill(0.);
 }
  
 /**
  * @brief Call func repeatedly and populate the lookup table with the values.
  * 
  * **Before calling this, make sure to set the minimum and maximum values in the LookupTable::boundaries property**
  * 
  * Depending on the quality setting used, there may be padding added to the lookup table on either end, so func should expect inputs that are actually lower than the minimum or higher than the maximum. The func object may clamp the input value to the allowable range or, if the table is expected to be a loop of some sort, wrap the input around to the output.
  * 
  * @param func A callable object to provide values for the lookup table.
  */
 void calculateTable(WaveformFunction func)
 {
  for (int i = 0; i < Size + Padding(); ++i)
  {
   SampleType x = boundaries.LERP(static_cast<SampleType>(i - Offset())*RecSize);
   table[i] = func(x);
  }
 }
 
 /**
  * @brief Perform a table lookup.
  * 
  * @param x The input to lookup.
  * @return SampleType The interpolated output value.
  */
 SampleType lookup(SampleType x)
 {
  SampleType y, ym1, y0, y1, y2;
  
  x = boundaries.fastBoundary(x);
  x = boundaries.normalise(x)*Size;
  IntegerAndFraction xIF(x);
  int i0 = xIF.intRep() + Offset();
  
  switch(Quality)
  {
   case ProcessQuality::LowQuality:
    y = table[i0];
    break;
    
   case ProcessQuality::MidQuality:
    y0 = table[i0];
    y1 = table[i0 + 1];
    y = LERP(xIF.fracPart(), y0, y1);
    break;
    
   case ProcessQuality::HighQuality:
    ym1 = table[i0 - 1];
    y0 = table[i0];
    y1 = table[i0 + 1];
    y2 = table[i0 + 2];
    y = hermite(xIF.fracPart(), ym1, y0, y1, y2);
    break;
  }
  
  return y;
 }
 
 /**
  * @brief An alias for lookup.
  * 
  * @param x The input to lookup.
  * @return SampleType The interpolated output value.
  */
 SampleType operator()(SampleType x) { return lookup(x); }
};









}



#endif /* XDDSP_Functions_h */
