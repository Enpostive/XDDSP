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








template <int ChannelCount = 1>
class Coupler
{
protected:
 virtual SampleType get(int channel, int index) = 0;
 
public:
 static constexpr int Count = ChannelCount;
 
 virtual ~Coupler() {}
 
 SampleType operator()(int channel, int index)
 { return get(channel, index); }

 SampleType operator()(int index)
 { return get(0, index); }
 
 template <typename T>
 void fastTransfer(const std::array<T*, Count> &p, int transferSize)
 {
  for (int c = 0; c < Count; ++c)
  {
   for (int i = 0; i < transferSize; ++i)
   {
    p[c][i] = get(c, i);
   }
  }
 }
};










template <int BufferCount>
class OutputBuffer : public Parameters::ParameterListener
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
class Output : public Coupler<OutputCount>
{
protected:
 virtual SampleType get(int channel, int index) override
 {
  return buffer(channel, index);
 }
 
public:
 static constexpr int Count = OutputCount;
 
 OutputBuffer<Count> buffer;
 
 explicit Output(Parameters &p) :
 buffer(p)
 {}
 
 Output(const Output<Count> &rhs) :
 buffer(rhs.buffer)
 {}
 
 void reset()
 { buffer.reset(); }
};










class ModulationSource : public Coupler<1>
{
 Coupler<1> &connection;
 
protected:
 SampleType get(int channel, int index)
 { return connection(channel, index); }
 
public:
 explicit ModulationSource(Coupler<1> &connection)
 : connection(connection)
 {}
 
 ModulationSource(const ModulationSource &rhs)
 : connection(rhs.connection)
 {}
 
 SampleType operator()(int channel, int index)
 { return connection(channel, index); }
 
 SampleType operator()(int index)
 { return connection(index); }
};










class ModulationCoordinator;

class ModulationDestination : public Coupler<1>
{
 friend class ModulationCoordinator;
 friend class Parameters;
 
 OutputBuffer<1> modulationSignal;
 
protected:
 virtual SampleType get(int channel, int index) override
 { return range.fastBoundary(modulationSignal(channel, index)); }

public:
 static constexpr int Count = 1;
 MinMax<> range {-1., 1.};
 
 ModulationDestination(Parameters &p) :
 modulationSignal(p)
 {}
 
 ModulationDestination(const ModulationDestination& rhs) :
 modulationSignal(rhs.modulationSignal)
 {}
};

class ModulationCoordinator
{
 Parameters &dspParam;
 ModulationDestination *d {nullptr};
 
protected:
 explicit ModulationCoordinator(Parameters &p) :
 dspParam(p)
 {}
 
 void addSignal(int index, SampleType signal)
 {
  if (d)
  {
   d->modulationSignal(0, index) += signal;
  }
 }
 
public:
 void selectDestination(int index)
 {
  d = dspParam.getModulationDestination(index);
 }
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
