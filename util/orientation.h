#pragma once

enum ixOrientation {
  RIGHT=      0x01,
  LEFT=       0x02,
  DOWN=       0x04,
  UP=         0x08,
  HORIZONTAL= 0x03,     // RIGHT and LEFT bytes form HORIZONTAL - used to know if orientation is horizontal
  VERTICAL=   0x0C      // DOWN  and UP bytes   form VERTICAL   - used to know if orientation is vertical
};










