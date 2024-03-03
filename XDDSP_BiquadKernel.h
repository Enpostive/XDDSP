//
//  XDDSP_BiquadKernel.h
//  XDDSP
//
//  Created by Adam Jackson on 26/5/2022.
//

#ifndef XDDSP_BiquadKernel_h
#define XDDSP_BiquadKernel_h

#include "XDDSP_Parameters.h"
#include <complex>
#include <tuple>









namespace XDDSP
{










class BiquadFilterKernel;
class BiquadFilterPublicInterface;









class BiquadFilterCoefficients : public Parameters::ParameterListener
{
public:
 enum
 {
  LowPass,
  HighPass,
  BandPass,
  Notch,
  Parametric,
  LowShelf,
  HighShelf,
  AllPass,
  Custom
 };
 
private:
 friend class BiquadFilterKernel;
 
 Parameters &dspParam;
 
 int m {LowPass};
 SampleType cF {22000.};
 SampleType qF {0.7};
 SampleType g {0.};
 bool invert {false};
 bool cascade {false};
 
 double b0 {1.};
 double b1 {0.};
 double b2 {0.};
 double a1 {0.};
 double a2 {0.};
 
 void setCoefficients()
 {
  double w = fastBoundary(cF, 10., 22000.);
  w *= dspParam.sampleInterval();
  
  double Q = fastBoundary(qF, 0.1, 10.);
  Q = cascade ? sqrt(Q) : Q;

  double G = cascade ? sqrt(g) : g;
  
  double norm;
  
  if (m == AllPass)
  {
   double sinw {sin(2 * M_PI * w)};
   double cosw {cos(2 * M_PI * w)};
   double alpha {sinw / (2.0 * Q)};
   norm = 1.0 / (1.0 + alpha);
   a2 = b0 = (1.0 - alpha) * norm;
   b1 = a1 = -2.0 * cosw * norm;
   b2 = 1.0;
  }
  else
  {
   double K = tan(M_PI * w);
   
   switch (m)
   {
    default:
     m = LowPass;
     
    case LowPass:
     norm = 1.0 / (1.0 + K / Q + K * K);
     b0 = b2 = K * K * norm;
     b1 = 2 * b0;
     a1 = 2 * (K * K - 1.0) * norm;
     a2 = (1.0 - K / Q + K * K) * norm;
     break;
     
    case HighPass:
     norm = 1.0 / (1.0 + K / Q + K * K);
     b0 = b2 = norm;
     b1 = -2.0 * b0;
     a1 = 2.0 * (K * K - 1.0) * norm;
     a2 = (1.0 - K / Q + K * K) * norm;
     break;
     
    case BandPass:
     norm = 1.0 / (1.0 + K / Q + K * K);
     b0 = K / Q * norm;
     b1 = 0;
     b2 = -b0;
     a1 = 2.0 * (K * K - 1.0) * norm;
     a2 = (1.0 - K / Q + K * K) * norm;
     break;
     
    case Notch:
     norm = 1.0 / (1.0 + K / Q + K * K);
     b2 = b0 = (1.0 + K * K) * norm;
     b1 = 2.0 * (K * K - 1.0) * norm;
     a1 = b1;
     a2 = (1.0 - K / Q + K * K) * norm;
     break;
     
    case Parametric:
     if (invert)
     {
      norm = 1.0 / (1.0 + G/Q * K + K * K);
      b0 = (1.0 + 1.0/Q * K + K * K) * norm;
      b1 = 2.0 * (K * K - 1.0) * norm;
      b2 = (1.0 - 1.0/Q * K + K * K) * norm;
      a1 = b1;
      a2 = (1.0 - G/Q * K + K * K) * norm;
     }
     else
     {
      norm = 1.0 / (1.0 + 1.0/Q * K + K * K);
      b0 = (1.0 + G/Q * K + K * K) * norm;
      b1 = 2.0 * (K * K - 1.0) * norm;
      b2 = (1 - G/Q * K + K * K) * norm;
      a1 = b1;
      a2 = (1.0 - 1.0/Q * K + K * K) * norm;
     }
     break;
     
    case LowShelf:
     if (invert)
     {
      norm = 1.0 / (1.0 + sqrt(2.0*G) * K + G * K * K);
      b0 = (1.0 + 1.414213562373095 * K + K * K) * norm;
      b1 = 2.0 * (K * K - 1.0) * norm;
      b2 = (1.0 - 1.414213562373095 * K + K * K) * norm;
      a1 = 2.0 * (G * K * K - 1.0) * norm;
      a2 = (1.0 - sqrt(2.0*G) * K + G * K * K) * norm;
     }
     else
     {
      norm = 1.0 / (1.0 + 1.414213562373095 * K + K * K);
      b0 = (1.0 + sqrt(2.0*G) * K + G * K * K) * norm;
      b1 = 2.0 * (G * K * K - 1.0) * norm;
      b2 = (1.0 - sqrt(2.0*G) * K + G * K * K) * norm;
      a1 = 2.0 * (K * K - 1.0) * norm;
     }
     break;
     
    case HighShelf:
     if (invert)
     {
      norm = 1.0 / (G + sqrt(2.0 * G) * K + K * K);
      b0 = (1.0 + 1.414213562373095 * K + K * K) * norm;
      b1 = 2.0 * (K * K - 1.0) * norm;
      b2 = (1.0 - 1.414213562373095 * K + K * K) * norm;
      a1 = 2.0 * (K * K - G) * norm;
      a2 = (G - sqrt(2.0 * G) * K + K * K) * norm;
     }
     else
     {
      norm = 1.0 / (1.0 + 1.414213562373095 * K + K * K);
      b0 = (G + sqrt(2.0 * G) * K + K * K) * norm;
      b1 = 2.0 * (K * K - G) * norm;
      b2 = (G - sqrt(2.0 * G) * K + K * K) * norm;
      a1 = 2.0 * (K * K - 1.0) * norm;
      a2 = (1 - 1.414213562373095 * K + K * K) * norm;
     }
     break;
     
    case Custom:
     break;
   }
  }
 }
 
 void calculateGain(SampleType gain)
 {
  g = powf(10.0, fabs(gain) / 20.0);
  invert = gain < 0.0;
 }
 
public:
 BiquadFilterCoefficients(Parameters &p) :
 Parameters::ParameterListener(p),
 dspParam(p)
 {
  updateSampleRate(p.sampleRate(), p.sampleInterval());
 }

 virtual void updateSampleRate(double sr, double isr) override
 {
  setCoefficients();
 }
 
 void setPassingFilterParameters(SampleType freq, SampleType q)
 {
  cF = freq;
  qF = q;
  setCoefficients();
 }
 
 void setLowPassFilter(SampleType freq, SampleType q)
 {
  m = LowPass;
  setPassingFilterParameters(freq, q);
 }
 
 void setHighPassFilter(SampleType freq, SampleType q)
 {
  m = HighPass;
  setPassingFilterParameters(freq, q);
 }
 
 void setBandPassFilter(SampleType freq, SampleType q)
 {
  m = BandPass;
  setPassingFilterParameters(freq, q);
 }
 
 void setNotchFilter(SampleType freq, SampleType q)
 {
  m = Notch;
  setPassingFilterParameters(freq, q);
 }
 
 void setAllPassFilter(SampleType freq, SampleType q)
 {
  m = AllPass;
  setPassingFilterParameters(freq, q);
 }
 
 void setAllFilterParams(SampleType freq,
                         SampleType q,
                         SampleType gain)
 {
  calculateGain(gain);
  setPassingFilterParameters(freq, q);
 }
 
 void setParametricFilter(SampleType freq,
                          SampleType q,
                          SampleType gain)
 {
  m = Parametric;
  setAllFilterParams(freq, q, gain);
 }
 
 void setShelvingFilter(SampleType freq,
                        SampleType q,
                        SampleType gain,
                        bool highShelf)
 {
  m = highShelf ? HighShelf : LowShelf;
  calculateGain(gain);
  setAllFilterParams(freq, q, gain);
 }
 
 void setFilterMode(int mode)
 {
  m = mode;
  setCoefficients();
 }
 
 void setFrequency(SampleType freq)
 {
  cF = freq;
  setCoefficients();
 }
 
 void setQFactor(SampleType q)
 {
  qF = q;
  setCoefficients();
 }
 
 void setGain(SampleType g)
 {
  calculateGain(g);
  setCoefficients();
 }
 
 void setCascade(bool casc)
 {
  cascade = casc;
  setCoefficients();
 }
 
 void setCustomFilter(double _b0,
                      double _b1,
                      double _b2,
                      double _a1,
                      double _a2)
 {
  m = Custom;
  b0 = _b0;
  b1 = _b1;
  b2 = _b2;
  a1 = _a1;
  a2 = _a2;
 }
 
 std::complex<double> filterResponseAtHz(SampleType hz)
 {
  const double w = 2.*M_PI*hz*dspParam.sampleInterval();
  const std::complex<double> mi{0, -1};
  const std::complex<double> exp1 = std::exp(mi*w);
  const std::complex<double> exp2 = std::exp(2.*mi*w);
  const std::complex<double> numerator = b0 + b1*exp1 + b2*exp2;
  const std::complex<double> denominator = 1. + a1*exp1 + a2*exp2;
  const std::complex<double> result = numerator/denominator;
  if (cascade) return result*result;
  return result;
 }
 
 SampleType calculateMagnitudeResponseAtHz(SampleType hz)
 {
  return std::abs(filterResponseAtHz(hz));
 }
};










class BiquadFilterKernel
{
 SampleType d1;
 SampleType d2;
 SampleType d3;
 SampleType d4;
public:
 void reset()
 {
  d1 = d2 = d3 = d4 = 0.;
 }
 
 SampleType process(const BiquadFilterCoefficients &coeff, SampleType xn)
 {
  SampleType s, t;
  if (coeff.cascade)
  {
   /*
   s = coeff.b0 * xn + d1;
   d1 = coeff.b1 * xn - coeff.a1 * s + d2;
   d2 = coeff.b2 * xn - coeff.a2 * s;
   
   t = coeff.b0 * s + d3;
   d3 = coeff.b1 * s - coeff.a1 * t + d4;
   d4 = coeff.b2 * s - coeff.a2 * t;
   */
   s = std::fma(coeff.b0, xn, d1);
   d1 = std::fma(coeff.b1, xn, std::fma(-coeff.a1, s, d2));
   d2 = std::fma(coeff.b2, xn, -coeff.a2*s);
   
   t = std::fma(coeff.b0, s, d3);
   d3 = std::fma(coeff.b1, s, std::fma(-coeff.a1, t, d4));
   d4 = std::fma(coeff.b2, s, -coeff.a2*t);
  }
  else
  {
   /*
   t = coeff.b0 * xn + d1;
   d1 = coeff.b1 * xn - coeff.a1 * t + d2;
   d2 = coeff.b2 * xn - coeff.a2 * t;
    */
   t = std::fma(coeff.b0, xn, d1);
   d1 = std::fma(coeff.b1, xn, std::fma(-coeff.a1, t, d2));
   d2 = std::fma(coeff.b2, xn, -coeff.a2*t);
  }
  return t;
 }
};










class BiquadFilterPublicInterface
{
 BiquadFilterCoefficients &coeff;
public:
 BiquadFilterPublicInterface(BiquadFilterCoefficients &_coeff) :
 coeff(_coeff)
 {}
 
 std::complex<double> filterResponseAtHz(SampleType hz)
 {
  return coeff.filterResponseAtHz(hz);
 }
 
 SampleType calculateMagnitudeResponseAtHz(SampleType hz)
 {
  return coeff.calculateMagnitudeResponseAtHz(hz);
 }
};










}


#endif /* XDDSP_BiquadKernel_h */
