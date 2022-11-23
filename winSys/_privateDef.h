#pragma once

// window borders
#define _BRD_TOP          0
#define _BRD_RIGHT        1
#define _BRD_BOTTOM       2
#define _BRD_LEFT         3
#define _BRD_TOPLEFT      4
#define _BRD_TOPRIGHT     5
#define _BRD_BOTTOMRIGHT  6
#define _BRD_BOTTOMLEFT   7

// VBO id's
#define _VBOID_BG               0
#define _VBOID_BRD_TOP          4
#define _VBOID_BRD_RIGHT        8
#define _VBOID_BRD_BOTTOM       12
#define _VBOID_BRD_LEFT         16
#define _VBOID_BRD_TOPLEFT      20
#define _VBOID_BRD_TOPRIGHT     24
#define _VBOID_BRD_BOTTOMRIGHT  28
#define _VBOID_BRD_BOTTOMLEFT   32

#define _VBOID_SCR_UP         0
#define _VBOID_SCR_RIGHT      4
#define _VBOID_SCR_DOWN       8
#define _VBOID_SCR_LEFT       12
#define _VBOID_SCR_HORIZ      16
#define _VBOID_SCR_VERT       20
#define _VBOID_SCR_BACK_HORIZ 24
#define _VBOID_SCR_BACK_VERT  28



// style <_type> var
#define _IX_SUBSTYLE_BASE          0
#define _IX_SUBSTYLE_WINDOW        1
#define _IX_SUBSTYLE_TITLE         2
#define _IX_SUBSTYLE_BUTTON        3
#define _IX_SUBSTYLE_BUTTONPRESSED 4
#define _IX_SUBSTYLE_EDIT          5
#define _IX_SUBSTYLE_TEXT          6
#define _IX_SUBSTYLE_SCROLL        7


#define _mINSIDE(_x, _y, _dx, _dy) ((mx>= (_x)) && (mx<= ((_x)+ (_dx))) && (my>= (_y)) && (my<= ((_y)+ (_dy))))






