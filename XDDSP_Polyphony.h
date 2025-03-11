/*
  ==============================================================================

    XDDSP_Polyphony.h
    Created: 11 Apr 2023 9:45:05am
    Author:  Adam Jackson

  ==============================================================================
*/

#ifndef XDDSP_Polyphony_h
#define XDDSP_Polyphony_h

#include "XDDSP_Types.h"
#include "XDDSP_Parameters.h"
#include "XDDSP_Classes.h"
#include "XDDSP_Functions.h"











namespace XDDSP
{










// An extension of Parameters that contains parameters suitable for a MIDI polyphonic synthesiser
class PolySynthParameters : public Parameters
{
 SampleType tuning {440.};
 SampleType portTime {0.};
 bool glissandoSetting {false};
 bool legatoSetting {false};
 int pbr {2};
 
public:
 // using MIDILookup = LookupTable<127>;
 using MIDILookup = LookupTable<127>;
 
 MIDILookup midiNoteFreq;
 
 
 PolySynthParameters()
 { setTuning(440.); }
 
 virtual ~PolySynthParameters() {}
 
 void setTuning(SampleType a)
 {
  if (a > 0.)
  {
   tuning = a;
   midiNoteFreq.boundaries.setMinMax(0., 127.);
   midiNoteFreq.calculateTable([=](SampleType note)
                               {
    return a*semitoneRatio(note - ABeforeMiddleC);
   });
  }
 }
 
 void setPitchBendRange(int pbr)
 { if (pbr >= 0) pbr = pbr; }
 
 void setGlissando(bool g)
 { glissandoSetting = g; }
 
 void setLegato(bool l)
 {
  legatoSetting = l;
  updateCustomParameter(Parameters::BuiltinParameterCategory,
                        Parameters::BuiltinCustomParameters::Legato);
 }
 
 void setPortamenteauTime(SampleType t)
 { if (t >= 0.) portTime = t; }
 
 int pitchBendRange() const
 { return pbr; }
 
 SampleType portamenteauTime() const
 { return portTime; }
 
 int portTimeSamples() const
 { return portTime*sampleRate(); }
 
 bool legato() const
 { return legatoSetting; }
 
 bool glissando() const
 { return glissandoSetting; }
};










// See the TestVoice template in XDDSP.h
template <typename InternalComponent, int ComponentCount>
class SummingArray : public Component<SummingArray<InternalComponent, ComponentCount>>
{
 // Private data members here
 std::array<InternalComponent, ComponentCount> components;
 
public:
 static constexpr int Count = InternalComponent::Count;
 static constexpr int CountComponents = ComponentCount;
 
 // Specify your outputs like this
 Output<Count> sumOut;
 
 // Include a definition for each input in the constructor
 SummingArray(Parameters &p) :
 components(makeComponentArray<ComponentCount, InternalComponent>(p)),
 sumOut(p)
 {}
 
 // This function is responsible for clearing the output buffers to a default state when
 // the component is disabled.
 void reset()
 {
  for (auto &c : components) c.reset();
  sumOut.reset();
 }
 
 // stepProcess is called repeatedly with the start point incremented by step size
 void stepProcess(int startPoint, int sampleCount)
 {
  for (auto &j : components) j.process(startPoint, sampleCount);
  
  for (int i = startPoint, s = sampleCount; s--; ++i)
  {
   for (int c = 0; c < Count; ++c)
   {
    SampleType sum = 0.;
    for (auto &j : components) sum += j.signalOut(c, i);
//    if (isnan(sum))
//    {
//     ;
//    }
    sumOut.buffer(c, i) = sum;
   }
  }
 }
 
 InternalComponent& operator[](unsigned int i)
 {
  return components[i];
 }
 
 auto begin()
 {
  return components.begin();
 }
 
 auto end()
 {
  return components.end();
 }
};










template <int OutputCount, int RampLengthms = 5>
class MIDIScheduler : public Component<MIDIScheduler<OutputCount, RampLengthms>>, public Parameters::ParameterListener
{
 SampleType smoothFactor;
 
 std::array<SampleType, OutputCount> value;
 std::array<SampleType, OutputCount> target;
 
 struct MIDIEvent
 {
  SampleType newValue;
  int samplePosition;
  int channel;
 };
 
 std::vector<MIDIEvent> schedule;
 
public:
 static constexpr int Count = OutputCount;
 
 Output<Count> signalOut;
 
 MIDIScheduler(Parameters &p) :
 Component<MIDIScheduler>(p),
 Parameters::ParameterListener(p)
 {
  schedule.reserve(100);
  updateSampleRate(p.sampleRate(), p.sampleInterval());
 }
 
 void addEvent(int channel, SampleType newValue, int samplePosition)
 {
  auto it = schedule.begin();
  while ((it != schedule.end()) && (it->samplePosition < samplePosition)) ++it;
  schedule.insert(it, {newValue, samplePosition, channel});
 }
 
 void stepProcess(int startPoint, int sampleCount)
 {
  auto it = schedule.begin();
  int i, s;
  
  for (i = startPoint, s = sampleCount;
       (it != schedule.end()) && (it->samplePosition < sampleCount) && (s--);
       ++i)
  {
   if (it->samplePosition == i)
   {
    target[it->channel] = it->newValue;
    ++it;
   }
   for (int c = 0; c < Count; ++c)
   {
    expTrack(value[c], target[c], smoothFactor);
    signalOut.buffer(c, i) = value[c];
   }
  }
  
  for (; s >= 0; --s, ++i)
  {
   for (int c = 0; c < Count; ++c)
   {
    expTrack(value[c], target[c], smoothFactor);
    signalOut(c, i) = value[c];
   }
  }
  
  it = schedule.erase(schedule.begin(), it);
 }
 
 void advanceMidiEvents(int sampleCount)
 {
  auto it = schedule.begin();
  while (it != schedule.end())
  {
   it->samplePosition -= sampleCount;
   ++it;
  }
 }

 virtual void updateSampleRate(SampleType sr, SampleType isr) override
 { smoothFactor = expCoef(0.001*RampLengthms*sr); }
};










template <typename VoiceComponent, int MaxVoiceCount, int AutoEnable = 0>
class MIDIPoly : public Parameters::ParameterListener
{
 SummingArray<VoiceComponent, MaxVoiceCount> &voiceArray;
 PolySynthParameters &polyParam;

 
 
 

 struct NoteSchedule
 {
  enum
  {
   PitchBend = -1,
   AllNotesOff = -2,
   AllSoundOff = -3
  };
  
  int note;
  int velocity;
  int samplePosition;
  
  NoteSchedule(int n, int v, int s) :
  note(n), velocity(v), samplePosition(s) {}
 };

 
 
 
 
 struct Voice
 {
  int noteNumber {0};
  bool noteOn {false};
  std::vector<VoiceComponent*> voiceComponents;
  
  void startNote(int ni, SampleType note, SampleType onVel, SampleType lastNote, int portTime, bool retrigger)
  {
   noteNumber = ni;
   if (!noteOn) retrigger = true;
   for (auto voice: voiceComponents)
   {
    if (lastNote > 0) voice->noteIn.setControl(lastNote);
    voice->noteIn.setRamp(0, portTime, note);
    voice->velocityIn.setControl(onVel);
    if (retrigger)
    {
     voice->noteOn();
     noteOn = true;
     if (AutoEnable) voice->setEnabled(true);
    }
   }
  }
  
  void stopNote()
  {
   for (auto voice: voiceComponents)
   {
    voice->noteOff();
   }
   noteOn = false;
  }
  
  bool isActive() { return voiceComponents[0]->isActive(); }
  
  void killNote()
  {
   for (auto &voice: voiceComponents)
   {
    voice->noteStop();
    if (AutoEnable) voice->setEnabled(false);
   }
  }
  
  void setEnabled(bool e)
  {
   for (auto &voice: voiceComponents)
   {
    voice->setEnabled(e);
   }
  }
 };
 
 
 
 
 
 std::array<std::unique_ptr<Voice>, MaxVoiceCount> voices;
 std::vector<NoteSchedule> schedule;
 std::vector<int> voiceOrder;
 SampleType lastNote;
 
 int voiceLimit {MaxVoiceCount};
 int voiceCount {MaxVoiceCount};
 int unison {1};

 
 
 
 
 void allocateVoiceAndStart(const NoteSchedule &ns)
 {
  // Keep track of which voice we are allocating, and how many voices have notes on
  int allocated = voiceCount;
  bool isNotesOn {voiceOrder.size() > 0};
  
   // Check if there is another voice already playing the same note
   for (int i = 0; i < voiceCount; ++i)
   {
   if (voices[i]->noteNumber == ns.note) allocated = i;
   }
   
   // If a voice is already playing the note, steal it!
   if (allocated < voiceCount)
   {
   int i;
   for (i = 0;
   i < voiceOrder.size() && voiceOrder[i] != allocated;
   ++i)
   {}
   
   if (i < voiceOrder.size()) voiceOrder.erase(voiceOrder.begin() + i);
   }
   else
  {
   allocated = 0;
   
   // If there are available voices which are not active, simply get the first available
   if (voiceOrder.size() < voiceCount)
   {
    while (allocated < voiceCount && voices[allocated]->isActive()) ++allocated;
    if (allocated == voiceCount) allocated = 0;
   }
   else
   {
    // Otherwise, get the oldest note and steal it
    // First, find the oldest note that has been released
    while (allocated < voiceOrder.size() && voices[voiceOrder[allocated]]->noteOn)
    {
     ++allocated;
    }
    if (allocated == voiceOrder.size()) allocated = 0;
    int t = allocated;
    allocated = voiceOrder[allocated];
    voiceOrder.erase(voiceOrder.begin() + t);
   }
  }
  
  SampleType noteFreq = ns.note;
  // Do we bend the note? If we are not in glissando mode, we only bend if there are already notes being played
  bool bendNote = polyParam.glissando() || isNotesOn;
  voiceOrder.push_back(allocated);
  SampleType fVel = static_cast<SampleType>(ns.velocity)/127.;
  if (bendNote)
  {
   SampleType ln = lastNote;
   voices[allocated]->startNote(ns.note, noteFreq, fVel, ln, polyParam.portTimeSamples(), true);
  }
  else
  {
   voices[allocated]->startNote(ns.note, noteFreq, fVel, noteFreq, 0, true);
  }
  // Keep the frequency that we calculated for the next bend
  if (onNoteOn) onNoteOn(allocated);
  lastNote = noteFreq;
 }

 
 
 
 
 void stopVoice(const NoteSchedule &ns)
 {
  int noteFind = 0;
  
  while (noteFind < MaxVoiceCount &&
         !(voices[noteFind]->noteOn && voices[noteFind]->noteNumber == ns.note))
  {
   ++noteFind;
  }
  
  if (noteFind < MaxVoiceCount)
  {
   voices[noteFind]->stopNote();
   if (onNoteOff) onNoteOff(noteFind);
  }
 }
 




 void startLegatoNote(const NoteSchedule &ns)
 {
  SampleType fVel = static_cast<SampleType>(ns.velocity)/127.;
  if (voiceOrder.size() > 0)
  {
   // Note already playing, push new note and bend voice
   voiceOrder.push_back(ns.note);
   voices[0]->startNote(ns.note, ns.note, fVel, -1., polyParam.portTimeSamples(), false);
  }
  else
  {
   // First note, push new note and trigger voice
   voiceOrder.push_back(ns.note);
   voices[0]->startNote(ns.note, ns.note, fVel,  ns.note, polyParam.portTimeSamples(), true);
   if (onNoteOn) onNoteOn(0);
  }
 }
 




 void stopLegatoNote(const NoteSchedule &ns)
 {
  // Find note in voiceOrder
  auto it = std::find(voiceOrder.begin(), voiceOrder.end(), ns.note);
  if (it == voiceOrder.end())
  {
   // Note not found or empty vector....ignore
  }
  else if (it == --voiceOrder.end())
  {
   // Check to see if it is the only note playing
   if (voiceOrder.size() == 1)
   {
    // Note is only note playing. Stop voice
    voices[0]->stopNote();
    voiceOrder.pop_back();
    if (onNoteOff) onNoteOff(0);
   }
   else
   {
    // Note is last note played. Pop note and bend voice back to new last note
    SampleType ln = voices[0]->noteNumber;
    voiceOrder.pop_back();
    voices[0]->startNote(voiceOrder.back(),
                         voiceOrder.back(),
                         voices[0]->voiceComponents[0]->velocityIn.getControl(), ln,
                         polyParam.portTimeSamples(),
                         false);
   }
  }
  else
  {
   // Note is in the order but it is not currently playing. Remove it from the vector
   voiceOrder.erase(it);
  }
 }

 
 
 
 
 void doNoteAction(const NoteSchedule &ns)
 {
  switch (ns.velocity)
  {
   case NoteSchedule::AllNotesOff:
   {
    if (polyParam.legato())
    {
     voiceOrder.clear();
     voices[0]->stopNote();
    }
    else
    {
     for (auto it: voiceOrder)
     {
      voices[it]->stopNote();
     }
    }
   }
    break;
    
   case NoteSchedule::AllSoundOff:
   {
    resetAllNotes();
   }
    break;
    
   default:
   {
    if (polyParam.legato())
    {
     if (ns.velocity == 0)
     {
      stopLegatoNote(ns);
     }
     else
     {
      startLegatoNote(ns);
     }
    }
    else
    {
     if (ns.velocity == 0)
     {
      stopVoice(ns);
     }
     else
     {
      allocateVoiceAndStart(ns);
     }
    }
   }
    break;
  }
 }
 




 void purgeInactiveVoices()
 {
  if (!polyParam.legato())
  {
   for (auto it = voiceOrder.begin(); it != voiceOrder.end(); )
   {
    if (!voices[*it]->isActive())
    {
     voices[*it]->killNote();
     if (AutoEnable) voices[*it]->setEnabled(false);
     it = voiceOrder.erase(it);
    }
    else ++it;
   }
  }
 }
 




public:
 MIDIPoly(Parameters &p, SummingArray<VoiceComponent, MaxVoiceCount> &va) :
 Parameters::ParameterListener(p),
 voiceArray(va),
 polyParam(dynamic_cast<PolySynthParameters&>(p))
 {
  for (int i = 0; i < MaxVoiceCount; ++i)
  {
   voices[i] = std::make_unique<Voice>();
   if (AutoEnable) voiceArray[i].setEnabled(false);
  }
  
  setUnisonMode(1);
 }
 
 std::function<void (int)> onNoteOn;
 std::function<void (int)> onNoteOff;
 
 void setVoiceLimit(int l)
 {
  resetAllNotes();
  dsp_assert(l >= 1 && l < MaxVoiceCount);
  voiceLimit = l;
  setUnisonMode(1);
 }
 
 void setUnisonMode(int u)
 {
  resetAllNotes();
  dsp_assert(u >= 1 && u < MaxVoiceCount);
  if (u > voiceLimit) voiceLimit = u;
  voiceCount = voiceLimit / u;
  for (auto &v: voices)
  {
   v->voiceComponents.clear();
  }
  for (int i = 0; i < voiceCount; ++i)
  {
   for (int v = 0; v < u; ++v)
   {
    int w = i*u + v;
    voices[i]->voiceComponents.push_back(&(voiceArray[w]));
   }
  }
 }

 void updateCustomParameter(int category, int index) override
 {
  if (category == Parameters::BuiltinParameterCategory &&
      index == Parameters::BuiltinCustomParameters::Legato)
  {
   resetAllNotes();
  }
 }
 
 void resetAllNotes()
 {
  for (auto &v: voices)
  {
   if (v)
   {
    v->stopNote();
    v->killNote();
   }
  }
  voiceOrder.clear();
 }
 
 void scheduleNoteEvent(int note, int velocity, int samplePosition)
 {
  auto it = schedule.begin();
  while (it != schedule.end() &&
         (it->samplePosition <= samplePosition ||
          (it->note == note &&
           it->velocity < velocity)))
  {
   ++it;
  }
  schedule.insert(it, NoteSchedule(note, velocity, samplePosition));
 }
 
 void scheduleAllNotesOff(int samplePosition)
 {
  scheduleNoteEvent(0, NoteSchedule::AllNotesOff, samplePosition);
 }
 
 void scheduleAllSoundOff(int samplePosition)
 {
  scheduleNoteEvent(0, NoteSchedule::AllSoundOff, samplePosition);
 }
 
 void reset()
 {
  resetAllNotes();
  schedule.clear();
 }

 void process(int startPosition, int sampleCount)
 {
  int i = startPosition;
  int s = sampleCount;
  while (!schedule.empty() && s > 0)
  {
   int ns = schedule[0].samplePosition - i;
   if (ns > s) ns = s;
   if (ns > 0)
   {
    voiceArray.process(i, ns);
    s -= ns;
    i += ns;
   }
   if (i == schedule[0].samplePosition)
   {
    doNoteAction(schedule[0]);
    schedule.erase(schedule.begin());
   }
  }
  
  if (s > 0) voiceArray.process(i, s);
  
  purgeInactiveVoices();
 }

 void advanceMidiEvents(int sampleCount)
 {
  for (auto & sch: schedule)
  {
   sch.samplePosition -= sampleCount;
  }
 }
};









}










#endif
