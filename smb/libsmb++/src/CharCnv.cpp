/*
    This file is part of the smb++ library. Character set conversion.
    Copyright (C) 1999 Erik Forsberg
    forsberg@lysator.liu.se

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program, see the file COPYING; if not, write
    to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
    MA 02139, USA.
*/

#include "defines.h"
#include <string.h>
#ifndef USE_SAMBA

#include "CharCnv.h"

unsigned char iso8859_1_data[][2] =
{
    { '\240', '\377' }, { '\241', '\255' },  { '\242', '\275' }, 
    { '\243', '\234' }, { '\244', '\317' },  { '\245', '\276' },
    { '\246', '\335' }, { '\247', '\365' },  { '\250', '\371' },
    { '\251', '\270' }, { '\252', '\246' },  { '\253', '\256' },
    { '\254', '\252' }, { '\255', '\360' },  { '\256', '\251' },
    { '\257', '\356' }, { '\260', '\370' },  { '\261', '\361' },
    { '\262', '\375' }, { '\263', '\374' },  { '\264', '\357' },
    { '\265', '\346' }, { '\266', '\364' },  { '\267', '\372' },
    { '\270', '\367' }, { '\271', '\373' },  { '\272', '\247' },
    { '\273', '\257' }, { '\274', '\254' },  { '\275', '\253' },
    { '\276', '\363' }, { '\277', '\250' },  { '\300', '\267' },
    { '\301', '\265' }, { '\302', '\266' },  { '\303', '\307' },
    { '\304', '\216' }, { '\305', '\217' },  { '\306', '\222' },
    { '\307', '\200' }, { '\310', '\324' },  { '\311', '\220' },
    { '\312', '\322' }, { '\313', '\323' },  { '\314', '\336' },
    { '\315', '\326' }, { '\316', '\327' },  { '\317', '\330' },
    { '\320', '\321' }, { '\321', '\245' },  { '\322', '\343' },
    { '\323', '\340' }, { '\324', '\342' },  { '\325', '\345' },
    { '\326', '\231' }, { '\327', '\236' },  { '\330', '\235' },
    { '\331', '\353' }, { '\332', '\351' },  { '\333', '\352' },
    { '\334', '\232' }, { '\335', '\355' },  { '\336', '\350' }, 
    { '\337', '\341' }, { '\340', '\205' },  { '\341', '\240' },
    { '\342', '\203' }, { '\343', '\306' },  { '\344', '\204' },
    { '\345', '\206' }, { '\346', '\221' },  { '\347', '\207' },
    { '\350', '\212' }, { '\351', '\202' },  { '\352', '\210' },
    { '\353', '\211' }, { '\354', '\215' },  { '\355', '\241' },
    { '\356', '\214' }, { '\357', '\213' },  { '\360', '\320' },
    { '\361', '\244' }, { '\362', '\255' },  { '\363', '\242' },
    { '\364', '\223' }, { '\365', '\344' },  { '\366', '\224' },
    { '\367', '\366' }, { '\370', '\233' },  { '\371', '\227' },
    { '\372', '\243' }, { '\372', '\226' },  { '\374', '\201' },
    { '\375', '\354' }, { '\376', '\347' },  { '\377', '\230' }
};

CharCnv::CharCnv(const char *char_set_param)
{
  const char *defaultCharset="iso8859-1";
  char *char_set=(char*)char_set_param; // discard const ?
  if ((!char_set) || (!char_set[0])) char_set=(char*)defaultCharset;
  int i;
  int mapno;
  int maplen;
  for(i=0;i<128;i++) unix2winmap[i] = win2unixmap[i] = i;
  if(!strcmp(char_set, "iso8859-1")) {
    maplen = 96;
    mapno = 0;
  }
  else {
    for(i=128;i<256;i++) unix2winmap[i] = win2unixmap[i] = i;
    return;
  }
  for(i=128;i<256;i++)  unix2winmap[i] = win2unixmap[i] =  CTRL_Z;
  for(i=0;i<maplen;i++) {
    switch (mapno) {
    case 0:
      unix2winmap[iso8859_1_data[i][0]] = iso8859_1_data[i][1];
      win2unixmap[iso8859_1_data[i][1]] = iso8859_1_data[i][0];
      break;
    }
  }
}

char *CharCnv::unix2win(char *str) 
{
  char *p;
  for(p=str;*p;p++)
    *p = unix2winmap[(unsigned char)*p];
  return str;
}
    
char *CharCnv::win2unix(char *str)
{
  char *p;
  for(p=str;*p;p++) *p = win2unixmap[(unsigned char)*p];
  return str;
}


#endif
