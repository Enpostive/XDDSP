# XDDSP Tutorial

Welcome to this quick tutorial, where we will go over the use of the XDDSP library. First we will introduce some of the basic concepts and the low level classes that encapsulate those concepts. Then we will build a simple component which includes a filter and an LFO. Then we will go over the steps needed to integrate the resulting component into an application.

XDDSP is a header-only template library with a few design goals, which we will explore in the next section. 

---

# Design Goals

## A common base class for components.

The base class for the components is called [Component](@ref XDDSP::Component). [Component](@ref XDDSP::Component) is a template base class utilising CRTP (Curiously Recurring Template Pattern). It encapsulates a couple of common features that make up a typical DSP processing loop. The prominent features are step sizes and trigger points.

## Component inputs and outputs to have human readable names.

A second base class is used called [Coupler](@ref XDDSP::Coupler) which also uses CRTP. [Coupler](@ref XDDSP::Coupler) encapsulates mechanisms for fetching input samples in a consistent way and supports multi-channel connections and bulk transfers. Every input into a component may be specified as a template argument to specify the kind of coupler, and a constructor parameter which initialises the coupler, often with a compile time reference.

Each component class in the library has inputs and outputs exposed as public members which can be addressed by name from other code outside of the component. The [Coupler](@ref XDDSP::Coupler) class is designed to be exposed this way, encapsulating input and output logic and making it easy to specify the connections between components when designing your DSP network. I have found that most code completion features in modern IDEs can locate and auto-complete input and output names.

## Maximising code-encapsulation and reusability.

XDDSP is extremely modular. Each component is made to do just one thing and provide all the necessary connectivity required for its job. This means that, for example, the filter functionality is completely separate from the dynamics processing functionaliy and both are as easily interoperable as with the rest of the functions provided.

Furthermore, common features such as control inputs from the user interface or samples from sample buffers are just as connectable as other components, each being encapsulated inside a class derived from [Coupler](@ref XDDSP::Coupler).

## Portability and independence from other libraries.

The library is written in C++17, which has become a widely accepted standard with many compilers being available across many platforms. The only prerequisite to use is the C++ standard library. This does mean that XDDSP currently lacks certain important features, such as the ability to load media from disk. However, you may find that implementing a coupler to interface with your favourite codec library is trivial, enabling any component in the library to interface with it.

## As many opportunities for automatic optimisation as possible.

As XDDSP is predominantly a header-only template library, many C++ compilers can exploit inlining and loop-consolidation tricks to reduce code size and increase performance. Code I have compiled with Clang and anaylised using Ghidra have multi-layer connections between components optimised away and many internal process loops have been merged into bigger loops.

---

# Building A Custom Component

Custom components in XDDSP consist of template classes which inherit from the [Component](@ref XDDSP::Component) template. [Component](@ref XDDSP::Component) exposes an interface to enable and disable the component, along with 6 virtual methods. Any class which inherits from this CRTP class is called a component.

## Component Enabling

You can check if any component is enabled by calling [Component::isEnabled](@ref Component_isEnabled). You can change the enabled status by calling [Component::isDisabled](@ref Component_isDisabled). Note that disabling the component will automatically reset it to mute its output buffers.

You put your code to reset the component into [Component::reset](@ref XDDSP::Component::reset). At a minimum, it is expected that [Component::reset](@ref XDDSP::Component::reset) will clear the output buffers of the component by filling them with zeroes.

## The Process Loop

[Component](@ref XDDSP::Component) declares a method called [Component::process](@ref XDDSP::Component::process). This method should be used by any code external to the component to run the process loop. Care should be taken to ensure that components are processed in the proper order, that is starting with components connected to the inputs and ending with components connected to outputs.

[Component::startProcess](@ref XDDSP::Component::startProcess) is called at the beginning of the process loop and the return value is kept as the step size. The step size is allowed to change to be whatever is necessary, as long as it is not bigger than the sampleCount parameter. The intention with step sizes is to allow control signals to be calculated at intervals as opposed to being calculated every sample. A good example is a biquad filter with a frequency input. Even a keen listener is not going to notice when filter coefficents are only updated every 16 samples or so. Being able to process in smaller steps also allows the process loop to "interrupt" the process at a predetermined point in the loop. 

[Component::stepProcess](@ref XDDSP::Component::stepProcess) is called repeatedly with step sized chunks. The process loop may call stepProcess with smaller chunks from time to time.

Your DSP code may call the protected method [Component::setNextTrigger](@ref Component_setNextTrigger) to set a trigger point. Because of how symbol resolution works in C++17, this method can only be accessed by referring indirectly through `this` like `this->setNextTrigger(trigger)`.

[Component::triggerProcess](@ref XDDSP::Component::triggerProcess) is called when a trigger point is reached.

[Component::finishProcess](@ref XDDSP::Component::finishProcess) is called after the process loop finishes.

The [Component](@ref XDDSP::Component) class gives you default implementations for all of these methods. All of the defaults are empty methods, except for [Component::startProcess](@ref XDDSP::Component::startProcess), the default implementation of which uses a template argument as the return value, unless sampleCount is smaller. This means that implementing a fixed step size is just a matter of providing the template argument in the [Component](@ref XDDSP::Component) template parameters.

The intention is that you call [Component::process](@ref XDDSP::Component::process) from the audio processing thread. Your implementations of these methods should be completely non-blocking and optimised for performance. Avoid memory allocations or deallocations, file operations or any other form of blocking IO. Mutex locking for inter-thread communication is sometimes needed to meet this requirement, it is best to keep those critical sections as short as possible.

**Note: These methods are currently virtual. In a future version these methods will not be virtual and thus the object won't be polymorphic. All new software should avoid keeping pointers of [ComponentBaseClass](@ref XDDSP::ComponentBaseClass) and only keep pointers or references to derived classes instead.**

## Handy template

Here is a helpful template which you can copy into your application to use as a component class.

```cpp
#include "XDDSP/XDDSP.h"

// Input types are specified as template arguments to the class. Input types go
// first, because they never have defaults.
template <typename SignalIn>
class NewComponent : public Component<NewComponent<SignalIn>> {
 public:
  // A multi-channel component will look at the channel count of the input and put out the same channel count.
  static constexpr int Count = SignalIn::Count;
  
  // Specify your inputs as public members here
  SignalIn signalIn;
  
  // Specify your outputs like this
  Output<Count> signalOut;
  
  // Include a definition for each input in the constructor
  NewComponent(Parameters &p, SignalIn _signalIn) :
  signalIn(_signalIn),
  signalOut(p)
  {}
  
  // This function is responsible for clearing the output buffers to a default
  // state when the component is disabled.
  void reset() {
    signalOut.reset();
  }
  
  // startProcess prepares the component for processing one block and returns the 
  // step size. By default, it returns the entire sampleCount as one big step, so
  // this function can be deleted if not required.
  int startProcess(int startPoint, int sampleCount) {
    return std::min(sampleCount, StepSize);
  }

  // stepProcess is called repeatedly with the start point incremented by step size
  void stepProcess(int startPoint, int sampleCount) {
    for (int c = 0; c < Count; ++c) {
      for (int i = startPoint, s = sampleCount; s--; ++i) {
        // DSP work done here
      }
    }
  }

  // triggerProcess is called once if 'samplesToNextTrigger' reaches zero
  // components can use 'setNextTrigger' to set a trigger point. This function can
  // be deleted if not needed.
  void triggerprocess(int triggerPoint) {
  }

  // finishProcess is called after the block has been processed. This function
  // can be deleted if not needed.
  void finishProcess() {
  }
};
```

---

## Adding DSP Code to a Component

For the purposes of completeness, we will look at a very simple example component which zeroes out denormal numbers. To be clear, there is no reason why such a component would actually need to exist as there are better ways to remove denormals. But the example serves well to demonstrate how actual processing is performed inside a component.

Let's take the above template and do a little excersise of spot-the-difference (the comments will give you clues).

```cpp
#include "XDDSP/XDDSP.h"

// First we use find/replace to change the name of our new component into something meaningful.
template <typename SignalIn>
class RemoveDenormals : public Component<RemoveDenormals<SignalIn>> {
 public:
 
  // Declare how many channels this instance will use.
  static constexpr int Count = SignalIn::Count;
  
  SignalIn signalIn;
  
  Output<Count> signalOut;
  
  RemoveDenormals(Parameters &p, SignalIn _signalIn) :
  signalIn(_signalIn),
  signalOut(p) // <--- Notice that we pass the parameters object to the output so that it can create the correct buffer size.
  {}
  
  void reset() {
    signalOut.reset();
  }
  
  // startProcess can be removed from our template because we can just use the default implementation.
  
  // stepProcess is modified to have the actual code.
  void stepProcess(int startPoint, int sampleCount) {
    
    // Here you can do per-step calculations, such as updating filter co-efficients.
    
    for (int c = 0; c < Count; ++c) {
      for (int i = startPoint, s = sampleCount; s--; ++i) {

        // Put the next sample inside x
        SampleType x = signalIn(c, i);

        // Check to see if x is a denormal. If it is, zero it out.
        if (std::fpclassify(x) == FP_SUBNORMAL)
          x = 0.;
        
        // Write the output to the output buffer.
        signalOut.buffer(c, i) = x;
      }
    }
  }

  // triggerProcess and finishProcess are deleted as they are not needed.
};
```

The code throughout the library provides lots of different examples of how this template can be modified for different kinds of process.

One thing to notice is now a component differentiates between single-channel and multi-channel operation. Our example looks at the `Count` property of the input coupler to determine how many channels it has. Then we set a static constant to the same value and make it public. That `Count` property is used to construct the `signalOut` coupler with the same number of channels.

---

## Adding Subcomponents to Your Custom Component

A subcomponent can be added to a component by instantiating the class template of the required component (for a list see [Component Library Reference](#component_library_reference)) inside the parent component. It is safe to add new subcomponents as public members with their controls exposed, as the input method is determined by which Coupler classes are used as template parameters. These Coupler classes are in effect the inputs to the component and the behaviour of each coupler is fixed at compile time.

At runtime, most couplers cannot be default constructed, so the parent class constructor must take the input couplers as parameters at run time. The actual couplers are usually copy constructed from the parameters. 

Below we have a new custom component for our App. The new component has only three subcomponents, the LFO, the control modulator and the filter.

### LFO

This code instantiates a custom oscillator using the [FuncOscillator](@ref XDDSP::FuncOscillator) template.

```cpp
...
FuncOscillator<ControlConstant<>, ControlConstant<>> lfo;
...
```

In our case, we have chosen a pair of [ControlConstant](@ref XDDSP::ControlConstant) couplers as the inputs to the oscillator. The first input is a frequency in Hz, and the second is a phase modulation input, which we will ignore for now.

The LFO is initialised in the class constructor initialisation list:

```cpp
...
ModulatingFilter(Parameters &p, SignalIn _signalIn) :
 signalIn(_signalIn),
 lfo(p, {1.1}, {0.}), // <--- The LFO
 ...
```

Every component takes a [Parameters](@ref XDDSP::Parameters) object as the first parameter, and this is passed to each subcomponent. The parameters object is an object for holding commonly needed parameters, such as the current sample rate, and is explained later at [DSP Global Parameters](#parameters).

After the parameters parameter come the couplers. Each one is copy constructed into their respective places. Thanks to some handy syntax sugar, the constructor for the [ControlConstant](@ref XDDSP::ControlConstant) is simply given as `{1.1}` or `{0.}`, using the curly braces list-initialisation syntax. 

For example, the value can be set using [ControlConstant::setControl](@ref XDDSP::ControlConstant::setControl)

```cpp
lfo.frequencyIn.setControl(4.1); // An example of setting the frequency of the LFO to 4.1Hz
```

The shape of the waveform defaults to a sine wave at the frequency specified. The func property can be overwritten with a new function, for example:

```cpp
lfo.func = [&](SampleType x) { return x < 0.5 }; // An example of setting the waveform to a square wave.
```

### Control Modulator

The control modulator component takes a signal from an LFO or an envelope and rescales it to fit between a minimum and maximum value. An LFO outputs a signal between -1 and 1, while an envelope outputs a signal between 0 and 1. Either way, the input signal is allowed to be any value and the output signal will still be valid.

```cpp
ControlModulator<Connector<decltype(lfo.signalOut)>, ControlConstant<>, ControlConstant<>, ControlModulatorModes::BiExponential> freq;
```

The control modulator takes three input signals. The [ControlConstant](@ref XDDSP::ControlConstant) appears for the second and third inputs and they behave the same as before. The first input uses a different coupler called [Connector](@ref XDDSP::Connector). This coupler takes the type of output as a template parameter. In this instance, we are using `decltype(lfo.signalOut)` as the type for the connector because we intend to connect to `lfo.signalOut` later on.

The initialisation looks like:

```cpp
...
lfo(p, {1.1}, {0.}),
freq(p, {lfo.signalOut}, {500.}, {2000.}), // <--- The control modulator
flt(p, {signalIn}, {freq.signalOut}, {0.7}, {0.}),
...
```

The parameters object comes first, followed by a list-initialised coupler connecting to the `lfo.signalOut` output. Then the two control constants are initialised to output a minimum of 500Hz and a maximum of 2000Hz.

### Filter

We will use the [DynamicBiquad](@ref XDDSP::DynamicBiquad) template for the actual filter.

```cpp
DynamicBiquad<Connector<decltype(signalIn)>, Connector<decltype(freq.signalOut)>, ControlConstant<>, ControlConstant<>> flt;
```

We use [Connector](@ref XDDSP::Connector) to connect the input signal and the control modulator signal as inputs, and [ControlConstant](@ref XDDSP::ControlConstant) is used for the quality and gain inputs.

Note in the documentation for [DynamicBiquad](@ref XDDSP::DynamicBiquad), the template actually has five arguments, the signal, frequency, quality, gain and step size arguments. Because the step size argument defaults to 16, we don't need to specify an argument. With this default behaviour, the component will only update its filter coefficients every 16 samples. Here is an example which changes the step size to 32:

```cpp
DynamicBiquad<Connector<decltype(signalIn)>, Connector<decltype(freq.signalOut)>, ControlConstant<>, ControlConstant<>, 32> flt;
```

The filter constructor connects the connectors to signalIn and freq.signalOut, and sets the filter quality and gain values to 0.7 and 0dB respectively. We can also see here the signalOut constructor, which is an [Connector](@ref XDDSP::Connector) that is left to be used as the main output for the component.

```cpp
...
flt(p, {signalIn}, {freq.signalOut}, {0.7}, {0.}), // <--- The filter
signalOut({flt.signalOut})
...
```

### The process loop

The process loop needs to be modified to include the process loops of the subcomponents.

```cpp
...
void reset() {
 lfo.reset();
 freq.reset();
 flt.reset();
}
...
void stepProcess(int startPoint, int sampleCount) {
 lfo.process(startPoint, sampleCount);
 freq.process(startPoint, sampleCount);
 flt.process(startPoint, sampleCount);
}
...
```

As you can see, the minimum requirement is to call the reset and process methods of each subcomponent inside the respective parent methods. The order in which you call each process method is important. You need to follow the signal flow. For example, `lfo` should be processed **before** `freq` because `freq` takes the output of `lfo` as its input.


### Completed Example

Once again, we can take the template given above and make some trivial changes to implement our custom component. The comments describe what changes have been made.

```cpp

// I've removed all the previous comments and added new comments to highlight changes
// The first change is to use find/replace to rename the component to something meaningful
template <typename SignalIn>
class ModulatingFilter : public Component<ModulatingFilter<SignalIn>> {
 
 public:
 static constexpr int Count = SignalIn::Count;
 
 SignalIn signalIn;
 
 
 // Here is the LFO. The frequency and phase of the LFO are exposed as control constants.
 // ControlConstant<> can be used directly as an input as it is a type of coupler.
 FuncOscillator<ControlConstant<>, ControlConstant<>> lfo;
 
 // Here is a modulator which takes the LFO signal and scales it to the frequency in Hz for the filter.
 // The minimum and maximum frequency of the filter are exposed here as control constants.
 // Here, we wrap the type of output inside a Connector<> to create the right coupler for the type.
 ControlModulator<Connector<decltype(lfo.signalOut)>, ControlConstant<>, ControlConstant<>, ControlModulatorModes::BiExponential> freq;
 
 // Here is the filter. The filter exposes methods to change the various modes.
 // The q and gain are exposed as control constants.
 DynamicBiquad<Connector<decltype(signalIn)>, Connector<decltype(freq.signalOut)>, ControlConstant<>, ControlConstant<>> flt;

 
 Connector<decltype(flt.signalOut)> signalOut;
 
 ModulatingFilter(Parameters &p, SignalIn _signalIn) :
 signalIn(_signalIn),
 lfo(p, {1.1}, {0.}),
 freq(p, {lfo.signalOut}, {500.}, {2000.}),
 flt(p, {signalIn}, {freq.signalOut}, {0.7}, {0.}),
 signalOut({flt.signalOut})
 {}
 
 void reset() {
  lfo.reset();
  freq.reset();
  flt.reset();
 }
 
 // startProcess has been removed because we don't need it.
 
 // stepProcess is modified to call the process methods of the subcomponents.
 void stepProcess(int startPoint, int sampleCount) {
  lfo.process(startPoint, sampleCount);
  freq.process(startPoint, sampleCount);
  flt.process(startPoint, sampleCount);
 }
 
 // triggerProcess and finishProcess have been removed because they are not needed.
};
```

---

## Bringing Important Controls Forward

We can develop our component further by bringing important controls into the parent component, instead of leaving them as members of the subcomponents.

```cpp
SignalIn signalIn;
ControlConstant<> lfoFreq {1.1};
ControlConstant<> minHz {500.};
ContronConstant<> maxHz {2000.};
ControlConstant<> fltQ {0.7};
ControlConstant<> fltGaindB {0.0};
...
FuncOscillator<Connector<decltype(lfoFreq)>, ControlConstant<>> lfo;

ControlModulator<Connector<decltype(lfo.signalOut)>, Connector<decltype(minHz)>, Connector<decltype(maxHz)>, ControlModulatorModes::BiExponential> freq;
 
DynamicBiquad<Connector<decltype(signalIn)>, Connector<decltype(freq.signalOut)>, Connector<decltype(fltQ)>, Connector<decltype(fltGaindB)>> flt;
...
lfo(p, {lfoFreq}, {0.}),
freq(p, {lfo.signalOut}, {minHz}, {maxHz}),
flt(p, {signalIn}, {freq.signalOut}, {fltQ}, {fltGaindB}),
...
```

Here, we have moved the [ControlConstant](@ref XDDSP::ControlConstant) objects out as public members, where they can be initialised in place. Each subcomponent has a [Connector](@ref XDDSP::Connector) where control constants were used before. I'll leave it as an exersise to the reader to see what happens when the control constant objects are replaced with an [Ramp](@ref XDDSP::Ramp) objects. Remember that control constants are couplers, while the ramp is a subcomponent.

The control constants can also be made private to prevent outside objects changing the settings.

Here is a more complete example of controls brought forward:

```cpp
template <typename SignalIn>
class ModulatingFilter : public Component<ModulatingFilter<SignalIn>> {
 private:
 // Hiding the phaseIn control of the lfo.
 ControlConstant<> lfoPhase {0.};
 
 public:
 static constexpr int Count = SignalIn::Count;
 
 SignalIn signalIn;
 ControlConstant<> lfoFreq {1.1};
 ControlConstant<> minHz {500.};
 ContronConstant<> maxHz {2000.};
 ControlConstant<> fltQ {0.7};
 ControlConstant<> fltGaindB {0.0};
 
 FuncOscillator<Connector<decltype(lfoFreq)>, Connector<decltype(lfoPhase)>> lfo;
 ControlModulator<Connector<decltype(lfo.signalOut)>, Connector<decltype(minHz)>, Connector<decltype(maxHz)>, ControlModulatorModes::BiExponential> freq;
 DynamicBiquad<Connector<decltype(signalIn)>, Connector<decltype(freq.signalOut)>, Connector<decltype(fltQ)>, Connector<decltype(fltGaindB)>> flt;

 Connector<decltype(flt.signalOut)> signalOut;
 
 ModulatingFilter(Parameters &p, SignalIn _signalIn) :
 signalIn(_signalIn),
 lfo(p, {lfoFreq}, {lfoPhase}),
 freq(p, {lfo.signalOut}, {minHz}, {maxHz}),
 flt(p, {signalIn}, {freq.signalOut}, {fltQ}, {fltGaindB}),
 signalOut({flt.signalOut})
 {}
 
 void reset() {
  lfo.reset();
  freq.reset();
  flt.reset();
 }
 
 void stepProcess(int startPoint, int sampleCount) {
  lfo.process(startPoint, sampleCount);
  freq.process(startPoint, sampleCount);
  flt.process(startPoint, sampleCount);
 }
};
```

In some circumstances, it makes sense to **not** bring controls forward and just leave controls as members of the subcomponents. For example, if you have a component which has two LFOs, you might leave the frequency setting of each LFO as a control constant on the actual subcomponent which can make external code clearer.

Take this example:

```cpp
...
class DualLFO : public Component<DualLFO>
{
 FuncOscillator<ControlConstant<>, ControlConstant<>> lfo1;
 FuncOscillator<ControlConstant<>, ControlConstant<>> lfo2;
}
...
DualLFO dualLFO;
dualLFO.lfo1.frequencyIn.setControl(5); 
dualLFO.lfo2.frequencyIn.setControl(7); 
```

It is clear in this example which lines are setting the frequency of LFO 1 and LFO 2.

---

# Integration

After creating your DSP network inside your component, it is time to bring it all together and start running audio through it. The library has a few different coupler types that you can use to connect your network inputs to the other parts of your app. We have already looked at [ControlConstant](@ref XDDSP::ControlConstant), you can view the [full list of available couplers](#coupler_types) which come in handy for connecting components to each other and to external parts. Some useful examples include [BufferCoupler](@ref XDDSP::BufferCoupler), [BufferReader](@ref XDDSP::BufferReader), [PluginInput](@ref XDDSP::PluginInput) or a custom coupler of your own making.

Once you have decided on which coupler to use to connect your network inputs, you need a way to set basic parameters like sample rate and buffer size. The library provides the [Parameters](@ref XDDSP::Parameters) class to do this job.

We have to integrate the DSP network audio processing loop into the app to process the audio. This will involve calling [Component::process](@ref XDDSP::Component::process). Later in this chapter, we will be giving examples of this using the JUCE audio plugin development package.

## DSP Global Parameters {#parameters}

The [Parameters](@ref XDDSP::Parameters) class is dedicated to handling the kinds of parameters which are required in many places across the DSP network, such as sample rate and buffer size. Your app must construct at least one parameters object and pass it to every component in the network.

Where appropriate, you may want to derive a new class from [Parameters](@ref XDDSP::Parameters) to contain extra parameters. While the [Parameters](@ref XDDSP::Parameters) class is intended to manage parameters which are needed across the entire network, there is no rules about what kind of parameters can or cannot be added in your derived class. There is one built-in derivative called [PolySynthParameters](@ref XDDSP::PolySynthParameters) which handles parameters like the number of voices and oscillator tuning.

[Parameters](@ref XDDSP::Parameters) supports listeners and a subclass called [Parameters::ParameterListener](@ref XDDSP::Parameters::ParameterListener) can be extended by any class which wants to listen for changes. Currently, callbacks have been created which advertise changes to sample rate, buffer size and custom parameters.

For more information, see [Parameters](@ref XDDSP::Parameters).

## Reading output 

All couplers support bulk transfers of sample data via a method called [Coupler::fastTransfer](@ref XDDSP::Coupler::fastTransfer). However, the primary use of this method is only to copy the final output from your custom component to your app output.

This means that your custom class can use **any** coupler for its output. The most useful output couplers are [Output](@ref XDDSP::Output), [Connector](@ref XDDSP::Connector) and [PConnector](@ref XDDSP::PConnector). But you might want to use something like [Switch](@ref XDDSP::Switch) or [SignalModifier](@ref XDDSP::SignalModifier) as an output or even [SamplePlaybackHead](@ref XDDSP::SamplePlaybackHead).

The benefit of using a coupler for your components output is that it can be connected by another component as an input for that, meaning that your code stays modular and reusable.

## The final setup

Let's look at a setup which I have been primarily using this library for: Developing audio plugins using [JUCE](https://juce.com/). 

If you are not familiar with [JUCE](https://juce.com/), thankfully JUCE is fairly easy to understand. We won't go in depth into developing a complete plugin with JUCE as that is outside of the scope of this document. Instead we will focus on some simple code snippets which should give you an idea of how this library fits into a typical app.

I have created my ModulatingFilter class as above, inside the XDDSP namespace. I have created a project called DocumentExample, and the class names in this section follow the naming conventions adopted by JUCE.

Inside the DocumentationExampleAudioProcessor class, we add a [Parameters](@ref XDDSP::Parameters) object and  our ModulatingFilter giving it a [PluginInput](@ref XDDSP::PluginInput) coupler with 2 channels for the input. I also like to add a mutex, which I use to block parameter changes from happening while the process loop is running.


```cpp
...
 XDDSP::Parameters dspParam;
 XDDSP::ModulatingFilter<PluginInput<2>> dsp;
 
 std::mutex mtx;
...
```

Next we move on to PluginProcessor.cpp where we add the code to set up the parameters inside `prepareToPlay`.

```cpp
void DocumentationExampleAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
 // Here, JUCE tells us what the sample rate and buffer sizes are, so we add the code to put that into the parameters object.
 dspParam.setSampleRate(sampleRate);
 dspParam.setBufferSize(samplesPerBlock);
}
```

Staying in PluginProcessor.cpp we add the code to call the process loop inside `processBlock`.

```cpp
void DocumentationExampleAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
 // Lock the mutex.
 std::unique_lock lock(mtx);
 
 // Disable denormals using a handy JUCE class.
 juce::ScopedNoDenormals noDenormals;
 
 // Connect the modulating filter input to the audio buffer.
 dsp.signalIn.connectFloats({buffer.getWritePointer(0), buffer.getWritePointer(1)}, buffer.getNumSamples());
 
 // OPTIONAL: Extract the transport information from the plugin host.
 syncTransport();
 
 // Run the process loop.
 dsp.process(0, buffer.getNumSamples());
 
 // Bulk transfer the output samples back into the audio buffer.
 dsp.signalOut.fastTransfer<float>({buffer.getWritePointer(0), buffer.getWritePointer(1)}, buffer.getNumSamples());
}
```

And here is some example code to syncronise the transport section from the plugin host, if it is available.

```cpp
void DocumentationExampleAudioProcessor::syncTransport()
{
 // Get the playhead
 juce::AudioPlayHead* playhead = getPlayHead();
 if (playhead)
 {
  // Set aside some variables to get the info.
  double tempo;
  double ppq;
  double seconds;
  
  // Get the play position from the playhead.
  auto optPositionInfo = playhead->getPosition();
  juce::AudioPlayHead::PositionInfo posInfo = optPositionInfo.orFallback(juce::AudioPlayHead::PositionInfo());
  
  // Get the current transport information from the parameters to fall back on.
  dspParam.getTransportInformation(tempo, ppq, seconds);
  
  // Syncronise the temporary variables with whatever is available.
  tempo = posInfo.getBpm().orFallback(tempo);
  ppq = posInfo.getPpqPosition().orFallback(ppq);
  seconds = posInfo.getTimeInSeconds().orFallback(seconds);
  
  // Set the new transport information back to the parameters object.
  dspParam.setTransportInformation(tempo, ppq, seconds);
 }
 else
 {
  // There's no transport information in this case, so we clear the transport information in the parameters object.
  dspParam.clearTransportInformation();
 }
}
```

We also set up parameter listeners to pass on user control changes to the DSP network. There are many ways to do this and you might have a favourite configuration, but my configuration involves a custom ParameterListener class which takes a lamba like this:

```cpp
lfoFrequencyParameter.onChange = [&](float newValue)
{
 // Lock the mutex, so we don't change the parameter while the process loop is running.
 std::unique_lock lock(mtx);
 
 // Make the change. The new value given by the parameter listener is already bounds checked.
 dsp.lfoFreq.setControl(newValue);
};
```

Here we are setting the LFO frequency by setting the control value from the [ControlConstant](@ref XDDSP::ControlConstant) we brought foward earlier.
