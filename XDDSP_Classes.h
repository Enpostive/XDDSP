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








template <typename Derived, int ChannelCount = 1>
class Coupler
{
#define THIS static_cast<Derived*>(this)
public:
 static constexpr int Count = ChannelCount;
 
 SampleType operator()(int channel, int index)
 { return THIS->get(channel, index); }

 SampleType operator()(int index)
 { return THIS->get(0, index); }
 
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










template <int OutputCount = 1>
class Output final : public Coupler<Output<OutputCount>, OutputCount>
{
public:
 static constexpr int Count = OutputCount;
 
 OutputBuffer<Count> buffer;
 
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




class ComponentBaseClass
{
 bool enabled {true};

protected:
 int samplesToNextTrigger = -1;
 
 void setNextTrigger(int point)
 {
  samplesToNextTrigger = point;
 }

public:
 virtual ~ComponentBaseClass() {}

 bool isEnabled() { return enabled; }
 
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










template <class Derived, int StepSize = INT_MAX>
class Component : public ComponentBaseClass
{
 Derived &THIS;

public:
 Component() :
 THIS(static_cast<Derived&>(*this))
 {}
 
 // This function is responsible for clearing the output buffers to a default state when
 // the component is disabled.
 void reset()
 {}

 // startProcess prepares the component for processing one block and returns the step
 // size. By default, it returns the entire sampleCount as one big step.
 int startProcess(int startPoint, int sampleCount)
 { return std::min(sampleCount, StepSize); }
 
 // stepProcess is called repeatedly with the start point incremented by step size
 void stepProcess(int startPoint, int sampleCount)
 {}
 
 // triggerProcess is called once if 'samplesToNextTrigger' reaches zero
 // components can use 'setNextTrigger' to set a trigger point
 void triggerProcess(int triggerPoint)
 {}
 
 // finishProcess is called after all the blocks have been processed
 void finishProcess()
 {}
  
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
