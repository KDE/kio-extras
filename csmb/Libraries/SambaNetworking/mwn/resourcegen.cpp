/* Name: resourcegen.cpp

   Description: This file is a part of the libmwn library.

   Author:	Oleg Noskov (olegn@corel.com)

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.


*/


#include <stdio.h>
#include <time.h>
typedef const char *LPCSTR;
#include "stringres_mwn.h"
#define RESOURCE_ARRAY gStringList

void main(int, char**)
{
	int i;
	time_t t;
	
	time(&t);
	struct tm *pTM = localtime(&t);
	char timebuf[100];
	
	strftime(timebuf, 99, "%Y-%m-%d %H:%M %Z", pTM);

	printf
	(
		"# libmwn shared library resources.\n"
		"# Copyright (C) 1999 Corel Corporation.\n"
		"# FIRST AUTHOR <olegn@corel.com>, 1999.\n"
		"#\n"
		"\n"
		"msgid \"\"\n"
		"msgstr \"\"\n"
		"\"Project-Id-Version: libmwn version 1.0\\n\"\n"
		"\"POT-Creation-Date: %s\\n\"\n"
		"\"PO-Revision-Date: YEAR-MO-DA HO:MI+ZONE\\n\"\n"
		"\"Last-Translator: FULL NAME <EMAIL@ADDRESS>\\n\"\n"
		"\"Language-Team: LANGUAGE <LL@li.org>\\n\"\n"
		"\"MIME-Version: 1.0\\n\"\n"
		"\"Content-Type: text/plain; charset=CHARSET\\n\"\n"
		"\"Content-Transfer-Encoding: ENCODING\\n\"\n"
		"\n",
		timebuf
	);

	for (int i=0; i < sizeof(RESOURCE_ARRAY)/sizeof(LPCSTR); i++)
	{
		printf("#: stringres_mwn.h: %d\nmsgid \"", i+6);
		LPCSTR p = RESOURCE_ARRAY[i];

		while (*p != '\0')
		{
			switch (*p)
			{
				case '\n':
					printf("\\n");
				break;

				case '\t':
					printf("\\t");
				break;

				case '\r':
					printf("\\r");
				break;

				case '\\':
				case '\"':
					putchar('\\');
				
				default:
					putchar(*p);
			}

			p++;
    }
		
		printf("\"\nmsgstr \"\"\n\n");
	}
}

