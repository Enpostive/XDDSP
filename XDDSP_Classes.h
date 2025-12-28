//
//  XDDSP_Classes.h
//  XDDSP
//
//  Created by Adam Jackson on 24/5/2022.
//

#ifndef XDDSP_Classes_h
#define XDDSP_Classes_h

#include "XDDSP_Types.h"
#include "XDDSP_Functions.h"
#include "XDDSP_Parameters.h"

namespace XDDSP
{









constexpr int IntegerMaximum = INT_MAX;







/**
 * @brief A template class which encapsulates the connection paradigm
 * 
 * This is a CRTP base class for all connector types. This template expects one method called 'get' to be defined in the derived class. 'get' is used by the methods defined here to retrieve the requested input
 * 
 * @tparam Derived The derived class 
 * @tparam ChannelCount The number of channels the coupler needs to support
 */
template <typename Derived, int ChannelCount = 1>
class Coupler
{
#define THIS static_cast<Derived*>(this)
public:
 static constexpr int Count = ChannelCount;
 
 /**
  * @brief Retrieve one sample from the specified channel
  * 
  * @param channel The selected channel
  * @param index The index of the sample to get
  * @return SampleType The sample at the index in that channel is returned
  */
 SampleType operator()(int channel, int index)
 { return THIS->get(channel, index); }

 /**
  * @brief Retrieve one sample from channel 0
  * 
  * @param index The index of the sample to get
  * @return SampleType The sample at the index in that channel is returned
  */
 SampleType operator()(int index)
 { return THIS->get(0, index); }
 
 /**
  * @brief Copies 'transferSize' number of samples from each channel into the buffers pointed to by 'p'
  * 
  * @tparam T The datatype of the destination buffer, is usually inferred from the parameter
  * @param p An array of pointers to buffers
  * @param transferSize The number of samples to copy per channel
  */
 template <typename T>
 void fastTransfer(const std::array<T*, Count> &p, int transferSize)
 {
  for (int c = 0; c < Count; ++c)
  {
   for (int i = 0; i < transferSize; ++i)
   {
    p[c][i] = THIS->get(c, i);
   }
  }
 }
#undef THIS
};










/**
 * @brief An implementation of a buffer to be used to store output data from a DSP process.
 * 
 * Coupler exposes a similar interface to XDDSP::Coupler which returns references to the samples requested, enabling them to be written by DSP code. This object listens to XDDSP::Parameters and automatically resizes the buffer when it is commanded to.
 * 
 * @tparam BufferCount The number of channels to support
 */
template <int BufferCount>
class OutputBuffer final : public Parameters::ParameterListener
{
 Parameters& dspParam;
 
 std::vector<SampleType> buffer;
 
 SampleType& get(int channel, int index)
 {
  dsp_assert(channel >= 0 && channel < BufferCount);
  return buffer[channel*dspParam.bufferSize() + index];
 }

public:
 static constexpr int Count = BufferCount;
 
 OutputBuffer(Parameters &p) :
 Parameters::ParameterListener(p),
 dspParam(p)
 { updateBufferSize(p.bufferSize()); }
 
 OutputBuffer(const OutputBuffer<Count> &rhs) :
 Parameters::ParameterListener(rhs.dspParam),
 dspParam(rhs.dspParam),
 buffer(rhs.buffer)
 {}
 
 virtual void updateBufferSize(int bs) override
 {
  buffer.resize(bs*Count, 0.);
  buffer.shrink_to_fit();
 }
 
 SampleType* operator[](int channel)
 { return &get(channel, 0); }
 
 SampleType& operator()(int channel, int index)
 { return get(channel, index); }
 
 SampleType& operator()(int index)
 { return get(0, index); }
 
 void reset()
 { buffer.assign(buffer.size(), 0.); }
};










/**
 * @brief An output which encapsulates an output buffer inside a coupler so that it can be readily connected to by other components
 * 
 * @tparam OutputCount The number of output channels to provide
 */
template <int OutputCount = 1>
class Output final : public Coupler<Output<OutputCount>, OutputCount>
{
public:
 static constexpr int Count = OutputCount;
 
 OutputBuffer<Count> buffer;
 
 /**
  * @brief Is called from the Coupler base class to fetch the actual sample from the buffer
  * 
  * @param channel The selected channel
  * @param index The index of the sample to get
  * @return SampleType The sample at the index in that channel is returned
  */
 SampleType get(int channel, int index)
 {
  return buffer(channel, index);
 }

 explicit Output(Parameters &p) :
 buffer(p)
 {}
 
 Output(const Output<Count> &rhs) :
 buffer(rhs.buffer)
 {}
 
 void reset()
 { buffer.reset(); }
};










/**
 * @brief A foundational base class for the CRTP base class to inherit from.
 *        Pointers of this type can point to any component class which enables component containers to work.
 * 
 * This class is deprecated in favour of making the CRTP class into the foundation class. Virtual methods will be removed entirely. This shouldn't break too much code, except for perhaps removing a few 'override' specifiers.
 * 
 */
class ComponentBaseClass
{
 bool enabled {true};

protected:
 int samplesToNextTrigger = -1;
 
 /**
  * @anchor Component_setNextTrigger
  * @brief Set the Next Trigger object
  *        A component can use this->setNextTrigger(time) to set the number of samples in the future to trigger. Only one trigger is kept, subsequent calls override the previous calls.
  * 
  * @param point The number of samples in the future to wait until triggering. Use -1 to cancel the trigger
  */
 void setNextTrigger(int point)
 {
  samplesToNextTrigger = point;
 }

public:
 virtual ~ComponentBaseClass() {}

 /**
  * @anchor Component_isEnabled
  * @brief Returns whether the component is enabled. If the component is disabled, the inner process loop is not run when process is called.
  * 
  * @return true if the parameter is enabled
  * @return false otherwise
  */
 bool isEnabled() { return enabled; }
 
 /**
  * @anchor Component_isDisabled
  * @brief Sets whether the component is enabled or not
  * 
  * @param e Pass true to enable the device, false otherwise
  */
 void setEnabled(bool e)
 {
  enabled = e;
  if (!enabled) reset();
 }

 virtual void reset() = 0;
 virtual int startProcess(int startPoint, int sampleCount) = 0;
 virtual void stepProcess(int startPoint, int sampleCount) = 0;
 virtual void triggerProcess(int triggerPoint) = 0;
 virtual void finishProcess() = 0;
 virtual void process(int startPoint, int sampleCount) = 0;
};










/**
 * @brief A CRTP component template which encapsulates the implementation of the process loop logic.
 *        A component process loop is split up into 4 parts: reset, start, step and finish. Reset is called as required by the application to reset the component. The start code is called once at the start of each process buffer to process. The step code is called repeatedly to do the actual processing. The finish code is called after the last step call. The process loop can also interrupt itself at a pre-determined time to call a trigger.
 * 
 * @tparam Derived 
 * @tparam StepSize 
 */
template <class Derived, int StepSize = INT_MAX>
class Component : public ComponentBaseClass
{
 Derived &THIS;

public:
 Component() :
 THIS(static_cast<Derived&>(*this))
 {}
 
 /**
  * @brief This can be implemented to contain the code used to reset the component to a default known state.
  *
  * At a minimum, it is expected that this method fills all output buffers with zeroes.
  */
 void reset()
 {}

 /**
  * @brief This can be implemented to return a step size to use.
  *        It can return the same value each time, or it can return an arbitrary step size not bigger than the sampleCount argument provided. Alternatively, the default implementation will use the template argument provided.
  * 
  * @param startPoint The location of the first sample to process
  * @param sampleCount The number of samples to process this time
  * @return int 
  */
 int startProcess(int startPoint, int sampleCount)
 { return std::min(sampleCount, StepSize); }
 
 /**
  * @brief Is called repeatedly by Component::process with the start point incremented by step size. The actual DSP processing code lives here.
  * 
  * @param startPoint The location of the first sample to process
  * @param sampleCount The number of samples to process this time
  */
 void stepProcess(int startPoint, int sampleCount)
 {}
 
 /**
  * @brief Is called when a trigger point is reached.
  * 
  * @param triggerPoint The loation in the buffer where the trigger was set.
  */
 void triggerProcess(int triggerPoint)
 {}
 
 /**
  * @brief Is called after all the blocks have been processed
  * 
  */
 void finishProcess()
 {}
  
 /**
  * @brief **This is the main entry point of the component**. This method should be called after all the input sources have been processed.
  * 
  * @param startPoint The location of the first sample to process
  * @param sampleCount The number of samples to process this time
  */
 void process(int startPoint, int sampleCount)
 {
  if (isEnabled())
  {
   int currentPoint = startPoint;
   int stepSize = THIS.startProcess(startPoint, sampleCount);
   bool triggerActive = samplesToNextTrigger > -1;
   int samplesToProcessNow = (triggerActive ?
                              std::min(sampleCount, samplesToNextTrigger) :
                              sampleCount);

   do
   {
    while (stepSize < samplesToProcessNow)
    {
     THIS.stepProcess(currentPoint, stepSize);
     currentPoint += stepSize;
     sampleCount -= stepSize;
     samplesToProcessNow -= stepSize;
     if (triggerActive) samplesToNextTrigger -= stepSize;
    }
    
    if (triggerActive && samplesToNextTrigger < stepSize)
    {
     stepProcess(currentPoint, samplesToNextTrigger);
     currentPoint += samplesToNextTrigger;
     sampleCount -= samplesToNextTrigger;
     samplesToNextTrigger = -1;
     THIS.triggerProcess(currentPoint);
     triggerActive = samplesToNextTrigger > -1;
     samplesToProcessNow = (triggerActive ?
                            std::min(sampleCount, samplesToNextTrigger) :
                            sampleCount);
    }
   }
   while (samplesToProcessNow < sampleCount);
    
   THIS.stepProcess(currentPoint, sampleCount);
   if (triggerActive) samplesToNextTrigger -= sampleCount;
   THIS.finishProcess();
  }
 }
};









/// @brief Deprecated.
class ComponentContainer : public Component<ComponentContainer>
{
 std::vector<ComponentBaseClass*> partsList;
 
protected:
 void addPart(ComponentBaseClass& part)
 { partsList.push_back(&part); }
 
public:
 void reset()
 {
  for (auto &part : partsList) part->reset();
 }
 
 void stepProcess(int startPoint, int sampleCount)
 {
  for (auto &part : partsList) part->process(startPoint, sampleCount);
 }
};










}

#endif /* XDDSP_Classes_h */
