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

#ifndef _CHARCNV_H
#define _CHARCNV_H
#include "defines.h"
#ifndef USE_SAMBA

#define CTRL_Z 26
#define NR_OF_MAPS 4


class CharCnv
{
private:
  char unix2winmap[256];
  char win2unixmap[256];
private:
  void update_map(char *str);
  
  void init_iso8859_2();
  void init_iso8859_5();
  void init_iso8859_7();
  void init_koi8_r();
  void init_roman8();
public:
  char *unix2win(char *str) const;
  char *win2unix(char *str) const;
  CharCnv(const char *char_set_param=0);
};

#endif
#endif
