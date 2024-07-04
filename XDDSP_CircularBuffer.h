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









/*
 The CircularBuffer class encapsulates code and memory for implementing a circular buffer. The maximum size is specified as a power of two index (ie. size = 2^bits and maxDelay = size - 1). The buffer is allocated a fixed size determined at compile time.
 All classes work best when the code calls 'tapIn' first, then calls 'tapOut' for each required tap
 */


template <int BufferSizeBits, typename T = SampleType>
class CircularBuffer
{
 static constexpr PowerSize Size {BufferSizeBits};
 std::array<T, Size.size()> buffer;
 uint32_t bc;
public:
 CircularBuffer()
 {
  reset();
 }
 
 uint32_t getSize() const { return Size.size(); }
 
 void setMaximumLength(uint32_t l)
 {
  // Ignored but defined to enable it to compile in place with the other classes
 }
 
 void reset()
 {
  buffer.fill(0.);
  bc = 0;
 }
 
 T tapIn(T input)
 {
  bc = (bc + 1) & Size.mask();
  buffer[bc] = input;
  return input;
 }
 
 T tapOut(uint32_t delay) const
 {
  if (delay > Size.mask()) delay = Size.mask();
  return buffer[(bc - delay) & Size.mask()];
 }

 T& tapOut(uint32_t delay)
 {
  if (delay > Size.mask()) delay = Size.mask();
  return buffer[(bc - delay) & Size.mask()];
 }
};









/*
 The DynamicCircularBuffer implements the same code except that it will resize itself dynamically depending on the requested delay length. The size of the buffer will always be a power of two big enough to accomodate.
 */

template <typename T = SampleType>
class DynamicCircularBuffer
{
 PowerSize size;
 std::vector<T> buffer;
 uint32_t bc;
public:
 DynamicCircularBuffer()
 {
  setMaximumLength(32);
 }
 
 uint32_t getSize() const { return size.size(); }
 
 void setMaximumLength(uint32_t s)
 {
  size.setToNextPowerTwo(s);
  reset();
 }
 
 void reset()
 {
  buffer.assign(size.size(), 0.);
  bc = 0;
 }
 
 T tapIn(T input)
 {
  bc = (bc + 1) & size.mask();
  buffer[bc] = input;
  return input;
 }
 
 T tapOut(uint32_t delay) const
 {
  if (delay > size.mask()) delay = size.mask();
  return buffer[(bc - delay) & size.mask()];
 }
 
 T& tapOut(uint32_t delay)
 {
  if (delay > size.mask()) delay = size.mask();
  return buffer[(bc - delay) & size.mask()];
 }
};










/*
 The ModulusCircularBuffer is guaranteed to only occupy enough memory to accomodate the required buffer, however it is slower as it uses modulus operations instead of bitmasks
 */

template <typename T = SampleType>
class ModulusCircularBuffer
{
 uint32_t size {1};
 
 std::vector<T> buffer;
 uint32_t bc;
public:
 ModulusCircularBuffer()
 {
  setMaximumLength(32);
 }
 
 uint32_t getSize() const { return size; }
 
 void setMaximumLength(uint32_t s)
 {
  size = s;
  
  buffer.assign(size, 0.);
  
  // Guarantee that only enough memory for the buffer is actually allocated in most
  // circumstances
  buffer.shrink_to_fit();
  
  bc = 0;
 }
 
 void reset()
 {
  buffer.assign(size, 0.);
  bc = 0;
 }
 
 T tapIn(T input)
 {
  bc = (bc + 1) % size;
  buffer[bc] = input;
  return input;
 }
 
 T tapOut(uint32_t delay) const
 {
  if (delay >= size) delay = size - 1;
  return buffer[(bc - delay + size) % size];
 }
 
 T& tapOut(uint32_t delay)
 {
  if (delay >= size) delay = size - 1;
  return buffer[(bc - delay + size) % size];
 }
 
 // Allows the buffer to be used with just one modulus operation per sample instead of two
 // This function will only delay by the maximum delay amount
 T oneTapRun(T input)
 {
  buffer[bc] = input;
  bc = (bc + 1) % size;
  return buffer[bc];
 }
};










}


#endif /* XDDSP_CircularBuffer_h */
