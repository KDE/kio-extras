#include <kcmdlineargs.h>
#include <klocale.h>
#include <kapplication.h>
#include <kautomount.h>
#include <kurl.h>
#include <kmessagebox.h>

#include "kio_devices_mounthelper.h"
#include "kio_devices_mounthelper.moc"

KIODevicesMountHelperApp::KIODevicesMountHelperApp():KApplication() {
	KCmdLineArgs *args = KCmdLineArgs::parsedArgs();;

	KURL url(args->url(0));

	if (args->isSet("u"))
	{
		KAutoUnmount *um=new KAutoUnmount(url.queryItem("mp"),QString::null);
		connect(um,SIGNAL(finished()),this,SLOT(finished()));		
		connect(um,SIGNAL(error()),this,SLOT(error()));		
/*		KMessageBox::information(0,
                         url.prettyURL(),
                         "unmount"); */
	}
	else
	{
		KAutoMount *m=new KAutoMount(false,QString::null,url.queryItem("dev"),QString::null,QString::null,false);
		connect(m,SIGNAL(finished()),this,SLOT(finished()));		
		connect(m,SIGNAL(error()),this,SLOT(error()));		

/*		KMessageBox::information(0,
                         url.prettyURL(),
                         "unmount");*/
	}
}

      
void KIODevicesMountHelperApp::error()
{
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
