#ifndef __smb_h__
#define __smb_h__ "$Id"

#include <qstring.h>

#include <kio_interface.h>
#include <kio_base.h>

#include <SMBIO.h>

#include <string.h>
#include <list>

#include <sys/types.h>


class SmbProtocol : public IOProtocol
{
public:
	SmbProtocol( Connection *_conn );
	virtual ~SmbProtocol();
  
	virtual void slotGet( const char *_url );
	virtual void slotGetSize( const char *_url );

	virtual void slotCopy( const char *_source, const char *_dest );
	virtual void slotCopy( QStringList& _source, const char *_dest );

	virtual void slotListDir( const char *_url );
	virtual void slotTestDir( const char *_url );

	virtual void slotData( void *_p, int _len );
	virtual void slotDataEnd();

	Connection* connection() { return ConnectionSignals::m_pConnection; }

	void jobError( int _errid, const char *_txt );
  
protected:

	SMBIO smbio;

	struct Copy {
		QString absSource;
		QString relDest;
		mode_t mode;
		off_t size;
	};

	struct CopyDir {
		QString absSource;
		QString relDest;
		mode_t mode;
	};

	struct Del {
		QString m_strAbsSource;
		QString m_strRelDest;
		mode_t m_mode;
		off_t m_size;
	};

	long listRecursive( const char *smbURL, const char *dest, list<Copy>& _files, list<CopyDir>& _dirs, bool dirChecked=false);
	void doCopy( QStringList& _source, const char *_dest );
};

class SmbIOJob : public IOJob
{
public:
	SmbIOJob( Connection *_conn, SmbProtocol *_Smb );

	virtual void slotError( int _errid, const char *_txt );

protected:
	SmbProtocol* m_pSmb;
};

#endif // __smb_h__
