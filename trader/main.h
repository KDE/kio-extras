#ifndef __main_h__
#define __main_h__

#include <kio_interface.h>
#include <kio_base.h>

#include <map>
#include <string>
#include <list>

#include "typerepo.h"
#include "trader.h"

typedef CosTradingRepos::ServiceTypeRepository::TypeStruct ServiceType;
typedef CosTrading::Offer OfferInfo;

struct TraderInfo
{
  ServiceType* findServiceType( const char *_type );
  bool findServiceTypeByPrefix( const char *_prefix, list<string>& _lst );
  void findServiceTypePostfixes( const char *_prefix, list<string>& _lst );
  OfferInfo* TraderInfo::findOffer( const char *_type, const char *_name, OfferInfo& _offer );
  
  map<string,ServiceType> m_mapTypes;
  CosTrading::Lookup_var m_vLookup;
};

class TraderProtocol : public IOProtocol
{
public:
  TraderProtocol( Connection *_conn );
  
  virtual void slotGet( const char *_url );
  virtual void slotListDir( const char *_url );
  virtual void slotTestDir( const char *_url );
  
  virtual void slotData( void *_p, int _len );
  virtual void slotDataEnd();
  
  Connection* connection() { return ConnectionSignals::m_pConnection; }

protected:
  TraderInfo* findTrader( const char *_trader );
  void preferences( const char* _trader, const char *_type, TraderInfo* _info, ServiceType& _type, string& _html );
  void describeServiceType( const char* _trader, const char *_type, TraderInfo* _info, ServiceType& _type, string& _html );
  void describeOffer( const char* _trader, const char *_type, const char *_name, TraderInfo* _info, OfferInfo& _offer, string& _html );
  void setPreferences( const char *_query, string& _html );
  
  bool splitPath( const char *_path, string& _trader, string& _type );
  bool splitPath( const char *_path, string& _trader, string& _type, string &_offer );
  
  int m_cmd;
  map<string,TraderInfo> m_mapTraders;
};

#endif

