//
//  XDDSP_Parameters.h
//  XDDSP
//
//  Created by Adam Jackson on 24/5/2022.
//

#ifndef XDDSP_Parameters_h
#define XDDSP_Parameters_h

#include "XDDSP_Types.h"

namespace XDDSP
{










class ModulationSource;
class ModulationDestination;








/*
 The Parameters class contains all the information about the DSP process.
 If every component refers to the same Parameters instance then only one
 instance needs to be maintained.
 
 This class contains an index of modulation sources. During initialisation, the modulation
 section keeps a stack of component names. As each source or destination is registered,
 the stack is used to give that registration a complete name.
 
 This class can be extended to contain other DSP parameters that are specific to
 the process being modelled, such as the cutoff point of a filter.
 */
class Parameters
{
public:
 /*
  This constant reserves a category of custom parameter for built-in extensions such as
  PolySynthParameters
  */
 
 static constexpr int BuiltinParameterCategory = -1;
 struct BuiltinCustomParameters
 {
  enum
  {
   Legato = 0,
   BuiltInParametersCount
  };
 };
 
 class ParameterListener
 {
  Parameters &param;
  
  void addAsListener()
  {
   param.listeners.emplace_back(this);
  }
  
  void removeFromListeners()
  {
   auto it = std::find(param.listeners.begin(), param.listeners.end(), this);
   if (it != param.listeners.end()) param.listeners.erase(it);
  }
  
 public:
  ParameterListener(Parameters &p) :
  param(p)
  {
   addAsListener();
  }
  
  virtual ~ParameterListener()
  {
   removeFromListeners();
  }
  
  virtual void updateSampleRate(double sr, double isr) {}
  virtual void updateBufferSize(int bs) {}
  virtual void updateCustomParameter(int category, int index) {}
 };
 
private:
 double sr {44100};
 double isr {1./44100.};
 int bs {1};
 
 bool transportValid {false};
 double trans_tempo {120.};
 double trans_ppq {0.};
 double trans_seconds {0.};
 
 unsigned int sampleOffset {0};
 double secondsOffset {0.};
 double ppqOffset {0.};
 
 std::vector<ParameterListener*> listeners;
 std::vector<std::string> componentNameStack;
 
 std::string formCompleteName(const std::string &name)
 {
  std::string result;
  for (auto &n : componentNameStack)
  {
   result.append(n);
  }
  result.append(name);
  return result;
 }
 
 struct ModSource
 {
  std::string name;
  ModulationSource* output;
 };
 
 std::vector<ModSource> modSources;
 
 struct ModDestination
 {
  std::string name;
  ModulationDestination* input;
 };
 
 std::vector<ModDestination> modDestinations;
 
 void updateSampleRate()
 {
  for (auto &l: listeners) l->updateSampleRate(sr, isr);
 }
 
 void updateBufferSize()
 {
  for (auto &l: listeners) l->updateBufferSize(bs);
 }
 
 
protected:
 
 void updateCustomParameter(int category, int index)
 {
  for (auto &l: listeners) l->updateCustomParameter(category, index);
 }
 
public:
 Parameters()
 {
  registerModulationSource(nullptr, "None");
  registerModulationDestination(nullptr, "None");
 }
 
 virtual ~Parameters()
 {}
 
 
 void setSampleRate(double newsr)
 {
  if (newsr > 0)
  {
   sr = newsr;
   isr = 1./newsr;
   updateSampleRate();
  }
 }
 
 double sampleRate() const
 { return sr; }
 
 void setSampleInterval(double newisr)
 {
  if (newisr > 0)
  {
   isr = newisr;
   sr = 1./newisr;
   updateSampleRate();
  }
 }
 
 double sampleInterval() const
 { return isr; }
 
 void setBufferSize(int newbs)
 {
  if (newbs > 0)
  {
   bs = newbs;
   updateBufferSize();
  }
 }
 
 double samplesToms(double samples) const
 { return samples*isr*1000.; }
 
 double msToSamples(double ms) const
 { return ms*sr*0.001; }
 
 int bufferSize() const
 { return bs; }
 
 void setTransportInformation(double tempo, double ppq, double seconds)
 {
  transportValid = true;
  trans_tempo = tempo;
  trans_ppq = ppq;
  trans_seconds = seconds;
 }
 
 void setTransportInformation(Parameters &p)
 {
  transportValid = p.transportValid;
  trans_tempo = p.trans_tempo;
  trans_ppq = p.trans_ppq;
  trans_seconds = p.trans_seconds;
 }
 
 void clearTransportInformation()
 { transportValid = false; }
 
 bool getTransportInformation(double &tempo, double &ppq, double &seconds) const
 {
  tempo = trans_tempo;
  ppq = trans_ppq + ppqOffset;
  seconds = trans_seconds + secondsOffset;
  return transportValid;
 }
 
 double getTempo() const
 { return trans_tempo; }
 
 double getSamplePosition() const
 { return sr*(trans_seconds + secondsOffset); }
 
 void setSampleOffset(unsigned int offset)
 {
  sampleOffset = offset;
  secondsOffset = sampleOffset*isr;
  ppqOffset = secondsOffset*trans_tempo/60.;
 }
 
 unsigned int getSampleOffset() const
 { return sampleOffset; }
 
 void enterModulatedComponent(const std::string &name)
 { componentNameStack.push_back(name); }
 
 void exitModulatedComponent()
 { componentNameStack.pop_back(); }
 
 void registerModulationSource(ModulationSource *ms, const std::string &name)
 { modSources.push_back({.name = formCompleteName(name), .output = ms}); }
 
 void registerModulationDestination(ModulationDestination *md, const std::string &name)
 { modDestinations.push_back({.name = formCompleteName(name), .input = md}); }
 
 int getModulationSourcesCount() const
 { return (int)modSources.size(); }
 
 std::string getModulationSourceName(int index) const
 { return modSources.at(index).name; }
 
 ModulationSource* getModulationSource(int index) const
 { return modSources.at(index).output; }
 
 int getModulationDestinationsCount() const
 { return (int)modDestinations.size(); }
 
 std::string getModulationDestinationName(int index) const
 { return modDestinations.at(index).name; }
 
 ModulationDestination* getModulationDestination(int index) const
 { return modDestinations.at(index).input; }
 
 void clearModulationBuffers();
};










}

#endif /* XDDSP_Parameters_h */
