/* Name: FilePermissions.cpp

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

#include "FilePermissions.h"
#include <sys/types.h>
#include <sys/stat.h>
#include "propres.h"
#include <pwd.h>
#include <grp.h>
#include <unistd.h> // for geteuid(), getegid()
#include "qstrlist.h"
#include "expcommon.h"
#include "treeitem.h"
#include "filesystem.h"

#define Inherited CFilePermissionsData

////////////////////////////////////////////////////////////////////////////

CFilePermissions::CFilePermissions
(
	LPCSTR Path,
  LPCSTR ShortName,
  QPixmap *pPixmap,
	QWidget* parent,
	const char* name
)
	:
	Inherited( parent, name ),
	m_Path(Path)
{
	struct stat st;

	m_pIconLabel->setPixmap(*pPixmap);
  m_Name->setText(ShortName);

  m_pOwnershipGroupBox->setTitle(LoadString(knOWNERSHIP));
	m_pAccessPermissionsGroupBox->setTitle(LoadString(knACCESS_PERMISSIONS));

	m_pUserLabel->setText(LoadString(knOWNER));
	m_pGroupLabel->setText(LoadString(knGROUP));
	m_pOthersLabel->setText(LoadString(knOTHERS));
  m_pSpecialLabel->setText(LoadString(knSPECIAL));
	m_pSetUIDLabel->setText(LoadString(knSET_UID));
	m_pSetGIDLabel->setText(LoadString(knSET_GID));
	m_pStickyLabel->setText(LoadString(knSTICKY));
	m_pOwnerLabel->setText(LoadString(knOWNER));
	m_pGroupLabel2->setText(LoadString(knGROUP));

	int nLabelWidth = m_pUserLabel->sizeHint().width();

	if (nLabelWidth < m_pGroupLabel->sizeHint().width())
		nLabelWidth = m_pGroupLabel->sizeHint().width();

	if (nLabelWidth < m_pOthersLabel->sizeHint().width())
		nLabelWidth = m_pOthersLabel->sizeHint().width();

	if (nLabelWidth < 60)
		nLabelWidth = 60;

	int nCheckboxStep = 68;

	m_pUserLabel->setGeometry(30, 122, nLabelWidth, 16);
	m_pGroupLabel->setGeometry(30, 148, nLabelWidth, 16);
	m_pOthersLabel->setGeometry(30, 177, nLabelWidth, 16);

	m_IRUSR->move(40 + nLabelWidth, m_IRUSR->y());
	m_IRGRP->move(40 + nLabelWidth, m_IRGRP->y());
	m_IROTH->move(40 + nLabelWidth, m_IROTH->y());

  m_IWUSR->move(m_IRUSR->x() + nCheckboxStep, m_IWUSR->y());
	m_IWGRP->move(m_IRGRP->x() + nCheckboxStep, m_IWGRP->y());
	m_IWOTH->move(m_IROTH->x() + nCheckboxStep, m_IWOTH->y());

	m_IXUSR->move(m_IWUSR->x() + nCheckboxStep, m_IXUSR->y());
	m_IXGRP->move(m_IWGRP->x() + nCheckboxStep, m_IXGRP->y());
	m_IXOTH->move(m_IWOTH->x() + nCheckboxStep, m_IXOTH->y());

	m_ISUID->move(m_IXUSR->x() + nCheckboxStep, m_ISUID->y());
	m_ISGID->move(m_IXGRP->x() + nCheckboxStep, m_ISGID->y());
	m_ISVTX->move(m_IXOTH->x() + nCheckboxStep, m_ISVTX->y());

	m_pSetUIDLabel->setGeometry(m_ISUID->x() + 22, m_pSetUIDLabel->y(), m_pSetUIDLabel->sizeHint().width(), m_pSetUIDLabel->sizeHint().height());
	m_pSetGIDLabel->setGeometry(m_ISGID->x() + 22, m_pSetGIDLabel->y(), m_pSetGIDLabel->sizeHint().width(), m_pSetGIDLabel->sizeHint().height());
	m_pStickyLabel->setGeometry(m_ISVTX->x() + 22, m_pStickyLabel->y(), m_pStickyLabel->sizeHint().width(), m_pStickyLabel->sizeHint().height());

	if (!lstat(Path, &st))
	{
		BOOL IsMyFile = (geteuid() == st.st_uid);

		struct passwd *pw = getpwuid(st.st_uid);

    if (NULL == pw)
      m_CurrentOwner.sprintf("%u", st.st_uid);
    else
      m_CurrentOwner = pw->pw_name;

		struct group *ge = getgrgid(st.st_gid);

    if (NULL == ge)
      m_CurrentGroup.sprintf("%u", st.st_gid);
    else
      m_CurrentGroup = ge->gr_name;

		m_ISUID->setChecked((st.st_mode & S_ISUID) == S_ISUID);
		m_ISGID->setChecked((st.st_mode & S_ISGID) == S_ISGID);
		m_ISVTX->setChecked((st.st_mode & S_ISVTX) == S_ISVTX);
		m_IRUSR->setChecked((st.st_mode & S_IRUSR) == S_IRUSR);
		m_IWUSR->setChecked((st.st_mode & S_IWUSR) == S_IWUSR);
		m_IXUSR->setChecked((st.st_mode & S_IXUSR) == S_IXUSR);
		m_IRGRP->setChecked((st.st_mode & S_IRGRP) == S_IRGRP);
		m_IWGRP->setChecked((st.st_mode & S_IWGRP) == S_IWGRP);
		m_IXGRP->setChecked((st.st_mode & S_IXGRP) == S_IXGRP);
		m_IROTH->setChecked((st.st_mode & S_IROTH) == S_IROTH);
		m_IWOTH->setChecked((st.st_mode & S_IWOTH) == S_IWOTH);
		m_IXOTH->setChecked((st.st_mode & S_IXOTH) == S_IXOTH);

    const QColorGroup &CG = m_ISUID->colorGroup();

    QColorGroup cg(CG.foreground(), CG.background(), CG.light(), CG.dark(), CG.midlight(), CG.dark(), CG.background());
    QPalette pal(CG, cg, CG);

    m_ISUID->setPalette(pal);
    m_ISGID->setPalette(pal);
    m_ISVTX->setPalette(pal);
    m_IRUSR->setPalette(pal);
    m_IWUSR->setPalette(pal);
    m_IXUSR->setPalette(pal);
    m_IRGRP->setPalette(pal);
    m_IWGRP->setPalette(pal);
    m_IXGRP->setPalette(pal);
    m_IROTH->setPalette(pal);
    m_IWOTH->setPalette(pal);
    m_IXOTH->setPalette(pal);
    m_Owner->setPalette(pal);
    m_Group->setPalette(pal);

		if (IsReadOnlyFileSystemPath(Path) || S_ISLNK(st.st_mode))
		{
			m_ISUID->setEnabled(FALSE);
			m_ISGID->setEnabled(FALSE);
			m_ISVTX->setEnabled(FALSE);
			m_IRUSR->setEnabled(FALSE);
			m_IWUSR->setEnabled(FALSE);
			m_IXUSR->setEnabled(FALSE);
			m_IRGRP->setEnabled(FALSE);
			m_IWGRP->setEnabled(FALSE);
			m_IXGRP->setEnabled(FALSE);
			m_IROTH->setEnabled(FALSE);
			m_IWOTH->setEnabled(FALSE);
			m_IXOTH->setEnabled(FALSE);
			m_Owner->setEnabled(FALSE);
			m_Group->setEnabled(FALSE);
		}

		if (S_ISDIR(st.st_mode))
		{
			m_pReadLabel->setText(LoadString(knREAD_ENTRIES));
			m_pWriteLabel->setText(LoadString(knWRITE_ENTRIES));
			m_pExecLabel->setText(LoadString(knACCESS_ENTRIES));
		}
		else
		{
			m_pReadLabel->setText(LoadString(knREAD));
			m_pWriteLabel->setText(LoadString(knWRITE));
			m_pExecLabel->setText(LoadString(knEXEC));
		}

		m_pReadLabel->resize(m_pReadLabel->sizeHint().width(), m_pReadLabel->height());
		m_pReadLabel->move(m_IRUSR->x() + (m_IRUSR->width() - m_pReadLabel->width()) / 2, m_pReadLabel->y());

		m_pWriteLabel->resize(m_pWriteLabel->sizeHint().width(), m_pWriteLabel->height());
		m_pWriteLabel->move(m_IWUSR->x() + (m_IWUSR->width() - m_pWriteLabel->width()) / 2, m_pWriteLabel->y());

		m_pExecLabel->resize(m_pExecLabel->sizeHint().width(), m_pExecLabel->height());
		m_pExecLabel->move(m_IXUSR->x() + (m_IXUSR->width() - m_pExecLabel->width()) / 2, m_pExecLabel->y());

		m_pSpecialLabel->resize(m_pSpecialLabel->sizeHint().width(), m_pSpecialLabel->height());
		m_pSpecialLabel->move(m_ISUID->x() + (m_ISUID->width() - m_pSpecialLabel->width()) / 2, m_pSpecialLabel->y());

		if (IsMyFile || IsSuperUser())
		{
			// Fill 'Owner' combo
			if (IsSuperUser())
			{
        setpwent();

				QStrList list;

        if (NULL == pw)    // add "number" if owner not found
          list.inSort((LPCSTR)m_CurrentOwner
#ifdef QT_20
  .latin1()
#endif
          );

				while (NULL != (pw=getpwent()))
				{
					list.inSort(pw->pw_name);
				}
				endpwent();

				m_Owner->insertStrList(&list);

				QStrListIterator it(list);
				int i;

				for (i=0; NULL != it.current(); ++it,++i)
				{
					if (!strcmp(it.current(), (LPCSTR)m_CurrentOwner
#ifdef QT_20
  .latin1()
#endif
          ))
					{
						m_Owner->setCurrentItem(i);
						break;
					}
				}
			}
			else
			{
				m_Owner->insertItem(m_CurrentOwner);
				m_Owner->setEnabled(FALSE);
			}

			// Fill 'Group' combo
			setgrent();

			QStrList list;

      if (NULL == ge) // Add "number" if group was not found
        list.inSort((LPCSTR)m_CurrentGroup
#ifdef QT_20
  .latin1()
#endif
        );

      while (NULL != (ge=getgrent()))
			{
				if (IsSuperUser() || m_CurrentGroup == ge->gr_name)
				{
					list.inSort(ge->gr_name);
				}
				else
				{
					char **members = ge->gr_mem;
					char *member;

					while (NULL != (member = *members++))
					{
						if (!strcmp(member, (LPCSTR)m_CurrentOwner
#ifdef QT_20
  .latin1()
#endif
            ))
						{
							list.inSort(ge->gr_name);
							break;
						}
					}
				}
			}
			endgrent();

			m_Group->insertStrList(&list);

			QStrListIterator it(list);
			int i;

			for (i=0; NULL != it.current(); ++it,++i)
			{
        if (!strcmp(it.current(), (LPCSTR)m_CurrentGroup
#ifdef QT_20
  .latin1()
#endif
        ))
				{
					m_Group->setCurrentItem(i);
					break;
				}
			}
		}
		else
		{
			m_Owner->insertItem(m_CurrentOwner);
			m_Group->insertItem(m_CurrentGroup);
			m_Owner->setEnabled(FALSE);
			m_Group->setEnabled(FALSE);
			m_ISUID->setEnabled(FALSE);
			m_ISGID->setEnabled(FALSE);
			m_ISVTX->setEnabled(FALSE);
			m_IRUSR->setEnabled(FALSE);
			m_IWUSR->setEnabled(FALSE);
			m_IXUSR->setEnabled(FALSE);
			m_IRGRP->setEnabled(FALSE);
			m_IWGRP->setEnabled(FALSE);
			m_IXGRP->setEnabled(FALSE);
			m_IROTH->setEnabled(FALSE);
			m_IWOTH->setEnabled(FALSE);
			m_IXOTH->setEnabled(FALSE);
		}
	}
}

////////////////////////////////////////////////////////////////////////////

CFilePermissions::~CFilePermissions()
{
}

////////////////////////////////////////////////////////////////////////////

BOOL CFilePermissions::Apply()
{
	struct stat st;

	if (!lstat((LPCSTR)m_Path
#ifdef QT_20
  .latin1()
#endif
                            , &st))
	{
		mode_t oldmode = st.st_mode;

		st.st_mode &= ~(S_ISUID | S_ISGID | S_ISVTX | S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH);

		if (m_ISUID->isChecked())
			st.st_mode |= S_ISUID;

		if (m_ISGID->isChecked())
			st.st_mode |= S_ISGID;

		if (m_ISVTX->isChecked())
			st.st_mode |= S_ISVTX;

		if (m_IRUSR->isChecked())
			st.st_mode |= S_IRUSR;

		if (m_IWUSR->isChecked())
			st.st_mode |= S_IWUSR;

		if (m_IXUSR->isChecked())
			st.st_mode |= S_IXUSR;

		if (m_IRGRP->isChecked())
			st.st_mode |= S_IRGRP;

		if (m_IWGRP->isChecked())
			st.st_mode |= S_IWGRP;

		if (m_IXGRP->isChecked())
			st.st_mode |= S_IXGRP;

		if (m_IROTH->isChecked())
			st.st_mode |= S_IROTH;

		if (m_IWOTH->isChecked())
			st.st_mode |= S_IWOTH;

		if (m_IXOTH->isChecked())
			st.st_mode |= S_IXOTH;

		if (oldmode != st.st_mode && chmod((LPCSTR)m_Path
#ifdef QT_20
  .latin1()
#endif
    , st.st_mode))
			return FALSE; // unable to chmod

		uid_t newUID = st.st_uid;
		gid_t newGID = st.st_gid;

		if (m_Owner->isEnabled())
		{
			QString strNewOwner = m_Owner->currentText();

			if (m_CurrentOwner != strNewOwner)
			{
				struct passwd *pw = getpwnam((LPCSTR)strNewOwner
#ifdef QT_20
  .latin1()
#endif
                                                  );

				if (NULL != pw)
				{
					newUID = pw->pw_uid;
				}
			}
		}

		if (m_Group->isEnabled() && m_Group->count() > 0)
		{
			QString strNewGroup = m_Group->currentText();

			if (m_CurrentGroup != strNewGroup)
			{
				struct group *ge = getgrnam((LPCSTR)strNewGroup
#ifdef QT_20
  .latin1()
#endif
        );

				if (NULL != ge)
				{
					newGID = ge->gr_gid;
				}
			}
		}

		if (st.st_uid != newUID || st.st_gid != newGID)
		{
			if (LCHOWN((LPCSTR)m_Path
#ifdef QT_20
  .latin1()
#endif
      , newUID, newGID))
				return FALSE; // unable to change owner/group
		}
	}

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////

