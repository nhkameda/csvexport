/*  VisualPlace plugin: generic CSV output based
 *
 *  Copyright (c) 2013-2022, CompuPhase
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
 *
 *  $Id: csvexport.h 7140 2024-03-16 16:40:04Z thiadmer $
 */

#define C_NONE          0
#define C_SEQUENCE      1   /* row sequence number */
#define C_DESIGNATOR    2
#define C_VALUE         3
#define C_PACKAGE       4
#define C_PRODUCTNR     5
#define C_X             6
#define C_Y             7
#define C_ROTATION      8
#define C_SIDE          9
#define C_STAGE         10
#define C_MOUNT         11  /* mount y/n */

#define B_NONE          0
#define B_SEQUENCE      1   /* row sequence number */
#define B_DESIGNATOR    2   /* list */
#define B_VALUE         3
#define B_PACKAGE       4
#define B_PRODUCTNR     5
#define B_QUANTITY      6
#define B_STAGE         7
#define B_MOUNT         8   /* mount y/n */
#define B_USER1         9
#define B_USER2         10
#define B_USER3         11
#define B_USER4         12
#define B_USER5         13
#define B_USER6         14

#define IDC_STATICTEXT  (-1)

#define IDC_COLUMN1     200 /* column selection */
#define IDC_COLUMN2     201
#define IDC_COLUMN3     202
#define IDC_COLUMN4     203
#define IDC_COLUMN5     204
#define IDC_COLUMN6     205
#define IDC_COLUMN7     206
#define IDC_COLUMN8     207
#define IDC_COLUMN9     208
#define IDC_COLUMN10    209
#define IDC_COLUMN11    210

#define IDC_ZERO_PASV   220
#define IDC_ZERO_DIODE  221
#define IDC_ZERO_MISC   222
#define IDC_ANGLE_RANGE 223
#define IDC_ANGLE_CLOCK 224

#define IDC_SEPARATOR   230 /* comma, semicolon, tab */
#define IDC_UNIT_MM     231
#define IDC_UNIT_INCH   232
#define IDC_ENQUOTE     233
#define IDC_ALIGN_FID   234 /* align to fiducial */
#define IDC_INCLUDE_FID 235 /* include fiducials as virtual components */
#define IDC_SWAP_Y      236 /* invert Y axis */
#define IDC_INC_NOMOUNT 237
#define IDC_ADDHEADER   238
#define IDC_MARKSTAGE   239

#define IDC_TEMPLATE    240
#define IDC_NEWTEMPLATE 241
#define IDC_SIDE        242
#define IDC_STAGE       243
#define IDC_EDITNAME    244
#define IDC_EXTENSION   245
#define IDC_MOUNTSTAGE  246
#define IDC_STAGEMARKER 247

