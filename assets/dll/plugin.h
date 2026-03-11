/*  VisualPlace Plug-in support
 *
 *  Copyright 2010-2025 CompuPhase
 *
 *  This software is provided "as-is", without any express or implied warranty.
 *  In no event will the authors be held liable for any damages arising from
 *  the use of this software.
 *
 *  Permission is granted to anyone to use this software for any purpose,
 *  including commercial applications, and to alter it and redistribute it
 *  freely, subject to the following restrictions:
 *
 *  1.  The origin of this software must not be misrepresented; you must not
 *      claim that you wrote the original software. If you use this software in
 *      a product, an acknowledgment in the product documentation would be
 *      appreciated but is not required.
 *  2.  Altered source versions must be plainly marked as such, and must not be
 *      misrepresented as being the original software.
 *  3.  This notice may not be removed or altered from any source distribution.
 */
#ifndef PLUGIN_H
#define PLUGIN_H

#if !defined _WINDOWS_ && !defined _INC_WINDOWS && !defined _WINDOWS_H
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
#endif

#if !defined sizearray
  #define sizearray(a)  (sizeof(a)/sizeof((a)[0]))
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define PLUGIN_INTERFACE_VERSION  3
#define VALUELENGTH       128 /* for the value/description field */
#define FIELDLENGTH       64  /* for all other fields (package, manufacturer, etc.) */
#define USERFIELDS        6

enum SIDE {
  SIDE_UNKNOWN,
  SIDE_TOP,
  SIDE_BOTTOM,
  SIDE_HORZ,              /* side by side (not used by plug-ins) */
  SIDE_VERT,              /* vertically stacked (not used by plug-ins) */
};

enum VPERROR {
  VPERROR_NONE,           /* no error */
  VPERROR_ACCESS,         /* file access error / file open failure */
  VPERROR_FORMAT,         /* invalid or unsupported format */
  VPERROR_MEMORY,         /* memory allocation error (heap corruption or corrupt file) */
};

#define PLUGIN_INPUT      0x01
#define PLUGIN_ACTION     0x02
#define PLUGIN_WRCENTROID 0x04
#define PLUGIN_WRBOM      0x08
#define PLUGIN_INVENTORY  0x10
#define PLUGIN_ACTIVE     0x80

#define FLAG_NOMOUNT      0x00001 /* component is marked as "no to be mounted" */
#define FLAG_SIDEWAYS     0x00002 /* IPC 7351 declares component to be sideways
                                     at zero degrees (e.g. a 2-pin component) */
#define FLAG_THROUGHHOLE  0x00004 /* component is marked as through-hole (non-SMT) */
#define FLAG_NO_POS       0x00008 /* component has no placement information */
#define FLAG_NO_ROTATION  0x00010 /* orientation in "rotation" field is invalid */

typedef struct tagPLUGININFO {
  char description[FIELDLENGTH];  /* the title, or description, of the plug-in */
  short priority;                 /* the load-order priority, 1=low, 10=high (default=5) */
  short version;                  /* plug-in interface version */
  short errorcode;                /* as of version 3.3: reason for failure */
  short errorline;                /* as of version 3.3: line number in the file (if relevant) */
} PLUGININFO;

typedef struct tagPARTINFO {
  char ref[FIELDLENGTH];          /* the designator, e.g. R1 */
  char value[VALUELENGTH];        /* the value or description for the component, e.g. "4.7uF 16V X5R" */
  char package[FIELDLENGTH];      /* the package/footprint description, e.g. "C0805" */
  char productnr[FIELDLENGTH];    /* the unique product number */
  char userfield[USERFIELDS][FIELDLENGTH]; /* user field, e.g. stock, preferred seller, ... */
  double x, y, rotation;          /* units are inches & degrees */
  short side;                     /* 1=top (component), 2=bottom (copper), 0=unknown */
  short stage;                    /* assembly stage, currently ignored in input (but set on output) */
  short flags;                    /* misc. flags (ignored on input, set on output) */
  short bom_row;                  /* row in BoM (ignored on input, set on output) */
} PARTINFO;

#if defined VP_PLUGIN_LEGACY
#define user1 userfield[0]
#define user2 userfield[1]
#define user3 userfield[2]
#define user4 userfield[3]
#endif

/* functions common to all plug-in types */
typedef BOOL (WINAPI *VP_INFO)(PLUGININFO *info);

/* "input" plug-ins: for EDA-suite support (importing production files) */
typedef int  (WINAPI *VP_LOADCENTROID)(LPCSTR filename, PARTINFO *info, int size,
                                       int side, double x_origin, double y_origin,
                                       double pcbwidth, double pcbheight);
typedef int  (WINAPI *VP_LOADBOM)(LPCSTR filename, PARTINFO *info, int size);
typedef int  (WINAPI *VP_POSTLOAD)(PARTINFO *info, int size);
typedef int  (WINAPI *VP_USERFIELDS)(LPCSTR userfields[], int number);
typedef int  (WINAPI *VP_INVENTORY)(LPCSTR filename, PARTINFO *info, int size);

/* output plug-ins: for creating centroid and BOM files */
typedef BOOL (WINAPI *VP_WRITECENTROID)(LPCSTR filename, const PARTINFO *info, int size);
typedef BOOL (WINAPI *VP_WRITEBOM)(LPCSTR filename, const PARTINFO *info, int size);

/* action plug-ins: for interfacing with other software */
typedef BOOL (WINAPI *VP_NOTIFYSELECT)(const PARTINFO *item, int count);

/* action and output plug-ins: loading/storing configuration */
typedef BOOL (WINAPI *VP_CONFIGURE)(HWND hwndParent, LPCSTR Profile, LPCSTR Project,
                                    BOOL Silent);

/* prototypes of the declared functions */
#if defined PLUGINEXPORT
#define VPAPI PLUGINEXPORT
#else
#define VPAPI WINAPI
#endif

BOOL VPAPI vp_Info(PLUGININFO *info);

int  VPAPI vp_LoadCentroid(LPCSTR filename, PARTINFO *info, int size,
                           int side, double x_origin, double y_origin,
                           double pcbwidth, double pcbheight);
int  VPAPI vp_LoadBOM(LPCSTR filename, PARTINFO *info, int size);
int  VPAPI vp_PostLoad(PARTINFO *info, int size);
void VPAPI vp_UserFields(LPCSTR userfields[], int number);
int  VPAPI vp_Inventory(LPCSTR filename, PARTINFO *info, int size);

BOOL VPAPI vp_WriteCentroid(LPCSTR filename, const PARTINFO *info, int size);
BOOL VPAPI vp_WriteBOM(LPCSTR filename, const PARTINFO *info, int size);

void VPAPI vp_NotifySelect(const PARTINFO *items, int count);
BOOL VPAPI vp_Configure(HWND hwndParent, LPCSTR Profile, LPCSTR Project, BOOL Silent);

#ifdef __cplusplus
}
#endif

#endif /* PLUGIN_H */
