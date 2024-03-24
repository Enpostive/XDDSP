/*
  ==============================================================================

    XDDSP_WindowFunctions.h
    Created: 19 Aug 2023 11:54:22am
    Author:  Adam Jackson

  ==============================================================================
*/

#ifndef XDDSP_WINDOWFUNCTIONS
#define XDDSP_WINDOWFUNCTIONS

#include "XDDSP_Types.h"










namespace XDDSP
{










namespace WindowFunction
{

template <typename T>
T sqr(T x) { return x*x; }

class Rectangle
{
protected:
 const SampleType length;
 
 constexpr SampleType window(SampleType x)
 { return (x >= 0.) * (x <= length); }

public:
 constexpr Rectangle(SampleType length) :
 length(length)
 {}
 
 constexpr SampleType operator()(SampleType x)
 { return window(x); }
};

class Triangle : public Rectangle
{
public:
 Triangle(SampleType length) :
 Rectangle(length)
 {}
 
 constexpr SampleType operator()(SampleType x)
 { return (1. - fabs((x - length/2.)/(length/2.)))*window(x); }
};

class Welch : public Rectangle
{
public:
 Welch(SampleType length) :
 Rectangle(length)
 {}
 
 constexpr SampleType operator()(SampleType x)
 { return (1. - sqr(fabs((x - length/2.)/(length/2.))))*window(x); }
};

class Sine : public Rectangle
{
public:
 Sine(SampleType length) :
 Rectangle(length)
 {}
 
 constexpr SampleType operator()(SampleType x)
 { return (sin(M_PI*x/length))*window(x); }
};

class CosineWindow : public Rectangle
{
 SampleType a0, a1;
public:
 CosineWindow(SampleType length, SampleType a = 0.5) :
 Rectangle(length), a0(a), a1(1. - a0)
 {}
  
 constexpr SampleType operator()(SampleType x)
 { return (a0 + a1*cos(2.*M_PI*x/length))*window(x); }
};

/*
class NewWindow : public Rectangle
{
public:
 NewWindow(SampleType length) :
 Rectangle(length)
 {}
  
 constexpr SampleType operator()(SampleType x)
 { return ()*window(x); }
};
*/


}










template <typename WindowType, typename T = SampleType>
void applyWindowFunction(WindowType window, T* data, unsigned int length)
{
 for (int i = 0; i < length; ++i) data[i] *= window(i);
}

template <typename WindowType, typename T, unsigned long length>
void applyWindowFunction(WindowType window, std::array<T, length> &data)
{ applyWindowFunction(window, data.data(), data.size()); }

template <typename WindowType, typename T>
void applyWindowFunction(WindowType window, std::vector<T> &data)
{ applyWindowFunction(window, data.data(), data.size()); }










}










#endif // XDDSP_WINDOWFUNCTIONS
