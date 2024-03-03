//
//  XDDSP_Oscillators.h
//  XDDSP2
//
//  Created by Adam Jackson on 27/5/2022.
//

#ifndef XDDSP_Oscillators_h
#define XDDSP_Oscillators_h

#include "XDDSP_Types.h"
#include "XDDSP_Blep.h"










namespace XDDSP2
{










/*
 This structure is used by the oscillator classes to communicate to wave generators
 
struct OscillatorState
{
 double phase;
 double unModPhase;
 double lastPhase;
 double lastT;
 double lastPhaseMod;
 double nextStepSize;
 double lastStepSize;
 uint64_t cycles;
 bool syncEvent;
 
 void reset()
 {
  syncEvent = false;
  unModPhase = 0.;
  cycles = 0;
  lastT = 0.;
 }
 
 void step(double phaseMod = 0.)
 {
  syncEvent = false;
  lastPhaseMod = phaseMod;
  lastPhase = phase;
  unModPhase += nextStepSize;
  phase = unModPhase + phaseMod;
  lastStepSize = phase - lastPhase;
  phase = phase - floor(phase);
  if (unModPhase >= 1.)
  {
   unModPhase -= 1.;
   ++cycles;
  }
 }
 
 // Call this on sample after sync event happens
 // t = time in samples since actual sync event happened
 void sync(double t)
 {
  syncEvent = true;
  cycles = 0;
  lastPhase = phase;
  unModPhase = t*nextStepSize;
  if (unModPhase < 0.)
  {
   unModPhase = 0.;
  }
  phase = unModPhase + lastPhaseMod;
  lastStepSize = phase - lastPhase;
  phase = phase - floor(phase);
 }
 
 // Call this when the oscillator is taking a time signal from another source
 // t can go on into infinity and this code will wrap it back between 0 and 1
 void time(double t, double phaseMod = 0.)
 {
  syncEvent = false;
  lastPhaseMod = phaseMod;
  lastPhase = phase;
  double tf = floor(t);
  unModPhase = t - tf;
  cycles = tf;
  phase = unModPhase + phaseMod;
  lastStepSize = phase - lastPhase;
  phase = phase - floor(phase);
  nextStepSize = t - lastT;
  lastT = t;
 }
};










template <
typename FrequencyIn,
typename PhaseModIn,
typename WaveGenerator
>
class FreeOscillator :
public Component<FreeOscillator<FrequencyIn, PhaseModIn, WaveGenerator>>
{
 static_assert(WaveGenerator::Count == FrequencyIn::Count || WaveGenerator::Count == 0, "WaveGenerator count is different to FrequencyIn count");
 static_assert(PhaseModIn::Count == FrequencyIn::Count, "PhaseModIn count is different to FrequencyIn count");
 // Private data members here
 Parameters &dspParam;
 std::array<OscillatorState, FrequencyIn::Count> state;
 
public:
 static constexpr int Count = FrequencyIn::Count;
 
 WaveGenerator wave;
 
 // Specify your inputs as public members here
 FrequencyIn frequencyIn;
 PhaseModIn phaseModIn;
 
 // Specify your outputs like this
 Output<Count> signalOut;
 
 // Include a definition for each input in the constructor
 FreeOscillator(Parameters &p, FrequencyIn _frequencyIn, PhaseModIn _phaseModIn, WaveGenerator _wave) :
 dspParam(p),
 wave(_wave),
 frequencyIn(_frequencyIn),
 phaseModIn(_phaseModIn),
 signalOut(p)
 {}
 
 void reset()
 {
  for (auto &s : state) s.reset();
  wave.reset();
  signalOut.reset();
 }
 
 void setPhase(int channel, SampleType phase)
 { state[channel].unModPhase = phase; }
 
 void setPhase(SampleType phase)
 { for (auto &s : state) s.unModPhase = phase; }
 
 void stepProcess(int startPoint, int sampleCount)
 {
  for (int c = 0; c < Count; ++c)
  {
   for (int i = startPoint, s = sampleCount; s--; ++i)
   {
    signalOut(c, i) = wave(c, i, state[c]);
    state[c].nextStepSize = frequencyIn(c, i)*dspParam.sampleInterval();
    state[c].step(phaseModIn(c, i));
   }
  }
 }
};










template <
typename FrequencyIn,
typename SyncSignalIn,
typename PhaseModIn,
typename WaveGenerator
>
class SyncableOscillator : public Component<SyncableOscillator<FrequencyIn, SyncSignalIn, PhaseModIn, WaveGenerator>>
{
 static_assert(WaveGenerator::Count == FrequencyIn::Count || WaveGenerator::Count == 0, "WaveGenerator count is different to FrequencyIn count");
 static_assert(FrequencyIn::Count == SyncSignalIn::Count || SyncSignalIn::Count == 1, "SyncSignalIn count is invalid: must be 1 or match FrequencyIn count");
 static_assert(PhaseModIn::Count == FrequencyIn::Count, "PhaseModIn count is different to FrequencyIn count");
 Parameters &dspParam;
 std::array<OscillatorState, FrequencyIn::Count> state;
 std::array<SampleType, SyncSignalIn::Count> syncSignalHistory;

 // Private data members here
public:
 static constexpr int Count = FrequencyIn::Count;

 WaveGenerator wave;

 // Specify your inputs as public members here
 FrequencyIn frequencyIn;
 SyncSignalIn syncSignalIn;
 PhaseModIn phaseModIn;
 
 // Specify your outputs like this
 Output<Count> signalOut;
 
 // Include a definition for each input in the constructor
 SyncableOscillator(Parameters &p,
                    FrequencyIn _frequencyIn,
                    SyncSignalIn _syncSignalIn,
                    PhaseModIn _phaseModIn,
                    WaveGenerator _wave) :
 dspParam(p),
 wave(_wave),
 frequencyIn(_frequencyIn),
 syncSignalIn(_syncSignalIn),
 phaseModIn(_phaseModIn),
 signalOut(p)
 {
  reset();
 }
 
 // This function is responsible for clearing the output buffers to a default state when
 // the component is disabled.
 void reset()
 {
  for (auto &s : state) s.reset();
  wave.reset();
  signalOut.reset();
  syncSignalHistory.fill(0.);
 }
 
 void stepProcess(int startPoint, int sampleCount)
 {
  for (int i = startPoint, s = sampleCount; s--; ++i)
  {
   if (SyncSignalIn::Count == 1)
   {
    // Detect sync events from one signal
    LinearEstimator intersect(syncSignalHistory[0], syncSignalIn(i));
    syncSignalHistory[0] = syncSignalIn(i);
    if (intersect.intersectionDirection() == -1)
    {
     // Apply them to every oscillator when they occur
     SampleType x = intersect.x();
     for (int c = 0; c < Count; ++c)
     {
      state[c].sync(x);
     }
    }
    for (int c = 0; c < Count; ++c)
    {
     signalOut(c, i) = wave(c, i, state[c]);
     state[c].nextStepSize = frequencyIn(c, i)*dspParam.sampleInterval();
     state[c].step(phaseModIn(c, i));
    }
   }
   else
   {
    for (int c = 0; c < Count; ++c)
    {
     // Detect sync events from each signal
     LinearEstimator intersect(syncSignalHistory[c], syncSignalIn(c, i));
     syncSignalHistory[c] = syncSignalIn(c, i);
     if (intersect.intersectionDirection() == -1)
     {
      // Apply them to each oscillator when they occur
      state[c].sync(intersect.x());
     }

     signalOut(c, i) = wave(c, i, state[c]);
     state[c].nextStepSize = frequencyIn(c, i)*dspParam.sampleInterval();
     state[c].step(phaseModIn(c, i));
    }
   }
  }
 }
};










template <
typename TimeIn,
typename PhaseModIn,
typename WaveGenerator
>
class TimeSyncedOscillator :
public Component<TimeSyncedOscillator<TimeIn, PhaseModIn, WaveGenerator>>
{
 static_assert(WaveGenerator::Count == TimeIn::Count || WaveGenerator::Count == 0, "WaveGenerator count is different to TimeIn count");
 static_assert(PhaseModIn::Count == TimeIn::Count, "PhaseModIn count is different to TimeIn count");

 // Private data members here
 SampleType multiplier {1.};
 std::array<OscillatorState, TimeIn::Count> state;

public:
 static constexpr int Count = TimeIn::Count;

 WaveGenerator wave;

 // Specify your inputs as public members here
 TimeIn timeIn;
 PhaseModIn phaseModIn;
 
 // Specify your outputs like this
 Output<Count> signalOut;
 
 // Include a definition for each input in the constructor
 TimeSyncedOscillator(Parameters &p,
                      TimeIn _timeIn,
                      PhaseModIn _phaseModIn,
                      WaveGenerator _wave) :
 wave(_wave),
 timeIn(_timeIn),
 phaseModIn(_phaseModIn),
 signalOut(p)
 {}
 
 // This function is responsible for clearing the output buffers to a default state when
 // the component is disabled.
 void reset()
 {
  for (auto &s : state) s.reset();
  wave.reset();
  signalOut.reset();
 }
 
 // stepProcess is called repeatedly with the start point incremented by step size
 void stepProcess(int startPoint, int sampleCount)
 {
  for (int c = 0; c < Count; ++c)
  {
   for (int i = startPoint, s = sampleCount; s--; ++i)
   {
    state[c].time(multiplier*timeIn(c, i), phaseModIn(c, i));
    signalOut(c, i) = wave(c, i, state[c]);
   }
  }
 }
 
 void setMultiplier(SampleType m)
 { multiplier = m; }
};










template <typename... InputPack>
class SwitchWave
{
 template <typename... Inputs>
 struct _SwitchContainer
 {
  static constexpr int Count = -1;
  static constexpr int Index = -1;
  
  SampleType fetch(int selection, int channel, int index, const OscillatorState &state)
  { return 0.; }
  
  void reset()
  {}
 };
 
 template <typename Input, typename... Inputs>
 struct _SwitchContainer<Input, Inputs...> : _SwitchContainer<Inputs...>
 {
  static constexpr int Count = Input::Count;
  static constexpr int Index = _SwitchContainer<Inputs...>::Index + 1;
  
  static_assert(_SwitchContainer<Inputs...>::Count == -1 ||
                _SwitchContainer<Inputs...>::Count == 0 ||
                Count == -1 ||
                Count == 0 ||
                _SwitchContainer<Inputs...>::Count == Count, "Channel count mismatch");
  
  _SwitchContainer(Input _input, Inputs... _inputs) :
  _SwitchContainer<Inputs...>(_inputs...), input(_input)
  {}
  
  SampleType fetch(int selection, int channel, int index, const OscillatorState &state)
  {
   if (selection == Index) return input(channel, index, state);
   return _SwitchContainer<Inputs...>::fetch(selection, channel, index, state);
  }
  
  void reset()
  {
   input.reset();
   _SwitchContainer<Inputs...>::reset();
  }
  
  Input input;
 };
 
 int selected {0};
 _SwitchContainer<InputPack...> switchContainer;
 
 template <std::size_t, class> struct elem_type_holder;
 
 template <class T, class... Ts>
 struct elem_type_holder<0, _SwitchContainer<T, Ts...>>
 {
  typedef T type;
 };
 
 template <std::size_t k, class T, class... Ts>
 struct elem_type_holder<k, _SwitchContainer<T, Ts...>>
 {
  typedef typename elem_type_holder<k - 1, _SwitchContainer<Ts...>>::type type;
 };
 
 template <std::size_t k, class... Ts>
 typename std::enable_if<k == 0, typename elem_type_holder<0, _SwitchContainer<Ts...>>::type&>::type get(_SwitchContainer<Ts...>& t)
 {
  return t.input;
 }
 
 template <std::size_t k, class T, class... Ts>
 typename std::enable_if<k != 0, typename elem_type_holder<k, _SwitchContainer<T, Ts...>>::type&>::type get(_SwitchContainer<T, Ts...>& t)
 {
  _SwitchContainer<Ts...>& base = t;
  return get<k - 1>(base);
 }
 
public:
 static constexpr int Count = _SwitchContainer<InputPack...>::Count;
 
 SwitchWave(InputPack... inputs) :
 switchContainer(inputs...)
 {}
 
 template <std::size_t k, typename T>
 T& input()
 { return get<k>(switchContainer); }
 
 void select(int s)
 {
  dsp_assert(s >= 0 && s < sizeof...(InputPack));
  selected = sizeof...(InputPack) - s - 1;
 }
 
 SampleType operator()(int channel, int index, const OscillatorState &state)
 { return switchContainer.fetch(selected, channel, index, state); }
 
 void reset()
 { return switchContainer.reset(); }
};










class SineWaveGenerator
{
public:
 static constexpr int Count = 0;
 
 void reset()
 {}
 
 SampleType operator()(int channel, int index, const OscillatorState &state)
 {
  return std::sin(2.*state.phase*M_PI);
 }
};










class NaiveSawWaveGenerator
{
public:
 static constexpr int Count = 0;
 
 void reset()
 {}
 
 SampleType operator()(int channel, int index, const OscillatorState &state)
 {
  return 1. - 2.*state.phase;
 }
};










class NaiveRampWaveGenerator
{
public:
 static constexpr int Count = 0;
 
 void reset()
 {}
 
 SampleType operator()(int channel, int index, const OscillatorState &state)
 {
  return -1. + 2.*state.phase;
 }
};










template <typename PulseWidthIn>
class NaiveSquareWaveGenerator
{
public:
 static constexpr int Count = (PulseWidthIn::Count == 1) ? 0 : PulseWidthIn::Count;
 
 PulseWidthIn pulseWidthIn;
 
 NaiveSquareWaveGenerator(PulseWidthIn pulseWidthIn) :
 pulseWidthIn(pulseWidthIn)
 {}
 
 void reset()
 {}
 
 SampleType operator()(int channel, int index, const OscillatorState &state)
 {
  if (Count) return signum(state.phase - pulseWidthIn(channel, index));
  return signum(state.phase - pulseWidthIn(index));
 }
};










class NaiveTriangleWaveGenerator
{
public:
 static constexpr int Count = 0;
 
 void reset()
 {}
 
 SampleType operator()(int channel, int index, const OscillatorState &state)
 {
  // Correct the phase of the triangle wave (out of phase by 90 degrees)
  SampleType phase = state.phase + 0.25;
  phase -= floor(phase);
  return ((phase > 0.5) ? 3. - 4.*phase : -1. + 4*phase);
 }
};










struct WaveTableInterpolation
{
 enum
 {
  None,
  Linear,
  Hermite
 };
};

template <int Quality>
class WaveTable
{
 std::vector<std::vector<SampleType>> table;
 
 PowerSize tableSize;
 SampleType tableScale;
 SampleType tableStepScale;
 
 SampleType tableLookup(const std::vector<SampleType> &samples, SampleType index)
 {
  SampleType result = 0.;
  
  IntegerAndFraction iaf(index);
  
  switch (Quality)
  {
   case WaveTableInterpolation::None:
   {
    result = samples[iaf.intRep() & tableSize.mask()];
   }
    break;
    
   case WaveTableInterpolation::Linear:
   {
    SampleType x0 = samples[iaf.intRep() & tableSize.mask()];
    SampleType x1 = samples[(iaf.intRep() + 1) & tableSize.mask()];
    result = LERP(iaf.fracPart(), x0, x1);
   }
    break;
    
   case WaveTableInterpolation::Hermite:
   {
    SampleType xm1 = samples[(iaf.intRep() - 1) & tableSize.mask()];
    SampleType x0 = samples[iaf.intRep() & tableSize.mask()];
    SampleType x1 = samples[(iaf.intRep() + 1) & tableSize.mask()];
    SampleType x2 = samples[(iaf.intRep() + 2) & tableSize.mask()];
    result = hermite(iaf.fracPart(), xm1, x0, x1, x2);
   }
    break;
  }
  
  return result;
 }
 
 void addToTable()
 {
  table.push_back(std::vector<SampleType>(tableSize.size()));
 }
 
public:
 
 WaveTable()
 { setTableSizeBits(8); }
 
 WaveTable(int sizeBits)
 { setTableSizeBits(sizeBits); }
 
 void setTableSizeBits(int bits)
 {
  tableSize.setBits(bits);
  table.clear();
  tableScale = 1./tableSize.size();
 }
 
 void addWave(const std::vector<SampleType> &samples)
 {
  dsp_assert(samples.size() > 0);
  
  addToTable();
  
  double scale = samples.size()*tableScale;
  double index = 0.;
  for (int i = 0; i < tableSize.size(); ++i, index += scale)
  {
   table.back()[i] = tableLookup(samples, index);
  }
 }
 
 // Calls the function between 0 and 1 to generate a waveform
 void addWave(WaveformFunction func)
 {
  addToTable();
  double index = 0.;
  for (int i = 0; i < tableSize.size(); ++i, index += tableScale)
  {
   table.back()[i] = func(index);
  }
 }
 
 SampleType readTable(SampleType tableIndex, SampleType tableStep)
 {
  SampleType result = 0.;
  
  tableIndex *= tableSize.size();
  
  if (table.size() == 1)
  {
   result = tableLookup(table[0], tableIndex);
  }
  else if (table.size() > 1)
  {
   IntegerAndFraction iaf(tableStep*(table.size() - 1));
   if (iaf.fracPart() == 0.)
   {
    result = tableLookup(table[iaf.intRep()], tableIndex);
   }
   else
   {
    SampleType x0 = tableLookup(table[iaf.intRep()], tableIndex);
    SampleType x1 = tableLookup(table[iaf.intRep() + 1], tableIndex);
    result = LERP(iaf.fracPart(), x0, x1);
   }
  }
  
  return result;
 }
};

typedef WaveTable<WaveTableInterpolation::None> LowQualityWaveTable;
typedef WaveTable<WaveTableInterpolation::Linear> MediumQualityWaveTable;
typedef WaveTable<WaveTableInterpolation::Hermite> HighQualityWaveTable;










template <typename TableStepIn, typename WaveTableType>
class WaveTableWaveGenerator
{
 WaveTableType *table {nullptr};
public:
 static constexpr int Count = 0;
 
 TableStepIn tableStepIn;
 
 WaveTableWaveGenerator(TableStepIn tableStepIn) :
 tableStepIn(tableStepIn)
 {}
 
 WaveTableWaveGenerator(const WaveTableWaveGenerator<TableStepIn, WaveTableType> &rhs) :
 tableStepIn(rhs.tableStepIn)
 {}
 
 void setTable(WaveTableType *t)
 { table = t; }
 
 void reset()
 {}
 
 SampleType operator()(int channel, int index, const OscillatorState &state)
 {
  SampleType result = 0.;
  if (table)
  {
   result = table->readTable(state.phase, tableStepIn(index));
  }
  return result;
 }
};

template <typename TableStepIn>
using LowQualityWaveGenerator = WaveTableWaveGenerator<TableStepIn, LowQualityWaveTable> ;

template <typename TableStepIn>
using MediumQualityWaveGenerator = WaveTableWaveGenerator<TableStepIn, MediumQualityWaveTable> ;

template <typename TableStepIn>
using HighQualityWaveGenerator = WaveTableWaveGenerator<TableStepIn, HighQualityWaveTable> ;










class RandomHoldWaveGenerator
{
 RandomNumberBuffer noise;
 
public:
 static constexpr int Count = 0;
 
 void reset()
 {}
 
 SampleType operator()(int channel, int index, const OscillatorState &state)
 {
  return noise.lookupRandomNumber(static_cast<int>(state.cycles));
 }
};










class RandomGlideWaveGenerator
{
 RandomNumberBuffer noise;
 
public:
 static constexpr int Count = 0;
 
 void reset()
 {}
 
 SampleType operator()(int channel, int index, const OscillatorState &state)
 {
  SampleType x0 = noise.lookupRandomNumber(static_cast<int>(state.cycles));
  SampleType x1 = noise.lookupRandomNumber(static_cast<int>(state.cycles + 1));
  return LERP(state.phase, x0, x1);
 }
};









*/

}

#endif /* XDDSP_Oscillators_h */
