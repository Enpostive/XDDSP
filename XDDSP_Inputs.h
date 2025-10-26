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










template <typename Source>
class Connector final : public Coupler<Connector<Source>, Source::Count>
{
 Source &connection;
 
public:
 SampleType get(int channel, int index)
 { return connection(channel, index); }
 
 Connector(Source &_connection) :
 connection(_connection)
 {}
 
 Connector(const Connector<Source> &rhs):
 connection(rhs.connection)
 {}
};










template <int ChannelCount = 1>
class PConnector : public Coupler<PConnector<ChannelCount>, ChannelCount>
{
 using Getter = SampleType(*)(void*, int, int);
 
 void *connection {nullptr};
 Getter gm = nullptr;
 
public:
 static constexpr int Count = ChannelCount;
 PConnector() {}
 
 template <typename Source>
 PConnector(Source &connection)
 {
  connect(connection);
 }
 
 template <typename Source>
 void connect(Source &_connection)
 {
  connection = &_connection;
  gm = [](void* obj, int ch, int i){ return (*static_cast<Source*>(obj))(ch, i); };
 }
 
 void disconnect()
 {
  connection = nullptr;
  gm = nullptr;
 }
 
 SampleType get(int channel, int index)
 { return gm ? gm(connection, channel, index) : 0.0; }
};










template <typename Source, int ChannelCount, int Channel, int OutputChannelCount = 1>
class ChannelPicker final : public Coupler<ChannelPicker<Source, ChannelCount, Channel, OutputChannelCount>, OutputChannelCount>
{
 Source &connection;
 
public:
 SampleType get(int channel, int index)
 { return connection(Channel, index); }
 
 static constexpr int Count = OutputChannelCount;
 
 ChannelPicker(Source &_connection) :
 connection(_connection)
 {}
 
 ChannelPicker(const ChannelPicker<Source, ChannelCount, Channel, OutputChannelCount> &rhs) :
 connection(rhs.connection)
 {}
};










template <typename BufferSampleType, int ChannelCount = 1>
class BufferCoupler final : public Coupler<BufferCoupler<BufferSampleType, ChannelCount>, ChannelCount>
{
 std::array<BufferSampleType*, ChannelCount> p;
 
public:
 SampleType get(int channel, int index)
 {
  dsp_assert(channel >= 0 && channel < Count);
  dsp_assert(p[channel] != nullptr);
  return p[channel][index];
 }
 
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
class ControlConstant final : public Coupler<ControlConstant<ConstantCount>, ConstantCount>
{
 static_assert(ConstantCount > 0, "ControlConstant must have at least one channel");
 
 std::array<SampleType, ConstantCount> setting;
 std::array<SampleType, ConstantCount> c;
 
public:
 SampleType get(int channel, int index)
 {
  return c[channel];
 }
 
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
class AudioPropertiesInput final : public Coupler<AudioPropertiesInput<TimeMode, ChannelCount>, ChannelCount>
{
 static constexpr SampleType PerMinute = 1./60.;
 
 Parameters &dspParam;
 SampleType multiplier {1.};
 
public:
 SampleType get(int channel, int index)
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
 
 static constexpr int Count = ChannelCount;
 
 AudioPropertiesInput(Parameters &p) : dspParam(p)
 {}
 
 AudioPropertiesInput(Parameters &p, SampleType _m) : dspParam(p), multiplier(_m)
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
class MultiIn
{
protected:
 using Getter = SampleType(*)(void*, int, int);
 
 struct Conn
 {
  void* self {nullptr};
  Getter getter {nullptr};
  
  inline SampleType operator()(int ch, int i) const
  {
   return getter ? getter(self, ch, i) : 0.0;
  }
 };
 
 std::array<Conn, NoConnections> connections;
 
public:
 static constexpr int Count = ChannelCount;
 
 template <typename... Inputs>
 explicit MultiIn(Inputs&... ins)
 {
  static_assert(sizeof...(Inputs) == NoConnections,
                "Must provide exactly NoConnections inputs");
  size_t idx = 0;
  (connect(idx++, ins), ...);
 }
 
 template <typename C>
 void connect(size_t idx, C& c)
 {
  static_assert(C::Count == ChannelCount,
                "Channel count mismatch in MultiIn::connect");
  connections[idx].self = &c;
  connections[idx].getter = [](void* obj, int ch, int i) -> SampleType
  {
   return (*static_cast<C*>(obj))(ch, i);
  };
 }
 
 const std::array<Conn, NoConnections>& getConnections() const
 {
  return connections;
 }
};



template <int NoConnections, int ChannelCount = 1>
class Switch final : public Coupler<Switch<NoConnections, ChannelCount>, ChannelCount>, MultiIn<NoConnections, ChannelCount>
{
 int selected;
 using Base = MultiIn<NoConnections, ChannelCount>;
 
public:
 using Base::Base;
 
 SampleType get(int channel, int index)
 {
  return this->connections[selected](channel, index);
 }
 
 static constexpr int Count = ChannelCount;
 
 void select(int s)
 {
  dsp_assert(s >= 0 && s < NoConnections);
  selected = s;
 }
 
 int getSelection() const
 { return selected; }
};










template <int NoConnections, int ChannelCount = 1>
class Sum final : public Coupler<Sum<NoConnections, ChannelCount>, ChannelCount>, MultiIn<NoConnections, ChannelCount>
{
 using Base = MultiIn<NoConnections, ChannelCount>;
 
public:
 using Base::Base;

 virtual SampleType get(int channel, int index) override
 {
  SampleType sum = 0.;
  for (auto& c : this->connections) sum += c(channel, index);
  return sum;
 }
 
 static constexpr int Count = ChannelCount;
};










template <int NoConnections, int ChannelCount = 1>
class Product final : public Coupler<Product<NoConnections, ChannelCount>, ChannelCount>, MultiIn<NoConnections, ChannelCount>
{
 using Base = MultiIn<NoConnections, ChannelCount>;
 
public:
 using Base::Base;

 SampleType get(int channel, int index)
 {
  SampleType prod = 1.;
  for (auto& c : this->connections) prod *= c(channel, index);
  return prod;
 }
 
 static constexpr int Count = ChannelCount;
};










template <typename Source, int ChannelCount = 1>
class SignalModifier final : public Coupler<SignalModifier<Source, ChannelCount>, ChannelCount>
{
 Source &connection;
 
public:
 SampleType get(int channel, int index)
 {
  if (func) return func(connection(channel, index));
  return connection(channel, index);
 }
 
 static constexpr int Count = ChannelCount;

 WaveformFunction func;

 SignalModifier(Source &_connection) :
 connection(_connection)
 {}
 
 SignalModifier(const SignalModifier &rhs) :
 connection(rhs.connection),
 func(rhs.func)
 {}
};










template <typename Source, typename BufferSampleType, int ChannelCount, int Quality = ProcessQuality::LowQuality>
class SamplePlaybackHead final : public Coupler<SamplePlaybackHead<Source, BufferSampleType, ChannelCount, Quality>, ChannelCount>
{
 static_assert(Quality == ProcessQuality::LowQuality ||
               Quality == ProcessQuality::MidQuality ||
               Quality == ProcessQuality::HighQuality,
               "Invalid quality specifier");
 
 using size_t = std::size_t;
 
 std::array<const BufferSampleType*, ChannelCount> buffer;
 size_t bufferSize {0};
 SampleType bufferLength {static_cast<SampleType>(bufferSize)};
 
 Source &input;
 
public:
 SampleType get(int channel, int index)
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
 
 SamplePlaybackHead(Source &_input) :
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
class BufferReader final : public Coupler<BufferReader<BufferSampleType, ChannelCount>, ChannelCount>
{
 std::array<const BufferSampleType*, ChannelCount> buffer;
 std::array<int, ChannelCount> length;
 
public:
 SampleType get(int channel, int index)
 {
  if (index >= 0 &&
      buffer[channel] &&
      length[channel] > index) return buffer[channel][index];
  return 0.;
 }

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
class PluginInput final : public Coupler<PluginInput<ChannelCount>, ChannelCount>
{
 std::array<float*, ChannelCount> floatInputs;
 std::array<double*, ChannelCount> doubleInputs;
 
 int length {0};
 
public:
 SampleType get(int channel, int index)
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
