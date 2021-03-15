#pragma once





class ixWinEvent: public segData {
  int32 veryimportantmessage;
};






class ixWinEventSys {
public:

  segList events;

  // constructor / destructor

  ixWinEventSys();
  ~ixWinEventSys();
  void delData();


protected:

  void _update();              // called to check on the segList's idle segments, from 3 to 3 minutes.

  uint64 _timeStart;                  // used to check from 3 to 3 minutes the events list segList for idle allocated segments
  static const uint64 _threeMinutes;  // in nanoseconds

  // friendships
  friend class ixWinSys;
};



















