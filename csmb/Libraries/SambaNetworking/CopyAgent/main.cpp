/* Name: main.cpp

   Description: This file is a part of the CopyAgent

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
#include "common.h"
#include "qapplication.h"
#include "CopyProgressDlg.h"
#include "kurl.h"
/*#include "httpreader.h" */
#include "kapp.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <qmessagebox.h>
#include <unistd.h>
#include "trashentry.h"
#include <stdlib.h>
#include "commonfunc.h"
#include "PropDialog.h"
#include "filesystem.h"
#include "aps.h"

#ifdef QT_20
#include "kglobal.h"
#endif

BOOL gbSilentMode = FALSE;
extern BOOL gbAutoOverwriteMode;
BOOL gbPostDataMode = FALSE;
BOOL gbDestinationFinalized = FALSE;

struct statfs gStatFS;
BOOL gbStatFSDone = FALSE;

dev_t gDestinationDevice = (dev_t)-1;

QString gsDestination;
QString gsOperation;
QStringArray gSource;

typedef void (*LPFN_Error)(int KError, QString ErrorMessage, int SysError);

//extern LPFN_Error pErrorHandler;

/* The following flags are used as return values for CopyAgent. Should any of
	 these value change, CRightPanel should be modified as well. */
#define F_SUCCESS		0
#define F_FAILURE		2
#define F_INFO			4

#define F_INTRANET	1

// Returns one of F_SUCCESS, F_FAILURE, or F_INFO possibly OR'd with
// F_INTRANET. F_SUCCESS indicates a normal success, F_FAILURE a failure,
// F_INFO that a .info file has been generated (containing a redirection
// address. F_INTRANET is set if the file is from an intranet (as far as we can
// tell).

/*
int LoadFile(const char *_url, const char *localname, const char *datafilename)
{
	QString URL(_url);

	int retcode = F_FAILURE;
	KURL Url(URL);
	CHTTPReader pro;

	if (NULL != datafilename)
	{
		FILE *fdata = fopen(datafilename, "r");

		if (NULL != fdata)
		{
			fseek(fdata,0L,2);
			long datasize = ftell(fdata);
			fseek(fdata,0L,0);
			char *postdata = new char[datasize+1];
			fread(postdata, datasize, 1, fdata);
			postdata[datasize] = '\0';
			fclose(fdata);

			pro.SetData(postdata);
		}
	}

	if (!pro.Open(&Url, 1))
	{
		unsigned long dwSize = pro.Size();
		if (!dwSize)
			return F_FAILURE;

		long dwNow;

		FILE *f = fopen(localname, "w");

		if (NULL != f)
		{
			char buf[65536];

			retcode = F_SUCCESS;
      if (pro.bIntranet)
      	retcode |= F_INTRANET;

			while (dwSize > 0)
			{
        dwNow = dwSize;

				if (dwNow > (long)sizeof(buf))
					dwNow = sizeof(buf);

				dwSize -= dwNow;

				long nBytesRead = pro.Read(buf, dwNow);

				if (-1 == nBytesRead || 0 == nBytesRead)
				{
					if (ftell(f) > 0)
						break;

					fclose(f);
					return F_FAILURE;
				}

				fwrite(buf, 1, (size_t)nBytesRead, f);
			}

			fclose(f);

			if (pro.url != URL)
			{
				QString infoname = localname;
				infoname += ".info";

				f = fopen((LPCSTR)infoname
#ifdef QT_20
  .latin1()
#endif
        , "w");

				if (NULL != f)
				{
					fputs((LPCSTR) pro.url
#ifdef QT_20
  .latin1()
#endif
                                  , f);
					fclose(f);

					retcode = F_INFO;
		      if (pro.bIntranet)
		      	retcode |= F_INTRANET;

					return retcode;
				}
			}
		}
		else
			printf("Unable to write %s\n", localname);
	}
	else
	{
		printf("Unable to open URL %s\n", (LPCSTR)URL
#ifdef QT_20
  .latin1()
#endif
    );
	}

	return retcode;
}
*/

void Error(int KError, QString ErrorMessage, int SysError)
{
	if (gbSilentMode)
	{
		FILE *f = fopen((LPCSTR)gsDestination
#ifdef QT_20
  .latin1()
#endif
                                    , "a");
		fprintf(f, "%s\n", (LPCSTR)ErrorMessage
#ifdef QT_20
  .latin1()
#endif
    );
		fclose(f);
	}
	else
	{
		QMessageBox::critical(qApp->mainWidget(), "Web Browser", (LPCSTR)ErrorMessage
#ifdef QT_20
  .latin1()
#endif
    );
	}
}

BOOL DeletePrinterHandler(LPCSTR PrinterName)
{
	BOOL retcode = FALSE;

  Aps_PrinterHandle hPrinter;

	if (APS_SUCCESS == Aps_OpenPrinter(PrinterName, &hPrinter) &&
      APS_SUCCESS == Aps_PrinterRemove(hPrinter))
		retcode = TRUE;

	return retcode;
}

CSMBErrorCode PrintFileHandler(LPCSTR pszPrinterName, LPCSTR pszFileName)
{
	pid_t pid;

	// We need to fork because our UI must remain alive

	if ((pid = fork()) < 0)
	{
		printf("Unable to fork\n");
		return keNetworkError;
	}
	else
	{
		if (pid == 0)
		{
			/* child */

			if (IsPrinterUrl(pszPrinterName))
				pszPrinterName += 10;

			Aps_PrinterHandle hPrinter;
			CSMBErrorCode retcode = keSuccess; // Assume success

			if (APS_SUCCESS == Aps_OpenPrinter(pszPrinterName, &hPrinter))
			{
				if (APS_SUCCESS != Aps_DispatchJob(hPrinter, pszFileName, NULL, NULL, NULL))
					retcode = keErrorAccessDenied;

				Aps_ReleaseHandle(hPrinter);
			}
			else
				retcode = keUnknownHost;

			exit(retcode);
		}
	}

	// parent

	int status;

	while (!waitpid(pid, &status, WNOHANG))
	{
		qApp->processEvents(500);
	}

	return keErrorAccessDenied;
	return (CSMBErrorCode)status;
}

BOOL HasAutoMountEntries()
{
	BOOL retcode = FALSE;

	char buf[1024];

	sprintf(buf, "%s/.automount", getenv("HOME"));

	FILE *f = fopen(buf, "r");

	if (NULL != f)
	{
		while (!feof(f))
		{
			fgets(buf, sizeof(buf)-1, f);

			if (feof(f))
				break;

			buf[strlen(buf)-1] = '\0';

			if (strlen(buf) > 0)
			{
				retcode = TRUE;
				break;
			}
		}

		fclose(f);
	}

	return retcode;
}

int main(int argc, char **argv)
{
	//for (int j=0;j<argc;j++)
	//{
//		printf("%s%s%s", j?" ":"",argv[j],j==argc-1?"\n":"");
//	}

	void GetDefaultCredentials();
	//pErrorHandler = &Error;

	gpDeletePrinterHandler = &DeletePrinterHandler;
	gpPrintFileHandler = &PrintFileHandler;

	QString Source1;
	BOOL bHasUNCOrFTP = FALSE;

	for (int i=1; i < argc; i++)
	{
		if (!strcmp(argv[i], "-s"))
			gbSilentMode = TRUE;
		else
			if (!strcmp(argv[i], "-r"))
				gbDestinationFinalized = TRUE;
			else
				if (!strcmp(argv[i], "-d"))
					gbPostDataMode = TRUE;
				else
					if (!strcmp(argv[i], "-o"))
						gbAutoOverwriteMode = TRUE;
					else
						if (argv[i][0] != '-' && gsOperation.isEmpty())
						{
              gsOperation = argv[i];

							if (gsOperation == "emptytrash")
							{
								KApplication a(argc, argv);
								EmptyTrash();
								exit(0);
							}

							if (gsOperation == "addprinter")
							{
								KApplication a(argc, argv);
								addAPrinter();
								exit(0);
							}

							if ((gsOperation == "mount" ||
									gsOperation == "umount")
									&& !HasAutoMountEntries())
								exit(0);
						}
						else
							if (argv[i][0] != '-' && gsDestination.isEmpty())
							{
								gsDestination = argv[i];
								URLDecodeSmart(gsDestination);

								if (!bHasUNCOrFTP && (IsUNCPath((LPCSTR)gsDestination
#ifdef QT_20
  .latin1()
#endif
                ) || IsFTPUrl((LPCSTR)gsDestination
#ifdef QT_20
  .latin1()
#endif
                )))
								{
									bHasUNCOrFTP = TRUE;
								}

								struct stat st;

								if (!lstat((LPCSTR)gsDestination
#ifdef QT_20
  .latin1()
#endif

                , &st))
								{
									gDestinationDevice = st.st_dev;
                  gbStatFSDone = (0 == statfs((LPCSTR)gsDestination
#ifdef QT_20
  .latin1()
#endif

                  , &gStatFS));
								}
								else
								{
									if (gsDestination[0] == '/' && gsDestination[1] != '/')
									{
										QString s = GetParentURL((LPCSTR)gsDestination
#ifdef QT_20
  .latin1()
#endif

                    );

										if (!access((LPCSTR)s
#ifdef QT_20
  .latin1()
#endif
                    , 0))
										{
											gbStatFSDone = (0 == statfs((LPCSTR)s
#ifdef QT_20
  .latin1()
#endif
                      , &gStatFS));
										}
									}
								}
							}
							else
								if (argv[i][0] != '-')
								{
									LPCSTR Path = argv[i];

									if (strlen(Path) > 7 && !strnicmp(Path, "file://", 7))
										Path += 7;

									Source1 = Path;
  								URLDecodeSmart(Source1);

									if (!bHasUNCOrFTP && (IsUNCPath((LPCSTR)Source1
#ifdef QT_20
  .latin1()
#endif
                  ) || IsFTPUrl((LPCSTR)Source1
#ifdef QT_20
  .latin1()
#endif
                  )))
									{
										bHasUNCOrFTP = TRUE;
									}

									gSource.Add(Source1);
								}
	}

	if (bHasUNCOrFTP && IsTrashFolder((LPCSTR)gsDestination
#ifdef QT_20
  .latin1()
#endif

  ))
		return -1; // we don't allow to trash from the network...

	KApplication a(argc, argv, "CorelExplorer");

#ifdef QT_20
	KGlobal::locale()->insertCatalogue("mwn");
#else
	a.getLocale()->insertCatalogue("mwn");
#endif

/*
	if (!Source1.isEmpty() &&
			Source1.left(7) == "http://")
	{
		//printf("Loading %s\n", (LPCSTR)Source1);
		exit(LoadFile((LPCSTR)Source1
#ifdef QT_20
  .latin1()
#endif
                                , (LPCSTR)gsDestination
#ifdef QT_20
  .latin1()
#endif
                                                      , gbPostDataMode ? (LPCSTR)gsDestination
#ifdef QT_20
  .latin1()
#endif

                                                                                                : NULL));
	}
*/
	if (gsOperation == "pass")
	{
		QWidget w;
		a.setMainWidget(&w);
		w.setCaption("Authentication");

		char Result[1024];
		memset(Result, 0, 4);
		SuperUserExecute("", "echo TEST", Result, sizeof(Result));
		BOOL bSuccess = !strncmp(Result, "TEST", 4);
		return strncmp(Result, "TEST", 4) ? -1 : 0;
	}

	if (gsOperation == "prop")
	{
#ifdef QT_20
		KGlobal::locale()->insertCatalogue("propdlg");
#else
		a.getLocale()->insertCatalogue("propdlg");
#endif
		void PropInitResources();

		PropInitResources();
		ReadConfiguration();
		GetFileSystemList(&gFileSystemList);

		CPropDialog dlg((LPCSTR)gsDestination
#ifdef QT_20
  .latin1()
#endif
    , false, NULL);
		a.setMainWidget(&dlg);
		dlg.exec();
		return 0;
	}

	if (gsOperation == "move")
	{
		GetFileSystemList(&gFileSystemList);
	}

	CCopyProgressDlg dlg;

	if (bHasUNCOrFTP)
		GetDefaultCredentials();

	a.setMainWidget(&dlg);

	if (gbSilentMode)
	{
		a.exec();
	}
	else
		dlg.exec();

	return 0;
}


