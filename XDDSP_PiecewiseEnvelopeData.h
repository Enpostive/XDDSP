/*
  ==============================================================================

    PiecewiseEnvelopeData.h
    Created: 23 Apr 2023 11:03:54am
    Author:  Adam Jackson

  ==============================================================================
*/

#ifndef XDDSP_PIECEWISEENVELOPEDATA_H
#define XDDSP_PIECEWISEENVELOPEDATA_H










namespace XDDSP
{










  /**
   * @brief Implements a listener which is notified of changes to a piecewise envelope.
   * 
   */
class PiecewiseEnvelopeListener
{
public:
 PiecewiseEnvelopeListener() {}
 virtual ~PiecewiseEnvelopeListener() {}
 
 // Used to trigger repaints on editors
 virtual void piecewiseEnvelopeChanged() {}
 
 // Called when changes are about to be made
 virtual void piecewiseEnvelopeBeginChange() {}

 // Called when changes have been made
 virtual void piecewiseEnvelopeEndChange() {}
};










/**
 * @brief A class containing a data structure to describe a piecewise envelope and code to synthesise the envelope.
 * 
 */
template <unsigned int MaxPoints = 10, int CurveResolution = 5>
class PiecewiseEnvelopeData
{
public:
 typedef PiecewiseEnvelopeListener Listener;
private:
 
 static constexpr SampleType CurveStepSize = 1./static_cast<SampleType>(CurveResolution - 1);
 
 std::vector<PiecewiseEnvelopeListener*> listeners;
 
 struct Point
 {
  SampleType value {0.};
  SampleType curve {0.};
  std::vector<SampleType> samples {};
  SampleType time {0};
  SampleType length {0};
  SampleType timeGradient;
 };
 
 std::vector<Point> points;
 int pointCount {0};
 int loopStartPoint {-1};
 int loopEndPoint {-1};
 bool constrainEdits {true};

 void calculateSamples(int point)
 {
  if (point < 0 || point > pointCount - 2) return;
  
  SampleType x0 = points[point].value;
  SampleType x1 = points[point + 1].value;
  
  points[point].samples[0] = x0;
  
  SampleType curveExponent = pow(2., points[point].curve);
  for (int i = 1; i < CurveResolution - 1; ++i)
  {
   SampleType x = static_cast<SampleType>(i)*CurveStepSize;
   SampleType y = exponentialCurve(x0, x1, x, curveExponent);
   points[point].samples[i] = y;
  }
  
  points[point].samples[CurveResolution - 1] = x1;
  points[point].length = points[point + 1].time - points[point].time;
  points[point].timeGradient = static_cast<SampleType>(CurveResolution - 1)/points[point].length;
 }
 
 void sortPoints()
 {
  std::sort(points.begin(), points.begin() + pointCount,
            [](const Point& a, const Point& b)
            {
   return a.time < b.time;
  });
  
  for (int i = 0; i < pointCount - 1; ++i) calculateSamples(i);
 }
 
 void insertPoint(int index, const Point &point)
 {
  for (int i = pointCount; i > index; --i)
  {
   points[i] = points[i - 1];
  }
  points[index] = point;
  ++pointCount;
  if (index > 0) calculateSamples(index - 1);
  calculateSamples(index);
 }
 
 void sendUpdateMessage()
 {
  for (auto l: listeners)
  {
   l->piecewiseEnvelopeChanged();
  }
 }
 
 int doAddPoint(SampleType time, SampleType value, SampleType curve)
 {
  if (pointCount == MaxPoints) return -1;
  
  Point newPoint =
  {
   .value = value,
   .curve = curve,
   .time = time
  };
  
  int insertIndex;
  for (insertIndex = 0;
       insertIndex < pointCount && points[insertIndex].time <= time;
       ++insertIndex)
  {}
  insertPoint(insertIndex, newPoint);
  return insertIndex;
 }
 
 void doRemovePoint(int index)
 {
  if (index < 0 || index >= pointCount) return;
  for (int i = index; i < pointCount - 1; ++i)
  {
   points[i] = points[i + 1];
  }
  --pointCount;
  if (index > 0) calculateSamples(index - 1);
  calculateSamples(index);
 }
 
 void clearLoop()
 {
  loopStartPoint = loopEndPoint = -1;
 }
 
 
public:
 
 /**
  * @brief Add a listener to the list of listeners to be alerted when a change is made to this envelope.
  * 
  * @param l The listener to add.
  */
 void addListener(PiecewiseEnvelopeListener *l)
 {
  listeners.emplace_back(l);
 }
 
 /**
  * @brief Remove a listener from the list of listeners.
  * 
  * @param l The listener to remove.
  */
 void removeListener(PiecewiseEnvelopeListener *l)
 {
  auto it = std::find(listeners.begin(), listeners.end(), l);
  if (it != listeners.end()) listeners.erase(it);
 }
 
 /**
  * @brief Return the number of points currently being used to describe the envelope.
  * 
  * @return int The number of points.
  */
 int getPointCount() const
 { return pointCount; }
 
 /**
  * @brief Get a point from the structure
  * 
  * @param index The index of the point.
  * @param time The time of the point is returned back though this parameter.
  * @param value The value of the point is returned back though this parameter.
  * @param curve The curve setting of the point is returned back though this parameter.
  * @return true Returned if the point exists.
  * @return false Returned if no such point exists.
  */
 bool getPoint(int index, SampleType &time, SampleType &value, SampleType &curve)
 {
  if (index < 0 || index >= pointCount) return false;
  
  time = points[index].time;
  value = points[index].value;
  curve = points[index].curve;
  return true;
 }
 
 /**
  * @brief Remove all points from the structure.
  * 
  */
 void clearPoints()
 {
  pointCount = 0;
  clearLoop();
  sendUpdateMessage();
 }
 
 /**
  * @brief Adds a new point to the structure and returns the new index of that point.
  * 
  * The points are always kept in time order. Adding a new point might push points that are already in the list further up the list.
  * 
  * @param time The time of the new point.
  * @param value The value of the new point.
  * @param curve The curve of the new point.
  * @return int The index of the new point.
  */
 int addPoint(SampleType time, SampleType value, SampleType curve)
 {
  int index = doAddPoint(time, value, curve);
  if (index >= loopStartPoint && index <= loopEndPoint)
  {
   if (isLoopSustainPoint()) ++loopStartPoint;
   ++loopEndPoint;
  }
  else if (index < loopStartPoint)
  {
   ++loopStartPoint;
   ++loopEndPoint;
  }
  sendUpdateMessage();
  return index;
 }
 
 /**
  * @brief Remove the point at some index.
  * 
  * @param index The index of the point to remove.
  */
 void removePoint(int index)
 {
  doRemovePoint(index);
  if (index >= loopStartPoint && index <= loopEndPoint)
  {
   if (isLoopSustainPoint()) clearLoop();
   else --loopEndPoint;
  }
  else if (index < loopStartPoint)
  {
   --loopStartPoint;
   --loopEndPoint;
  }
  sendUpdateMessage();
 }
 
 /**
  * @brief Enable or disable constrained edits.
  * 
  * When a point change is requested, the change may or may not be constrained, depending on this setting. When set to true, points cannot be moved to go before the point before it or after the point after it. When set to false, the points are reordered as necessary to keep the points in time order.
  * 
  * @param ce The constrain edits flag.
  */
 void setConstrainEdits(bool ce)
 { constrainEdits = ce; }
 
 /**
  * @brief Change a point on the curve.
  * 
  * When a point change is requested, the change may or may not be constrained, depending on the setting given in PiecewiseEnvelopeData::setContrainEdits. When set to true, points cannot be moved to go before the point before it or after the point after it. When set to false, the points are reordered as necessary to keep the points in time order. The index of the changed point might change, so the caller is returned the index of the point after chages are made.
  * 
  * @param index The index of the point to change.
  * @param time The new time of the point.
  * @param value The new value of the point.
  * @param curve The new curve of the point.
  * @return int The new index of the point, or the old one if the index didn't change.
  */
 int changePoint(int index, SampleType time, SampleType value, SampleType curve)
 {
  if (constrainEdits)
  {
   if (pointCount > 1)
   {
    if (index == 0)
    {
     time = fastMin(time, points[1].time);
    }
    else if (index == pointCount - 1)
    {
     time = fastMax(time, points[pointCount - 2].time);
    }
    else
    {
     time = fastBoundary(time, points[index - 1].time, points[index + 1].time);
    }
   }
   points[index].time = time;
   points[index].value = value;
   points[index].curve = curve;
   if (index > 0) calculateSamples(index - 1);
   calculateSamples(index);
   sendUpdateMessage();
  }
  else
  {
   doRemovePoint(index);
   index = doAddPoint(time, value, curve);
   sendUpdateMessage();
  }
  return index;
 }
 
 /**
  * @brief Change just the curve of a point.
  * 
  * @param index Index of the point to change.
  * @param curve The new curve setting.
  */
 void changePointCurve(int index, SampleType curve)
 {
  points[index].curve = curve;
  calculateSamples(index);
  sendUpdateMessage();
 }
 
 /**
  * @brief Get the length of the envelope.
  * 
  * @return SampleType The time position of the last point in the envelope.
  */
 SampleType getEnvelopeLength()
 {
  if (pointCount == 0) return 0.;
  return points[pointCount - 1].time;
 }
 
 /**
  * @brief Calculate an envelope value at any given time.
  * 
  * @param sampleTime Time value to find the envelope value for.
  * @return SampleType The calculated envelope value.
  */
 SampleType resolveRandomPoint(SampleType sampleTime)
 {
  if (pointCount == 0) return 0.;
  if (pointCount == 1) return points[0].value;
  int point;
  for (point = 0;
       point < pointCount - 1 && points[point + 1].time <= sampleTime;
       ++point)
  {}
  if (point == pointCount - 1) return points[point].value;
  if (points[point].time > sampleTime) return points[point].value;
  
  const SampleType segmentTime = sampleTime - points[point].time;
  if (segmentTime == 0) return points[point].value;
  
  const SampleType lineSamplePos = segmentTime*points[point].timeGradient;
  const SampleType lineSample = static_cast<int>(lineSamplePos);
  const SampleType lineSampleFrac = lineSamplePos - lineSample;
  
  if (lineSampleFrac == 0.) return points[point].samples[lineSample];
  
  return LERP(lineSampleFrac,
              points[point].samples[lineSample],
              points[point].samples[lineSample + 1]);
 }
 
 /**
  * @brief Get the loop start point index.
  * 
  * @return int The index of the start loop point.
  */
 int getLoopStartPoint()
 { return loopStartPoint; }
 
 /**
  * @brief Get the loop end point index.
  * 
  * @return int The index of the end loop point.
  */
 int getLoopEndPoint()
 { return loopEndPoint; }
 
 /**
  * @brief Get the time of the start of the loop.
  * 
  * @return SampleType The time of the start of the loop.
  */
 SampleType getLoopStartTime()
 {
  if (loopStartPoint == -1) return 0.;
  return points[loopStartPoint].time;
 }
 
 /**
  * @brief Get the time of the end of the loop.
  * 
  * @return SampleType The time of the end of the loop.
  */
 SampleType getLoopEndTime()
 {
  if (loopEndPoint == -1) return 0.;
  return points[loopEndPoint].time;
 }
 
 /**
  * @brief Test if the loop start point and loop end point are the same point.
  * 
  * @return true If the loop start point and loop end point are the same point.
  * @return false If the loop start and loop end points are different.
  */
 bool isLoopSustainPoint()
 { return (loopStartPoint == loopEndPoint) && (loopStartPoint > -1); }
 
 /**
  * @brief Set a loop point.
  * 
  * Calling setLoopPoint once will set a sustain point
  * Calling two times with different indexes will set a start and end loop point
  * Calling after setting a loop will change the loop end point or start point
  * depending on if index < loopStartPoint
  * 
  * @param index 
  */
 void setLoopPoint(int index)
 {
  if (index < 0 || index > (pointCount - 1)) return;
  if ((loopStartPoint == loopEndPoint) && (loopStartPoint == -1))
  {
   loopStartPoint = loopEndPoint = index;
  }
  else if (index < loopStartPoint) loopStartPoint = index;
  else loopEndPoint = index;
  sendUpdateMessage();
 }
 
 /**
  * @brief Clear the loop points.
  * 
  */
 void clearLoopPoints()
 {
  clearLoop();
  sendUpdateMessage();
 }
 
 /**
  * @brief Create a string representation that can be used to restore the state of the piecewise envelope.
  * 
  * @return std::string The string representation of the envelope.
  */
 std::string saveStateToString()
 {
  std::string state = std::to_string(pointCount);
  state += " " + std::to_string(loopStartPoint);
  state += " " + std::to_string(loopEndPoint);
  for (int i = 0; i < pointCount; ++i)
  {
   state += " " + std::to_string(points[i].time) + " ";
   state += std::to_string(points[i].value) + " ";
   state += std::to_string(points[i].curve);
  }
  
  return state;
 }
 
 /**
  * @brief Restore the state of the envelope from a save string.
  * 
  * @param state The state to restore the envelope to.
  * @return true If successful.
  * @return false If a failure occurred. The state of the envelope might be undefined.
  */
 bool loadStateFromString(const std::string &state)
 {
  auto str = state.c_str();
  char *p_end;
  
  int pc = static_cast<int>(std::strtol(str, &p_end, 10));
  if (str == p_end) return false;
  
  str = p_end;
  loopStartPoint = static_cast<int>(std::strtol(str, &p_end, 10));
  if (str == p_end) return false;
  
  str = p_end;
  loopEndPoint = static_cast<int>(std::strtol(str, &p_end, 10));
  if (str == p_end) return false;
  
  if (pc > MaxPoints) return false;
  pointCount = 0;
  
  for (int i = 0; i < pc; ++i)
  {
   if (str == p_end) return false;
   str = p_end;
   SampleType time = std::strtod(str, &p_end);
   if (str == p_end) return false;
   str = p_end;
   SampleType value = std::strtod(str, &p_end);
   if (str == p_end) return false;
   str = p_end;
   SampleType curve = std::strtod(str, &p_end);
   
   doAddPoint(time, value, curve);
  }
  
  sendUpdateMessage();
  return true;
 }
 
 /**
  * @brief Can be called to indicate that a change is about to be made.
  * 
  */
 void beginEdit()
 {
  for (auto &l : listeners)
  {
   l->piecewiseEnvelopeBeginChange();
  }
 }
 
 /**
  * @brief Can be called to indicate that a change was just completed.
  * 
  */
 void endEdit()
 {
  for (auto &l : listeners)
  {
   l->piecewiseEnvelopeEndChange();
  }
 }
 

};









}









#endif // XDDSP_PIECEWISEENVELOPEDATA_H
