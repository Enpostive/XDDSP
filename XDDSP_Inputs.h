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










  /**
   * @brief This is a simple straight connector to another coupler.
   * 
   * @tparam Source The class of the coupler being connected to.
   */
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










/**
 * @brief A connector which can be connected or reconnected to another coupler type at run time.
 * 
 * @tparam ChannelCount The number of channels to provide
 */
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
 
 /**
  * @brief Connect to a coupler.
  * 
  * @tparam Source The class of the coupler object. This is usually inferred from the type of the parameter.
  * @param _connection The coupler object to connect to.
  */
 template <typename Source>
 void connect(Source &_connection)
 {
  connection = &_connection;
  gm = [](void* obj, int ch, int i){ return (*static_cast<Source*>(obj))(ch, i); };
 }
 
 /**
  * @brief Disconnects from the coupler connected to, if any. Disconnected connectors return 0 values.
  * 
  */
 void disconnect()
 {
  connection = nullptr;
  gm = nullptr;
 }
 
 /**
  * @brief Is called from the Coupler base class to fetch the actual sample
  * 
  * @param channel The selected channel
  * @param index The index of the sample to get
  * @return SampleType The sample at the index in that channel is returned
  */
 SampleType get(int channel, int index)
 { return gm ? gm(connection, channel, index) : 0.0; }


};









/**
 * @brief An input for picking one channel from a multi-channel input
 * 
 * @tparam Source The class of the coupler being connected to.
 * @tparam Channel The channel to pick
 * @tparam OutputChannelCount Duplicate the picked channel into this many channels (default 1)
 */
template <typename Source, int Channel, int OutputChannelCount = 1>
class ChannelPicker final : public Coupler<ChannelPicker<Source, Channel, OutputChannelCount>, OutputChannelCount>
{
 Source &connection;
 
public:
 /**
  * @brief Is called from the Coupler base class to fetch the actual sample
  * 
  * @param channel The selected channel
  * @param index The index of the sample to get
  * @return SampleType The sample at the index in that channel is returned
  */
 SampleType get(int channel, int index)
 { return connection(Channel, index); }
 
 static constexpr int Count = OutputChannelCount;
 
 ChannelPicker(Source &_connection) :
 connection(_connection)
 {}
 
 ChannelPicker(const ChannelPicker<Source, Channel, OutputChannelCount> &rhs) :
 connection(rhs.connection)
 {}
};










/**
 * @brief A coupler for coupling buffers that are different types (eg. connecting float to double)
 * 
 * This object is meant to be used where connecting the DSP network to another network which uses a different floating point type. The size of the buffer is expected to be the same as the size used by the component using the input and no checking is performed!
 * 
 * @tparam BufferSampleType The type of the buffer being connected to
 * @tparam ChannelCount The number of channels to make available
 */
template <typename BufferSampleType, int ChannelCount = 1>
class BufferCoupler final : public Coupler<BufferCoupler<BufferSampleType, ChannelCount>, ChannelCount>
{
 std::array<BufferSampleType*, ChannelCount> p;
 
public:
 /**
  * @brief Is called from the Coupler base class to fetch the actual sample
  * 
  * @param channel The selected channel
  * @param index The index of the sample to get
  * @return SampleType The sample at the index in that channel is returned
  */
 SampleType get(int channel, int index)
 {
  dsp_assert(channel >= 0 && channel < Count);
  dsp_assert(p[channel] != nullptr);
  return p[channel][index];
 }
 
 static constexpr int Count = ChannelCount;
 
 /**
  * @brief Construct a new Buffer Coupler object with nothing connected. Use connect to make the connection before reading any samples!
  * 
  */
 BufferCoupler()
 {
  p.fill(nullptr);
 }
 
 /**
  * @brief Construct a new Buffer Coupler object from an array of pointers, each pointing to one channel. The size of each buffer is expected to be the same as the size used by the component using the input and no checking is performed!
  * 
  * @param _p 
  */
 explicit BufferCoupler(const std::array<BufferSampleType*, ChannelCount> &_p) :
 p(_p)
 {}
 
 /**
  * @brief Construct a copy of the Buffer Coupler object
  * 
  * @param rhs The other object to copy
  */
 BufferCoupler(const BufferCoupler<BufferSampleType, ChannelCount> &rhs) :
 p(rhs.p)
 {}
 
 /**
  * @brief Connect the coupler to the buffers pointed to in _p. The size of each buffer is expected to be the same as the size used by the component using the input and no checking is performed!
  * 
  * @param _p An array of pointers to buffers
  */
 void connect(const std::array<BufferSampleType*, ChannelCount> &_p)
 { p = _p; }
 
 /**
  * @brief Connect every channel to the same buffer. The size of the buffer is expected to be the same as the size used by the component using the input and no checking is performed!
  * 
  * @param _p A pointer to the buffer
  */
 void connect(BufferSampleType* _p)
 { for (auto& pp : p) pp = _p; }
 
 /**
  * @brief Connect one channel to one buffer. The size of the buffer is expected to be the same as the size used by the component using the input and no checking is performed!
  * 
  * @param channel The channel to connect
  * @param _p A pointer to the buffer
  */
 void connect(int channel, BufferSampleType* _p)
 {
  dsp_assert(channel >= 0 && channel < Count);
  dsp_assert(_p != nullptr);
  p[channel] = _p;
 }
};










/**
 * @brief A coupler which outputs a constant signal on each channel.
 *        A modifier function may be given to modify given control values before they enter the DSP network.
 * 
 * @tparam ConstantCount The number of channels to make available.
 */
template <int ConstantCount = 1>
class ControlConstant final : public Coupler<ControlConstant<ConstantCount>, ConstantCount>
{
 static_assert(ConstantCount > 0, "ControlConstant must have at least one channel");
 
 std::array<SampleType, ConstantCount> setting;
 std::array<SampleType, ConstantCount> c;
 
public:
 /**
  * @brief Is called from the Coupler base class to fetch the actual sample
  * 
  * @param channel The selected channel
  * @param index The index of the sample to get
  * @return SampleType The sample at the index in that channel is returned
  */
 SampleType get(int channel, int index)
 {
  return c[channel];
 }
 
 static constexpr int Count = ConstantCount;

 /**
  * @brief Provide a modifier function which is called to modify given control values before they enter the DSP network.
  * 
  */
 WaveformFunction func;
 
 /**
  * @brief Construct a new Control Constant object and set the initial output value to 0. This initial value is not modified by the modifier function.
  * 
  */
 ControlConstant()
 {
  setControl(0.);
 }
 
 /**
  * @brief Construct a new Control Constant object and set the initial output value. This initial value is not modified by the modifier function.
  * 
  * @param c The value to output.
  */
 ControlConstant(SampleType c)
 { setControl(c); }
 
 /**
  * @brief Construct a copy of another Control Constant object
  * 
  * @param rhs The object to copy
  */
 ControlConstant(const ControlConstant<ConstantCount> &rhs) :
 setting(rhs.setting),
 c(rhs.c),
 func(rhs.func)
 {}
 
 /**
  * @brief Returns the original unmodified control value.
  * 
  * @param channel The channel to look at
  * @return SampleType The original unmodified control value.
  */
 SampleType getControl(int channel)
 { return setting.at(channel); }
 
 /**
  * @brief Returns the original unmodified control value for channel 0
  * 
  * @return SampleType The original unmodified control value for channel 0
  */
 SampleType getControl()
 { return getControl(0); }
 
 /**
  * @brief Set a control value for one channel.
  *        The control value is modified by the modifier function before being sent into the DSP network.
  * 
  * @param channel The channel to set.
  * @param control The value to set. This value is modified by the modifier function.
  */
 void setControl(int channel, SampleType control)
 {
  setting.at(channel) = control;
  if (func) control = func(control);
  c.at(channel) = control;
 }
 
 /**
  * @brief Set a control value for all channels.
  *        The control value is modified by the modifier function before being sent into the DSP network.
  * 
  * @param control The value to set. This value is modified by the modifier function.
  */
 void setControl(SampleType control)
 {
  for (int i = 0; i < Count; ++i) setControl(i, control);
 }
 
 /**
  * @brief Recalculate the modified control values. Deprecated as no longer required.
  * 
  */
 void refreshControl()
 {
  if (func) for (int i = 0; i < Count; ++i) c[i] = func(setting[i]);
 }
};










/**
 * @brief These are the different modes available to be used in AudioPropertiesInput
 * 
 */
struct AudioPropertiesInputMode
{
 enum InputMode
 {
  BeatsPerMinute, // Reports the song tempo in BPM
  LengthQuarterNoteSeconds, // Reports the length of a quarter note in seconds
  LengthQuarterNoteSamples, // Reports the length of a quarter note in samples
  FrequencyQuaterNote, // Reports the song tempo in Hz
  SamplesPerSecond, // Reports the sample rate
  SecondsPerSample // Reports the sample interval, that is the reciprocal of the sample rate.
 };
};

/**
 * @brief A coupler for producing signals based on a chosen DSP Parameter (such as sample rate or song tempo)
 *        This coupler is similar to ControlConstant, except that the control signal is taken from the Parameters Object and scaled by a multiplier.
 * 
 * @tparam TimeMode See AudioPropertiesInputMode for a list of available modes.
 * @tparam ChannelCount The number of channels to be made available (all channels output the same signal).
 */
template <int TimeMode, int ChannelCount = 1>
class AudioPropertiesInput final : public Coupler<AudioPropertiesInput<TimeMode, ChannelCount>, ChannelCount>
{
 static constexpr SampleType PerMinute = 1./60.;
 
 Parameters &dspParam;
 SampleType multiplier {1.};
 
public:
 /**
  * @brief Is called from the Coupler base class to fetch the actual sample
  * 
  * @param channel The selected channel
  * @param index The index of the sample to get
  * @return SampleType The sample at the index in that channel is returned
  */
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
 
 /**
  * @brief Construct a new Audio Properties Input object
  * 
  * @param p The Parameters object which the signals are taken from.
  */
 AudioPropertiesInput(Parameters &p) : dspParam(p)
 {}
 
 /**
  * @brief Construct a new Audio Properties Input object and set the multiplier
  * 
  * @param p The Parameters object which the signals are taken from.
  * @param _m The multiplier to use.
  */
 AudioPropertiesInput(Parameters &p, SampleType _m) : dspParam(p), multiplier(_m)
 {}
 
 /**
  * @brief Construct a copy of another Audio Properties Input object
  * 
  * @param rhs The object to copy
  */
 AudioPropertiesInput(const AudioPropertiesInput<TimeMode> &rhs) :
 dspParam(rhs.dspParam),
 multiplier(rhs.multiplier)
 {}
 
 /**
  * @brief Get the current multiplier
  * 
  * @return SampleType The multiplier
  */
 SampleType getMultiplier()
 { return multiplier;}
 
 /**
  * @brief Set the multiplier
  * 
  * @param _m The new multiplier
  */
 void setMultiplier(SampleType _m)
 { multiplier = _m; }
};










/**
 * @brief A base class which encapsulates code for variadic couplers. User applications won't need to instantiate this directly.
 * 
 * @tparam NoConnections 
 * @tparam ChannelCount 
 */
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
 
 /**
  * @brief Construct a new coupler with the required number of inputs.
  *        Each input must have the same number of channels as the output.
  * 
  * @tparam Inputs Template argument which is inferred from the parameters given.
  * @param ins Variadic parameter where the inputs are provided for the coupler. Each input must have the same number of channels as the output.
  */
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










/**
 * @brief A coupler which takes multiple coupler inputs and provides a switch to choose one input for it's output.
 *        Each input must have the same number of channels as the output.
 * 
 * @tparam NoConnections The number of connections to expect.
 * @tparam ChannelCount The number of channels to make available.
 */
template <int NoConnections, int ChannelCount = 1>
class Switch final : public Coupler<Switch<NoConnections, ChannelCount>, ChannelCount>, MultiIn<NoConnections, ChannelCount>
{
 int selected;
 using Base = MultiIn<NoConnections, ChannelCount>;
 
public:
 using Base::Base;
 
 /**
  * @brief Is called from the Coupler base class to fetch the actual sample
  * 
  * @param channel The selected channel
  * @param index The index of the sample to get
  * @return SampleType The sample at the index in that channel is returned
  */
 SampleType get(int channel, int index)
 {
  return this->connections[selected](channel, index);
 }
 
 static constexpr int Count = ChannelCount;
 
 /**
  * @brief Switch to another input. The changeover is instantaneous, be careful of discontinuities appearing in the signal.
  * 
  * @param s The input to switch to.
  */
 void select(int s)
 {
  dsp_assert(s >= 0 && s < NoConnections);
  selected = s;
 }
 
 /**
  * @brief Get the current selection.
  * 
  * @return int The current selection.
  */
 int getSelection() const
 { return selected; }
};










/**
 * @brief A coupler which sums it's coupled inputs.
 *        Each input must have the same number of channels as the output.
 * 
 * @tparam NoConnections The number of connections to expect.
 * @tparam ChannelCount The number of channels to make available.
 */
template <int NoConnections, int ChannelCount = 1>
class Sum final : public Coupler<Sum<NoConnections, ChannelCount>, ChannelCount>, MultiIn<NoConnections, ChannelCount>
{
 using Base = MultiIn<NoConnections, ChannelCount>;
 
public:
 using Base::Base;

 /**
  * @brief Is called from the Coupler base class to fetch the actual sample
  * 
  * @param channel The selected channel
  * @param index The index of the sample to get
  * @return SampleType The sample at the index in that channel is returned
  */
 SampleType get(int channel, int index)
 {
  SampleType sum = 0.;
  for (auto& c : this->connections) sum += c(channel, index);
  return sum;
 }
 
 static constexpr int Count = ChannelCount;
};










/**
 * @brief A coupler which multiplies it's coupled inputs together to produce the output.
 *        Each input must have the same number of channels as the output.
 * 
 * @tparam NoConnections The number of connections to expect.
 * @tparam ChannelCount The number of channels to make available.
 */
template <int NoConnections, int ChannelCount = 1>
class Product final : public Coupler<Product<NoConnections, ChannelCount>, ChannelCount>, MultiIn<NoConnections, ChannelCount>
{
 using Base = MultiIn<NoConnections, ChannelCount>;
 
public:
 using Base::Base;

 /**
  * @brief Is called from the Coupler base class to fetch the actual sample
  * 
  * @param channel The selected channel
  * @param index The index of the sample to get
  * @return SampleType The sample at the index in that channel is returned
  */
 SampleType get(int channel, int index)
 {
  SampleType prod = 1.;
  for (auto& c : this->connections) prod *= c(channel, index);
  return prod;
 }
 
 static constexpr int Count = ChannelCount;
};










/**
 * @brief A signal modifier coupler which applies a specified function to it's input.
 *        This coupler is meant to be a convenient way to apply a simple stateless function, such as a waveshaper, to a signal. The function may be called multiple times for any sample and may be called out of order.
 * 
 * @tparam Source The class of the input coupler.
 * @tparam ChannelCount The number of channels to make available.
 */
template <typename Source, int ChannelCount = 1>
class SignalModifier final : public Coupler<SignalModifier<Source, ChannelCount>, ChannelCount>
{
 Source &connection;
 
public:
 /**
  * @brief Is called from the Coupler base class to fetch the actual sample
  * 
  * @param channel The selected channel
  * @param index The index of the sample to get
  * @return SampleType The sample at the index in that channel is returned
  */
 SampleType get(int channel, int index)
 {
  if (func) return func(connection(channel, index));
  return connection(channel, index);
 }
 
 static constexpr int Count = ChannelCount;

 /**
  * @brief Provide a modifier function which is called to modify the input signal.
  * 
  */
 WaveformFunction func;

 SignalModifier(Source &_connection) :
 connection(_connection)
 {}
 
 SignalModifier(const SignalModifier &rhs) :
 connection(rhs.connection),
 func(rhs.func)
 {}
};










/**
 * @brief A coupler which outputs samples from a sample buffer.
 * 
 * This coupler takes an input signal and interprets that as a sample index into a sample. The sample can be of any floating point type and interpolation is performed to return values between samples. Bounds checking is also provided. Unconnected channels return zero samples. All connected channels are expected to be the same size.
 * 
 * @tparam Source The class of the input coupler.
 * @tparam BufferSampleType The type for the sample.
 * @tparam ChannelCount The number of channels to make available.
 * @tparam Quality The desired quality level. Low quality is nearest neighbour interpolation. Mid quality is linear interpolation. High quality is hermite interpolation.
 */
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
 /**
  * @brief Is called from the Coupler base class to fetch the actual sample
  * 
  * @param channel The selected channel
  * @param index The index of the sample to get
  * @return SampleType The sample at the index in that channel is returned
  */
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
 
 /**
  * @brief Connect one channel to the playback head.
  * 
  * @param channel The channel to connect.
  * @param ptr The pointer to the buffer containing the sample content.
  */
 void connectChannel(int channel, const BufferSampleType *ptr)
 {
  dsp_assert(channel >= 0 && channel < ChannelCount);
  buffer[channel] = ptr;
 }
 
 /**
  * @brief Get the current length of the sample
  * 
  * @return std::size_t The length of the sample
  */
 std::size_t length()
 { return bufferSize + 1; }
 
 /**
  * @brief Set the length of the sample.
  * 
  * @param lengthSamples The length of the sample in samples.
  */
 void setLength(std::size_t lengthSamples)
 {
  bufferSize = lengthSamples - 1;
  bufferLength = static_cast<SampleType>(bufferSize);
 }
};










/**
 * @brief A coupler for connecting various length buffers to each channel.
 *        Each buffer is expected to be the same type but can have different lengths. Bounds checking is performed.
 * 
 * @tparam BufferSampleType 
 * @tparam ChannelCount 
 */
template <typename BufferSampleType, int ChannelCount = 1>
class BufferReader final : public Coupler<BufferReader<BufferSampleType, ChannelCount>, ChannelCount>
{
 std::array<const BufferSampleType*, ChannelCount> buffer;
 std::array<int, ChannelCount> length;
 
public:
 /**
  * @brief Is called from the Coupler base class to fetch the actual sample
  * 
  * @param channel The selected channel
  * @param index The index of the sample to get
  * @return SampleType The sample at the index in that channel is returned
  */
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
 
 /**
  * @brief Connect one channel to one buffer.
  * 
  * @param channel The channel to connect.
  * @param ptr The pointer to the buffer.
  * @param len The length of the buffer.
  */
 void connectChannel(int channel, const BufferSampleType *ptr, int len)
 {
  dsp_assert(channel >= 0 && channel < ChannelCount);
  dsp_assert(len >= 0);
  buffer[channel] = ptr;
  length[channel] = len;
 }
};










/**
 * @brief A coupler which is convenient for connecting as the input for the entire network.
 *        Simple bounds checking is provided in debug mode to alert the developer if samples outside of the buffer are being requested.
 * 
 * @tparam ChannelCount The number of channels to make available.
 */
template <int ChannelCount = 2>
class PluginInput final : public Coupler<PluginInput<ChannelCount>, ChannelCount>
{
 std::array<float*, ChannelCount> floatInputs;
 std::array<double*, ChannelCount> doubleInputs;
 
 int length {0};
 
public:
 /**
  * @brief Is called from the Coupler base class to fetch the actual sample
  * 
  * @param channel The selected channel
  * @param index The index of the sample to get
  * @return SampleType The sample at the index in that channel is returned
  */
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
 
 /**
  * @brief Connect the coupler to float buffers.
  * 
  * @param p An array of pointers to buffers of float type
  * @param sampleCount The number of samples available to be processed
  */
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
