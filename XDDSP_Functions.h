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










/*
 Some optimised and convenient math routines
 */


template <typename T>
inline T boundary(T x, T low, T high)
{
 return std::max(low, std::min(high, x));
}

template <typename T>
inline T clip(T x, T limit)
{
 return boundary(x, -limit, limit);
}

inline SampleType fastMax(SampleType a, SampleType b)
{
 a -= b;
 a += fabs(a);
 a *= 0.5;
 a += b;
 return a;
}

inline SampleType fastMin(SampleType a, SampleType b)
{
 a = b - a;
 a += fabs(a);
 a *= 0.5;
 a = b - a;
 return a;
}

inline SampleType fastBoundary(SampleType x, SampleType min, SampleType max)
{
 SampleType x1 = fabs(x - min);
 SampleType x2 = fabs(x - max);
 x = x1 + (min + max);
 x -= x2;
 x *= 0.5;
 return x;
}

inline SampleType fastClip(SampleType x, SampleType limit)
{
 return fastBoundary(x, -limit, limit);
}

inline SampleType linear2dB(SampleType l)
{
 return log(l) / 0.115129254649702;
}

inline SampleType dB2Linear(SampleType dB)
{
 return exp(dB * 0.115129254649702);
}

inline SampleType LERP(SampleType fracPos, SampleType x0, SampleType x1)
{
 // return x0 + (x1 - x0)*fracPos;
 return std::fma(x1 - x0, fracPos, x0);
}

inline SampleType hermite(SampleType fracPos, SampleType xm1, SampleType x0, SampleType x1, SampleType x2)
{
 const SampleType c = 0.5*(x1 - xm1);
 const SampleType v = x0 - x1;
 const SampleType w = c + v;
 const SampleType a = w + v + 0.5*(x2 - x0);
 const SampleType b_neg = w + a;
 
 return ((((a*fracPos) - b_neg)*fracPos + c)*fracPos + x0);
}

inline SampleType exponentialCurve(SampleType min,
                                 SampleType max,
                                 SampleType input,
                                 SampleType exp)
{
 return min + (max - min)*pow(input, exp);
}

inline SampleType exponentialDeltaCurve(SampleType min,
                                        SampleType delta,
                                        SampleType input,
                                        SampleType exp)
{
 return min + (delta)*pow(input, exp);
}


/*
 out = min + (max - min)*pow(input, exp)
 
 out - min
 --------- = pow(input, exp)
 (max - min)
 
 
      out - min    1
 pow(-----------, ---) = input
     (max - min)  exp
 
 */

inline SampleType inverseExponentialDeltaCurve(SampleType min,
                                               SampleType delta,
                                               SampleType output,
                                               SampleType exp)
{
 return pow((output - min)/delta, 1./exp);
}

/*
 inline SampleType fastexp5(SampleType x)
 {
 return (120.+x*(120.+x*(60.+x*(20.+x*(5.+x)))))*0.0083333333f;
 }
 */

inline SampleType expCoef(SampleType samples)
{
 return exp(-4.605170185988091 / samples);
}

template <typename T>
inline constexpr T signum(T x)
{
 return (static_cast<T>(0) < x) - (x < static_cast<T>(0));
}

inline void expTrack(SampleType &value, SampleType target, SampleType factor)
{
 // value = target + factor*(value - target);
 value = std::fma(value - target, factor, target);
}

constexpr int ABeforeMiddleC = 69;

inline SampleType semitoneRatio(SampleType st)
{
 constexpr SampleType oneOverTwelve = 1./12.;
 // return powf(1.059463094359295, st);
 return pow(2, st*oneOverTwelve);
}

inline int lowestBitSet(uint32_t word)
{
 static const int BitPositionLookup[32] = // hash table
 {
  0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8,
  31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
 };
 return BitPositionLookup[((uint32_t)((word & -word) * 0x077CB531U)) >> 27];
}










/*
 Some helper classes encapsulating commonly used programming patterns
 */




// TopBottom mode causes MinMax to not swap _min and _max if _min > _max
template <int TopBottom = 0>
class MinMax
{
 SampleType _min {0.};
 SampleType _max {1.};
 SampleType _delta {_max - _min};
 
public:
 MinMax() {}
 
 MinMax(SampleType min, SampleType max)
 {
  setMinMax(min, max);
 }
 
 MinMax(const MinMax& rhs) :
 _min(rhs._min),
 _max(rhs._max),
 _delta(rhs._delta)
 {}
 
 void setMinMax(SampleType min, SampleType max)
 {
  if ((TopBottom == 0) && (min > max)) std::swap(min, max);
  _min = min;
  _max = max;
  _delta = _max - _min;
 }
 
 void setMin(SampleType min)
 { setMinMax(min, _max); }
 
 void setMax(SampleType max)
 { setMinMax(_min, max); }
 
 SampleType min()
 { return _min; }
 
 SampleType max()
 { return _max; }
 
 SampleType delta()
 { return _delta; }
 
 SampleType fastBoundary(SampleType input)
 {
  if (TopBottom == 0) return ::XDDSP::fastBoundary(input, _min, _max);
  return (_min < _max) ?
  ::XDDSP::fastBoundary(input, _min, _max) :
  ::XDDSP::fastBoundary(input, _max, _min);
 }
 
 SampleType LERP(SampleType input)
 { return ::XDDSP::LERP(input, _min, _max); }
 
 SampleType normalise(SampleType input)
 { return (input - _min)/_delta; }
 
 SampleType expCurve(SampleType input, SampleType exponent)
 { return exponentialDeltaCurve(_min, _delta, input, exponent); }
 
 SampleType invCurve(SampleType input, SampleType exponent)
 { return inverseExponentialDeltaCurve(_min, _delta, input, exponent); }
};










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










class PowerSize
{
 uint32_t b;
 uint32_t s;
 uint32_t m;
public:
 constexpr PowerSize():
 b(8),
 s(1 << b),
 m(s - 1)
 {}
 
 constexpr PowerSize(uint32_t bits):
 b(bits),
 s(1 << bits),
 m(s - 1)
 {}
 
 constexpr uint32_t bits() const
 { return b; }
 
 constexpr uint32_t size() const
 { return s; }
 
 constexpr uint32_t mask() const
 { return m; }
 
 void setBits(int bits)
 {
  dsp_assert(bits >= 0 && bits < 31);
  b = bits;
  s = 1 << b;
  m = s - 1;
 }
 
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
 
 static PowerSize fromNextPowerTwo(uint32_t rs)
 {
  PowerSize r;
  r.setToNextPowerTwo(rs);
  return r;
 }
};










template <typename IntType = int>
class IntegerAndFraction
{
 SampleType iP;
 SampleType fP;
 IntType i;
public:
 constexpr IntegerAndFraction(SampleType whole) :
 iP(trunc(whole)),
 fP(whole - iP),
 i(static_cast<IntType>(iP))
 {
  dsp_assert(!isnan(whole));
 }
 
 constexpr SampleType intPart() const
 { return iP; }
 
 constexpr SampleType fracPart() const
 { return fP; }
 
 constexpr IntType intRep() const
 { return i; }
};










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
 
 SampleType calculateInflectionPoint()
 {
  return -b/(3.*a);
 }
 
 void getIntersectionValues(SampleType &frac, SampleType &slope)
 {
  frac = d;
  slope = mu;
 }
};










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
 MinMax<> boundaries;

 LookupTable()
 {
  table.fill(0.);
 }
  
 void calculateTable(WaveformFunction func)
 {
  for (int i = 0; i < Size + Padding(); ++i)
  {
   SampleType x = boundaries.LERP(static_cast<SampleType>(i - Offset())*RecSize);
   table[i] = func(x);
  }
 }
 
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
 
 SampleType operator()(SampleType x) { return lookup(x); }
};









}



#endif /* XDDSP_Functions_h */
