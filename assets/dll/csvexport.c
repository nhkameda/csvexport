/*  VisualPlace plugin: generic CSV output based
 *
 *  Copyright (c) CompuPhase, 2013-2022
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
#define WINVER      0x0500  /* needed for winutf8 */
#if defined UNICODE && !defined _UNICODE
  #define _UNICODE
#endif
#include <windows.h>
#include <htmlhelp.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "csvexport.h"

#include "rosette.h"
#include "strsep.h"
#include "winutf8.h"

#define PLUGINEXPORT __declspec(dllexport) __stdcall
#include "plugin.h"

#ifndef VISUALPLACE_H
#define SEC_SETTINGS        "Settings"
#define SEC_STAGENAMES      "PlacementStages"
#define KEY_LOCALDATA       "localdata"

#define MAX_MOUNTSTAGES     6
#endif

static int LastError = VPERROR_NONE;


#if !defined sizearray
  #define sizearray(a)  (sizeof(a)/sizeof((a)[0]))
#endif
#if !defined USERFIELDS
  #define USERFIELDS  6
#endif
#if !defined REFSTRING_MAX
  #define REFSTRING_MAX 32768
#endif

#if !defined U_REFRESH
  #define U_REFRESH     (WM_APP + 100)
#endif

BOOL CALLBACK NewTemplateDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK CentroidDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK BOMDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

static HINSTANCE hinstDLL;
static const char Section[] = "CSV-Export";
static HWND hwndApplication;
static char szProfile[_MAX_PATH];
static char szProject[_MAX_PATH];
static char szHelpFile[_MAX_PATH];
static char szLclDataDir[_MAX_PATH];
static char szTemplate[100];
static char szTemplateType[40];
static char UserFields[USERFIELDS][FIELDLENGTH];
static int CurrentSide;
static int CurrentStage;
static unsigned StageActive;  /* bit flag */
static char StageMarker = '#';

static const char stagemarkers[] = { '#',':','@','/','\\' };


enum {  /* options for side selection */
  BOTH_SEPARATE,
  BOTH_COMBINED,
  TOP_ONLY,
  BOTTOM_ONLY
};


/* ***** some helper functions ***** */
static size_t strlcpy(char *dst,const char *src,size_t size)   
{
  size_t srclen;
  if (size == 0)
    return 0;
  size--;
  srclen = strlen(src);
  if (srclen > size)
    srclen = size;
  memcpy(dst, src, srclen);
  dst[srclen] = '\0';
  return srclen;
}
/* ***** end of helper functions ***** */


/* Especially Watcom C/C++ does not like DLLs that do not have a LibMain()
 * set. Apparently, the start address is not set well, and some required
 * initializations are not done.
 */
BOOL WINAPI DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID lpRes)
{
  (void)lpRes;
  switch (dwReason) {
  case DLL_PROCESS_ATTACH:
    hinstDLL=hinst;
    break;
  case DLL_PROCESS_DETACH:
    break;
  }
  return TRUE;
}


/* vp_Info()
 *
 * info         A pointer to a structure that this function should fill in.
 */
BOOL PLUGINEXPORT vp_Info(PLUGININFO *info)
{
  strcpy(info->description, "Generic CSV|*.csv");
  info->priority = 5;
  info->version = PLUGIN_INTERFACE_VERSION;
  info->errorcode = LastError;
  info->errorline = 0;
  return TRUE;
}


/* vp_Configure()
 *
 * hwndParent   The handle to the main VisualPlace window.
 * Profile      The full path to the INI file where VisualPlace stores its
 *              configuration data. The plugin may use that file to store its
 *              own configuration in too.
 * Project      The full path to the active project, or NULL if no project is open.
 * Silent       If TRUE, the plugin should read the configuration from the INI
 *              file (or another location) without popping up a dialog. If FALSE,
 *              a dialog should always be presented.
 */
BOOL PLUGINEXPORT vp_Configure(HWND hwndParent, LPCSTR Profile, LPCSTR Project, BOOL Silent)
{
  char *ptr;
  char str[_MAX_PATH];

  LastError = VPERROR_NONE;

  /* copy variables */
  hwndApplication = hwndParent;

  strncpy(szProfile, Profile, sizearray(szProfile));
  szProfile[sizearray(szProfile) - 1] = '0';

  strncpy(szProject, Project, sizearray(szProject));
  szProject[sizearray(szProject) - 1] = '0';

  GetModuleFileNameU(hinstDLL,szHelpFile,sizearray(szHelpFile));
  ptr=strrchr(szHelpFile,'\\');
  assert(ptr!=NULL);
  strcpy(ptr+1,"csvexport.chm");

  /* the template file must be in the data directory, which is configurable; by
     default, it is in the same root path of where the INI file is */
  strcpy(szLclDataDir,szProfile);
  ptr=strrchr(szLclDataDir,'\\');
  assert(ptr!=NULL);
  *ptr='\0';
  if ((ptr=strrchr(szLclDataDir,'\\'))!=NULL && _stricmp(ptr,"\\bin")==0)
    strcpy(ptr,"\\data\\");
  else
    strcat(szLclDataDir,"\\data\\");
  /* with the calculated value as the default, look up the local data from the INI file */
  GetPrivateProfileStringU(SEC_SETTINGS,KEY_LOCALDATA,"",str,sizearray(str),szProfile);
  if (strlen(str)!=0)
    strcpy(szLclDataDir,str);

  return TRUE;
}

static DWORD ReadSettingString(const char *Key,const char *Default,char *Value,int Size)
{
  DWORD result;

  if (strlen(szTemplate)>0) {
    char path[_MAX_PATH],section[150];
    sprintf(path,"%scsvexport.ini",szLclDataDir);
    sprintf(section,"%s:%s",szTemplateType,szTemplate);
    result=GetPrivateProfileStringU(section,Key,Default,Value,Size,path);
  } else {
    result=GetPrivateProfileStringU(Section,Key,Default,Value,Size,szProfile);
  }
  return result;
}

static unsigned ReadSettingInt(const char *Key,int Default)
{
  unsigned result=Default;
  char str[32];

  ReadSettingString(Key,"",str,sizearray(str));
  if (strlen(str)>0) {
    char *ptr;
    result=(unsigned)strtol(str,&ptr,10);
    if (*ptr!='\0' && *ptr>' ')
      result=Default; /* invalid number, reset to the default */
  }
  return result;
}

static BOOL WriteSettingString(const char *Key,const char *Value)
{
  BOOL result;

  if (strlen(szTemplate)>0) {
    char path[_MAX_PATH],section[150];
    sprintf(path,"%scsvexport.ini",szLclDataDir);
    sprintf(section,"%s:%s",szTemplateType,szTemplate);
    result=WritePrivateProfileStringU(section,Key,Value,path);
  } else {
    result=WritePrivateProfileStringU(Section,Key,Value,szProfile);
  }
  return result;
}

static BOOL WriteSettingInt(const char *Key,unsigned Value)
{
  char val[32];
  sprintf(val,"%u",Value);
  return WriteSettingString(Key,val);
}

static char *skipwhite(const char *s)
{
  while (*s!='\0' && *s<=' ')
    s++;
  return (char*)s;
}

/** GetStageName() returns the name of an assembly stage. If a stage is
 *  undefined, the result is either an empty string (if "defaultname" is FALSE),
 *  or the name "Base" or "(undefined)" (if "defaultname" is TRUE). Only the
 *  first stage is predefined as "Base".
 *
 *  Optionally, the root name is also returned; if the stage is marked as
 *  cummulative, the root name is the name that the stage is cummulated to. If
 *  a stage is not cummulative, the root name is a zero-length string.
 *
 *  \param name         [out] The stage name.
 *  \param maxname      [in] The size of the buffer for the name (in
 *                      characters).
 *  \param stage        [in] The sequence numver of the stage, a value between 0
 *                      and MAX_MOUNTSTAGES.
 *  \param rootname     [out] The name of the parent stage, for a cummulative
 *                      stage (or an empty string if the stage is not
 *                      cummulative). This parameter may be NULL.
 *  \param maxroot      [in] The size of the buffer for the root name (in
 *                      characters).
 *
 *  \note Adapted from projectsupport.c
 */
static char *GetStageName(char *name,size_t maxname,int stage,char *rootname,size_t maxroot)
{
  char *ptr;
  char value[128]="";
  char key[32];

  assert(name!=NULL && maxname>0);
  assert(stage>=0 && stage<MAX_MOUNTSTAGES);
  assert(strlen(szProject)>0);
  if (rootname!=NULL && maxroot>0)
    *rootname='\0';
  sprintf(key,"stage%d",stage+1);
  GetPrivateProfileStringU(SEC_STAGENAMES,key,"",value,sizearray(value),szProject);

  if ((ptr=strchr(value,','))!=NULL) {
    *ptr++='\0';
    if (rootname!=NULL)
      strlcpy(rootname,ptr,maxroot);
  }
  strlcpy(name,value,maxname);
  return name;
}

/** IsStageActive() checks whether a stage is active, either directly or
 *  indirectly. A stage can be "indirectly active" when it is the "root" stage
 *  of the active stage (and that active stage happens to be "cummulative").
 *
 *  \note Copied from projectsupport.c
 */
static BOOL IsStageActive(int stage)
{
  return (StageActive & (1 << stage))!=0;
}

/** SetStageMask() sets the bit mask of active stage, plus any stages that it
 *  depends on.
 *
 *  \param stage  The stage to set active; set this parameter 1 to include all
 *                stages (all stages are active).
 *
 *  \note Adapted from SetCurrentStage() in projectsupport.c.
 */
static unsigned SetStageMask(int stage)
{
  char name[FIELDLENGTH],rootname[FIELDLENGTH];
  unsigned active_mask;

  assert(stage>=0 && stage<MAX_MOUNTSTAGES || stage==-1);
  if (stage==-1)
    return ~0;

  active_mask=(1 << stage);
  for ( ;; ) {
    int idx;
    GetStageName(name,sizearray(name),stage,rootname,sizearray(rootname));
    if (strlen(rootname)==0 || strlen(name)==0 || _stricmp(name,rootname)==0)
      break;    /* no root name, not a valid stage (no stage name), or stage depends on itself... */
    /* find the stage number for "rootname" */
    for (idx=0; idx<MAX_MOUNTSTAGES; idx++) {
      if (idx==stage)
        continue;
      GetStageName(name,sizearray(name),idx,NULL,0);
      if (_stricmp(name,rootname)==0)
        break;  /* found */
    }
    if (idx>=MAX_MOUNTSTAGES)
      break;    /* root name not found in the list of stages */
    stage=idx;  /* root name is found, include it in the active stages */
    active_mask|=(1 << stage);
  }
  return active_mask;
}

/* vp_WriteCentroid()
 *
 * filename     The full path name of the file that should be created.
 * info         The records in the "items" parameter.
 * size         The number of records in the "info" parameter.
 */
BOOL PLUGINEXPORT vp_WriteCentroid(LPCSTR filename, const PARTINFO *info, int size)
{
static const char *unit_names[] = { "mm", "inch", "mil" };
  FILE *fp;
  int idx,val,def,record,numcolumns,seq;
  int column[IDC_COLUMN11-IDC_COLUMN1+1];
  char item[30],str[256];
  char separator,prefix;
  BOOL enquote,invert_y,clockwise;
  BOOL include_nomount,is_nomount;
  int side_selected,stage_selected;
  int angle_range,unit;
  double offsx,offsy,pos,unit_mult;
  char szTopSideFilename[_MAX_PATH];
  char extension[30]="",*ext;

  LastError=VPERROR_NONE;
  CurrentSide=info[0].side & 0xff;
  side_selected=ReadSettingInt("side",BOTH_SEPARATE);
  /* if the current side is "Bottom" and the option for a single file is set,
   * use the filename that was also used for the top layer.
   */
  if (CurrentSide==SIDE_BOTTOM) {
    if (side_selected==TOP_ONLY)
      return TRUE;  /* nothing to do, because only the top side was requested */
    GetPrivateProfileStringU(Section,"recent","",szTopSideFilename,sizearray(szTopSideFilename),szProfile);
    if (side_selected==BOTH_SEPARATE || side_selected==BOTTOM_ONLY) {
      int len;
      if ((ext=strrchr(szTopSideFilename,'.'))!=NULL && strchr(ext,'\\')==NULL) {
        strcpy(extension,ext);  /* save extension of the "top" filename ... */
        *ext='\0';              /* ... then strip it off */
      }
      len=strlen(szTopSideFilename);
      if (len>4 && _stricmp(szTopSideFilename+len-4,"-top")==0)
        szTopSideFilename[len-4]='\0';
      strcat(szTopSideFilename,"-bottom");
      strcat(szTopSideFilename,extension);
    }
    filename=szTopSideFilename;
    WritePrivateProfileStringU(Section,"recent",NULL,szProfile);
  } else {
    if (DialogBoxU(hinstDLL,"CentroidDialog",hwndApplication,(DLGPROC)CentroidDlgProc)!=IDOK)
      return FALSE;
    assert(CurrentSide==SIDE_TOP);
    side_selected=ReadSettingInt("side",BOTH_SEPARATE);
    if (side_selected==BOTH_COMBINED) {
      /* possibly adjust the filename (remove the "-top" that was automatically
         appended by VisualPlace, in the case that a single file was requested) */
      char *ptr;
      strcpy(szTopSideFilename,filename);
      if ((ptr=strrchr(szTopSideFilename,'.'))!=NULL && strchr(ptr,'\\')==NULL
          && _strnicmp(ptr-4,"-top",4)==0)
      {
        memmove(ptr-4,ptr,strlen(ptr)+1);
        filename=szTopSideFilename;
      }
    }
    /* save the name needed for the bottom side */
    WritePrivateProfileStringU(Section,"recent",filename,szProfile);
    if (side_selected==BOTTOM_ONLY)
      return TRUE;  /* top layer explicitly skipped, nothing to do now */
  }

  /* Kameda: Fix export bottom error */
  strcpy(szTemplateType, "cpl");
  GetPrivateProfileString(Section, "template-centroid", "", szTemplate, sizearray(szTemplate), szProfile);

  /* count the number of columns & check a few items */
  include_nomount=FALSE;
  numcolumns=0;
  for (idx=IDC_COLUMN1; idx<=IDC_COLUMN11; idx++) {
    sprintf(item,"centroid%d",idx-IDC_COLUMN1+1);
    val=ReadSettingInt(item,0);
    column[idx-IDC_COLUMN1]=val;
    if (val>0) {
      numcolumns=idx-IDC_COLUMN1+1;
      if (val==C_MOUNT)
        include_nomount=TRUE;
    }
  }
  /* change default extension */
  ReadSettingString("ext-centroid","CSV",str,sizearray(str));
  _strlwr(str);
  if (str[0]=='.') {
    strcpy(extension,str);
  } else {
    strcpy(extension,".");
    strcat(extension,str);
  }
  if ((ext=strrchr(filename,'.'))!=NULL && strchr(ext,'\\')==NULL && _stricmp(ext,".csv")==0 && _stricmp(extension,".csv")!=0) {
    /* replace the default extension */
    if (filename!=szTopSideFilename) {
      strcpy(szTopSideFilename,filename);
      filename=szTopSideFilename;
    }
    ext=strrchr(filename,'.');
    assert(ext!=NULL);
    strcpy(ext,extension);
  }
  /* read options */
  side_selected=ReadSettingInt("side",BOTH_SEPARATE);
  stage_selected=ReadSettingInt("stage",0);
  enquote=ReadSettingInt("enquote-fields",TRUE);
  unit=ReadSettingInt("unit",0);
  invert_y=ReadSettingInt("invert-y",FALSE);
  clockwise=ReadSettingInt("angle-clockwise",FALSE);
  angle_range=ReadSettingInt("angle-range",0);
  val=ReadSettingInt("separator",0);
  switch (val) {
  case 1:
    separator=';';
    break;
  case 2:
    separator='\t';
    break;
  default:
    separator=',';
  }
  switch (unit) {
  case 1:
    unit_mult=1.0;    /* inch */
    break;
  case 2:
    unit_mult=1000.0; /* mil, can currently not be selected in the user-interface */
    break;
  default:
    unit_mult=25.4;   /* mm */
    unit=0;
  }

  /* read fiducial position (optionally swap the Y-axis) */
  offsx=offsy=0;
  if (ReadSettingInt("align-fiducial",FALSE)) {
    int fside,ftype;
    double xpos,ypos;
    int type=0;
    for (idx=0; ; idx++) {
      sprintf(item,"fid%d",idx+1);
      GetPrivateProfileStringU("Fiducials",item,"",str,sizearray(str),szProject);
      if (sscanf(str,"%d %d %lf %lf",&ftype,&fside,&xpos,&ypos)!=4)
        break;
      if (fside!=info[0].side)
        continue;
      if (type==0 || (ftype==2 && type<2) || (type==ftype && xpos<offsx)) {
        offsx=xpos;
        offsy=ypos;
        type=ftype;
      }
    }
    if (invert_y)
      offsy=-offsy;
  }

  if (CurrentSide==SIDE_BOTTOM && side_selected==BOTH_COMBINED)
    fp=fopen(filename,"at");  /* append data for bottom side in the same file */
  else
    fp=fopen(filename,"wt");
  if (fp==NULL) {
    LastError=VPERROR_ACCESS;
    return FALSE;
  }

  /* optionally write the current settings as comments to the CSV file */
  if (ReadSettingInt("csvheader",FALSE)) {
    static const char *dir[] = { "top", "left", "bottom", "right" };
    const char *ptr;
    if (side_selected==BOTH_COMBINED)
      ptr="both";
    else
      ptr=(CurrentSide==SIDE_TOP) ? "top" : "bottom";
    fprintf(fp,"# side: %s\n",ptr);
    strcpy(str,"all");
    if (stage_selected>0) {
      /* look up the selected stage */
      GetStageName(str,sizearray(str),stage_selected-1,NULL,0);
      if (strlen(str)==0) {
        stage_selected=0; /* reset to default, when selected stage name disappears */
        strcpy(str,"all");
      }
    }
    fprintf(fp,"# stage: %s\n",str);
    fprintf(fp,"# unit: %s\n",(unit==1) ? "inch" : "mm");
    fprintf(fp,"# rotation: degrees %s\n",clockwise ? "clockwise" : "counter-clockwise");
    val=ReadSettingInt("zero1",1);
    assert(val>=0 && val<4);
    fprintf(fp,"# zero-orientation for non-polarized 2-pin parts: pin 1 at %s\n",dir[val]);
    val=ReadSettingInt("zero2",1);
    assert(val>=0 && val<4);
    fprintf(fp,"# zero-orientation for polarized 2-pin parts: pin 1 at %s (pin 1 is cathode or + pole)\n",dir[val]);
    val=ReadSettingInt("zero3",0);
    assert(val>=0 && val<4);
    fprintf(fp,"# zero-orientation for parts with more than 2 pins: pin 1 at %s\n",dir[val]);
    fprintf(fp,"#\n");
  }

  /* write the header line (except when appending) */
  if (CurrentSide==SIDE_TOP || side_selected!=BOTH_COMBINED) {
    for (idx=0; idx<numcolumns; idx++) {
      str[0]='\0';
      if (enquote)
        strcat(str,"\"");
      switch (column[idx]) {
      case C_NONE:
        break;
      case C_SEQUENCE:
        strcat(str,"Row");
        break;
      case C_DESIGNATOR:
        strcat(str,"Designator");
        break;
      case C_VALUE:
        strcat(str,"Value");
        break;
      case C_PACKAGE:
        strcat(str,"Package");
        break;
      case C_PRODUCTNR:
        strcat(str,"ProductNr");
        break;
      case C_X:
        sprintf(str+strlen(str),"X(%s)",unit_names[unit]);
        break;
      case C_Y:
        sprintf(str+strlen(str),"Y(%s)",unit_names[unit]);
        break;
      case C_ROTATION:
        strcat(str,"Rotation");
        break;
      case C_SIDE:
        strcat(str,"Side");
        break;
      case C_STAGE:
        strcat(str,"Stage");
        break;
      case C_MOUNT:
        strcat(str,"Mount");
        break;
      default:
        assert(0);
      }
      if (enquote)
        strcat(str,"\"");
      fprintf(fp,"%s",str);
      if (idx+1<numcolumns)
        fprintf(fp,"%c",separator);
    }
    fprintf(fp,"\n");
  }

  /* write the records */
  seq=0;
  for (record=0; record<size; record++) {
    if (info[record].side!=SIDE_TOP && info[record].side!=SIDE_BOTTOM)
      continue;         /* skip any record with invalid information */
    is_nomount= (info[record].flags & FLAG_NOMOUNT)!=0;
    if (stage_selected>0 && !IsStageActive(info[record].stage))
      is_nomount=TRUE;  /* item in a different stage must be flagged as "do not mount" */
    if (is_nomount && !include_nomount)
      continue;         /* skip "dot not mount" records, unless a column for "do not mount" is present */
    seq++;
    for (idx=0; idx<numcolumns; idx++) {
      str[0]='\0';
      if (enquote)
        strcat(str,"\"");
      switch (column[idx]) {
      case C_NONE:
        break;
      case C_SEQUENCE:
        sprintf(str+strlen(str),"%d",seq);
        break;
      case C_DESIGNATOR:
        sprintf(str+strlen(str),"%s",info[record].ref);
        break;
      case C_VALUE:
        sprintf(str+strlen(str),"%s",info[record].value);
        break;
      case C_PACKAGE:
        sprintf(str+strlen(str),"%s",info[record].package);
        break;
      case C_PRODUCTNR:
        sprintf(str+strlen(str),"%s",info[record].productnr);
        break;
      case C_X:
        pos=info[record].x-offsx; /* relocate to fiducial */
        pos*=unit_mult;
        if (unit==0)
          sprintf(str+strlen(str),"%.2f",pos);
        else
          sprintf(str+strlen(str),"%.3f",pos);
        break;
      case C_Y:
        pos=info[record].y;
        if (invert_y)
          pos=-pos;             /* handle inversion */
        pos-=offsy;             /* relocate to fiducial */
        pos*=unit_mult;
        if (unit==0)
          sprintf(str+strlen(str),"%.2f",pos);
        else
          sprintf(str+strlen(str),"%.3f",pos);
        break;
      case C_ROTATION:
        pos=info[record].rotation;
        /* correct the rotation according to the selected zero-alignment */
        if (info[record].flags & FLAG_SIDEWAYS) {
          /* check active/passive based on prefix */
          prefix=toupper(info[record].ref[0]);
          if (prefix=='C' || prefix=='R' || prefix=='L' || prefix=='X')
            val=IDC_ZERO_PASV;
          else
            val=IDC_ZERO_DIODE;
          def=1;                /* IPC-7351 default = pin1 to the left */
        } else {
          val=IDC_ZERO_MISC;    /* always use the "misc" category */
          def=0;
        }
        sprintf(item,"zero%d",val-IDC_ZERO_PASV+1);
        val=ReadSettingInt(item,def);
        while (val!=def) {
          pos+=90;
          val=(val+1)%4;
        }
        /* other adjustments */
        if (clockwise)          /* handle inversion (clockwise rotation) */
          pos=360.0-pos;
        if (angle_range==0) {   /* handle angle ranges */
          while (pos<-0.5)
            pos+=360.0;
          while (pos>360.0-0.5)
            pos-=360.0;
        } else {
          assert(angle_range==1);
          while (pos<-180.0-.5)
            pos+=360.0;
          while (pos>180.0-0.5)
            pos-=360.0;
        }
        sprintf(str+strlen(str),"%.0f",pos);
        break;
      case C_SIDE:
        if (info[record].side==SIDE_TOP)
          strcat(str,"Top");
        else
          strcat(str,"Bottom");
        break;
      case C_STAGE: {
        char name[FIELDLENGTH];
        GetStageName(name,sizearray(name),info[record].stage,NULL,0);
        strcat(str,name);
        break;
      }
      case C_MOUNT:
        if (is_nomount)
          strcat(str,"N");
        else
          strcat(str,"Y");
        break;
      default:
        assert(0);
      }
      if (enquote)
        strcat(str,"\"");
      fprintf(fp,"%s",str);
      if (idx+1<numcolumns)
        fprintf(fp,"%c",separator);
    }
    fprintf(fp,"\n");
  }

  /* write the fiducials */
  if (ReadSettingInt("include-fiducials",FALSE)) {
    int fside,ftype,fidx;
    double xpos,ypos;
    for (fidx=0; ; fidx++) {
      sprintf(item,"fid%d",fidx+1);
      GetPrivateProfileStringU("Fiducials",item,"",str,sizearray(str),szProject);
      if (sscanf(str,"%d %d %lf %lf",&ftype,&fside,&xpos,&ypos)!=4)
        break;
      if (fside!=info[0].side)
        continue;
      seq++;
      for (idx=0; idx<numcolumns; idx++) {
        str[0]='\0';
        if (enquote)
          strcat(str,"\"");
        switch (column[idx]) {
        case C_NONE:
        case C_PACKAGE:
        case C_PRODUCTNR:
        case C_ROTATION:
        case C_STAGE:
          break;
        case C_SEQUENCE:
          sprintf(str+strlen(str),"%d",seq);
          break;
        case C_DESIGNATOR:
          sprintf(str+strlen(str),"Fid%d",fidx+1);
          break;
        case C_VALUE:
          strcat(str,"Fiducial");
          break;
        case C_X:
          pos=xpos-offsx; /* relocate to fiducial */
          pos*=unit_mult;
          if (unit==0)
            sprintf(str+strlen(str),"%.2f",pos);
          else
            sprintf(str+strlen(str),"%.3f",pos);
          break;
        case C_Y:
          pos=ypos;
          pos*=unit_mult;
          if (invert_y)
            pos=-pos;             /* handle inversion */
          pos-=offsy;             /* relocate to fiducial */
          if (unit==0)
            sprintf(str+strlen(str),"%.2f",pos);
          else
            sprintf(str+strlen(str),"%.3f",pos);
          break;
        case C_SIDE:
          if (fside==SIDE_TOP)
            strcat(str,"Top");
          else
            strcat(str,"Bottom");
          break;
        case C_MOUNT:
          strcat(str,"N");
          break;
        default:
          assert(0);
        }
        if (enquote)
          strcat(str,"\"");
        fprintf(fp,"%s",str);
        if (idx+1<numcolumns)
          fprintf(fp,"%c",separator);
      }
      fprintf(fp,"\n");
    }
  }

  fclose(fp);
  return TRUE;
}

BOOL CALLBACK CentroidDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  static HFONT hfont;
  HDC hdc;
  int idx, cur, flags, reply, height;
  char item[30], str[256], path[_MAX_PATH], *sections, *ptr;

  switch (msg) {
  case WM_INITDIALOG:
    /* first get the active template */
    strcpy(szTemplateType,"cpl");
    GetPrivateProfileStringU(Section,"template-centroid","",szTemplate,sizearray(szTemplate),szProfile);
    /* set a bold font for the PCB side */
    hdc=GetDC(hwnd);
    height=MulDiv(8,GetDeviceCaps(hdc,LOGPIXELSY),72);
    ReleaseDC(hwnd,hdc);
    hfont=CreateFontU(-height,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,ANSI_CHARSET,
                      OUT_DEVICE_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,
                      FF_DONTCARE,"Tahoma");
    SendDlgItemMessage(hwnd,IDC_SIDE,WM_SETFONT,(WPARAM)hfont,0);
    /* fill in the comboboxes */
    sprintf(path,"%scsvexport.ini",szLclDataDir);
    Combobox_AddStringU(GetDlgItem(hwnd,IDC_TEMPLATE),"-");
    sections=malloc(65536*sizeof(char));
    if (sections!=NULL) {
      GetPrivateProfileStringU(NULL,NULL,"",sections,65536,path);
      for (ptr=sections; strlen(ptr)>0; ptr+=strlen(ptr)+1)
        if (_strnicmp(ptr,"cpl:",4)==0)
          Combobox_AddStringU(GetDlgItem(hwnd,IDC_TEMPLATE),ptr+4);
      free(sections);
    }
    Combobox_AddStringU(GetDlgItem(hwnd,IDC_SIDE),"Top+bottom separate");
    Combobox_AddStringU(GetDlgItem(hwnd,IDC_SIDE),"Top+bottom combined");
    Combobox_AddStringU(GetDlgItem(hwnd,IDC_SIDE),"Top only");
    Combobox_AddStringU(GetDlgItem(hwnd,IDC_SIDE),"Bottom only");
    Combobox_AddStringU(GetDlgItem(hwnd,IDC_STAGE),"(All)");
    for (idx=0; ; idx++) {
      GetStageName(str,sizearray(str),idx,NULL,0);
      if (strlen(str)==0)
        break;
      Combobox_AddStringU(GetDlgItem(hwnd,IDC_STAGE),str);
    }
    for (idx=IDC_COLUMN1; idx<=IDC_COLUMN11; idx++) {
      Combobox_AddStringU(GetDlgItem(hwnd,idx),"(none)");
      Combobox_AddStringU(GetDlgItem(hwnd,idx),"Sequence");
      Combobox_AddStringU(GetDlgItem(hwnd,idx),"Designator");
      Combobox_AddStringU(GetDlgItem(hwnd,idx),"Value");
      Combobox_AddStringU(GetDlgItem(hwnd,idx),"Package");
      Combobox_AddStringU(GetDlgItem(hwnd,idx),"Product Nr");
      Combobox_AddStringU(GetDlgItem(hwnd,idx),"X");
      Combobox_AddStringU(GetDlgItem(hwnd,idx),"Y");
      Combobox_AddStringU(GetDlgItem(hwnd,idx),"Rotation");
      Combobox_AddStringU(GetDlgItem(hwnd,idx),"Side");
      Combobox_AddStringU(GetDlgItem(hwnd,idx),"Stage");
      Combobox_AddStringU(GetDlgItem(hwnd,idx),"Mount Y/N");
    }
    for (idx=IDC_ZERO_PASV; idx<=IDC_ZERO_MISC; idx++) {
      Combobox_AddStringU(GetDlgItem(hwnd,idx),"Up");
      Combobox_AddStringU(GetDlgItem(hwnd,idx),"Left");
      Combobox_AddStringU(GetDlgItem(hwnd,idx),"Down");
      Combobox_AddStringU(GetDlgItem(hwnd,idx),"Right");
    }
    Combobox_AddStringU(GetDlgItem(hwnd,IDC_ANGLE_RANGE),"0 .. 360");
    Combobox_AddStringU(GetDlgItem(hwnd,IDC_ANGLE_RANGE),"-180 .. 180");
    Combobox_AddStringU(GetDlgItem(hwnd,IDC_SEPARATOR),"Comma");
    Combobox_AddStringU(GetDlgItem(hwnd,IDC_SEPARATOR),"Semicolon");
    Combobox_AddStringU(GetDlgItem(hwnd,IDC_SEPARATOR),"Tab");
    /* set the template selection in the control, because the code for setting
       the configuration retrieves it from there */
    idx=Combobox_FindStringU(GetDlgItem(hwnd,IDC_TEMPLATE),-1,szTemplate,TRUE);
    if (idx<0)
      idx=Combobox_FindStringU(GetDlgItem(hwnd,IDC_TEMPLATE),-1,"-",TRUE);
    SendDlgItemMessage(hwnd,IDC_TEMPLATE,CB_SETCURSEL,idx,0);
    /* set the configuration */
    PostMessage(hwnd,U_REFRESH,0,0);
    return TRUE;

  case WM_COMMAND:
    switch (LOWORD(wParam)) {
    case IDCANCEL:
      EndDialog(hwnd, LOWORD(wParam));
      return TRUE;

    case IDOK:
      /* first get the template */
      szTemplate[0]='\0';
      idx=SendDlgItemMessage(hwnd,IDC_TEMPLATE,CB_GETCURSEL,0,0);
      if (idx>=0) {
        Combobox_GetLBTextU(GetDlgItem(hwnd,IDC_TEMPLATE),idx,szTemplate,sizearray(szTemplate));
        if (strcmp(szTemplate,"-")==0)
          szTemplate[0]='\0';
      }
      WritePrivateProfileStringU(Section,"template-centroid",szTemplate,szProfile);
      /* save the current settings, either in the global INI file or in a template */
      flags=0;
      reply=IDOK;
      for (idx=IDC_COLUMN1; idx<=IDC_COLUMN11 && reply==IDOK; idx++) {
        cur=(int)SendDlgItemMessage(hwnd,idx,CB_GETCURSEL,0,0);
        sprintf(item,"centroid%d",idx-IDC_COLUMN1+1);
        WriteSettingInt(item,cur);
        if (cur>0 && (flags & (1 << cur))!=0)
          reply=MessageBoxU(hwnd,"The same field is selected in multiple columns","Notice",MB_OKCANCEL);
        flags |= (1 << cur);
      }
      if (reply==IDOK && ((flags & (1 << C_X))==0 || (flags & (1 << C_Y))==0))
        reply=MessageBoxU(hwnd,"Both the X and Y positions should be assigned to a column","Warning",MB_OKCANCEL);
      if (reply==IDOK && (flags & (1 << C_ROTATION))==0)
        reply=MessageBoxU(hwnd,"The rotation should be assigned to a column","Warning",MB_OKCANCEL);
      if (reply!=IDOK)
        return TRUE;
      for (idx=IDC_ZERO_PASV; idx<=IDC_ZERO_MISC; idx++) {
        cur=(int)SendDlgItemMessage(hwnd,idx,CB_GETCURSEL,0,0);
        sprintf(item,"zero%d",idx-IDC_ZERO_PASV+1);
        WriteSettingInt(item,cur);
      }
      cur=(int)SendDlgItemMessage(hwnd,IDC_SIDE,CB_GETCURSEL,0,0);
      WriteSettingInt("side",cur);
      cur=(int)SendDlgItemMessage(hwnd,IDC_STAGE,CB_GETCURSEL,0,0);
      WriteSettingInt("stage",cur);
      GetDlgItemTextU(hwnd,IDC_EXTENSION,str,sizearray(str));
      ptr=skipwhite(str);
      if (*ptr=='.')
        ptr=skipwhite(ptr+1);
      WriteSettingString("ext-centroid",ptr);
      WriteSettingInt("angle-range",SendDlgItemMessage(hwnd,IDC_ANGLE_RANGE,CB_GETCURSEL,0,0));
      WriteSettingInt("angle-clockwise",IsDlgButtonChecked(hwnd,IDC_ANGLE_CLOCK)==BST_CHECKED);
      WriteSettingInt("separator",SendDlgItemMessage(hwnd,IDC_SEPARATOR,CB_GETCURSEL,0,0));
      WriteSettingInt("unit",IsDlgButtonChecked(hwnd,IDC_UNIT_INCH)==BST_CHECKED);
      WriteSettingInt("enquote-fields",IsDlgButtonChecked(hwnd,IDC_ENQUOTE)==BST_CHECKED);
      WriteSettingInt("align-fiducial",IsDlgButtonChecked(hwnd,IDC_ALIGN_FID)==BST_CHECKED);
      WriteSettingInt("include-fiducials",IsDlgButtonChecked(hwnd,IDC_INCLUDE_FID)==BST_CHECKED);
      WriteSettingInt("invert-y",IsDlgButtonChecked(hwnd,IDC_SWAP_Y)==BST_CHECKED);
      WriteSettingInt("csvheader",IsDlgButtonChecked(hwnd,IDC_ADDHEADER)==BST_CHECKED);
      EndDialog(hwnd, LOWORD(wParam));
      return TRUE;

    case IDHELP:
      // HtmlHelp(hwnd,szHelpFile,HH_DISPLAY_TOPIC,(DWORD)"1.htm");
      return TRUE;

    case IDC_TEMPLATE:
      if (HIWORD(wParam)==CBN_SELCHANGE)
        PostMessage(hwnd,U_REFRESH,0,0);
      break;

    case IDC_NEWTEMPLATE:
      str[0]='\0';
      if (DialogBoxParamU(hinstDLL,"NewTemplateDialog",hwndApplication,(DLGPROC)NewTemplateDlgProc,(LPARAM)str)==IDOK) {
        /* before adding the name, test that it does not yet exist */
        if (Combobox_FindStringU(GetDlgItem(hwnd,IDC_TEMPLATE),-1,str,TRUE)<0)
          Combobox_AddStringU(GetDlgItem(hwnd,IDC_TEMPLATE),str);
        idx=Combobox_FindStringU(GetDlgItem(hwnd,IDC_TEMPLATE),-1,str,TRUE);
        SendDlgItemMessage(hwnd,IDC_TEMPLATE,CB_SETCURSEL,idx,0);
      }
      return TRUE;
    }
    break;

  case WM_DESTROY:
    DeleteObject(hfont);
    break;

  case U_REFRESH:
    szTemplate[0]='\0';
    idx=SendDlgItemMessage(hwnd,IDC_TEMPLATE,CB_GETCURSEL,0,0);
    if (idx>=0) {
      Combobox_GetLBTextU(GetDlgItem(hwnd,IDC_TEMPLATE),idx,szTemplate,sizearray(szTemplate));
      if (strcmp(szTemplate,"-")==0)
        szTemplate[0]='\0';
    }
    /* see whether this is for the top or the bottom side (or for both sides) */
    cur=ReadSettingInt("side",BOTH_SEPARATE);
    SendDlgItemMessage(hwnd,IDC_SIDE,CB_SETCURSEL,cur,0);
    /* select the stage to output */
    cur=ReadSettingInt("stage",0);
    if (cur<=0 || cur>=SendDlgItemMessage(hwnd,IDC_STAGE,CB_GETCOUNT,0,0))
      cur=0;  /* 0 means "all" */
    SendDlgItemMessage(hwnd,IDC_STAGE,CB_SETCURSEL,cur,0);
    /* other settings */
    for (idx=IDC_COLUMN1; idx<=IDC_COLUMN11; idx++) {
      sprintf(item,"centroid%d",idx-IDC_COLUMN1+1);
      cur=ReadSettingInt(item,0);
      SendDlgItemMessage(hwnd,idx,CB_SETCURSEL,cur,0);
    }
    for (idx=IDC_ZERO_PASV; idx<=IDC_ZERO_MISC; idx++) {
      int def=(idx<=IDC_ZERO_DIODE) ? 1 : 0;
      sprintf(item,"zero%d",idx-IDC_ZERO_PASV+1);
      cur=ReadSettingInt(item,def);
      SendDlgItemMessage(hwnd,idx,CB_SETCURSEL,cur,0);
    }
    cur=ReadSettingInt("angle-range",0);
    SendDlgItemMessage(hwnd,IDC_ANGLE_RANGE,CB_SETCURSEL,cur,0);
    CheckDlgButton(hwnd,IDC_ANGLE_CLOCK,ReadSettingInt("angle-clockwise",FALSE) ? BST_CHECKED : BST_UNCHECKED);
    ReadSettingString("ext-centroid","CSV",str,sizearray(str));
    SetDlgItemTextU(hwnd,IDC_EXTENSION,str);
    cur=ReadSettingInt("separator",0);
    SendDlgItemMessage(hwnd,IDC_SEPARATOR,CB_SETCURSEL,cur,0);
    if (ReadSettingInt("unit",0)==0)
      CheckDlgButton(hwnd,IDC_UNIT_MM,BST_CHECKED);
    else
      CheckDlgButton(hwnd,IDC_UNIT_INCH,BST_CHECKED);
    CheckDlgButton(hwnd,IDC_ENQUOTE,ReadSettingInt("enquote-fields",TRUE) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwnd,IDC_ALIGN_FID,ReadSettingInt("align-fiducial",FALSE) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwnd,IDC_INCLUDE_FID,ReadSettingInt("include-fiducials",FALSE) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwnd,IDC_SWAP_Y,ReadSettingInt("invert-y",FALSE) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwnd,IDC_ADDHEADER,ReadSettingInt("csvheader",FALSE) ? BST_CHECKED : BST_UNCHECKED);
    break;
  }
  return FALSE;
}

BOOL CALLBACK NewTemplateDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  static char *TemplateName;

  switch (msg) {
  case WM_INITDIALOG:
    TemplateName=(char*)lParam;
    assert(TemplateName!=NULL);
    SetDlgItemTextU(hwnd,IDC_EDITNAME,TemplateName);
    SendDlgItemMessage(hwnd,IDC_EDITNAME,EM_LIMITTEXT,50,0);
    return TRUE;

  case WM_COMMAND:
    switch (LOWORD(wParam)) {
    case IDCANCEL:
      EndDialog(hwnd, LOWORD(wParam));
      return TRUE;
    case IDOK:
      assert(TemplateName!=NULL);
      GetDlgItemTextU(hwnd,IDC_EDITNAME,TemplateName,50);
      EndDialog(hwnd, LOWORD(wParam));
      return TRUE;
    }
    break;
  }
  return FALSE;
}

/* vp_WriteBOM()
 *
 * filename     The full path name of the file that should be created.
 * info         The records in the "items" parameter.
 * size         The number of records in the "info" parameter.
 */
BOOL PLUGINEXPORT vp_WriteBOM(LPCSTR filename, const PARTINFO *info, int size)
{
  FILE *fp;
  int idx,val,record,numcolumns,qty,seq,r2;
  int column[IDC_COLUMN11-IDC_COLUMN1+1];
  char item[30],*str;
  char extension[30],buffer[_MAX_PATH],*ext;
  char separator;
  BOOL markstage,enquote,include_nomount;
  BOOL ignore;

  LastError=VPERROR_NONE;

  /* read the user field names (needed for the dialog and this routine) */
  for (idx=0; idx<sizearray(UserFields); idx++) {
    sprintf(item,"userfield%d",idx+1);
    GetPrivateProfileStringU("Settings",item,"",UserFields[idx],sizearray(UserFields[idx]),szProfile);
    if (strlen(UserFields[idx])==0)
      sprintf(UserFields[idx],"User field (%d)",idx+1);
  }

  CurrentStage = 0; /* preset to include all assembly stages */

  if (DialogBoxU(hinstDLL,"BOMDialog",hwndApplication,(DLGPROC)BOMDlgProc)!=IDOK)
    return FALSE;

  /* count the number of columns & check a few items */
  numcolumns=0;
  for (idx=IDC_COLUMN1; idx<=IDC_COLUMN11; idx++) {
    sprintf(item,"bom%d",idx-IDC_COLUMN1+1);
    val=ReadSettingInt(item,0);
    column[idx-IDC_COLUMN1]=val;
    if (val>0)
      numcolumns=idx-IDC_COLUMN1+1;
  }
  /* change default extension */
  ReadSettingString("ext-bom","CSV",buffer,sizearray(buffer));
  _strlwr(buffer);
  if (buffer[0]=='.') {
    strcpy(extension,buffer);
  } else {
    strcpy(extension,".");
    strcat(extension,buffer);
  }
  if ((ext=strrchr(filename,'.'))!=NULL && strchr(ext,'\\')==NULL && _stricmp(ext,".csv")==0 && _stricmp(extension,".csv")!=0) {
    /* replace the default extension */
    strcpy(buffer,filename);
    filename=buffer;
    ext=strrchr(filename,'.');
    assert(ext!=NULL);
    strcpy(ext,extension);
  }
  /* read options */
  markstage=ReadSettingInt("mark-stage",FALSE);
  enquote=ReadSettingInt("enquote-fields",TRUE);
  include_nomount=ReadSettingInt("include-nomount",FALSE);
  val=ReadSettingInt("separator",0);
  switch (val) {
  case 1:
    separator=';';
    break;
  case 2:
    separator='\t';
    break;
  default:
    separator=',';
  }

  /* create the set of active stages */
  StageActive=SetStageMask(CurrentStage-1);

  if ((fp=fopen(filename,"wt"))==NULL) {
    LastError=VPERROR_ACCESS;
    return FALSE;
  }

  str=malloc(REFSTRING_MAX*sizeof(char));
  if (str==NULL) {
    LastError=VPERROR_MEMORY;
    fclose(fp);
    return FALSE;
  }

  /* write the header line */
  for (idx=0; idx<numcolumns; idx++) {
    str[0]='\0';
    if (enquote)
      strcat(str,"\"");
    switch (column[idx]) {
    case B_NONE:
      break;
    case B_SEQUENCE:
      strcat(str,"Row");
      break;
    case B_DESIGNATOR:
      strcat(str,"Designators");
      break;
    case B_VALUE:
      strcat(str,"Value");
      break;
    case B_PACKAGE:
      strcat(str,"Package");
      break;
    case B_PRODUCTNR:
      strcat(str,"ProductNr");
      break;
    case B_QUANTITY:
      strcat(str,"Qty");
      break;
    case B_STAGE:
      strcat(str,"Stage");
      break;
    case B_MOUNT:
      strcat(str,"Mount");
      break;
    case B_USER1:
    case B_USER2:
    case B_USER3:
    case B_USER4:
    case B_USER5:
    case B_USER6:
      val=column[idx]-B_USER1;
      assert(val>=0 && val<sizearray(UserFields));
      strcat(str,UserFields[val]);
      break;
    default:
      assert(0);
    }
    if (enquote)
      strcat(str,"\"");
    fprintf(fp,"%s",str);
    if (idx+1<numcolumns)
      fprintf(fp,"%c",separator);
  }
  fprintf(fp,"\n");

  /* write the records */
  seq=0;
  for (record=0; record<size; record++) {
    if (strlen(info[record].ref)==0 && strlen(info[record].value)==0 && strlen(info[record].package)==0)
      continue; /* skip any record with invalid information */
    if ((info[record].flags & FLAG_NOMOUNT)!=0 && !include_nomount)
      continue; /* skip "dot not mount" records, unless a column for "do not mount" is present */
    /* see if a row with the same package/value was already handled earlier */
    ignore=FALSE;
    for (r2=0; r2<record && !ignore; r2++) {
      if (_stricmp(info[r2].value,info[record].value)==0 && _stricmp(info[r2].package,info[record].package)==0
          && (include_nomount || (info[r2].flags & FLAG_NOMOUNT)==0))
        ignore=TRUE;
    }
    if (ignore)
      continue;
    /* count the records with the same package & value */
    qty=0;
    for (r2=record; r2<size; r2++) {
      if (_stricmp(info[r2].value,info[record].value)==0 && _stricmp(info[r2].package,info[record].package)==0
          && (include_nomount || (info[r2].flags & FLAG_NOMOUNT)==0))
        qty++;
    }
    seq++;
    for (idx=0; idx<numcolumns; idx++) {
      str[0]='\0';
      if (enquote)
        strcat(str,"\"");
      switch (column[idx]) {
      case B_NONE:
        break;
      case B_SEQUENCE:
        sprintf(str+strlen(str),"%d",seq);
        break;
      case B_DESIGNATOR:
        strcat(str,info[record].ref);
        if (markstage && info[record].stage>0) {
          if (enquote && StageMarker=='\\')
            strcat(str,"\\"); /* double backslash, to escape it */
          sprintf(str+strlen(str),"%c%d",StageMarker,info[record].stage);
        }
        /* run through all records with the same value/package, collect the
           designators */
        for (r2=record+1; r2<size; r2++) {
          if (_stricmp(info[r2].value,info[record].value)==0 && _stricmp(info[r2].package,info[record].package)==0
              && (include_nomount || (info[r2].flags & FLAG_NOMOUNT)==0)) {
            strcat(str," ");
            strcat(str,info[r2].ref);
            if (markstage && info[r2].stage>0) {
              if (enquote && StageMarker=='\\')
                strcat(str,"\\"); /* double backslash, to escape it */
              sprintf(str+strlen(str),"%c%d",StageMarker,info[r2].stage);
            }
          }
        }
        break;
      case B_VALUE:
        strcat(str,info[record].value);
        break;
      case B_PACKAGE:
        strcat(str,info[record].package);
        break;
      case B_PRODUCTNR:
        strcat(str,info[record].productnr);
        break;
      case B_QUANTITY:
        sprintf(str+strlen(str),"%d",qty);
        break;
      case B_STAGE: {
        char name[FIELDLENGTH];
        GetStageName(name,sizearray(name),info[record].stage,NULL,0);
        strcat(str,name);
        break;
      }
      case B_MOUNT:
        if ((info[record].flags & FLAG_NOMOUNT)!=0)
          strcat(str,"N");
        else
          strcat(str,"Y");
        break;
      case B_USER1:
      case B_USER2:
      case B_USER3:
      case B_USER4:
      case B_USER5:
      case B_USER6: {
        int f=column[idx]-B_USER1;
        strcat(str,info[record].userfield[f]);
        break;
      } /* case */
      default:
        assert(0);
      }
      if (enquote)
        strcat(str,"\"");
      fprintf(fp,"%s",str);
      if (idx+1<numcolumns)
        fprintf(fp,"%c",separator);
    }
    fprintf(fp,"\n");
  }

  free(str);
  fclose(fp);
  return TRUE;
}

BOOL CALLBACK BOMDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  int idx, cur, flags, reply;
  char item[30], str[256], path[_MAX_PATH], *sections;

  switch (msg) {
  case WM_INITDIALOG:
    /* first get the active template */
    strcpy(szTemplateType,"bom");
    GetPrivateProfileStringU(Section,"template-bom","",szTemplate,sizearray(szTemplate),szProfile);
    /* fill in the comboboxes */
    sprintf(path,"%scsvexport.ini",szLclDataDir);
    Combobox_AddStringU(GetDlgItem(hwnd,IDC_TEMPLATE),"-");
    sections=malloc(65536*sizeof(char));
    if (sections!=NULL) {
      char *ptr;
      GetPrivateProfileStringU(NULL,NULL,"",sections,65536,path);
      for (ptr=sections; strlen(ptr)>0; ptr+=strlen(ptr)+1)
        if (_strnicmp(ptr,"bom:",4)==0)
          Combobox_AddStringU(GetDlgItem(hwnd,IDC_TEMPLATE),ptr+4);
      free(sections);
    }
    Combobox_AddStringU(GetDlgItem(hwnd,IDC_MOUNTSTAGE),"All");
    for (idx=0; ; idx++) {
      GetStageName(str,sizearray(str),idx,NULL,0);
      if (strlen(str)==0)
        break;
      Combobox_AddStringU(GetDlgItem(hwnd,IDC_MOUNTSTAGE),str);
    }
    SendDlgItemMessage(hwnd,IDC_MOUNTSTAGE,CB_SETCURSEL,0,0);
    for (idx=IDC_COLUMN1; idx<=IDC_COLUMN11; idx++) {
      Combobox_AddStringU(GetDlgItem(hwnd,idx),"(none)");
      Combobox_AddStringU(GetDlgItem(hwnd,idx),"Sequence");
      Combobox_AddStringU(GetDlgItem(hwnd,idx),"Designators");
      Combobox_AddStringU(GetDlgItem(hwnd,idx),"Value");
      Combobox_AddStringU(GetDlgItem(hwnd,idx),"Package");
      Combobox_AddStringU(GetDlgItem(hwnd,idx),"Product Nr");
      Combobox_AddStringU(GetDlgItem(hwnd,idx),"Quantity");
      Combobox_AddStringU(GetDlgItem(hwnd,idx),"Stage");
      Combobox_AddStringU(GetDlgItem(hwnd,idx),"Mount Y/N");
      Combobox_AddStringU(GetDlgItem(hwnd,idx),UserFields[0]);
      Combobox_AddStringU(GetDlgItem(hwnd,idx),UserFields[1]);
      Combobox_AddStringU(GetDlgItem(hwnd,idx),UserFields[2]);
      Combobox_AddStringU(GetDlgItem(hwnd,idx),UserFields[3]);
    }
    Combobox_AddStringU(GetDlgItem(hwnd,IDC_SEPARATOR),"Comma");
    Combobox_AddStringU(GetDlgItem(hwnd,IDC_SEPARATOR),"Semicolon");
    Combobox_AddStringU(GetDlgItem(hwnd,IDC_SEPARATOR),"Tab");
    Combobox_AddStringU(GetDlgItem(hwnd,IDC_STAGEMARKER),"#");
    Combobox_AddStringU(GetDlgItem(hwnd,IDC_STAGEMARKER),":");
    Combobox_AddStringU(GetDlgItem(hwnd,IDC_STAGEMARKER),"/");
    Combobox_AddStringU(GetDlgItem(hwnd,IDC_STAGEMARKER),"\\");
    Combobox_AddStringU(GetDlgItem(hwnd,IDC_STAGEMARKER),"@");
    /* set the template selection in the control, because the code for setting
       the configuration retrieves it from there */
    idx=Combobox_FindStringU(GetDlgItem(hwnd,IDC_TEMPLATE),-1,szTemplate,TRUE);
    if (idx<0)
      idx=Combobox_FindStringU(GetDlgItem(hwnd,IDC_TEMPLATE),-1,"-",TRUE);
    SendDlgItemMessage(hwnd,IDC_TEMPLATE,CB_SETCURSEL,idx,0);
    /* set the configuration */
    PostMessage(hwnd,U_REFRESH,0,0);
    return TRUE;

  case WM_COMMAND:
    switch (LOWORD(wParam)) {
    case IDCANCEL:
      EndDialog(hwnd, LOWORD(wParam));
      return TRUE;

    case IDOK:
      /* first get the template */
      szTemplate[0]='\0';
      idx=SendDlgItemMessage(hwnd,IDC_TEMPLATE,CB_GETCURSEL,0,0);
      if (idx>=0) {
        Combobox_GetLBTextU(GetDlgItem(hwnd,IDC_TEMPLATE),idx,szTemplate,sizearray(szTemplate));
        if (strcmp(szTemplate,"-")==0)
          szTemplate[0]='\0';
      }
      WritePrivateProfileStringU(Section,"template-bom",szTemplate,szProfile);
      /* save the current settings, either in the global INI file or in a template */
      flags=0;
      reply=IDOK;
      for (idx=IDC_COLUMN1; idx<=IDC_COLUMN11 && reply==IDOK; idx++) {
        cur=(int)SendDlgItemMessage(hwnd,idx,CB_GETCURSEL,0,0);
        sprintf(item,"bom%d",idx-IDC_COLUMN1+1);
        WriteSettingInt(item,cur);
        if (cur>0 && (flags & (1 << cur))!=0)
          reply=MessageBoxU(hwnd,"The same field is selected in multiple columns","Notice",MB_OKCANCEL);
        flags |= (1 << cur);
      }
      if (reply==IDOK && (flags & (1 << B_DESIGNATOR))==0)
        reply=MessageBoxU(hwnd,"The designator list is absent.\nThe bill of materials may not be usable.","Warning",MB_OKCANCEL);
      if (reply==IDOK && ((flags & (1 << B_VALUE))==0 || (flags & (1 << B_PACKAGE))==0))
        reply=MessageBoxU(hwnd,"The value or package field is absent.\nThe bill of materials may not be usable.","Warning",MB_OKCANCEL);
      if (reply!=IDOK)
        return TRUE;
      WriteSettingInt("mark-stage",IsDlgButtonChecked(hwnd,IDC_MARKSTAGE)==BST_CHECKED);
      WriteSettingInt("separator",(int)SendDlgItemMessage(hwnd,IDC_SEPARATOR,CB_GETCURSEL,0,0));
      WriteSettingInt("enquote-fields",IsDlgButtonChecked(hwnd,IDC_ENQUOTE)==BST_CHECKED);
      WriteSettingInt("include-nomount",IsDlgButtonChecked(hwnd,IDC_INC_NOMOUNT)==BST_CHECKED);
      cur=SendDlgItemMessage(hwnd,IDC_STAGEMARKER,CB_GETCURSEL,0,0);
      StageMarker=stagemarkers[(cur>=0) ? cur : 0];
      sprintf(str,"%c",StageMarker);
      WriteSettingString("stagemarker",str);
      CurrentStage=SendDlgItemMessage(hwnd,IDC_MOUNTSTAGE,CB_GETCURSEL,0,0);
      if (CurrentStage<0)
        CurrentStage=0;
      EndDialog(hwnd, LOWORD(wParam));
      return TRUE;

    case IDHELP:
      // HtmlHelp(hwnd,szHelpFile,HH_DISPLAY_TOPIC,(DWORD_PTR)"1.htm");
      return TRUE;

    case IDC_TEMPLATE:
      if (HIWORD(wParam)==CBN_SELCHANGE)
        PostMessage(hwnd,U_REFRESH,0,0);
      break;

    case IDC_NEWTEMPLATE:
      str[0]='\0';
      if (DialogBoxParamU(hinstDLL,"NewTemplateDialog",hwndApplication,(DLGPROC)NewTemplateDlgProc,(LPARAM)str)==IDOK) {
        /* before adding the name, test that it does not yet exist */
        if (Combobox_FindStringU(GetDlgItem(hwnd,IDC_TEMPLATE),-1,str,TRUE)<0)
          Combobox_AddStringU(GetDlgItem(hwnd,IDC_TEMPLATE),str);
        idx=Combobox_FindStringU(GetDlgItem(hwnd,IDC_TEMPLATE),-1,str,TRUE);
        SendDlgItemMessage(hwnd,IDC_TEMPLATE,CB_SETCURSEL,idx,0);
      }
      return TRUE;

    case IDC_MARKSTAGE:
      if (HIWORD(wParam)==BN_CLICKED)
        EnableWindow(GetDlgItem(hwnd,IDC_STAGEMARKER),IsDlgButtonChecked(hwnd,IDC_MARKSTAGE)==BST_CHECKED);
      return TRUE;
    }
    break;

  case U_REFRESH:
    szTemplate[0]='\0';
    idx=SendDlgItemMessage(hwnd,IDC_TEMPLATE,CB_GETCURSEL,0,0);
    if (idx>=0) {
      Combobox_GetLBTextU(GetDlgItem(hwnd,IDC_TEMPLATE),idx,szTemplate,sizearray(szTemplate));
      if (strcmp(szTemplate,"-")==0)
        szTemplate[0]='\0';
    }
    /* read the most recent settings */
    for (idx=IDC_COLUMN1; idx<=IDC_COLUMN11; idx++) {
      sprintf(item,"bom%d",idx-IDC_COLUMN1+1);
      cur=ReadSettingInt(item,0);
      SendDlgItemMessage(hwnd,idx,CB_SETCURSEL,cur,0);
    }
    ReadSettingString("ext-bom","CSV",str,sizearray(str));
    SetDlgItemTextU(hwnd,IDC_EXTENSION,str);
    cur=ReadSettingInt("separator",0);
    SendDlgItemMessage(hwnd,IDC_SEPARATOR,CB_SETCURSEL,cur,0);
    CheckDlgButton(hwnd,IDC_MARKSTAGE,ReadSettingInt("mark-stage",FALSE) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwnd,IDC_ENQUOTE,ReadSettingInt("enquote-fields",TRUE) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwnd,IDC_INC_NOMOUNT,ReadSettingInt("include-nomount",TRUE) ? BST_CHECKED : BST_UNCHECKED);
    ReadSettingString("stagemarker","#",str,sizearray(str));
    cur=SendDlgItemMessage(hwnd,IDC_STAGEMARKER,CB_FINDSTRINGEXACT,-1,(LPARAM)str);
    SendDlgItemMessage(hwnd,IDC_STAGEMARKER,CB_SETCURSEL,(cur>=0) ? cur : 0,0);
    EnableWindow(GetDlgItem(hwnd,IDC_STAGEMARKER),IsDlgButtonChecked(hwnd,IDC_MARKSTAGE)==BST_CHECKED);
    break;
  }
  return FALSE;
}
