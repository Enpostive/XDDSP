# XDDSP Tutorial

Welcome to this quick tutorial, where we will go over the use of the XDDSP library. First we will introduce some of the basic concepts and the low level classes that encapsulate those concepts. Then we will build a simple component which includes a filter and an LFO. Then we will go over the steps needed to integrate the resulting component into an application.

### Design Goals

XDDSP is a header-only template library with a few design goals, which we will explore in this section.

#### A common base class for components.

The base class for the components is called XDDSP::Component. XDDSP::Component is a template base class utilising CRTP (Curiously Recurring Template Pattern). It encapsulates a couple of common features that make up a typical DSP processing loop including the ability to split a process loop into smaller steps and trigger points.

#### Component inputs and outputs to have human readable names.

A second base class is used called XDDSP::Coupler which also uses CRTP. XDDSP::Coupler encapsulates mechanisms for fetching input samples in a consistent way and supports multi-channel connections and bulk transfers. Every input into a component may be specified as a template argument to specify the kind of coupler, and a constructor parameter which initialises the coupler, often with a compile time reference.

Each component class in the library has inputs and outputs exposed as public members which can be addressed by name from other code outside of the component. The XDDSP::Coupler class is designed to be exposed this way, encapsulating input and output logic and making it easy to specify the connections between components when designing your DSP network. I have found that most code completion features in modern IDEs can locate and auto-complete input and output names.

#### Maximising code-encapsulation and reusability.

XDDSP is extremely modular. Each component is made to do just one thing and provide all the necessary connectivity required for its job. This means that, for example, the filter functionality is completely separate from the dynamics processing functionaliy and both are as easily interoperable as with the rest of the functions provided.

Furthermore, common features such as control inputs from the user interface or samples from sample buffers are just as connectable as other components, each being encapsulated inside a class derived from XDDSP::Coupler.

#### Portability and independence from other libraries.

The library is written in C++17, which has become a widely accepted standard with many compilers being available across many platforms. The only prerequisite to use is the C++ standard library. This does mean that XDDSP currently lacks certain important features, such as the ability to load media from disk. However, you may find that implementing a coupler to interface with your favourite codec library is trivial, enabling any component in the library to interface with it.

#### As many opportunities for automatic optimisation as possible.

As XDDSP is predominantly a header-only template library, many C++ compilers can exploit inlining and loop-consolidation tricks to reduce code size and increase performance. My own experience with Clang and Ghidra have shown that complex networks can be reduced into a single inner loop and multi-layer connections between components can be optimised away.

---

# Building A Custom Component

Custom components in XDDSP consist of template classes which inherit from the XDDSP::Component template. XDDSP::Component exposes an interface to enable and disable the component, along with 6 virtual methods. Any class which inherits from this CRTP class is called a component.

#### Component Enabling

You can check if any component is enabled by calling [XDDSP::Component::isEnabled](@ref Component_isEnabled). You can change the enabled status by calling [XDDSP::Component::isDisabled](@ref Component_isDisabled). Note that disabling the component will automatically reset it to mute its output buffers.

You put your code to reset the component into XDDSP::Component::reset. At a minimum, it is expected that XDDSP::Component::reset will clear the output buffers of the component by filling them with zeroes.

#### The Process Loop

XDDSP::Component declares a method called XDDSP::Component::process. This method should be used by any code external to the component to run the process loop. Care should be taken to ensure that components are processed in the proper order, that is starting with components connected to the inputs and ending with components connected to outputs.

The process loop in a component follows a specific routine. First, XDDSP::Component::startProcess is called and the return value is kept as the step size. The step size is allowed to change to be whatever is necessary, as long as it is not bigger than the sampleCount parameter. The intention with step sizes is to allow control signals to be calculated at intervals as opposed to being calculated every sample. A good example is a biquad filter with a frequency input. Even a keen listener is not going to notice when filter coefficents are only updated every 16 samples or so. Being able to process in smaller steps also allows the process loop to "interrupt" the process at a predetermined point in the loop. 

After getting the step size, the process loop repeatedly calles XDDSP::Component::stepProcess with step sized chunks. The process loop may call stepProcess with smaller chunks from time to time. The code inside XDDSP::Component::stepProcess may call [XDDSP::Component::setNextTrigger](@ref Component_setNextTrigger) to set a trigger point. Because of how symbol resolution works in C++17, this method can only be called by using a "this dereference" like `this->setNextTrigger(trigger)`.

When a trigger point is reached, the process loop calles XDDSP::Component::triggerProcess with the sample location of the trigger point.

After the process loop processes the buffer, it calls XDDSP::Component::finishProcess.

The XDDSP::Component class implements default methods for all of these methods. All of them are empty methods, except for XDDSP::Component::startProcess, the default implementation of which uses a template argument as the return value, unless sampleCount is smaller. This means that implementing a fixed sized step size is just a matter of providing the template argument in the XDDSP::Component template parameters.

All of these methods inside the process loop are called from the audio processing thread, meaning that they should be completely non-blocking and optimised for performance. Avoid memory allocations or deallocations, file operations or any other form of blocking IO. Mutex locking for inter-thread communication is sometimes needed to meet this requirement, it is best to keep those critical sections as short as possible.

**Note: In a future version these virtual methods will not be virtual and thus the object won't be polymorphic. All new software should avoid keeping pointers of XDDSP::ComponentBaseClass and only keep pointers or references to derived classes instead.**

---

#### Handy template

Here is a helpful template which you can copy into your application to use as a component class.

    #include "XDDSP/XDDSP.h"
    
    // Input types are specified as template arguments to the class. Input types go
    // first, because they never have defaults.
    template <typename SignalIn>
    class NewComponent : public Component<NewComponent<SignalIn>> {
      
      // Private data members here
      
     public:
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

---

### Adding subcomponents

Here we have modified the template to include a dynamic biquad and an LFO
