/* Name: filerun.cpp

   Description: This file is a part of the Corel File Manager application.

   Authors:	Oleg Noskov (olegn@corel.com)
            Aleksei Rousskikh

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

/*
 * NOTE: This source file was created using tab size = 2.
 * Please respect that setting in case of modifications.
 */

#include "common.h"
#include "kapp.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include "explres.h"

#if (QT_VERSION >= 200)
#include "kstddirs.h"
#include "kglobal.h"
#endif

BOOL FileRun(LPCSTR FilePath, bool inTerminal)
{
	if (*FilePath == '\0')
		return FALSE;

#if (QT_VERSION >= 200)
  QString tmp = KGlobal::dirs()->findResource("exe", "konsole");
#else
  QString tmp = KApplication::kde_bindir();
	
	if (tmp.right(1) != "/")
		tmp += "/";
	
	tmp += "konsole";
#endif

 	if (inTerminal)
	{
		char strScriptName[40];

		strcpy(strScriptName, "/tmp/_dlgrun_scriptXXXXXX");
		mktemp(strScriptName);

		FILE *fp = fopen(strScriptName, "w");

		if (NULL == fp)
			return FALSE; // unable to create temporary file

		QString strCmd(FilePath);

		if (strCmd.contains(' '))
			strCmd = '\"' + strCmd + '\"';

		fprintf(fp, "#!/bin/bash\nrm -f %s\n%s\necho\necho \"%s\"\nread nothing",
				strScriptName,
				(LPCSTR)strCmd
#ifdef QT_20
  .latin1()
#endif
                     ,
				(LPCSTR)LoadString(knPRESS_ENTER_TO_CONTINUE));

		fclose(fp);
		// Make it executable
		chmod(strScriptName, 0755);

		tmp += " -caption \"";
		tmp += FilePath;
		tmp += "\" -e " + QString(strScriptName) + " &"; // in background

		printf("InTerminal %s\n", (const char *)tmp
#ifdef QT_20
  .latin1()
#endif
    );
		system((LPCSTR)tmp
#ifdef QT_20
  .latin1()
#endif
    );
	}
	else
	{
	}

	return TRUE;
}

