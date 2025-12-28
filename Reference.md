# XDDSP Reference

In this document we list the many coupler, component and utility classes which are provided by XDDSP.

# Coupler Types {#coupler_types}

[Coupler](@ref XDDSP::Coupler)	- A template class which encapsulates the connection paradigm. All other couplers inherit from this one to keep the interface consistent. If you want to create your own couplers, inheriting from [Coupler](@ref XDDSP::Coupler) is best practice.

[Connector](@ref XDDSP::Connector)	- This is a simple straight connector to another coupler.

[PConnector](@ref XDDSP::PConnector)	- A connector which can be connected or reconnected to another coupler type at run time.

[ChannelPicker](@ref XDDSP::ChannelPicker)	- An input for picking one channel from a multi-channel input.

[BufferCoupler](@ref XDDSP::BufferCoupler)	- A coupler for coupling buffers that are different types (eg. connecting float to double).

[ControlConstant](@ref XDDSP::ControlConstant)	- A coupler which outputs a constant signal on each channel. A modifier function may be given to modify given control values before they enter the DSP network.

[AudioPropertiesInput](@ref XDDSP::AudioPropertiesInput)	- A coupler for producing signals based on a chosen DSP Parameter (such as sample rate or song tempo) This coupler is similar to ControlConstant, except that the control signal is taken from the Parameters Object and scaled by a multiplier.

[Switch](@ref XDDSP::Switch)	- A coupler which takes multiple coupler inputs and provides a switch to choose one input for its output. Each input must have the same number of channels as the output.

[Sum](@ref XDDSP::Sum)	- A coupler which sums its coupled inputs. Each input must have the same number of channels as the output.

[Product](@ref XDDSP::Product)	- A coupler which multiplies its coupled inputs together to produce the output. Each input must have the same number of channels as the output.

[SignalModifier](@ref XDDSP::SignalModifier)	- A signal modifier coupler which applies a specified function to its input.

[SamplePlaybackHead](@ref XDDSP::SamplePlaybackHead)	- A coupler which outputs samples from a sample buffer, using input from another coupler to choose the sample position.

[BufferReader](@ref XDDSP::BufferReader)	- A coupler for connecting various length buffers to each channel. Each buffer is expected to be the same type but can have different lengths. Bounds checking is performed.

[PluginInput](@ref XDDSP::PluginInput)	- A coupler which is convenient when the sample data type isn't known at compile time.

[Output](@ref XDDSP::Output)	- An output which encapsulates an [OutputBuffer](@ref XDDSP::OutputBuffer) inside a coupler so that it can be readily connected to by other components

---

# Component Library Reference {#component_library_reference}

[Component](@ref XDDSP::Component)	- A CRTP component template which encapsulates the implementation of the process loop logic. All components inherit from this class.

## Analytics

[LUFSBlockCollector](@ref XDDSP::LUFSBlockCollector)	- A subset of the LUFS standard. Measures input blocks RMS levels and reports the overall loudness.

[DebugWatch](@ref XDDSP::DebugWatch)	- An input only component which can come in handy when debugging your DSP Network.

[SignalProbe](@ref XDDSP::SignalProbe)	- A probe suitable for measureing minimum, maximum and instantaneous values in a signal and reading them from non-DSP code. Suitable for providing data to level meters.

[InterfaceBuffer](@ref XDDSP::InterfaceBuffer)	- A component which buffers samples from its input and makes them available in a thread safe manner. Suitable for providing data to scopes and other analysers.

## Delays

[LowQualityDelay](@ref XDDSP::LowQualityDelay)	- A simple delay component with no iterpolation.

[MultiTapDelay](@ref XDDSP::MultiTapDelay)	- A simple delay with multiple taps.

[MediumQualityDelay](@ref XDDSP::MediumQualityDelay)	- A simple delay component with linear interpolation.

[HighQualityDelay](@ref XDDSP::HighQualityDelay)	- A simple delay component with hermite interpolation.

## Signal Generators, Envelopes and Modulators

[Ramp](@ref XDDSP::Ramp)	- A component for generating ramp signals.

[RampTo](@ref XDDSP::RampTo)	- A component for ramping from a current value to a new value.

[ADSRGenerator](@ref XDDSP::ADSRGenerator)	- A component for generating ADSR envelopes.

[Trapezoid](@ref XDDSP::Trapezoid)	- This component outputs an Attack-Release envelope controlled by the time input.

[PiecewiseEnvelopeSampler](@ref XDDSP::PiecewiseEnvelopeSampler)	- Samples a piecewise envelope, using an input to control what part of the envelope is sampled.

[PiecewiseEnvelope](@ref XDDSP::PiecewiseEnvelope)	- Run a piecewise envelope.

[NoiseGenerator](@ref XDDSP::NoiseGenerator)	- A component which outputs white noise.

[PinkNoiseGenerator](@ref XDDSP::PinkNoiseGenerator)	- A component which outputs pink noise.

[AnalogNoiseSimulator](@ref XDDSP::AnalogNoiseSimulator)	- A component for generating convincing analog noise to add to a signal.

[FuncOscillator](@ref XDDSP::FuncOscillator)	- A basic oscillator which uses a callable object to generate a waveform.

[BandLimitedSawOscillator](@ref XDDSP::BandLimitedSawOscillator)	- A band-limited sawtooth oscillator.

[BandLimitedSquareOscillator](@ref XDDSP::BandLimitedSquareOscillator)	- A band-limited square wave oscillator with PWM input.

[BandLimitedTriangleOscillator](@ref XDDSP::BandLimitedTriangleOscillator)	- A band-limited triangle wave oscillator.

[MIDIScheduler](@ref XDDSP::MIDIScheduler)	- A component which takes MIDI CC events and outputs a corresponding signal.

[TimeSignal](@ref XDDSP::TimeSignal)	- A component which outputs three time signals.

[Counter](@ref XDDSP::Counter)	- A counter object.

[LoopCounter](@ref XDDSP::LoopCounter)	- A looping counter object.

## Signal Modifiers

[SimpleGain](@ref XDDSP::SimpleGain)	- A component which applies a gain signal to an input.

[Rectifier](@ref XDDSP::Rectifier)	- A component which rectifies a signal.

[SignalDelta](@ref XDDSP::SignalDelta)	- A component which takes an input signal and outputs a delta signal.

[Clipper](@ref XDDSP::Clipper)	- A component which constrains a signal between two other signals.

[Maximum](@ref XDDSP::Maximum)	- A component which takes multiple signals and outputs the signal which has the highest level.

[TopBottomSwitch](@ref XDDSP::TopBottomSwitch)	- Selects between two different signals depending on the sign of a third.

[ControlModulator](@ref XDDSP::ControlModulator)	- A component which downsamples a signal and translates it from an LFO or an envelope to a control range.

[Waveshaper](@ref XDDSP::Waveshaper)	- A component which applies a waveshaping function to an input.

## Dynamics

[ExponentialEnvelopeFollower](@ref XDDSP::ExponentialEnvelopeFollower)	- Creates an exponential envelope signal from an input signal.

[LinearEnvelopeFollower](@ref XDDSP::LinearEnvelopeFollower)	- Creates a linear envelope signal from an input signal.

[DynamicsProcessingGainSignal](@ref XDDSP::DynamicsProcessingGainSignal)	- Takes an envelope signal as input and produces a gain signal for dynamics control.

## Filters

[ConvolutionFilter](@ref XDDSP::ConvolutionFilter) -	A component for performing convolution on an input signal.

[OnePoleAveragingFilter](@ref XDDSP::OnePoleAveragingFilter)	- A component encapsulating a simple one-pole averaging filter, suitable for use as a simple lowpass filter, control smoother, RMS filter etc.

[SignalAverage](@ref XDDSP::SignalAverage)	- A component for outputing a signal which tracks the average level of the input signal. Uses a circular buffer and a rectangular average as an alternative to a one-pole filter.

[StaticBiquad](@ref XDDSP::StaticBiquad)	- A component encapsulating a simple static biquad filter.

[DynamicBiquad](@ref XDDSP::DynamicBiquad)	- A component containing a dynamic biquad filter.

[CrossoverFilter](@ref XDDSP::CrossoverFilter)	- A component encapsulating a Linkwitz-Riley filter suitable for building phase-aligned crossovers.

[FIRHilbertTransform](@ref XDDSP::FIRHilbertTransform)	- A component encapsulating a Hilbert Transform using an FIR.

[ConvolutionHilbertFilter](@ref XDDSP::ConvolutionHilbertFilter)	- A component encapsulating a Hilbert Transform using a convolution kernel.

[IIRHilbertApproximator](@ref XDDSP::IIRHilbertApproximator)	- A component encapsulating a hilbert approximator using an IIR filter.

## Signal Routing and Mixing

[Crossfader](@ref XDDSP::Crossfader)	- A component for crossfading between two signals.

[Panner](@ref XDDSP::Panner) -	A component for panning an input signal between two output signals.

[StereoPanner](@ref XDDSP::StereoPanner)	- A component for simplifying the panning of a stereo signal.

[MonoToStereoMixBus](@ref XDDSP::MonoToStereoMixBus)	- A component with connectable mono inputs.

[StereoToStereoMixBus](@ref XDDSP::StereoToStereoMixBus)	- A component with connectable stereo inputs.

[MixDown](@ref XDDSP::MixDown)	- A component which couples an array of inputs and sums them into a single output.

## Polyphony

[PolySynthParameters](@ref XDDSP::PolySynthParameters)	- An extension of Parameters that contains extra parameters suitable for a MIDI polyphonic synthesiser.

[SummingArray](@ref XDDSP::SummingArray)	- A special component which creates an array of internal component and sums an output to one output.

[MIDIPoly](@ref XDDSP::MIDIPoly)	- A special component which connects to a collection of voices to make a polyphonic component.

---

## Utility Classes

Here are some classes provided by the library that aren't components but may come in handy.

[MinMax](@ref XDDSP::MinMax)	- A class encapsulating some common min-max functionality.

[LogarithmicScale](@ref XDDSP::LogarithmicScale)	- To be deprecated in favour of adding this functionality to MinMax.

[PowerSize](@ref XDDSP::PowerSize)	- A class containing commonly used functionality in relation to powers of two.

[IntegerAndFraction](@ref XDDSP::IntegerAndFraction)	- A class encapsulating the best algorithm for splitting a sample into its integer and fraction components.

## Data Structures

[PiecewiseEnvelopeListener](@ref XDDSP::PiecewiseEnvelopeListener)	- Implements a listener which is notified of changes to a piecewise envelope.

[PiecewiseEnvelopeData](@ref XDDSP::PiecewiseEnvelopeData)	- A class containing a data structure to describe a piecewise envelope and code to synthesise the envelope.

## Filter Kernels

[BiquadFilterCoefficients](@ref XDDSP::BiquadFilterCoefficients) -	Holds the state of configuration for a biquad filter.

[BiquadFilterKernel](@ref XDDSP::BiquadFilterKernel)	- A simple biquad filter implementation.

[BiquadFilterPublicInterface](@ref XDDSP::BiquadFilterPublicInterface)	- A convenient class that can be used to expose the BiquadCoefficients filter response calculators without exposing the configurators.

[LinkwitzRileyFilterCoefficients](@ref XDDSP::LinkwitzRileyFilterCoefficients)	- A class encapsulating code for generating Linkwitz-Riley filter coefficients.

[LinkwitzRileyFilterKernel](@ref XDDSP::LinkwitzRileyFilterKernel)	- A class encapsulating a Linkwitz-Riley filter kernel.

## Band-limited Step and Band-limited Ramp

[BLEPLookup](@ref XDDSP::BLEPLookup)	- A class encapsulating the logic to perform lookups in the Band-Limited stEP and Band-Limited rAMP tables.

[BLEPGenerator](@ref XDDSP::BLEPGenerator)	- Objects of this class can be used to trigger and buffer the samples for band limited steps and ramps. These are useful for synthesizing band-limited oscillators and band-limited distortion algorithms.

## Circular Buffers

[CircularBuffer](@ref XDDSP::CircularBuffer)	- A class implementing a fixed size circular buffer.

[DynamicCircularBuffer](@ref XDDSP::DynamicCircularBuffer)	- A class implementing a circular buffer which can be resized.

[ModulusCircularBuffer](@ref XDDSP::ModulusCircularBuffer)	- A class implementing a memory efficient circular buffer with a performance penalty.

## Lookup Tables

[LookupTable](@ref XDDSP::LookupTable)	- A callable object which creates a lookup table from a function.

[WaveshapeLookupTable](@ref XDDSP::WaveshapeLookupTable)	- A callable function object which transforms the input signal using a lookup table.

[RandomNumberBuffer](@ref XDDSP::RandomNumberBuffer)	- A class for accessing a global lookup table made of deterministic random numbers.

## Output Buffers

[OutputBuffer](@ref XDDSP::OutputBuffer)	- An implementation of a buffer to be used to store output data from a DSP process.

## Analytics

[AutoCorrelator](@ref XDDSP::AutoCorrelator)	- A class which pre-allocates a processing buffer to perform autocorrelation.

[DynamicAutoCorrelator](@ref XDDSP::DynamicAutoCorrelator)	- A class with a buffer stored in an internal std::vector used for doing antocorrelation.

[LinearEstimator](@ref XDDSP::LinearEstimator)

[IntersectionEstimator](@ref XDDSP::IntersectionEstimator)	- A class for estimating the intersection of a cubic waveform and flat line fixed at some value.
