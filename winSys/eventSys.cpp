#include "ix/ix.h"




// three minutes= 180 seconds= 180000 miliseconds= 180000000 macroseconds= 180000000000 nanoseconds
const uint64 ixWinEventSys::_threeMinutes= 180000000000;

// constructor / destructor

ixWinEventSys::ixWinEventSys():
  events(20, sizeof(ixWinEvent), true),
  _timeStart(0) {

  

}

ixWinEventSys::~ixWinEventSys() {
  delData();
}

void ixWinEventSys::delData() {
}








void ixWinEventSys::_update() {
  if(osi.present- _timeStart> _threeMinutes)
    events.checkIdle();
  _timeStart= osi.present;
}

















