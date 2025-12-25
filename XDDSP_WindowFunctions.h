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










  /**
   * @brief A namespace containing classes which can generate various window shapes, useful for many DSP applications.
   * 
   */
namespace WindowFunction
{

template <typename T>
T sqr(T x) { return x*x; }

/**
 * @brief A callable class which generates a rectangle shaped window of a certain length.
 * 
 * This is also the base class for all other window function classes, as every other window also multiplies itself with the rectangle window.
 */
class Rectangle
{
protected:
 /**
  * @brief This constant is initialised with the length of the window upon construction.
  * 
  */
 const SampleType length;
 
 /**
  * @brief Produce a rectangle window sample for a given co-ordinate.
  * 
  * @param x The co-ordinate to produce the sample for.
  * @return SampleType The sample.
  */
 constexpr SampleType window(SampleType x)
 { return (x >= 0.) * (x <= length); }

public:
 constexpr Rectangle(SampleType length) :
 length(length)
 {}
 
 constexpr SampleType operator()(SampleType x)
 { return window(x); }
};





/**
 * @brief A callable class which produces a triangle window.
 * 
 */
class Triangle : public Rectangle
{
public:
 Triangle(SampleType length) :
 Rectangle(length)
 {}
 
 constexpr SampleType operator()(SampleType x)
 { return (1. - fabs((x - length/2.)/(length/2.)))*window(x); }
};





/**
 * @brief A callable class which produces a Welch window.
 * 
 */
class Welch : public Rectangle
{
public:
 Welch(SampleType length) :
 Rectangle(length)
 {}
 
 constexpr SampleType operator()(SampleType x)
 { return (1. - sqr(fabs((x - length/2.)/(length/2.))))*window(x); }
};





/**
 * @brief A callable class which produces a sine window.
 * 
 */
class Sine : public Rectangle
{
public:
 Sine(SampleType length) :
 Rectangle(length)
 {}
 
 constexpr SampleType operator()(SampleType x)
 { return (sin(M_PI*x/length))*window(x); }
};





/**
 * @brief A callable class which produces a cosine window, which is distinct from a sine window given that the sine window only covers half a period while this covers a full period.
 * 
 */
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





/**
 * @brief Produce a Gauss window.
 * 
 */
class Gauss : public Rectangle
{
 SampleType param;
 
public:
 Gauss(SampleType length, SampleType param = 0.5) :
 Rectangle(length),
 param(param)
 {}
  
 constexpr SampleType operator()(SampleType x)
 { return (exp(-0.5*sqr(((2.*x/length) - 1.)/param)))*window(x); }
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










/**
 * @brief Apply a window function to samples inside an array.
 * 
 * @tparam WindowType The class of window function, which is implied from the window parameter.
 * @tparam T The sample type, which is implied from the data parameter.
 * @param window An instance of one of the above window objects.
 * @param data A pointer to the sample data to apply the window to.
 * @param length The length of the data. Note that the window length may be different. The data is overwritten.
 */
template <typename WindowType, typename T = SampleType>
void applyWindowFunction(WindowType window, T* data, unsigned long length)
{
 for (unsigned long i = 0; i < length; ++i) data[i] *= window(i);
}

/**
 * @brief Apply a window function to samples inside a std::array.
 * 
 * @tparam WindowType The class of window function, which is implied from the window parameter.
 * @tparam T The sample type, which is implied from the data parameter.
 * @tparam length The length of the array, implied from the data parameter.
 * @param window An instance of one of the above window objects.
 * @param data A reference to the std::array containing the sample data to apply the window to. The array is overwritten.
 */
template <typename WindowType, typename T, unsigned long length>
void applyWindowFunction(WindowType window, std::array<T, length> &data)
{ applyWindowFunction(window, data.data(), data.size()); }

/**
 * @brief Apply a window function to samples inside a std::vector.
 * 
 * @tparam WindowType The class of window function, which is implied from the window parameter.
 * @tparam T The sample type, which is implied from the data parameter.
 * @param window An instance of one of the above window objects.
 * @param data A reference to the std::array containing the sample data to apply the window to. The array is overwritten.
 */
template <typename WindowType, typename T>
void applyWindowFunction(WindowType window, std::vector<T> &data)
{ applyWindowFunction(window, data.data(), data.size()); }










}










#endif // XDDSP_WINDOWFUNCTIONS
