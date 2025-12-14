//
//  XDDSP_CircularBuffer.h
//  XDDSP
//
//  Created by Adam Jackson on 26/5/2022.
//

#ifndef XDDSP_CircularBuffer_h
#define XDDSP_CircularBuffer_h

#include "XDDSP_Types.h"
#include "XDDSP_Functions.h"










namespace XDDSP
{










/**
 * @brief A class implementing a fixed size circular buffer.
 * 
 * This is not a component. For components implementing delay lines, see LowQualityDelay, MultiTapDelay, MediumQualityDelay and HighQualityDelay.
 * 
 * The CircularBuffer class encapsulates code and memory for implementing a circular buffer. The maximum size is specified as a power of two index (ie. size = 2^bits and maxDelay = size - 1). The buffer is allocated a fixed size determined at compile time. To implement a delay line, the code calls 'tapIn' first with the current sample, then calls 'tapOut' for each required tap. 
 * 
 * @tparam BufferSizeBits The size of the required buffer as a logarithm of 2 (ie. 3 = 8, 4 = 16, 8 = 256, 16 = 65536 etc.)
 * @tparam T The type to use in the buffer (this can be any type).
 */
template <int BufferSizeBits, typename T = SampleType>
class CircularBuffer
{
 static constexpr PowerSize Size {BufferSizeBits};
 std::array<T, Size.size()> buffer;
 uint32_t bc;
public:

 /**
  * @brief Construct a new Circular Buffer object
  * 
  */
 CircularBuffer()
 {
  reset(0.);
 }
 
 /**
  * @brief Get the size of the buffer.
  * 
  * @return uint32_t 
  */
 uint32_t getSize() const { return Size.size(); }
 
 /**
  * @brief This method is ignored and only included so that it can be swapped in with the dynamic circular buffers if necessary.
  * 
  * @param l Ignored.
  */
 void setMaximumLength(uint32_t l)
 {
  // Ignored but defined to enable it to compile in place with the other classes
 }
 
 /**
  * @brief Reset the buffer
  * 
  * @param fill A value to be copied into each buffer element.
  */
 void reset(T fill)
 {
  buffer.fill(fill);
  bc = 0;
 }
 
 /**
  * @brief Tap the next element into the buffer and advance the buffer.
  * 
  * @param input The next input.
  * @return T The same input is returned.
  */
 T tapIn(T input)
 {
  bc = (bc + 1) & Size.mask();
  buffer[bc] = input;
  return input;
 }
 
 /**
  * @brief Tap out an element from the buffer.
  *        There is no bounds checking, but an element is always returned.
  * 
  * @param delay How many elements back to go to find the required elmenet.
  * @return T The required element.
  */
 T tapOut(uint32_t delay) const
 {
  if (delay > Size.mask()) delay = Size.mask();
  return buffer[(bc - delay) & Size.mask()];
 }

 /**
  * @brief Return a reference to an element in the buffer.
  *        This enables the modification of elements that are in the buffer already.
  *        There is no bounds checking, but an element is always returned.
  * 
  * @param delay How many elements back to go to find the required elmenet.
  * @return T& The reference to the required element.
  */
 T& tapOut(uint32_t delay)
 {
  if (delay > Size.mask()) delay = Size.mask();
  return buffer[(bc - delay) & Size.mask()];
 }
};










 /**
  * @brief A class implementing a circular buffer which can be resized.
  * 
  * This is not a component. For components implementing delay lines, see LowQualityDelay, MultiTapDelay, MediumQualityDelay and HighQualityDelay.
  * 
  * The size of the buffer can be adjusted at any time but the resulting size will always be a power of two big enough to accomodate. To implement a delay line, the code calls 'tapIn' first with the current sample, then calls 'tapOut' for each required tap. 
  *
  * @tparam T The element type to use in the buffer.
  */
template <typename T = SampleType>
class DynamicCircularBuffer
{
 PowerSize size;
 std::vector<T> buffer;
 uint32_t bc;
public:
 /**
  * @brief Construct a new Dynamic Circular Buffer object
  * 
  */
 DynamicCircularBuffer()
 {
  setMaximumLength(32);
 }
 
 /**
  * @brief Get the size of the buffer.
  * 
  * @return uint32_t 
  */
 uint32_t getSize() const { return size.size(); }
 
 /**
  * @brief Set the length of the buffer. The size is rounded up to the next power of 2.
  * 
  * @param s The required size. The size is rounded up to the next power of 2.
  */
 void setMaximumLength(uint32_t s)
 {
  size.setToNextPowerTwo(s);
  buffer.resize(size.size());
 }
 
 /**
  * @brief Reset the buffer
  * 
  * @param fill A value to be copied into each buffer element.
  */
 void reset(T fill)
 {
  buffer.assign(size.size(), fill);
  bc = 0;
 }
 
 /**
  * @brief Tap the next element into the buffer and advance the buffer.
  * 
  * @param input The next input.
  * @return T The same input is returned.
  */
 T tapIn(T input)
 {
  bc = (bc + 1) & size.mask();
  buffer[bc] = input;
  return input;
 }
 
 /**
  * @brief Tap out an element from the buffer.
  *        There is no bounds checking, but an element is always returned.
  * 
  * @param delay How many elements back to go to find the required elmenet.
  * @return T The required element.
  */
 T tapOut(uint32_t delay) const
 {
  if (delay > size.mask()) delay = size.mask();
  return buffer[(bc - delay) & size.mask()];
 }
 
 /**
  * @brief Return a reference to an element in the buffer.
  *        This enables the modification of elements that are in the buffer already.
  *        There is no bounds checking, but an element is always returned.
  * 
  * @param delay How many elements back to go to find the required elmenet.
  * @return T& The reference to the required element.
  */
 T& tapOut(uint32_t delay)
 {
  if (delay > size.mask()) delay = size.mask();
  return buffer[(bc - delay) & size.mask()];
 }
};










 /**
  * @brief A class implementing a memory efficient circular buffer with a performance penalty.
  * 
  * This is not a component. For components implementing delay lines, see LowQualityDelay, MultiTapDelay, MediumQualityDelay and HighQualityDelay.
  * 
  * The size of the buffer can be adjusted at any time. The buffer can be made any size, however it is slower as it uses modulus operations instead of bitmasks. To implement a delay line, the code calls 'tapIn' first with the current sample, then calls 'tapOut' for each required tap. 
  * 
  * @tparam T 
  */
template <typename T = SampleType>
class ModulusCircularBuffer
{
 uint32_t size {1};
 
 std::vector<T> buffer;
 uint32_t bc;
public:
/**
 * @brief Construct a new Circular Buffer object
 * 
 */
 ModulusCircularBuffer()
 {
  setMaximumLength(32);
 }
 
 /**
  * @brief Get the size of the buffer.
  * 
  * @return uint32_t The size of the buffer.
  */
 uint32_t getSize() const { return size; }
 
 /**
  * @brief Set the length of the buffer.
  * 
  * @param s The new length of the buffer.
  */
 void setMaximumLength(uint32_t s)
 {
  size = s;
  
  buffer.resize(size);
  buffer.shrink_to_fit();
  
  bc = 0;
 }
 
 /**
  * @brief Reset the buffer
  * 
  * @param fill A value to be copied into each buffer element.
  */
 void reset(T fill)
 {
  buffer.assign(size, fill);
  bc = 0;
 }
 
 /**
  * @brief Tap the next element into the buffer and advance the buffer.
  * 
  * @param input The next input.
  * @return T The same input is returned.
  */
 T tapIn(T input)
 {
  bc = (bc + 1) % size;
  buffer[bc] = input;
  return input;
 }
 
 /**
  * @brief Tap out an element from the buffer.
  *        There is no bounds checking, but an element is always returned.
  * 
  * @param delay How many elements back to go to find the required elmenet.
  * @return T The required element.
  */
 T tapOut(uint32_t delay) const
 {
  if (delay >= size) delay = size - 1;
  return buffer[(bc - delay + size) % size];
 }
 
 /**
  * @brief Return a reference to an element in the buffer.
  *        This enables the modification of elements that are in the buffer already.
  *        There is no bounds checking, but an element is always returned.
  * 
  * @param delay How many elements back to go to find the required elmenet.
  * @return T& The reference to the required element.
  */
 T& tapOut(uint32_t delay)
 {
  if (delay >= size) delay = size - 1;
  return buffer[(bc - delay + size) % size];
 }
 
 /**
  * @brief Tap the next element into the buffer and tap out the element being replaced.
  *        This method can only return the element at the maximum delay length. It is included to enable a tapin and tapout operation to be done using only a single modulus operation, improving performance.
  * @param input The next input.
  * @return T The element being replaced.
  */
 T oneTapRun(T input)
 {
  buffer[bc] = input;
  bc = (bc + 1) % size;
  return buffer[bc];
 }
};










}


#endif /* XDDSP_CircularBuffer_h */
