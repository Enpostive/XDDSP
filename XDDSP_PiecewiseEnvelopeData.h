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










class PiecewiseEnvelopeListener
{
public:
 PiecewiseEnvelopeListener() {}
 virtual ~PiecewiseEnvelopeListener() {}
 
 // Used to trigger repaints on editors
 virtual void piecewiseEnvelopeChanged() {}
 
 // Used to notify audio hosts when changes are completed, for undo purposes
 virtual void piecewiseEnvelopeBeginChange() {}
 virtual void piecewiseEnvelopeEndChange() {}
};










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
  std::array<SampleType, CurveResolution> samples {};
  SampleType time {0};
  SampleType length {0};
  SampleType timeGradient;
 };
 
 std::array<Point, MaxPoints> points;
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
 
 
 void addListener(PiecewiseEnvelopeListener *l)
 {
  listeners.emplace_back(l);
 }
 
 void removeListener(PiecewiseEnvelopeListener *l)
 {
  auto it = std::find(listeners.begin(), listeners.end(), l);
  if (it != listeners.end()) listeners.erase(it);
 }
 
 int getPointCount() const
 { return pointCount; }
 
 bool getPoint(int index, SampleType &time, SampleType &value, SampleType &curve)
 {
  if (index < 0 || index >= pointCount) return false;
  
  time = points[index].time;
  value = points[index].value;
  curve = points[index].curve;
  return true;
 }
 
 void clearPoints()
 {
  pointCount = 0;
  clearLoop();
  sendUpdateMessage();
 }
 
 // Returns the index of the new point
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
 
 void setConstrainEdits(bool ce)
 { constrainEdits = ce; }
 
 // Returns the new index of the point, calling code should check to see if it changes
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
 
 // Doesn't return the new index, because the index never changes
 void changePointCurve(int index, SampleType curve)
 {
  points[index].curve = curve;
  calculateSamples(index);
  sendUpdateMessage();
 }
 
 SampleType getEnvelopeLength()
 {
  if (pointCount == 0) return 0.;
  return points[pointCount - 1].time;
 }
 
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
 
 int getLoopStartPoint()
 { return loopStartPoint; }
 
 int getLoopEndPoint()
 { return loopEndPoint; }
 
 SampleType getLoopStartTime()
 {
  if (loopStartPoint == -1) return 0.;
  return points[loopStartPoint].time;
 }
 
 SampleType getLoopEndTime()
 {
  if (loopEndPoint == -1) return 0.;
  return points[loopEndPoint].time;
 }
 
 bool isLoopSustainPoint()
 { return (loopStartPoint == loopEndPoint) && (loopStartPoint > -1); }
 
 // Calling setLoopPoint once will set a sustain point
 // Calling two times with different indexes will set a start and end loop point
 // Calling after setting a loop will change the loop end point or start point
 // depending on if index < loopStartPoint
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
 
 void clearLoopPoints()
 {
  clearLoop();
  sendUpdateMessage();
 }
 
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
 
 void beginEdit()
 {
  for (auto &l : listeners)
  {
   l->piecewiseEnvelopeBeginChange();
  }
 }
 
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
