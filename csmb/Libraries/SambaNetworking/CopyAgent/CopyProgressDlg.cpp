/* Name: CopyProgressDlg.cpp

   Description: This file is a part of the CopyAgent.

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

/**********************************************************************

	--- Qt Architect generated file ---

	File: CopyProgressDlg.cpp
	Last generated: Wed Mar 3 11:54:46 1999

 *********************************************************************/

#include "common.h"
#include "CopyProgressDlg.h"
#include "qapplication.h"
#include "filedel.h"
#include "filedelr.h"
#include "qtimer.h"
#include "smbfile.h"
#include "smbutil.h"
#include <time.h>
#include "qmessagebox.h"
#include <unistd.h>
#include <errno.h>
#include <sys/vfs.h>
#include "qkeycode.h"
#include "qobjectlist.h"
#include "qpushbutton.h"
#include "filesystem.h"

#define Inherited CCopyProgressDlgData

extern BOOL gbSilentMode;
extern BOOL gbAutoOverwriteMode;
extern QString gsOperation;
extern QString gsDestination;
extern QStringArray gSource;
extern BOOL gbDestinationFinalized;
extern dev_t gDestinationDevice;
extern struct statfs gStatFS;
extern BOOL gbStatFSDone;
int gnNumAnchors = 0;

////////////////////////////////////////////////////////////////////////////

void CCopyProgressDlg::show()
{
	if (!gbSilentMode)
	{
		QDialog::show();
	}
}

////////////////////////////////////////////////////////////////////////////

BOOL CCopyProgressDlg::CountFolder(LPCSTR Path)
{
  if (IsPrinterUrl(Path))
	{
		m_FileJob.append(new CFileJobElement(Path, 0, 0, 0, m_FileJob.m_dwNumFiles++, m_FileJob.m_dwTotalSize, 0, 0));
		return TRUE; // printers can be deleted
	}
	
	if (m_FileJob.m_Type == keFileJobMove &&
			IsReadOnlyFileSystemPath(Path))
	{
		QString s;

		s.sprintf(LoadString(knUNABLE_TO_MOVE_FROM_READONLY_FS), Path);

		QMessageBox::critical(qApp->mainWidget(), 
													LoadString(m_nOperationStringID),
													(LPCSTR)s, 
													LoadString(knOK));
		
		return FALSE; // Abort
	}

	struct stat st;
  int retcode;
 
TryStat:;
	qApp->processEvents();
  retcode = lstat(Path, &st);
	
//	printf("CountFolder(%s), len = %d\n", Path, strlen(Path));

  if (retcode && errno == ETXTBSY)
  {
    StartIdle();
    
    int n = ReportCommonFileError(Path, errno,  FALSE, m_nOperationStringID);
    StopIdle();

    if (!n)
      goto TryStat;

    return FALSE;	 // Abort
  }
  
	// Anchor
	gnNumAnchors++;
	m_FileJob.append(new CFileJobElement(Path, 0, 0, st.st_mode, m_FileJob.m_dwNumFiles, m_FileJob.m_dwTotalSize, -1, 0));
  
	if (m_FileJob.m_Type == keFileJobMove && gDestinationDevice != (dev_t)-1)
	{
		if (!retcode && st.st_dev == gDestinationDevice)
		{
			int BaseNameLength;

			LPCSTR x = Path + strlen(Path) - 1;

			if (((*x == '/') || (*x == '\\')) && x > Path) // ignore trailing slash...
			{
				x--;
			}
			
			while (*x != '/' && *x != '\\' && x > Path)
				x--;

			BaseNameLength = (x - Path);
			m_FileJob.append(new CFileJobElement(Path, st.st_mtime, st.st_size, st.st_mode, m_FileJob.m_dwNumFiles, m_FileJob.m_dwTotalSize, BaseNameLength, 0));

			return TRUE;
		}
	}

	return CountFolderContents(Path, m_FileJob.m_dwNumFiles, m_FileJob.m_dwNumFolders, m_FileJob.m_dwTotalSize, &m_FileJob, TRUE, (m_FileJob.m_Type == keFileJobDelete) || (m_FileJob.m_Type == keFileJobMove));
}

////////////////////////////////////////////////////////////////////////////

void CCopyProgressDlg::DoStuff()
{
	QTimer *tim = new QTimer(this);
	connect(tim, SIGNAL(timeout()), SLOT(ShowProgress()));
	tim->start(200);

	if (m_FileJob.m_Type == keFileJobMount)
	{
		m_pCopyMovieLabel->hide();
		m_pTextLabel1->setText("");
		setCaption(LoadString(knRESTORING_NETWORK_CONNECTIONS_DOTDOTDOT));
		
		setFixedSize(300,65);
		
		m_pTextLabel1->move(5,5);
		m_pCancelButton->move(width()-m_pCancelButton->width()-5, height()-m_pCancelButton->height()-5);
		qApp->processEvents();
	}
	else
	if (m_FileJob.m_Type == keFileJobUmount)
	{
		m_pCopyMovieLabel->hide();
		m_pTextLabel1->setText("");
		setCaption(LoadString(knCLOSING_NETWORK_CONNECTIONS_DOTDOTDOT));
		
		setFixedSize(300,65);
		
		m_pTextLabel1->move(5,5);
		m_pCancelButton->move(width()-m_pCancelButton->width()-5, height()-m_pCancelButton->height()-5);
		qApp->processEvents();
	}
	else
	{
		m_State = keBuildingFileList;
		if (m_FileJob.m_Type == keFileJobMove ||
				m_FileJob.m_Type == keFileJobCopy ||
				m_FileJob.m_Type == keFileJobDelete ||
				m_FileJob.m_Type == keFileJobPrint)
		{
			for (int i=0; i < gSource.count(); i++)
			{
				if (IsUNCPath(gSource[i]))
				{
					m_UNCPath.Add(gSource[i]);
					
					if (NetmapWithMessageLoop(gSource[i], m_UNCPath[m_UNCPath.count()-1]))
						exit(-1);	// unable to netmap....
				}
	TryCount:;			
				errno = 0;
				if (!CountFolder(gSource[i]))
				{
					if (errno)  // failed FTP connection will leave errno unchanged and display its own message...
					{
						StartIdle();
		
						int n = ReportCommonFileError((LPCSTR)gSource[i], errno,  FALSE, m_nOperationStringID);
						
						if (!n)
						{
							StopIdle();
							goto TryCount;
						}
					}
					PrepareExit();
					exit(-1);
				}
			}
	
			if (gbStatFSDone && 
					(m_FileJob.m_Type == keFileJobMove || m_FileJob.m_Type == keFileJobCopy) &&
					m_FileJob.m_dwTotalSize + m_FileJob.m_dwNumFiles * 512 > (double)gStatFS.f_bavail * (double)gStatFS.f_bsize)
			{
				QString s;
				
				s.sprintf(LoadString(knSIZE_WARNING_TEXT),
						(LPCSTR)SizeBytesFormat((double)gStatFS.f_bavail * (double)gStatFS.f_bsize),
						(LPCSTR)SizeBytesFormat(m_FileJob.m_dwTotalSize));
	
				if (QMessageBox::warning(qApp->mainWidget(), LoadString(knSIZE_WARNING), (LPCSTR)s, LoadString(knIGNORE), LoadString(knABORT), NULL, 1, 1))
				{
					PrepareExit();
					exit(0);
				}
			}
	
			m_State = keCopying;
	
			QByteArray x;
			
			extern unsigned char copy_movie_data[];
			extern int gCopyMovieLen;
			
			//extern unsigned char delete_permanent_movie_data[];
			extern int gDeletePermanentMovieLen;
			
			//extern unsigned char delete_trash_movie_data[];													  
			extern int gDeleteTrashMovieLen;
	
			if (m_FileJob.m_Type == keFileJobDelete)
			{
				x.duplicate((LPCSTR)&delete_permanent_movie_data[0], gDeletePermanentMovieLen);
	#ifdef MSICONS
				m_pCopyMovieLabel->setGeometry(18, 10, 304, 60);
	#else
				m_pCopyMovieLabel->setGeometry(18, 5, 64, 64);
	#endif
			}
			else
			{
				if (m_FileJob.m_Type == keFileJobMove && 
						IsTrashFolder(gsDestination))
				{
					x.duplicate((LPCSTR)&delete_trash_movie_data[0], gDeleteTrashMovieLen);
	#ifdef MSICONS
					m_pCopyMovieLabel->setGeometry(18, 10, 272, 60);
	#else
					m_pCopyMovieLabel->setGeometry(18, 5, 135, 64);
	#endif
				}
				else
				{
					x.duplicate((LPCSTR)&copy_movie_data[0], gCopyMovieLen);
	#ifdef MSICONS
					m_pCopyMovieLabel->setGeometry(18, 10, 260, 40);
	#else
					m_pCopyMovieLabel->setGeometry(18, 10, 275, 64);
	#endif
				}
	
				//m_BytesLabel->show();
				//m_ByteProgress->show();
				m_ByteProgress->setProgress(0);
			}
	
			m_Movie = QMovie(x);
			
			m_Movie.setBackgroundColor(backgroundColor());
			m_pCopyMovieLabel->setMovie(m_Movie);
	
			m_pTextLabel1->setText(LoadString(m_nOperationStringID));
			m_pTextLabel1->resize(m_pTextLabel1->sizeHint().width(), m_pTextLabel1->sizeHint().height());
			
			setCaption(LoadString(m_nOperationStringID));
			m_pCopyMovieLabel->movie()->unpause();
			
			//m_FilesLabel->show();
			//m_FileProgress->show();
			//m_FileProgress->setProgress(0);
			
			m_StartTime = time(NULL);
		}
	}

	if (m_FileJob.count() > 0 || 
			m_FileJob.m_Type == keFileJobMkdir ||
			m_FileJob.m_Type == keFileJobMount ||
			m_FileJob.m_Type == keFileJobUmount)
	{
		if (keDirectoryNotEmpty == m_FileJob.Run())
		{
			gbAutoOverwriteMode = TRUE;
			m_FileJob.m_bNeedDeleteWarning = FALSE;

			while (m_FileJob.m_Outstanding.count() > 0)
			{
				QString x (m_FileJob.m_Outstanding.getLast());
				m_FileJob.m_Outstanding.removeLast();
				m_FileJob.m_Ignore.append(x);

				//printf("Outstanding: %s\n", (LPCSTR)x);
				
				if (IsUNCPath(x))
				{
					m_UNCPath.Add(x);

					if (NetmapWithMessageLoop(x, m_UNCPath[m_UNCPath.count()-1]))
						exit(-1);	// unable to netmap....
				}

				m_FileJob.clear();
				CountFolderContents(x, m_FileJob.m_dwNumFiles, m_FileJob.m_dwNumFolders, m_FileJob.m_dwTotalSize, &m_FileJob, TRUE, TRUE);
				m_FileJob.Run();
				
				rmdir(x);
			}
		}
	}
	else
		tim->stop();
	
	if (gbSilentMode)
	{
		QTimer::singleShot(30, qApp, SLOT(quit()));
	}
	else
		QTimer::singleShot(30, this, SLOT(reject()));
}

////////////////////////////////////////////////////////////////////////////

CCopyProgressDlg::CCopyProgressDlg
(
	QWidget* parent,
	const char* name
)
	:
	Inherited( parent, name )
{
	connect(&gTreeExpansionNotifier, SIGNAL(FileJobWarning(int)), this, SLOT(OnFileJobWarning(int)));
	
	m_pTextLabel1->setGeometry( 18, 72, 480, 20 );
	m_FilesLabel->setText(LoadString(knFILES_COLON));
	m_BytesLabel->setText(LoadString(knBYTES_COLON));
	m_pCancelButton->setText(LoadString(knCANCEL));

	m_FileJob.m_bDestinationFinalized = gbDestinationFinalized;

	if (gsOperation == "copy")
	{
		m_FileJob.m_Type = keFileJobCopy;
		
		if (IsPrinterUrl(gsDestination))
		{
			m_nOperationStringID = knSTR_CREATING_PRINT_JOB;
			m_nPreparingStringID = knSTR_PREPARING_TO_CREATE_PRINT_JOB;
		}
		else
		{
			m_nOperationStringID = knSTR_COPYING;
			m_nPreparingStringID = knSTR_PREPARING_TO_COPY;
		}
	}
	else
		if (gsOperation == "move")
		{
			m_FileJob.m_Type = keFileJobMove;
			m_nOperationStringID = knSTR_MOVING;
			m_nPreparingStringID = knSTR_PREPARING_TO_MOVE;
			
			if (IsTrashFolder(gsDestination))
			{
				if (gbAutoOverwriteMode)
					m_FileJob.m_bNeedDeleteWarning = FALSE;
				else
				{
					BOOL bMultiple = gSource.count() > 1;
					QString msg;
							 
					if (bMultiple)
					{
						QString msg;
						msg.sprintf(LoadString(knCONFIRM_MULTIPLE_FILE_TO_DUMPSTER_QUESTION), gSource.count());
					
						QMessageBox mb(LoadString(knCONFIRM_MULTIPLE_FILE_DELETE), 
													 (LPCSTR)msg, 
													 QMessageBox::Warning, 
													 QMessageBox::Yes | QMessageBox::Default, 
													 QMessageBox::No, 
													 0);
						
						mb.setButtonText(QMessageBox::Yes, LoadString(knYES));
						mb.setButtonText(QMessageBox::No, LoadString(knNO));

						mb.setIconPixmap(*LoadPixmap(keConfirmDeletePermanentIcon));
						
						m_FileJob.m_bNeedDeleteWarning = FALSE;
	
						if (mb.exec() != QMessageBox::Yes)
						{
							if (gbSilentMode)
								QTimer::singleShot(30, qApp, SLOT(quit()));
							else
								QTimer::singleShot(30, this, SLOT(reject()));
							
							return;
						}
					}
				}
			}
		}
		else
			if (gsOperation == "del")
			{
				BOOL bMultiple = gSource.count() > 1;
				QString msg;

				if (gbSilentMode || gbAutoOverwriteMode)
          m_FileJob.m_bNeedDeleteWarning = FALSE;
        else
        {
          if (bMultiple)
  				{
  					QString msg;
  					msg.sprintf(LoadString(knCONFIRM_MULTIPLE_FILE_DELETE_QUESTION), gSource.count());
  				
  					QMessageBox mb(LoadString(knCONFIRM_MULTIPLE_FILE_DELETE), 
													 (LPCSTR)msg, 
													 QMessageBox::Warning, 
													 QMessageBox::Yes | QMessageBox::Default, 
													 QMessageBox::No, 
													 0);
						
						mb.setButtonText(QMessageBox::Yes, LoadString(knYES));
						mb.setButtonText(QMessageBox::No, LoadString(knNO));

  					mb.setIconPixmap(*LoadPixmap(keConfirmDeletePermanentIcon));
  
  					if (mb.exec() != QMessageBox::Yes)
            {
              PrepareExit();
              exit(-1);
            }
  				
  					m_FileJob.m_bNeedDeleteWarning = FALSE;
  				}
          else
    		 		m_FileJob.m_bNeedDeleteWarning = TRUE;
        }
				
        m_FileJob.m_Type = keFileJobDelete;
				m_nOperationStringID = knSTR_DELETING;
				m_nPreparingStringID = knSTR_PREPARING_TO_DELETE;
			}
			else
				if (gsOperation == "mkdir")
					m_FileJob.m_Type = keFileJobMkdir;
				else
					if (gsOperation == "mount")
						m_FileJob.m_Type = keFileJobMount;
					else
						if (gsOperation == "umount")
							m_FileJob.m_Type = keFileJobUmount;
						else
							if (gsOperation == "list")
								m_FileJob.m_Type = keFileJobList;
							else
								if (gsOperation == "print")
								{
									m_FileJob.m_Type = keFileJobPrint;
									m_nOperationStringID = knPRINTING;
									m_nPreparingStringID = knPRINTING;
								}
								else
									return;

	m_FileJob.m_Destination = gsDestination;

	setCaption(LoadString(m_nPreparingStringID));
	
	m_pTextLabel1->setText(LoadString(m_nPreparingStringID));
	m_pTextLabel1->resize(m_pTextLabel1->sizeHint().width(), m_pTextLabel1->sizeHint().height());

	setIconText(LoadString(m_FileJob.m_Type == keFileJobCopy ? knSTR_FILE_COPY : knSTR_FILE_MOVE));
	
	m_bIdling = FALSE;
	m_State = keStarting;

	extern unsigned char search_movie_data[];
	extern int gSearchMovieLen;
	QByteArray x;

	x.duplicate((LPCSTR)&search_movie_data[0], gSearchMovieLen);

	m_Movie = QMovie(x);
	m_Movie.setBackgroundColor(backgroundColor());

	m_pCopyMovieLabel = new QLabel(this);
	
	//m_pCopyMovieLabel->setGeometry(18, 10, 260, 40); //resize(260,40);
	
	m_pCopyMovieLabel->setGeometry(18, 10, knSEARCH_MOVIE_WIDTH, knSEARCH_MOVIE_HEIGHT);
	
	m_pCopyMovieLabel->setMovie(m_Movie);

	m_pCopyMovieLabel->setMargin(0);
	//m_pCopyMovieLabel->hide();
	//m_pCopyMovieLabel->setBackgroundMode(NoBackground);
	
	m_pTextLabel2->setText("");
	
	m_FilesLabel->hide();
	m_FileProgress->hide();
	m_BytesLabel->hide();
	m_ByteProgress->hide();
	
	QTimer::singleShot(30, this, SLOT(DoStuff()));
}

////////////////////////////////////////////////////////////////////////////

void CCopyProgressDlg::ShowProgress()
{
	if (gbIdling && !m_bIdling)
	{
		m_pCopyMovieLabel->movie()->pause();
		m_bIdling = TRUE;
		return;
	}

	if (m_bIdling)
	{
		if (!gbIdling)
		{
			m_pCopyMovieLabel->movie()->unpause();
			m_bIdling = FALSE;
		}

		return;
	}

	m_pTextLabel2->setText(""); //gsOperation);
	
	static int FileProgress = -1;
	static int ByteProgress = -1;
	
	if (m_State == keStarting)
	{
		if (m_FileJob.m_Type == keFileJobMount ||
				m_FileJob.m_Type == keFileJobUmount)
			m_pTextLabel1->setText(SqueezeString(CFileJob::m_NowAtFile, 50));
			m_pTextLabel1->resize(m_pTextLabel1->sizeHint().width(), m_pTextLabel1->sizeHint().height());
	}
	else
	{
		if (m_State == keBuildingFileList)
		{
			QString s;
			
			s.sprintf(LoadString(knBUILDING_FILE_LIST), 
				m_FileJob.m_dwNumFiles, 
				m_FileJob.m_dwNumFolders, 
				(LPCSTR)SizeBytesFormat(m_FileJob.m_dwTotalSize));
			
			m_pTextLabel1->setText((LPCSTR)s);
			m_pTextLabel1->resize(m_pTextLabel1->sizeHint().width(), m_pTextLabel1->sizeHint().height());
		}
		else
		{
			if (m_FileJob.m_Type == keFileJobPrint)
				return; 

			// Update file name

			CFileJobElement *pElement = m_FileJob.current();

			if (NULL != (LPCSTR)CFileJob::m_NowAtFile)
				m_pTextLabel2->setText(SqueezeString(CFileJob::m_NowAtFile, 50));

			// Update file progress bar
			int nNewProgress = 1;

			if (m_FileJob.count() == 0)
				printf("EMPTY JOB!!\n");
			else
      {
        if (m_FileJob.count() - gnNumAnchors == 1)
        {
          static int bFirstTime = TRUE;
          
          if (bFirstTime)
          {
            m_ByteProgress->move(m_FilesLabel->x(), (m_FilesLabel->y() + m_BytesLabel->y()) / 2);
            bFirstTime = FALSE;
          }
        }
				else
        {
          if (!m_FilesLabel->isVisible())
          {
            m_FilesLabel->show();
            m_FileProgress->show();
            m_BytesLabel->show();
          }
          
          nNewProgress = ((m_FileJob.m_dwNowAtFile-1) * 100) / (m_FileJob.count() - gnNumAnchors);

    			if (nNewProgress != FileProgress)
    			{
    				FileProgress = nNewProgress;
    				m_FileProgress->setProgress(FileProgress);
    			}
        }
      }

			// Update byte progress bar
			
      if (!m_ByteProgress->isVisible())
        m_ByteProgress->show();
      
      if (m_FileJob.m_dwTotalSize > 0)
				nNewProgress = (int)(((double)m_FileJob.m_dwNowAtByte * 100.) / m_FileJob.m_dwTotalSize);
      
			if (nNewProgress != ByteProgress)
			{
				ByteProgress = nNewProgress;
        m_ByteProgress->setProgress(ByteProgress);

				if (ByteProgress > 0)
				{
					time_t TimeNow = time(NULL);
					
					int TimeLeft = 1 + ((TimeNow - m_StartTime - gIdleTime) * (100-ByteProgress)) / ByteProgress;

					int hours = TimeLeft / 3600;
					int minutes = (TimeLeft % 3600) / 60;
					int seconds = TimeLeft % 60;

					QString Remaining;
					
					if (hours > 0)
						Remaining.sprintf("%d%% %s, %d:%.2d:%.2d %s", ByteProgress, (LPCSTR)LoadString(knSTR_DONE), hours, minutes, seconds, (LPCSTR)LoadString(knSTR_REMAINING));
					else
						if (minutes > 0)
						{
							Remaining.sprintf("%d%% %s, %.2d:%.2d %s", 
								ByteProgress, 
								(LPCSTR)LoadString(knSTR_DONE), 
								minutes, 
								seconds, 
								(LPCSTR)LoadString(knSTR_REMAINING));
						}
						else
						{
							Remaining.sprintf("%d%% %s, %d %s", 
								ByteProgress, 
								(LPCSTR)LoadString(knSTR_DONE),
								seconds, 
								(LPCSTR)LoadString(knSTR_SECONDS_REMAINING));
						}

					Remaining = LoadString(m_nOperationStringID) + " - " + Remaining;

					setCaption(Remaining);
				}
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////

void CCopyProgressDlg::PrepareExit()
{
	QString s;
	
  for (int i=0; i < m_UNCPath.count(); i++)
		netunmap(s, m_UNCPath[i]);

  m_UNCPath.clear();

	if (!CFileJob::m_UnfinishedFile.isEmpty())
	{
		unlink((LPCSTR)CFileJob::m_UnfinishedFile);
	}
}

////////////////////////////////////////////////////////////////////////////

CCopyProgressDlg::~CCopyProgressDlg()
{
	PrepareExit();
}

////////////////////////////////////////////////////////////////////////////

void CCopyProgressDlg::keyPressEvent(QKeyEvent *e)
{
  if (e->state() == 0 && e->key() == Qt::Key_Escape)
  {
    if (QMessageBox::warning(qApp->mainWidget(), 
                             LoadString(m_nOperationStringID), 
                             LoadString(knESCAPE_WARNING),
                             LoadString(knCONTINUE),
                             LoadString(knABORT), 
                             NULL, 
                             0, 
                             0))
    {
      PrepareExit();
      exit(-1);
    }
    else
      return;
  }
  else
    QDialog::keyPressEvent(e);
}

////////////////////////////////////////////////////////////////////////////

void CCopyProgressDlg::done(int r)
{
	if (!r)
  {
    PrepareExit();
    exit(-1);
  }

  CCopyProgressDlgData::done(r);
}

////////////////////////////////////////////////////////////////////////////

void CCopyProgressDlg::OnFileJobWarning(int bOn)
{
	if (bOn)
	{
 		m_pCopyMovieLabel->movie()->pause();
		m_pCopyMovieLabel->hide();
		gbIdling = TRUE;
	}
	else
	{
		m_pCopyMovieLabel->movie()->unpause();
		m_pCopyMovieLabel->show();
		gbIdling = FALSE;
	}
}

////////////////////////////////////////////////////////////////////////////

