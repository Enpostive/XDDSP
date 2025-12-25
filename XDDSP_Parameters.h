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










 /**
  * @brief An object which encapsulates all of the basic parameters of a DSP network.
  * 
  * The Parameters object is arguably the most important object in this library. Your app must create one of these objects and pass it as a parameter to every component instantiated. You set the sample rate, buffer size and transport information into the Parameters object, and the Parameters object takes the responsibility of updating every component listening to it.
  * 
  * Any part of your app which needs to receive updates about sample rate or buffer size should inherit from the Parameters::ParameterListener subclass to receive updates.
  * 
  * This class can be extended to contain other DSP parameters that are specific to the process being modelled, such as the cutoff point of a filter. Your extended class may also call updateCustomParameter to propagate custom parameter changes.
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
 
 /**
  * @brief This class is the base class for all objects which need to listen to a Parameters object.
  * 
  * The only constructor takes a Parameters object as its input and automatically registers the parent object as a listener upon construction, as well as removing itself as a listener upon destruction.
  */
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
  /**
   * @brief Construct a new ParameterListener object and add this as a listener.
   * 
   * @param p The Parameters object to listen to.
   */
  ParameterListener(Parameters &p) :
  param(p)
  {
   addAsListener();
  }
  
  /**
   * @brief Destroy the Parameter Listener object and remove this from the listeners list.
   */
  virtual ~ParameterListener()
  {
   removeFromListeners();
  }
  
  /**
   * @brief Get notified of an update to the sample rate.
   * 
   * Your class should override this method with its own method to be updated with sample rate changes.
   * 
   * @param sr The new sample rate.
   * @param isr The new sample interval, the reciprocal of the sample rate.
   */
  virtual void updateSampleRate(double sr, double isr) {}

  /**
   * @brief Get notified of an update to the buffer size.
   * 
   * Your class should override this method with its own method to be updated with buffer size changes.
   * 
   * @param bs The new buffer size.
   */
  virtual void updateBufferSize(int bs) {}

  /**
   * @brief Get notified of an update to any custom parameter.
   * 
   * Your class should override this method with its own method to be updated with custom parameter changes.
   * 
   * @param category The category of the custom parameter. -1 is reserved for built-in custom parameters.
   * @param index The index of the custom parameter.
   */
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
 {}
 
 virtual ~Parameters()
 {}
 
 /**
  * @brief Set the sample rate of the signal process.
  * 
  * The new sample rate is propagated to all listeners.
  * 
  * @param newsr The new sample rate.
  */
 void setSampleRate(double newsr)
 {
  if (newsr > 0)
  {
   sr = newsr;
   isr = 1./newsr;
   updateSampleRate();
  }
 }
 
 /**
  * @brief Get the current sample rate.
  * 
  * @return double The current sample rate.
  */
 double sampleRate() const
 { return sr; }
 
 /**
  * @brief Set the sample rate by specifying a sample interval.
  * 
  * @param newisr The new sample interval.
  */
 void setSampleInterval(double newisr)
 {
  if (newisr > 0)
  {
   isr = newisr;
   sr = 1./newisr;
   updateSampleRate();
  }
 }
 
 /**
  * @brief Get the current sample interval.
  * 
  * The sample interval is simply the reciprocal of the current sample rate. For example, for a 44100Hz sample rate, the sample interval will be 1/44100 seconds.
  * 
  * @return double The current sample interval.
  */
 double sampleInterval() const
 { return isr; }
 
 /**
  * @brief Set the buffer size that can be expected by the network.
  * 
  * The new buffer size is propagated to all listeners, which are expected to manage their own buffer memory.
  * 
  * Components should allow for the possibility of receiving buffers which are actually bigger than this size. It is not expected that a component would resize a buffer mid-process, but it is expected that a component would degrade safely as opposed to crash.
  * 
  * @param newbs The new buffer size.
  */
 void setBufferSize(int newbs)
 {
  if (newbs > 0)
  {
   bs = newbs;
   updateBufferSize();
  }
 }
 
 /**
  * @brief Convert samples to milliseconds using the current sample rate.
  * 
  * @param samples The length of time in samples.
  * @return double The length of time in milliseconds.
  */
 double samplesToms(double samples) const
 { return samples*isr*1000.; }
 
 /**
  * @brief Convert miliseconds to samples using the current sample rate.
  * 
  * @param ms The length of time in milliseconds.
  * @return double The length of time in samples.
  */
 double msToSamples(double ms) const
 { return ms*sr*0.001; }
 
 /**
  * @brief Get the current buffer size.
  * 
  * Components should allow for the possibility of receiving buffers which are actually bigger than this size. It is not expected that a component would resize a buffer mid-process, but it is expected that a component would degrade safely as opposed to crash.
  * 
  * @return int The current buffer size.
  */
 int bufferSize() const
 { return bs; }
 
 /**
  * @brief Set the transport information.
  * 
  * Also sets a flag indicating that the current transport information is valid. This information is not propagated through the network automatically.
  * 
  * @param tempo The current tempo in beats per minute.
  * @param ppq The current song position in quarter notes.
  * @param seconds The current song position in seconds.
  */
 void setTransportInformation(double tempo, double ppq, double seconds)
 {
  transportValid = true;
  trans_tempo = tempo;
  trans_ppq = ppq;
  trans_seconds = seconds;
 }
 
 /**
  * @brief Copy the transport information from another Parameters object.
  * 
  * @param p Another parameters object.
  */
 void setTransportInformation(Parameters &p)
 {
  transportValid = p.transportValid;
  trans_tempo = p.trans_tempo;
  trans_ppq = p.trans_ppq;
  trans_seconds = p.trans_seconds;
 }
 
 /**
  * @brief Clear the flag which indicates that transport information is valid.
  * 
  */
 void clearTransportInformation()
 { transportValid = false; }
 
 /**
  * @brief Get the most recent transport information notification.
  * 
  * @param tempo The current tempo in beats per minute.
  * @param ppq The current song position in quarter notes.
  * @param seconds The current song position in seconds.
  * @return true If the current transport information is valid.
  * @return false If the current transport information is invalid.
  */
 bool getTransportInformation(double &tempo, double &ppq, double &seconds) const
 {
  tempo = trans_tempo;
  ppq = trans_ppq + ppqOffset;
  seconds = trans_seconds + secondsOffset;
  return transportValid;
 }
 
 /**
  * @brief Get the current song tempo.
  * 
  * @return double The current song tempo.
  */
 double getTempo() const
 { return trans_tempo; }
 
 /**
  * @brief Get the current song position in samples, using the current sample rate.
  * 
  * @return double The current song position in samples.
  */
 double getSamplePosition() const
 { return sr*(trans_seconds + secondsOffset); }
 
 /**
  * @brief Set an offset to add to the song position reports.
  * 
  * @param offset The offset in samples.
  */
 void setSampleOffset(unsigned int offset)
 {
  sampleOffset = offset;
  secondsOffset = sampleOffset*isr;
  ppqOffset = secondsOffset*trans_tempo/60.;
 }
 
 /**
  * @brief Set an offset to add to the song position reports.
  * 
  * @return unsigned int The offset in samples.
  */
 unsigned int getSampleOffset() const
 { return sampleOffset; }
};










}

#endif /* XDDSP_Parameters_h */
