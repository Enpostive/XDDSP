//
//  XDDSP_Inputs.h
//  XDDSPTestingHarness
//
//  Created by Adam Jackson on 1/6/2022.
//

#ifndef XDDSP_Inputs_h
#define XDDSP_Inputs_h

#include "XDDSP_Types.h"
#include "XDDSP_Functions.h"
#include "XDDSP_Parameters.h"
#include "XDDSP_Classes.h"


/*
 Component Input Definitions
 ===========================
 
 The classes defined in this file are suitable to be used as template arguments to any
 component class. If it's not defined in this file, then it's not an input!
 */










namespace XDDSP
{










template <int ChannelCount = 1>
class Connector : public Coupler<ChannelCount>
{
 Coupler<ChannelCount> &connection;
 
protected:
 virtual SampleType get(int channel, int index) override
 { return connection(channel, index); }
 
public:
 static constexpr int Count = ChannelCount;
 
 Connector(Coupler<ChannelCount> &_connection) :
 connection(_connection)
 {}
 
 Connector(const Connector<ChannelCount> &rhs):
 connection(rhs.connection)
 {}
};










template <int ChannelCount = 1>
class PConnector : public Coupler<ChannelCount>
{
 Coupler<ChannelCount> *connection {nullptr};
 
protected:
 virtual SampleType get(int channel, int index) override
 {
  if (!connection) return 0.;
  return (*connection)(channel, index);
 }
 
public:
 static constexpr int Count = ChannelCount;
 PConnector() {}
 
 PConnector(Coupler<ChannelCount> &_connection) :
 connection(&_connection)
 {}
 
 void connect(Coupler<ChannelCount> &_connection)
 {
  connection = &_connection;
 }
 
 void disconnect()
 {
  connection = nullptr;
 }
};










template <int ChannelCount, int Channel, int OutputChannelCount = 1>
class ChannelPicker : public Coupler<OutputChannelCount>
{
 Coupler<ChannelCount> &connection;
 
protected:
 virtual SampleType get(int channel, int index) override
 { return connection(Channel, index); }
 
public:
 static constexpr int Count = OutputChannelCount;
 
 ChannelPicker(Coupler<ChannelCount> &_connection) :
 connection(_connection)
 {}
 
 ChannelPicker(const ChannelPicker<ChannelCount, Channel> &rhs) :
 connection(rhs.connection)
 {}
};










template <typename BufferSampleType, int ChannelCount = 1>
class BufferCoupler : public Coupler<ChannelCount>
{
 std::array<BufferSampleType*, ChannelCount> p;
 
protected:
 virtual SampleType get(int channel, int index) override
 {
  dsp_assert(channel >= 0 && channel < Count);
  dsp_assert(p[channel] != nullptr);
  return p[channel][index];
 }
 
public:
 static constexpr int Count = ChannelCount;
 
 BufferCoupler()
 {
  p.fill(nullptr);
 }
 
 explicit BufferCoupler(const std::array<BufferSampleType*, ChannelCount> &_p) :
 p(_p)
 {}
 
 BufferCoupler(const BufferCoupler<BufferSampleType, ChannelCount> &rhs) :
 p(rhs.p)
 {}
 
 void connect(const std::array<BufferSampleType*, ChannelCount> &_p)
 { p = _p; }
 
 void connect(BufferSampleType* _p)
 { for (auto& pp : p) pp = _p; }
 
 void connect(int channel, BufferSampleType* _p)
 {
  dsp_assert(channel >= 0 && channel < Count);
  dsp_assert(_p != nullptr);
  p[channel] = _p;
 }
};










template <int ConstantCount = 1>
class ControlConstant : public Coupler<ConstantCount>
{
 static_assert(ConstantCount > 0, "ControlConstant must have at least one channel");
 
 std::array<SampleType, ConstantCount> setting;
 std::array<SampleType, ConstantCount> c;
 
protected:
 virtual SampleType get(int channel, int index) override
 {
  return c[channel];
 }
 
public:
 static constexpr int Count = ConstantCount;
 WaveformFunction func;
 
 ControlConstant()
 {
  setControl(0.);
 }
 
 ControlConstant(SampleType c)
 { setControl(c); }
 
 ControlConstant(const ControlConstant<ConstantCount> &rhs) :
 setting(rhs.setting),
 c(rhs.c),
 func(rhs.func)
 {}
 
 SampleType getControl(int channel)
 { return setting.at(channel); }
 
 SampleType getControl()
 { return getControl(0); }
 
 void setControl(int channel, SampleType control)
 {
  setting.at(channel) = control;
  if (func) control = func(control);
  c.at(channel) = control;
 }
 
 void setControl(SampleType control)
 {
  for (int i = 0; i < Count; ++i) setControl(i, control);
 }
 
 void refreshControl()
 {
  if (func) for (int i = 0; i < Count; ++i) c[i] = func(setting[i]);
 }
};










struct AudioPropertiesInputMode
{
 enum
 {
  BeatsPerMinute,
  LengthQuarterNoteSeconds,
  LengthQuarterNoteSamples,
  FrequencyQuaterNote,
  SamplesPerSecond,
  SecondsPerSample
 };
};

template <int TimeMode, int ChannelCount = 1>
class AudioPropertiesInput : public Coupler<ChannelCount>
{
 static constexpr SampleType PerMinute = 1./60.;
 
 Parameters &dspParam;
 SampleType multiplier {1.};
 
protected:
 virtual SampleType get(int channel, int index) override
 {
  switch (TimeMode)
  {
   default:
   case AudioPropertiesInputMode::BeatsPerMinute:
    return dspParam.getTempo()*multiplier;
    
   case AudioPropertiesInputMode::LengthQuarterNoteSeconds:
    return 60.*multiplier/dspParam.getTempo();
    
   case AudioPropertiesInputMode::LengthQuarterNoteSamples:
    return 60.*dspParam.sampleRate()*multiplier/dspParam.getTempo();
    
   case AudioPropertiesInputMode::FrequencyQuaterNote:
    return multiplier*dspParam.getTempo()*PerMinute;
    
   case AudioPropertiesInputMode::SamplesPerSecond:
    return multiplier*dspParam.sampleRate();
    
   case AudioPropertiesInputMode::SecondsPerSample:
    return multiplier*dspParam.sampleInterval();
  }
 }
 
public:
 static constexpr int Count = ChannelCount;
 
 AudioPropertiesInput(Parameters &p) : dspParam(p)
 {}
 
 AudioPropertiesInput(const AudioPropertiesInput<TimeMode> &rhs) :
 dspParam(rhs.dspParam),
 multiplier(rhs.multiplier)
 {}
 
 SampleType getMultiplier()
 { return multiplier;}
 
 void setMultiplier(SampleType _m)
 { multiplier = _m; }
};










template <int NoConnections, int ChannelCount = 1>
class Switch : public Coupler<ChannelCount>
{
 int selected;
 
 std::array<Coupler<ChannelCount>*, NoConnections> connections;
 
protected:
 virtual SampleType get(int channel, int index) override
 {
  return (*(connections[selected]))(channel, index);
 }
 
public:
 static constexpr int Count = ChannelCount;

 Switch(std::array<Coupler<ChannelCount>*, NoConnections> inputs) :
 connections(inputs)
 {
  select(0);
 }
 
 void select(int s)
 {
  dsp_assert(s >= 0 && s < NoConnections);
  selected = s;
 }
 
 int getSelection() const
 { return selected; }
};










template <int NoConnections = 0, int ChannelCount = 1>
class Sum : public Coupler<ChannelCount>
{
 static_assert(NoConnections > 0, "Number of connections must be specified");
 
 std::array<Coupler<ChannelCount>*, NoConnections> connections;
 
protected:
 virtual SampleType get(int channel, int index) override
 {
  SampleType sum = 0.;
  for (Coupler<ChannelCount>* c : connections) sum += (*c)(channel, index);
  return sum;
 }
 
public:
 static constexpr int Count = ChannelCount;

 Sum(std::array<Coupler<ChannelCount>*, NoConnections> inputs) :
 connections(inputs)
 {}
};










template <int NoConnections = 0, int ChannelCount = 1>
class Product : public Coupler<ChannelCount>
{
 static_assert(NoConnections > 0, "Number of connections must be specified");
 
 std::array<Coupler<ChannelCount>*, NoConnections> connections;
 
protected:
 virtual SampleType get(int channel, int index) override
 {
  SampleType sum = 1.;
  for (Coupler<ChannelCount>* c : connections) sum *= (*c)(channel, index);
  return sum;
 }
 
public:
 static constexpr int Count = ChannelCount;

 Product(std::array<Coupler<ChannelCount>*, NoConnections> inputs) :
 connections(inputs)
 {}
};










template <int ChannelCount = 1>
class SignalModifier : public Coupler<ChannelCount>
{
 Coupler<ChannelCount> &connection;
 
protected:
 virtual SampleType get(int channel, int index) override
 {
  if (func) return func(connection(channel, index));
  return connection(channel, index);
 }
 
public:
 static constexpr int Count = ChannelCount;

 WaveformFunction func;

 SignalModifier(Coupler<ChannelCount> &_connection) :
 connection(_connection)
 {}
 
 SignalModifier(const SignalModifier &rhs) :
 connection(rhs.connection),
 func(rhs.func)
 {}
};










template <typename BufferSampleType, int ChannelCount, int Quality = ProcessQuality::LowQuality>
class SamplePlaybackHead : public Coupler<ChannelCount>
{
 static_assert(Quality == ProcessQuality::LowQuality ||
               Quality == ProcessQuality::MidQuality ||
               Quality == ProcessQuality::HighQuality,
               "Invalid quality specifier");
 
 using size_t = std::size_t;
 
 std::array<const BufferSampleType*, ChannelCount> buffer;
 size_t bufferSize {0};
 SampleType bufferLength {static_cast<SampleType>(bufferSize)};
 
 Coupler<ChannelCount> &input;
 
protected:
 virtual SampleType get(int channel, int index) override
 {
  dsp_assert(channel >= 0 && channel < ChannelCount);
  if (buffer[channel] == nullptr) return 0.;
  
  SampleType position = fastBoundary(input(channel, index), 0., bufferLength);
  
  IntegerAndFraction<size_t> posIF(position);
  size_t xm1, x1, x2;
  
  switch (Quality)
  {
   case ProcessQuality::LowQuality:
    return buffer[channel][posIF.intRep()];
    break;
    
   case ProcessQuality::MidQuality:
    x1 = std::min(posIF.intRep() + 1, bufferSize);
    return LERP(posIF.fracPart(),
                buffer[channel][posIF.intRep()],
                buffer[channel][x1]);
    
   case ProcessQuality::HighQuality:
    xm1 = posIF.intRep() - (posIF.intRep() != 0);
    x1 = std::min(posIF.intRep() + 1, bufferSize);
    x2 = std::min(posIF.intRep() + 2, bufferSize);
    return hermite(posIF.fracPart(),
                   buffer[channel][xm1],
                   buffer[channel][posIF.intRep()],
                   buffer[channel][x1],
                   buffer[channel][x2]);
  }
  
  return 0.;
 }
 
public:
 static constexpr int Count = ChannelCount;
 
 SamplePlaybackHead(Coupler<ChannelCount> &_input) :
 input(_input)
 {
  buffer.fill(nullptr);
 }
 
 void connectChannel(int channel, const BufferSampleType *ptr)
 {
  dsp_assert(channel >= 0 && channel < ChannelCount);
  buffer[channel] = ptr;
 }
 
 std::size_t length()
 { return bufferSize + 1; }
 
 void setLength(std::size_t lengthSamples)
 {
  bufferSize = lengthSamples - 1;
  bufferLength = static_cast<SampleType>(bufferSize);
 }
};










template <typename BufferSampleType, int ChannelCount = 1>
class BufferReader : public Coupler<ChannelCount>
{
 std::array<const BufferSampleType*, ChannelCount> buffer;
 std::array<int, ChannelCount> length;
 
protected:
 virtual SampleType get(int channel, int index) override
 {
  if (index >= 0 &&
      buffer[channel] &&
      length[channel] > index) return buffer[channel][index];
  return 0.;
 }

public:
 static constexpr int Count = ChannelCount;

 BufferReader()
 {
  buffer.fill(nullptr);
  length.fill(0);
 }
 
 BufferReader(const BufferReader& other)
 {
  buffer = other.buffer;
  length = other.length;
 }
 
 void connectChannel(int channel, const BufferSampleType *ptr, int len)
 {
  dsp_assert(channel >= 0 && channel < ChannelCount);
  dsp_assert(len >= 0);
  buffer[channel] = ptr;
  length[channel] = len;
 }
};










template <int ChannelCount = 2>
class PluginInput : public Coupler<ChannelCount>
{
 std::array<float*, ChannelCount> floatInputs;
 std::array<double*, ChannelCount> doubleInputs;
 
 int length {0};
 
protected:
 virtual SampleType get(int channel, int index) override
 {
  dsp_assert(index >= 0 && index < length);
  dsp_assert(channel >= 0 && channel < ChannelCount);
  if (floatInputs[0]) return floatInputs[channel][index];
  if (doubleInputs[0]) return doubleInputs[channel][index];

  return 0.;
 }

public:
 static constexpr int Count = ChannelCount;
 
 PluginInput()
 {
  floatInputs.fill(nullptr);
  doubleInputs.fill(nullptr);
 }
 
 PluginInput(const PluginInput &other)
 {
  floatInputs = other.floatInputs;
  doubleInputs = other.doubleInputs;
  length = other.length;
 }
 
 void connectFloats(const std::array<float*, ChannelCount> &p, int sampleCount)
 {
  floatInputs = p;
  doubleInputs.fill(nullptr);
  length = sampleCount;
 }
 
 void connectDoubles(const std::array<double*, ChannelCount> &p, int sampleCount)
 {
  floatInputs.fill(nullptr);
  doubleInputs = p;
  length = sampleCount;
 }
};










}

#endif /* XDDSP_Inputs_h */
