/* Name: waitkfm.cpp

    Description: This file is a part of the Corel File Manager application.

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

/*
 * NOTE: This source file was created using tab size = 2.
 * Please respect that setting in case of modifications.
 */

#include <stdio.h>
#include "kapp.h"
#include "qapplication.h"
#include "waitkfm.h"

CWaitKFM::CWaitKFM()
{
	m_bFinished = FALSE;
}

void CWaitKFM::exec(BOOL /*bNeedOpenWithDialog*/, 
										QString & /*sName*/, 
										LPCSTR /*ApplicationName*/)
{
#ifndef QT_20
	m_Name = sName;

	QString UNC;

	if (IsUNCPath(m_Name))
	{
		QString LocalPath;

		if (NetmapWithMessageLoop(LocalPath, m_Name))
			return;	// unable to netmap....

		m_UNC = m_Name;
		m_Name = LocalPath;
		m_bIsUNC = TRUE;
	}
	else
		m_bIsUNC = FALSE;

	connect(&m_kfm, SIGNAL(finished()), this, SLOT(OnKFMDone()));

	if (bNeedOpenWithDialog)
		m_kfm.execNotify(NULL, m_Name, qApp->mainWidget()->winId());
	else
		if (NULL != ApplicationName)
		{
			//printf("ExecNotify: %s %s\n", ApplicationName, (LPCSTR)m_Name);

			m_kfm.execNotify(ApplicationName, m_Name, qApp->mainWidget()->winId());

		}
    else
			m_kfm.execNotify(m_Name, NULL, qApp->mainWidget()->winId());
#endif
}

void CWaitKFM::OnKFMDone()
{
#ifndef QT_20
	m_bFinished = TRUE;

	if (m_bIsUNC)
	{
    FILE *fsync = fopen(((KApplication *)qApp)->localkdedir() + "/share/apps/kfm/tmp/pid_exec", "r+");

		pid_t pid = (pid_t)-1;

		if (NULL != fsync)
		{
			char buf[1024];

			while (!feof(fsync))
			{
				long offset = ftell(fsync);

				fgets(buf, sizeof(buf)-1, fsync);

				if (feof(fsync))
					break;

				if (buf[strlen(buf)-1] == '\n')
					buf[strlen(buf)-1] = '\0';

				buf[strlen(buf)-1] = '\0';

				int l = strlen(buf);

				//printf("Compare [%s] and [%s]\n", buf + l - Name.length(), (LPCSTR)Name);

				if (l > (int)m_Name.length() &&
						!strcmp(buf + l - m_Name.length(), m_Name))
				{
					fseek(fsync, offset, 0);
					memset(buf + l - m_Name.length(), '-', m_Name.length());
					fprintf(fsync, "%s\n", buf);
					fseek(fsync, 0L, 2);
					pid = atoi(buf);
					break;
				}
			}

			fclose(fsync);
		}

		//printf("PID = %d\n", pid);

		if ((pid_t)-1 != pid)
		{
			QString LocalPath;
			NetmapWithMessageLoop(LocalPath, m_UNC, &pid);
		}

		netunmap(m_Name, m_UNC);
	}

	delete this;
#endif
}

