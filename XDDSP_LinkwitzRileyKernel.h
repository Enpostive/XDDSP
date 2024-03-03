//
//  XDDSP_LinkwitzRileyKernel.h
//  XDDSP
//
//  Created by Adam Jackson on 26/5/2022.
//

#ifndef XDDSP_LinkwitzRileyKernel_h
#define XDDSP_LinkwitzRileyKernel_h










namespace XDDSP
{










class LinkwitzRileyFilterKernel;









class LinkwitzRileyFilterCoefficients : public Parameters::ParameterListener
{
 friend class LinkwitzRileyFilterKernel;
 
 Parameters &dspParam;

 double la0 {0.5};
 double la1 {0.};
 double la2 {0.};
 double la3 {0.};
 double la4 {0.};
 double ha0 {0.5};
 double ha1 {0.};
 double ha2 {0.};
 double ha3 {0.};
 double ha4 {0.};
 double b1 {0.};
 double b2 {0.};
 double b3 {0.};
 double b4 {0.};
 
 double fc {2000.};
 
 void setCoeff()
 {
  double srate = dspParam.sampleRate();
  
  double wc = 2.*M_PI*fc;
  double wc2 = wc*wc;
  double wc3 = wc2*wc;
  double wc4 = wc2*wc2;
  double k = wc/tan(M_PI*fc/srate);
  double k2 = k*k;
  double k3 = k2*k;
  double k4 = k2*k2;
  double sqrt2 = sqrt(2.);
  double sq_tmp1 = sqrt2*wc3*k;
  double sq_tmp2 = sqrt2*wc*k3;
  double a_tmp = 4*wc2*k2 + 2*sq_tmp1 + k4 + 2*sq_tmp2 + wc4;
  
  b1 = (4*(wc4 + sq_tmp1 - k4 - sq_tmp2))/a_tmp;
  b2 = (6*wc4 - 8*wc2*k2 + 6*k4)/a_tmp;
  b3 = (4*(wc4 - sq_tmp1 + sq_tmp2 - k4))/a_tmp;
  b4 = (k4 - 2*sq_tmp1 + wc4 - 2*sq_tmp2 + 4*wc2*k2)/a_tmp;
  
  la0 = wc4/a_tmp;
  la1 = 4*wc4/a_tmp;
  la2 = 6*wc4/a_tmp;
  la3 = la1;
  la4 = la0;
  
  ha0 = k4/a_tmp;
  ha1 = -4*k4/a_tmp;
  ha2 = 6*k4/a_tmp;
  ha3 = ha1;
  ha4 = ha0;
 }

public:
 LinkwitzRileyFilterCoefficients(Parameters &p) :
 Parameters::ParameterListener(p),
 dspParam(p)
 {
  updateSampleRate(p.sampleRate(), p.sampleInterval());
 }

 virtual void updateSampleRate(double sr, double isr) override
 {
  setCoeff();
 }
 
 void setFrequency(SampleType frequency)
 {
  fc = frequency;
  setCoeff();
 }
};










class LinkwitzRileyFilterKernel
{
 SampleType xm1;
 SampleType xm2;
 SampleType xm3;
 SampleType xm4;
 SampleType lym1;
 SampleType lym2;
 SampleType lym3;
 SampleType lym4;
 SampleType hym1;
 SampleType hym2;
 SampleType hym3;
 SampleType hym4;
 
public:
 LinkwitzRileyFilterKernel()
 {
  reset();
 }
 
 void reset()
 {
  xm1 = xm2 = xm3 = xm4 = lym1 = lym2 = lym3 = lym4 = hym1 = hym2 = hym3 = hym4 = 0.;
 }
 
 void process(const LinkwitzRileyFilterCoefficients &coeff,
              SampleType &lowOutput,
              SampleType &highOutput,
              SampleType input)
 {
  SampleType tempx, tempy;
  
  tempx = input;
  
  tempy = coeff.la0*tempx +
  coeff.la1*xm1 +
  coeff.la2*xm2 +
  coeff.la3*xm3 +
  coeff.la4*xm4 -
  coeff.b1*lym1 -
  coeff.b2*lym2 -
  coeff.b3*lym3 -
  coeff.b4*lym4;
  
  lym4 = lym3;
  lym3 = lym2;
  lym2 = lym1;
  lym1 = tempy;
  
  lowOutput = tempy;
  
  tempy = coeff.ha0*tempx +
  coeff.ha1*xm1 +
  coeff.ha2*xm2 +
  coeff.ha3*xm3 +
  coeff.ha4*xm4 -
  coeff.b1*hym1 -
  coeff.b2*hym2 -
  coeff.b3*hym3 -
  coeff.b4*hym4;
  
  hym4 = hym3;
  hym3 = hym2;
  hym2 = hym1;
  hym1 = tempy;
  
  highOutput = tempy;
  
  xm4 = xm3;
  xm3 = xm2;
  xm2 = xm1;
  xm1 = tempx;
 }
};










}

#endif /* XDDSP_LinkwitzRileyKernel_h */
