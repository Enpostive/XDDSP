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










/**
 * @brief An extension of Parameters that contains extra parameters suitable for a MIDI polyphonic synthesiser.
 * 
 */
class PolySynthParameters : public Parameters
{
 SampleType tuning {440.};
 SampleType portTime {0.};
 bool glissandoSetting {false};
 bool legatoSetting {false};
 int pbr {2};
 
public:
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
 
 /**
  * @brief Set the amount of pitch bend in response to pitch bend commands.
  * 
  * @param pbr Maximum pitch bend in semitones.
  */
 void setPitchBendRange(int pbr)
 { if (pbr >= 0) pbr = pbr; }
 
 /**
  * @brief Enable or disable glissando.
  * 
  * @param g True to enable, false to disable.
  */
 void setGlissando(bool g)
 { glissandoSetting = g; }
 
 /**
  * @brief Enable or disable legato.
  * 
  * @param l True to enable, false to disable.
  */
 void setLegato(bool l)
 {
  legatoSetting = l;
  updateCustomParameter(Parameters::BuiltinParameterCategory,
                        Parameters::BuiltinCustomParameters::Legato);
 }
 
 /**
  * @brief Set the time spent bending between notes in legato or glissando modes.
  * 
  * @param t Portamenteau time in seconds.
  */
 void setPortamenteauTime(SampleType t)
 { if (t >= 0.) portTime = t; }
 
 /**
  * @brief Get the current pitch bend range setting.
  * 
  * @return int The pitch bend in semitones.
  */
 int pitchBendRange() const
 { return pbr; }
 
 /**
  * @brief Get the current portamenteau time in seconds.
  * 
  * @return SampleType The current portamenteau time in seconds.
  */
 SampleType portamenteauTime() const
 { return portTime; }
 
 /**
  * @brief Get the current portamenteau time in samples.
  * 
  * @return SampleType The current portamenteau time in samples.
  */
 int portTimeSamples() const
 { return portTime*sampleRate(); }
 
 /**
  * @brief Get the current legato setting.
  * 
  * @return true Legato enabled.
  * @return false Legato disabled.
  */
 bool legato() const
 { return legatoSetting; }
 
 /**
  * @brief Get the current glissando setting.
  * 
  * @return true Glissando enabled.
  * @return false Glissando disabled.
  */
 bool glissando() const
 { return glissandoSetting; }
};










/**
 * @brief A special component which creates an array of internal component and sums an output to one output.
 * 
 * **The component given as InternalComponent must have an output named signalOut**
 * 
 * TODO: Add another template argument to specify the output to sum, instead of assuming the output is called signalOut.
 * 
 * @tparam InternalComponent The class of the component to make an array of.
 * @tparam ComponentCount The size of the array.
 */
template <typename InternalComponent, int ComponentCount>
class SummingArray : public Component<SummingArray<InternalComponent, ComponentCount>>
{
 std::array<InternalComponent, ComponentCount> components;
 
public:
 static constexpr int Count = InternalComponent::Count;
 static constexpr int CountComponents = ComponentCount;
 
 Output<Count> sumOut;
 
 SummingArray(Parameters &p) :
 components(makeComponentArray<ComponentCount, InternalComponent>(p)),
 sumOut(p)
 {}
 
 void reset()
 {
  for (auto &c : components) c.reset();
  sumOut.reset();
 }
 
 void stepProcess(int startPoint, int sampleCount)
 {
  for (auto &j : components) j.process(startPoint, sampleCount);
  
  for (int i = startPoint, s = sampleCount; s--; ++i)
  {
   for (int c = 0; c < Count; ++c)
   {
    SampleType sum = 0.;
    for (auto &j : components) sum += j.signalOut(c, i);
    sumOut.buffer(c, i) = sum;
   }
  }
 }
 
 /**
  * @brief Access each component individually.
  * 
  * @param i The index of the component to access.
  * @return InternalComponent& A reference to the component.
  */
 InternalComponent& operator[](unsigned int i)
 {
  return components[i];
 }
 
 /**
  * @brief Return an iterator to the first component in the array.
  * 
  * @return auto An iterator to the first component in the array.
  */
 auto begin()
 {
  return components.begin();
 }
 
 /**
  * @brief Return an iterator to the end of the array.
  * 
  * @return auto An iterator to the end of the array.
  */
 auto end()
 {
  return components.end();
 }
};










/**
 * @brief A component which takes MIDI events and outputs a signal for each.
 * 
 * TODO: Change the algorithm to use a heap to order events.
 * 
 * @tparam OutputCount The number of signals to output.
 * @tparam RampLengthms The length of the smoothing window in ms.
 */
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
 
 /**
  * @brief Add a MIDI event to a channel.
  * 
  * @param channel The channel to add the event to.
  * @param newValue The new value that will be output when the event is triggered.
  * @param samplePosition The number of samples to wait until triggering the event.
  */
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
 
 /**
  * @brief Advance all the MIDI events in the buffer by the sample count given. 
  * 
  * @param sampleCount The number of samples to advance the events in the buffer.
  */
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










/**
 * @brief A special component which connects to a collection of voices to make a polyphonic component.
 * 
 * **The component given as InternalComponent must have an input named noteIn, an input named velocityIn and an output named signalOut**
 * 
 * TODO: Add another template argument to specify the input names and an output to sum, instead of assuming the names given above.
 * 
 * @tparam VoiceComponent The component that makes up each individual voice.
 * @tparam MaxVoiceCount The maximum number of voices allowed.
 * @tparam AutoEnable When set to any value other than 0, will automatically enable and disable individual voices using internal logic. Keep in mind that components are reset when they are disabled, so this may not be desired behaviour.
 */
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
 /**
  * @brief Construct a new MIDIPoly object.
  * 
  * **Note: The parameters class passed to this constructor is XDDSP::PolySynthParameters**
  * 
  * @param p A XDDSP::PolySynthParameters object.
  * @param va A reference to the summing array.
  */
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
 
 /**
  * @brief A function which is called whenever a note on occurs.
  * 
  */
 std::function<void (int)> onNoteOn;

 /**
  * @brief A function which is called whenever a noteoff occurs.
  * 
  */
 std::function<void (int)> onNoteOff;
 
 /**
  * @brief Set the maximum number of voices allowed and disable unison mode.
  * 
  * @param l The number of voices allowed.
  */
 void setVoiceLimit(int l)
 {
  resetAllNotes();
  dsp_assert(l >= 1 && l < MaxVoiceCount);
  voiceLimit = l;
  setUnisonMode(1);
 }
 
 /**
  * @brief Enable or disable unison mode.
  * 
  * @param u True to enable unison mode, false to disable it.
  */
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
 
 /**
  * @brief Reset every voice and remove all MIDI events from the buffer
  * 
  */
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
 
 /**
  * @brief Add a MIDI note event to the MIDI event schedule.
  * 
  * @param note The pitch of the note.
  * @param velocity The velocity of the note. Use 0 to schedule a note off.
  * @param samplePosition The number of samples to wait until the event is triggered.
  */
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
 
 /**
  * @brief Add an All Notes Off MIDI message to the cue.
  * 
  * @param samplePosition The number of samples to wait until the event is triggered.
  */
 void scheduleAllNotesOff(int samplePosition)
 {
  scheduleNoteEvent(0, NoteSchedule::AllNotesOff, samplePosition);
 }
 
 /**
  * @brief Add an All Sound Off MIDI message to the cue.
  * 
  * @param samplePosition The number of samples to wait until the event is triggered.
  */
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
    parallelProcess(i, ns);
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
  
  if (s > 0)
  {
   parallelProcess(i, s);
   voiceArray.process(i, s);
  }
  
  purgeInactiveVoices();
 }

 
 /**
  * @brief Advance all the MIDI events in the buffer by the sample count given. 
  * 
  * @param sampleCount The number of samples to advance the events in the buffer.
  */
 void advanceMidiEvents(int sampleCount)
 {
  for (auto & sch: schedule)
  {
   sch.samplePosition -= sampleCount;
  }
 }

 /**
  * @brief Sub-classes can implement this method to enable the processing of a mono portion of the synthesiser.
  * 
  * @param startPoint The start point to begin processing.
  * @param sampleCount The number of samples to process.
  */
 virtual void parallelProcess(int startPoint, int sampleCount)
 {}
};









}










#endif
