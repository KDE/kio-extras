#include <kcmdlineargs.h>
#include <klocale.h>
#include <kapplication.h>
#include <kautomount.h>
#include <kurl.h>
#include <kmessagebox.h>
#include <dcopclient.h>
#include <qtimer.h>
#include <kdebug.h>

#include "kio_devices_mounthelper.h"
#include "kio_devices_mounthelper.moc"

QStringList KIODevicesMountHelperApp::deviceInfo(QString name)
{
        QByteArray data;
        QByteArray param;
        QCString retType;
        QStringList retVal;
        QDataStream streamout(param,IO_WriteOnly);
        streamout<<name;
	dcopClient()->attach();
        if ( dcopClient()->call( "kded",
                 "mountwatcher", "basicDeviceInfo(QString)", param,retType,data,false ) )
        {
          QDataStream streamin(data,IO_ReadOnly);
          streamin>>retVal;
        }
      return retVal;
}

KIODevicesMountHelperApp::KIODevicesMountHelperApp():KApplication() {
	KCmdLineArgs *args = KCmdLineArgs::parsedArgs();;

	KURL url(args->url(0));
		QStringList info=deviceInfo(url.fileName());
                QStringList::Iterator it=info.begin();
//		KMessageBox::information(0,url.url());
                if (it!=info.end())
                {
                	++it;
			if (it!=info.end())
			{
			        QString device=*it;

	                        if (it!=info.end())
        	                {
                	                QString mp=*it; 
                        	        {
	
						if (args->isSet("u"))
						{

							//KAutoUnmount *um=new KAutoUnmount(mp,QString::null);
							KIO::Job * job = KIO::unmount( mp );
						 	connect( job, SIGNAL( result( KIO::Job * ) ), this, SLOT( slotResult( KIO::Job * ) ) );

						}
						else
						{
							 KIO::Job* job = KIO::mount( false, QString::null.ascii(), device, QString::null);
							 connect( job, SIGNAL( result( KIO::Job * ) ), this, SLOT( slotResult( KIO::Job * ) ) );
						}
        	                                return;
                	                }
				}
                        }
                }
//		KMessageBox::information(0,"Wrong data");
		QTimer::singleShot(0,this,SLOT(finished()));
                return;

}

void KIODevicesMountHelperApp::slotResult(KIO::Job* job)
{
	if (job->error())
	{
		errorStr=job->errorText();
		QTimer::singleShot(0,this,SLOT(error()));
	}
	else
	{
//		KDirNotify_stub allDirNotify("*", "KDirNotify*");
//		allDirNotify.FilesAdded( mountpoint );
		QTimer::singleShot(0,this,SLOT(finished()));
	}
}
      
void KIODevicesMountHelperApp::error()
{
	KMessageBox::error(0,errorStr);
	kapp->quit();
}

void KIODevicesMountHelperApp::finished()
{
	kapp->quit();
}



static KCmdLineOptions options[] =
{
    { "u", I18N_NOOP("Unmount given URL"), 0 },
    { "m", I18N_NOOP("Mount given URL (default"), 0 },
    {"!+[URL]",   I18N_NOOP("devices:/ url to mount/unmount."), 0 },
    { 0, 0, 0}
};


int main(int argc, char **argv)
{
	printf("BLAH\n");
     KCmdLineArgs::init(argc, argv, "kio_devices_mounthelper", "kio_devices_mounthelper", "0.1");
	printf("BLAH1\n");

    KCmdLineArgs::addCmdLineOptions( options );
    KApplication::addCmdLineOptions();
	printf("BLAH2\n");

    KApplication *app=new  KIODevicesMountHelperApp();

    app->exec();

}
