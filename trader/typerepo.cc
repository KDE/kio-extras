/*
 *  MICO --- a free CORBA implementation
 *  Copyright (C) 1997-98 Kay Roemer & Arno Puder
 *
 *  This file was automatically generated. DO NOT EDIT!
 */

#include "typerepo.h"

//--------------------------------------------------------
//  Implementation of stubs
//--------------------------------------------------------
#ifdef HAVE_NAMESPACE
namespace CosTradingRepos { CORBA::TypeCodeConst ServiceTypeRepository::_tc_ServiceTypeNameSeq; };
#else
CORBA::TypeCodeConst CosTradingRepos::ServiceTypeRepository::_tc_ServiceTypeNameSeq;
#endif

#ifdef HAVE_NAMESPACE
namespace CosTradingRepos { CORBA::TypeCodeConst ServiceTypeRepository::_tc_PropertyMode; };
#else
CORBA::TypeCodeConst CosTradingRepos::ServiceTypeRepository::_tc_PropertyMode;
#endif

CORBA::Boolean operator<<=( CORBA::Any &_a, const CosTradingRepos::ServiceTypeRepository::PropertyMode &_e )
{
  _a.type( CosTradingRepos::ServiceTypeRepository::_tc_PropertyMode );
  return (_a.enum_put( (CORBA::ULong) _e ));
}

CORBA::Boolean operator>>=( const CORBA::Any &_a, CosTradingRepos::ServiceTypeRepository::PropertyMode &_e )
{
  CORBA::ULong _ul;
  if( !_a.enum_get( _ul ) )
    return FALSE;
  _e = (CosTradingRepos::ServiceTypeRepository::PropertyMode) _ul;
  return TRUE;
}

#ifdef HAVE_NAMESPACE
namespace CosTradingRepos { CORBA::TypeCodeConst ServiceTypeRepository::_tc_PropStruct; };
#else
CORBA::TypeCodeConst CosTradingRepos::ServiceTypeRepository::_tc_PropStruct;
#endif

#ifdef HAVE_EXPLICIT_STRUCT_OPS
CosTradingRepos::ServiceTypeRepository::PropStruct::PropStruct()
{
}

CosTradingRepos::ServiceTypeRepository::PropStruct::PropStruct( const PropStruct& _s )
{
  name = ((PropStruct&)_s).name;
  value_type = ((PropStruct&)_s).value_type;
  mode = ((PropStruct&)_s).mode;
}

CosTradingRepos::ServiceTypeRepository::PropStruct::~PropStruct()
{
}

CosTradingRepos::ServiceTypeRepository::PropStruct&
CosTradingRepos::ServiceTypeRepository::PropStruct::operator=( const PropStruct& _s )
{
  name = ((PropStruct&)_s).name;
  value_type = ((PropStruct&)_s).value_type;
  mode = ((PropStruct&)_s).mode;
  return *this;
}
#endif

CORBA::Boolean operator<<=( CORBA::Any &_a, const CosTradingRepos::ServiceTypeRepository::PropStruct &_s )
{
  _a.type( CosTradingRepos::ServiceTypeRepository::_tc_PropStruct );
  return (_a.struct_put_begin() &&
    (_a <<= ((CosTradingRepos::ServiceTypeRepository::PropStruct&)_s).name) &&
    (_a <<= ((CosTradingRepos::ServiceTypeRepository::PropStruct&)_s).value_type) &&
    (_a <<= ((CosTradingRepos::ServiceTypeRepository::PropStruct&)_s).mode) &&
    _a.struct_put_end() );
}

CORBA::Boolean operator>>=( const CORBA::Any &_a, CosTradingRepos::ServiceTypeRepository::PropStruct &_s )
{
  return (_a.struct_get_begin() &&
    (_a >>= _s.name) &&
    (_a >>= _s.value_type) &&
    (_a >>= _s.mode) &&
    _a.struct_get_end() );
}

#ifdef HAVE_NAMESPACE
namespace CosTradingRepos { CORBA::TypeCodeConst ServiceTypeRepository::_tc_PropStructSeq; };
#else
CORBA::TypeCodeConst CosTradingRepos::ServiceTypeRepository::_tc_PropStructSeq;
#endif

#ifdef HAVE_NAMESPACE
namespace CosTradingRepos { CORBA::TypeCodeConst ServiceTypeRepository::_tc_Istring; };
#else
CORBA::TypeCodeConst CosTradingRepos::ServiceTypeRepository::_tc_Istring;
#endif

#ifdef HAVE_NAMESPACE
namespace CosTradingRepos { CORBA::TypeCodeConst ServiceTypeRepository::_tc_PropertyName; };
#else
CORBA::TypeCodeConst CosTradingRepos::ServiceTypeRepository::_tc_PropertyName;
#endif

#ifdef HAVE_NAMESPACE
namespace CosTradingRepos { CORBA::TypeCodeConst ServiceTypeRepository::_tc_PropertyValue; };
#else
CORBA::TypeCodeConst CosTradingRepos::ServiceTypeRepository::_tc_PropertyValue;
#endif

#ifdef HAVE_NAMESPACE
namespace CosTradingRepos { CORBA::TypeCodeConst ServiceTypeRepository::_tc_Property; };
#else
CORBA::TypeCodeConst CosTradingRepos::ServiceTypeRepository::_tc_Property;
#endif

#ifdef HAVE_EXPLICIT_STRUCT_OPS
CosTradingRepos::ServiceTypeRepository::Property::Property()
{
}

CosTradingRepos::ServiceTypeRepository::Property::Property( const Property& _s )
{
  value_type = ((Property&)_s).value_type;
  is_file = ((Property&)_s).is_file;
  name = ((Property&)_s).name;
  value = ((Property&)_s).value;
}

CosTradingRepos::ServiceTypeRepository::Property::~Property()
{
}

CosTradingRepos::ServiceTypeRepository::Property&
CosTradingRepos::ServiceTypeRepository::Property::operator=( const Property& _s )
{
  value_type = ((Property&)_s).value_type;
  is_file = ((Property&)_s).is_file;
  name = ((Property&)_s).name;
  value = ((Property&)_s).value;
  return *this;
}
#endif

CORBA::Boolean operator<<=( CORBA::Any &_a, const CosTradingRepos::ServiceTypeRepository::Property &_s )
{
  _a.type( CosTradingRepos::ServiceTypeRepository::_tc_Property );
  return (_a.struct_put_begin() &&
    (_a <<= ((CosTradingRepos::ServiceTypeRepository::Property&)_s).value_type) &&
    (_a <<= CORBA::Any::from_boolean( ((CosTradingRepos::ServiceTypeRepository::Property&)_s).is_file )) &&
    (_a <<= ((CosTradingRepos::ServiceTypeRepository::Property&)_s).name) &&
    (_a <<= ((CosTradingRepos::ServiceTypeRepository::Property&)_s).value) &&
    _a.struct_put_end() );
}

CORBA::Boolean operator>>=( const CORBA::Any &_a, CosTradingRepos::ServiceTypeRepository::Property &_s )
{
  return (_a.struct_get_begin() &&
    (_a >>= _s.value_type) &&
    (_a >>= CORBA::Any::to_boolean( _s.is_file )) &&
    (_a >>= _s.name) &&
    (_a >>= _s.value) &&
    _a.struct_get_end() );
}

#ifdef HAVE_NAMESPACE
namespace CosTradingRepos { CORBA::TypeCodeConst ServiceTypeRepository::_tc_PropertySeq; };
#else
CORBA::TypeCodeConst CosTradingRepos::ServiceTypeRepository::_tc_PropertySeq;
#endif

#ifdef HAVE_NAMESPACE
namespace CosTradingRepos { CORBA::TypeCodeConst ServiceTypeRepository::_tc_Identifier; };
#else
CORBA::TypeCodeConst CosTradingRepos::ServiceTypeRepository::_tc_Identifier;
#endif

#ifdef HAVE_NAMESPACE
namespace CosTradingRepos { CORBA::TypeCodeConst ServiceTypeRepository::_tc_IncarnationNumber; };
#else
CORBA::TypeCodeConst CosTradingRepos::ServiceTypeRepository::_tc_IncarnationNumber;
#endif

#ifdef HAVE_EXPLICIT_STRUCT_OPS
CosTradingRepos::ServiceTypeRepository::IncarnationNumber::IncarnationNumber()
{
}

CosTradingRepos::ServiceTypeRepository::IncarnationNumber::IncarnationNumber( const IncarnationNumber& _s )
{
  high = ((IncarnationNumber&)_s).high;
  low = ((IncarnationNumber&)_s).low;
}

CosTradingRepos::ServiceTypeRepository::IncarnationNumber::~IncarnationNumber()
{
}

CosTradingRepos::ServiceTypeRepository::IncarnationNumber&
CosTradingRepos::ServiceTypeRepository::IncarnationNumber::operator=( const IncarnationNumber& _s )
{
  high = ((IncarnationNumber&)_s).high;
  low = ((IncarnationNumber&)_s).low;
  return *this;
}
#endif

CORBA::Boolean operator<<=( CORBA::Any &_a, const CosTradingRepos::ServiceTypeRepository::IncarnationNumber &_s )
{
  _a.type( CosTradingRepos::ServiceTypeRepository::_tc_IncarnationNumber );
  return (_a.struct_put_begin() &&
    (_a <<= ((CosTradingRepos::ServiceTypeRepository::IncarnationNumber&)_s).high) &&
    (_a <<= ((CosTradingRepos::ServiceTypeRepository::IncarnationNumber&)_s).low) &&
    _a.struct_put_end() );
}

CORBA::Boolean operator>>=( const CORBA::Any &_a, CosTradingRepos::ServiceTypeRepository::IncarnationNumber &_s )
{
  return (_a.struct_get_begin() &&
    (_a >>= _s.high) &&
    (_a >>= _s.low) &&
    _a.struct_get_end() );
}

#ifdef HAVE_NAMESPACE
namespace CosTradingRepos { CORBA::TypeCodeConst ServiceTypeRepository::_tc_TypeStruct; };
#else
CORBA::TypeCodeConst CosTradingRepos::ServiceTypeRepository::_tc_TypeStruct;
#endif

#ifdef HAVE_EXPLICIT_STRUCT_OPS
CosTradingRepos::ServiceTypeRepository::TypeStruct::TypeStruct()
{
}

CosTradingRepos::ServiceTypeRepository::TypeStruct::TypeStruct( const TypeStruct& _s )
{
  if_name = ((TypeStruct&)_s).if_name;
  props = ((TypeStruct&)_s).props;
  super_types = ((TypeStruct&)_s).super_types;
  values = ((TypeStruct&)_s).values;
  masked = ((TypeStruct&)_s).masked;
  incarnation = ((TypeStruct&)_s).incarnation;
}

CosTradingRepos::ServiceTypeRepository::TypeStruct::~TypeStruct()
{
}

CosTradingRepos::ServiceTypeRepository::TypeStruct&
CosTradingRepos::ServiceTypeRepository::TypeStruct::operator=( const TypeStruct& _s )
{
  if_name = ((TypeStruct&)_s).if_name;
  props = ((TypeStruct&)_s).props;
  super_types = ((TypeStruct&)_s).super_types;
  values = ((TypeStruct&)_s).values;
  masked = ((TypeStruct&)_s).masked;
  incarnation = ((TypeStruct&)_s).incarnation;
  return *this;
}
#endif

CORBA::Boolean operator<<=( CORBA::Any &_a, const CosTradingRepos::ServiceTypeRepository::TypeStruct &_s )
{
  _a.type( CosTradingRepos::ServiceTypeRepository::_tc_TypeStruct );
  return (_a.struct_put_begin() &&
    (_a <<= ((CosTradingRepos::ServiceTypeRepository::TypeStruct&)_s).if_name) &&
    (_a <<= ((CosTradingRepos::ServiceTypeRepository::TypeStruct&)_s).props) &&
    (_a <<= ((CosTradingRepos::ServiceTypeRepository::TypeStruct&)_s).super_types) &&
    (_a <<= ((CosTradingRepos::ServiceTypeRepository::TypeStruct&)_s).values) &&
    (_a <<= CORBA::Any::from_boolean( ((CosTradingRepos::ServiceTypeRepository::TypeStruct&)_s).masked )) &&
    (_a <<= ((CosTradingRepos::ServiceTypeRepository::TypeStruct&)_s).incarnation) &&
    _a.struct_put_end() );
}

CORBA::Boolean operator>>=( const CORBA::Any &_a, CosTradingRepos::ServiceTypeRepository::TypeStruct &_s )
{
  return (_a.struct_get_begin() &&
    (_a >>= _s.if_name) &&
    (_a >>= _s.props) &&
    (_a >>= _s.super_types) &&
    (_a >>= _s.values) &&
    (_a >>= CORBA::Any::to_boolean( _s.masked )) &&
    (_a >>= _s.incarnation) &&
    _a.struct_get_end() );
}

#ifdef HAVE_NAMESPACE
namespace CosTradingRepos { CORBA::TypeCodeConst ServiceTypeRepository::_tc_ListOption; };
#else
CORBA::TypeCodeConst CosTradingRepos::ServiceTypeRepository::_tc_ListOption;
#endif

CORBA::Boolean operator<<=( CORBA::Any &_a, const CosTradingRepos::ServiceTypeRepository::ListOption &_e )
{
  _a.type( CosTradingRepos::ServiceTypeRepository::_tc_ListOption );
  return (_a.enum_put( (CORBA::ULong) _e ));
}

CORBA::Boolean operator>>=( const CORBA::Any &_a, CosTradingRepos::ServiceTypeRepository::ListOption &_e )
{
  CORBA::ULong _ul;
  if( !_a.enum_get( _ul ) )
    return FALSE;
  _e = (CosTradingRepos::ServiceTypeRepository::ListOption) _ul;
  return TRUE;
}

#ifdef HAVE_NAMESPACE
namespace CosTradingRepos { CORBA::TypeCodeConst ServiceTypeRepository::_tc_SpecifiedServiceTypes; };
#else
CORBA::TypeCodeConst CosTradingRepos::ServiceTypeRepository::_tc_SpecifiedServiceTypes;
#endif

#ifdef HAVE_EXPLICIT_STRUCT_OPS
CosTradingRepos::ServiceTypeRepository::SpecifiedServiceTypes::SpecifiedServiceTypes()
{
}

CosTradingRepos::ServiceTypeRepository::SpecifiedServiceTypes::SpecifiedServiceTypes( const SpecifiedServiceTypes&_u )
{
  _discriminator = _u._discriminator;
  _m.incarnation = ((SpecifiedServiceTypes&)_u)._m.incarnation;
}

CosTradingRepos::ServiceTypeRepository::SpecifiedServiceTypes::~SpecifiedServiceTypes()
{
}

CosTradingRepos::ServiceTypeRepository::SpecifiedServiceTypes&
CosTradingRepos::ServiceTypeRepository::SpecifiedServiceTypes::operator=( const SpecifiedServiceTypes&_u )
{
  _discriminator = _u._discriminator;
  _m.incarnation = ((SpecifiedServiceTypes&)_u)._m.incarnation;
  return *this;
}
#endif

void CosTradingRepos::ServiceTypeRepository::SpecifiedServiceTypes::_d( CosTradingRepos::ServiceTypeRepository::ListOption _p )
{
  _discriminator = _p;
}

CosTradingRepos::ServiceTypeRepository::ListOption CosTradingRepos::ServiceTypeRepository::SpecifiedServiceTypes::_d() const
{
  return _discriminator;
}

void CosTradingRepos::ServiceTypeRepository::SpecifiedServiceTypes::incarnation( const CosTradingRepos::ServiceTypeRepository::IncarnationNumber& _p )
{
  _discriminator = CosTradingRepos::ServiceTypeRepository::since;
  _m.incarnation = _p;
}

const CosTradingRepos::ServiceTypeRepository::IncarnationNumber& CosTradingRepos::ServiceTypeRepository::SpecifiedServiceTypes::incarnation() const
{
  return (CosTradingRepos::ServiceTypeRepository::IncarnationNumber&) _m.incarnation;
}

CosTradingRepos::ServiceTypeRepository::IncarnationNumber& CosTradingRepos::ServiceTypeRepository::SpecifiedServiceTypes::incarnation()
{
  return _m.incarnation;
}

void CosTradingRepos::ServiceTypeRepository::SpecifiedServiceTypes::_default()
{
  _discriminator = all;
}

CORBA::Boolean operator<<=( CORBA::Any &_a, const CosTradingRepos::ServiceTypeRepository::SpecifiedServiceTypes &_u )
{
  _a.type( CosTradingRepos::ServiceTypeRepository::_tc_SpecifiedServiceTypes );
  if (!_a.union_put_begin())
    return FALSE;
  if( !(_a <<= ((CosTradingRepos::ServiceTypeRepository::SpecifiedServiceTypes&)_u)._discriminator) )
    return FALSE;
  switch( _u._d() ) {
    case CosTradingRepos::ServiceTypeRepository::since:
      if( !_a.union_put_selection( 0 ) )
        return FALSE;
      if( !(_a <<= ((CosTradingRepos::ServiceTypeRepository::SpecifiedServiceTypes&)_u)._m.incarnation) )
        return FALSE;
      break;
    default:
      break;
  }
  return _a.union_put_end();
}

CORBA::Boolean operator>>=( const CORBA::Any &_a, CosTradingRepos::ServiceTypeRepository::SpecifiedServiceTypes &_u )
{
  if( !_a.union_get_begin() )
    return FALSE;
  if( !(_a >>= _u._discriminator) )
    return FALSE;
  switch( _u._discriminator ) {
    case CosTradingRepos::ServiceTypeRepository::since:
      if( !_a.union_get_selection( 0 ) )
        return FALSE;
      if( !(_a >>= _u._m.incarnation) )
        return FALSE;
      break;
    default:
      break;
  }
  return _a.union_get_end();
}

#ifdef HAVE_NAMESPACE
namespace CosTradingRepos { CORBA::TypeCodeConst ServiceTypeRepository::_tc_ServiceTypeExists; };
#else
CORBA::TypeCodeConst CosTradingRepos::ServiceTypeRepository::_tc_ServiceTypeExists;
#endif

#ifdef HAVE_EXPLICIT_STRUCT_OPS
CosTradingRepos::ServiceTypeRepository::ServiceTypeExists::ServiceTypeExists()
{
}

CosTradingRepos::ServiceTypeRepository::ServiceTypeExists::ServiceTypeExists( const ServiceTypeExists& _s )
{
  name = ((ServiceTypeExists&)_s).name;
}

CosTradingRepos::ServiceTypeRepository::ServiceTypeExists::~ServiceTypeExists()
{
}

CosTradingRepos::ServiceTypeRepository::ServiceTypeExists&
CosTradingRepos::ServiceTypeRepository::ServiceTypeExists::operator=( const ServiceTypeExists& _s )
{
  name = ((ServiceTypeExists&)_s).name;
  return *this;
}
#endif

CORBA::Boolean operator<<=( CORBA::Any &_a, const CosTradingRepos::ServiceTypeRepository::ServiceTypeExists &_e )
{
  _a.type( CosTradingRepos::ServiceTypeRepository::_tc_ServiceTypeExists );
  return (_a.except_put_begin( "IDL:omg.org/CosTradingRepos/ServiceTypeRepository/ServiceTypeExists:1.0" ) &&
    (_a <<= ((CosTradingRepos::ServiceTypeRepository::ServiceTypeExists&)_e).name) &&
    _a.except_put_end() );
}

CORBA::Boolean operator>>=( const CORBA::Any &_a, CosTradingRepos::ServiceTypeRepository::ServiceTypeExists &_e )
{
  CORBA::String_var _repoid;
  return (_a.except_get_begin( _repoid ) &&
    (_a >>= _e.name) &&
    _a.except_get_end() );
}

void CosTradingRepos::ServiceTypeRepository::ServiceTypeExists::_throwit() const
{
  #ifdef HAVE_EXCEPTIONS
  throw ServiceTypeExists_var( (CosTradingRepos::ServiceTypeRepository::ServiceTypeExists*)_clone() );
  #else
  CORBA::Exception::_throw_failed( _clone() );
  #endif
}

const char *CosTradingRepos::ServiceTypeRepository::ServiceTypeExists::_repoid() const
{
  return "IDL:omg.org/CosTradingRepos/ServiceTypeRepository/ServiceTypeExists:1.0";
}

void CosTradingRepos::ServiceTypeRepository::ServiceTypeExists::_encode( CORBA::DataEncoder &_en ) const
{
  CORBA::Any _a;
  _a <<= *this;
  _a.marshal( _en );
}

CORBA::Exception *CosTradingRepos::ServiceTypeRepository::ServiceTypeExists::_clone() const
{
  return new ServiceTypeExists( *this );
}

CosTradingRepos::ServiceTypeRepository::ServiceTypeExists *CosTradingRepos::ServiceTypeRepository::ServiceTypeExists::_narrow( CORBA::Exception *_ex )
{
  if( _ex && !strcmp( _ex->_repoid(), "IDL:omg.org/CosTradingRepos/ServiceTypeRepository/ServiceTypeExists:1.0" ) )
    return (ServiceTypeExists *) _ex;
  return NULL;
}

#ifdef HAVE_NAMESPACE
namespace CosTradingRepos { CORBA::TypeCodeConst ServiceTypeRepository::_tc_InterfaceTypeMismatch; };
#else
CORBA::TypeCodeConst CosTradingRepos::ServiceTypeRepository::_tc_InterfaceTypeMismatch;
#endif

#ifdef HAVE_EXPLICIT_STRUCT_OPS
CosTradingRepos::ServiceTypeRepository::InterfaceTypeMismatch::InterfaceTypeMismatch()
{
}

CosTradingRepos::ServiceTypeRepository::InterfaceTypeMismatch::InterfaceTypeMismatch( const InterfaceTypeMismatch& _s )
{
  base_service = ((InterfaceTypeMismatch&)_s).base_service;
  base_if = ((InterfaceTypeMismatch&)_s).base_if;
  derived_service = ((InterfaceTypeMismatch&)_s).derived_service;
  derived_if = ((InterfaceTypeMismatch&)_s).derived_if;
}

CosTradingRepos::ServiceTypeRepository::InterfaceTypeMismatch::~InterfaceTypeMismatch()
{
}

CosTradingRepos::ServiceTypeRepository::InterfaceTypeMismatch&
CosTradingRepos::ServiceTypeRepository::InterfaceTypeMismatch::operator=( const InterfaceTypeMismatch& _s )
{
  base_service = ((InterfaceTypeMismatch&)_s).base_service;
  base_if = ((InterfaceTypeMismatch&)_s).base_if;
  derived_service = ((InterfaceTypeMismatch&)_s).derived_service;
  derived_if = ((InterfaceTypeMismatch&)_s).derived_if;
  return *this;
}
#endif

CORBA::Boolean operator<<=( CORBA::Any &_a, const CosTradingRepos::ServiceTypeRepository::InterfaceTypeMismatch &_e )
{
  _a.type( CosTradingRepos::ServiceTypeRepository::_tc_InterfaceTypeMismatch );
  return (_a.except_put_begin( "IDL:omg.org/CosTradingRepos/ServiceTypeRepository/InterfaceTypeMismatch:1.0" ) &&
    (_a <<= ((CosTradingRepos::ServiceTypeRepository::InterfaceTypeMismatch&)_e).base_service) &&
    (_a <<= ((CosTradingRepos::ServiceTypeRepository::InterfaceTypeMismatch&)_e).base_if) &&
    (_a <<= ((CosTradingRepos::ServiceTypeRepository::InterfaceTypeMismatch&)_e).derived_service) &&
    (_a <<= ((CosTradingRepos::ServiceTypeRepository::InterfaceTypeMismatch&)_e).derived_if) &&
    _a.except_put_end() );
}

CORBA::Boolean operator>>=( const CORBA::Any &_a, CosTradingRepos::ServiceTypeRepository::InterfaceTypeMismatch &_e )
{
  CORBA::String_var _repoid;
  return (_a.except_get_begin( _repoid ) &&
    (_a >>= _e.base_service) &&
    (_a >>= _e.base_if) &&
    (_a >>= _e.derived_service) &&
    (_a >>= _e.derived_if) &&
    _a.except_get_end() );
}

void CosTradingRepos::ServiceTypeRepository::InterfaceTypeMismatch::_throwit() const
{
  #ifdef HAVE_EXCEPTIONS
  throw InterfaceTypeMismatch_var( (CosTradingRepos::ServiceTypeRepository::InterfaceTypeMismatch*)_clone() );
  #else
  CORBA::Exception::_throw_failed( _clone() );
  #endif
}

const char *CosTradingRepos::ServiceTypeRepository::InterfaceTypeMismatch::_repoid() const
{
  return "IDL:omg.org/CosTradingRepos/ServiceTypeRepository/InterfaceTypeMismatch:1.0";
}

void CosTradingRepos::ServiceTypeRepository::InterfaceTypeMismatch::_encode( CORBA::DataEncoder &_en ) const
{
  CORBA::Any _a;
  _a <<= *this;
  _a.marshal( _en );
}

CORBA::Exception *CosTradingRepos::ServiceTypeRepository::InterfaceTypeMismatch::_clone() const
{
  return new InterfaceTypeMismatch( *this );
}

CosTradingRepos::ServiceTypeRepository::InterfaceTypeMismatch *CosTradingRepos::ServiceTypeRepository::InterfaceTypeMismatch::_narrow( CORBA::Exception *_ex )
{
  if( _ex && !strcmp( _ex->_repoid(), "IDL:omg.org/CosTradingRepos/ServiceTypeRepository/InterfaceTypeMismatch:1.0" ) )
    return (InterfaceTypeMismatch *) _ex;
  return NULL;
}

#ifdef HAVE_NAMESPACE
namespace CosTradingRepos { CORBA::TypeCodeConst ServiceTypeRepository::_tc_HasSubTypes; };
#else
CORBA::TypeCodeConst CosTradingRepos::ServiceTypeRepository::_tc_HasSubTypes;
#endif

#ifdef HAVE_EXPLICIT_STRUCT_OPS
CosTradingRepos::ServiceTypeRepository::HasSubTypes::HasSubTypes()
{
}

CosTradingRepos::ServiceTypeRepository::HasSubTypes::HasSubTypes( const HasSubTypes& _s )
{
  the_type = ((HasSubTypes&)_s).the_type;
  sub_type = ((HasSubTypes&)_s).sub_type;
}

CosTradingRepos::ServiceTypeRepository::HasSubTypes::~HasSubTypes()
{
}

CosTradingRepos::ServiceTypeRepository::HasSubTypes&
CosTradingRepos::ServiceTypeRepository::HasSubTypes::operator=( const HasSubTypes& _s )
{
  the_type = ((HasSubTypes&)_s).the_type;
  sub_type = ((HasSubTypes&)_s).sub_type;
  return *this;
}
#endif

CORBA::Boolean operator<<=( CORBA::Any &_a, const CosTradingRepos::ServiceTypeRepository::HasSubTypes &_e )
{
  _a.type( CosTradingRepos::ServiceTypeRepository::_tc_HasSubTypes );
  return (_a.except_put_begin( "IDL:omg.org/CosTradingRepos/ServiceTypeRepository/HasSubTypes:1.0" ) &&
    (_a <<= ((CosTradingRepos::ServiceTypeRepository::HasSubTypes&)_e).the_type) &&
    (_a <<= ((CosTradingRepos::ServiceTypeRepository::HasSubTypes&)_e).sub_type) &&
    _a.except_put_end() );
}

CORBA::Boolean operator>>=( const CORBA::Any &_a, CosTradingRepos::ServiceTypeRepository::HasSubTypes &_e )
{
  CORBA::String_var _repoid;
  return (_a.except_get_begin( _repoid ) &&
    (_a >>= _e.the_type) &&
    (_a >>= _e.sub_type) &&
    _a.except_get_end() );
}

void CosTradingRepos::ServiceTypeRepository::HasSubTypes::_throwit() const
{
  #ifdef HAVE_EXCEPTIONS
  throw HasSubTypes_var( (CosTradingRepos::ServiceTypeRepository::HasSubTypes*)_clone() );
  #else
  CORBA::Exception::_throw_failed( _clone() );
  #endif
}

const char *CosTradingRepos::ServiceTypeRepository::HasSubTypes::_repoid() const
{
  return "IDL:omg.org/CosTradingRepos/ServiceTypeRepository/HasSubTypes:1.0";
}

void CosTradingRepos::ServiceTypeRepository::HasSubTypes::_encode( CORBA::DataEncoder &_en ) const
{
  CORBA::Any _a;
  _a <<= *this;
  _a.marshal( _en );
}

CORBA::Exception *CosTradingRepos::ServiceTypeRepository::HasSubTypes::_clone() const
{
  return new HasSubTypes( *this );
}

CosTradingRepos::ServiceTypeRepository::HasSubTypes *CosTradingRepos::ServiceTypeRepository::HasSubTypes::_narrow( CORBA::Exception *_ex )
{
  if( _ex && !strcmp( _ex->_repoid(), "IDL:omg.org/CosTradingRepos/ServiceTypeRepository/HasSubTypes:1.0" ) )
    return (HasSubTypes *) _ex;
  return NULL;
}

#ifdef HAVE_NAMESPACE
namespace CosTradingRepos { CORBA::TypeCodeConst ServiceTypeRepository::_tc_AlreadyMasked; };
#else
CORBA::TypeCodeConst CosTradingRepos::ServiceTypeRepository::_tc_AlreadyMasked;
#endif

#ifdef HAVE_EXPLICIT_STRUCT_OPS
CosTradingRepos::ServiceTypeRepository::AlreadyMasked::AlreadyMasked()
{
}

CosTradingRepos::ServiceTypeRepository::AlreadyMasked::AlreadyMasked( const AlreadyMasked& _s )
{
  name = ((AlreadyMasked&)_s).name;
}

CosTradingRepos::ServiceTypeRepository::AlreadyMasked::~AlreadyMasked()
{
}

CosTradingRepos::ServiceTypeRepository::AlreadyMasked&
CosTradingRepos::ServiceTypeRepository::AlreadyMasked::operator=( const AlreadyMasked& _s )
{
  name = ((AlreadyMasked&)_s).name;
  return *this;
}
#endif

CORBA::Boolean operator<<=( CORBA::Any &_a, const CosTradingRepos::ServiceTypeRepository::AlreadyMasked &_e )
{
  _a.type( CosTradingRepos::ServiceTypeRepository::_tc_AlreadyMasked );
  return (_a.except_put_begin( "IDL:omg.org/CosTradingRepos/ServiceTypeRepository/AlreadyMasked:1.0" ) &&
    (_a <<= ((CosTradingRepos::ServiceTypeRepository::AlreadyMasked&)_e).name) &&
    _a.except_put_end() );
}

CORBA::Boolean operator>>=( const CORBA::Any &_a, CosTradingRepos::ServiceTypeRepository::AlreadyMasked &_e )
{
  CORBA::String_var _repoid;
  return (_a.except_get_begin( _repoid ) &&
    (_a >>= _e.name) &&
    _a.except_get_end() );
}

void CosTradingRepos::ServiceTypeRepository::AlreadyMasked::_throwit() const
{
  #ifdef HAVE_EXCEPTIONS
  throw AlreadyMasked_var( (CosTradingRepos::ServiceTypeRepository::AlreadyMasked*)_clone() );
  #else
  CORBA::Exception::_throw_failed( _clone() );
  #endif
}

const char *CosTradingRepos::ServiceTypeRepository::AlreadyMasked::_repoid() const
{
  return "IDL:omg.org/CosTradingRepos/ServiceTypeRepository/AlreadyMasked:1.0";
}

void CosTradingRepos::ServiceTypeRepository::AlreadyMasked::_encode( CORBA::DataEncoder &_en ) const
{
  CORBA::Any _a;
  _a <<= *this;
  _a.marshal( _en );
}

CORBA::Exception *CosTradingRepos::ServiceTypeRepository::AlreadyMasked::_clone() const
{
  return new AlreadyMasked( *this );
}

CosTradingRepos::ServiceTypeRepository::AlreadyMasked *CosTradingRepos::ServiceTypeRepository::AlreadyMasked::_narrow( CORBA::Exception *_ex )
{
  if( _ex && !strcmp( _ex->_repoid(), "IDL:omg.org/CosTradingRepos/ServiceTypeRepository/AlreadyMasked:1.0" ) )
    return (AlreadyMasked *) _ex;
  return NULL;
}

#ifdef HAVE_NAMESPACE
namespace CosTradingRepos { CORBA::TypeCodeConst ServiceTypeRepository::_tc_NotMasked; };
#else
CORBA::TypeCodeConst CosTradingRepos::ServiceTypeRepository::_tc_NotMasked;
#endif

#ifdef HAVE_EXPLICIT_STRUCT_OPS
CosTradingRepos::ServiceTypeRepository::NotMasked::NotMasked()
{
}

CosTradingRepos::ServiceTypeRepository::NotMasked::NotMasked( const NotMasked& _s )
{
  name = ((NotMasked&)_s).name;
}

CosTradingRepos::ServiceTypeRepository::NotMasked::~NotMasked()
{
}

CosTradingRepos::ServiceTypeRepository::NotMasked&
CosTradingRepos::ServiceTypeRepository::NotMasked::operator=( const NotMasked& _s )
{
  name = ((NotMasked&)_s).name;
  return *this;
}
#endif

CORBA::Boolean operator<<=( CORBA::Any &_a, const CosTradingRepos::ServiceTypeRepository::NotMasked &_e )
{
  _a.type( CosTradingRepos::ServiceTypeRepository::_tc_NotMasked );
  return (_a.except_put_begin( "IDL:omg.org/CosTradingRepos/ServiceTypeRepository/NotMasked:1.0" ) &&
    (_a <<= ((CosTradingRepos::ServiceTypeRepository::NotMasked&)_e).name) &&
    _a.except_put_end() );
}

CORBA::Boolean operator>>=( const CORBA::Any &_a, CosTradingRepos::ServiceTypeRepository::NotMasked &_e )
{
  CORBA::String_var _repoid;
  return (_a.except_get_begin( _repoid ) &&
    (_a >>= _e.name) &&
    _a.except_get_end() );
}

void CosTradingRepos::ServiceTypeRepository::NotMasked::_throwit() const
{
  #ifdef HAVE_EXCEPTIONS
  throw NotMasked_var( (CosTradingRepos::ServiceTypeRepository::NotMasked*)_clone() );
  #else
  CORBA::Exception::_throw_failed( _clone() );
  #endif
}

const char *CosTradingRepos::ServiceTypeRepository::NotMasked::_repoid() const
{
  return "IDL:omg.org/CosTradingRepos/ServiceTypeRepository/NotMasked:1.0";
}

void CosTradingRepos::ServiceTypeRepository::NotMasked::_encode( CORBA::DataEncoder &_en ) const
{
  CORBA::Any _a;
  _a <<= *this;
  _a.marshal( _en );
}

CORBA::Exception *CosTradingRepos::ServiceTypeRepository::NotMasked::_clone() const
{
  return new NotMasked( *this );
}

CosTradingRepos::ServiceTypeRepository::NotMasked *CosTradingRepos::ServiceTypeRepository::NotMasked::_narrow( CORBA::Exception *_ex )
{
  if( _ex && !strcmp( _ex->_repoid(), "IDL:omg.org/CosTradingRepos/ServiceTypeRepository/NotMasked:1.0" ) )
    return (NotMasked *) _ex;
  return NULL;
}

#ifdef HAVE_NAMESPACE
namespace CosTradingRepos { CORBA::TypeCodeConst ServiceTypeRepository::_tc_ValueTypeRedefinition; };
#else
CORBA::TypeCodeConst CosTradingRepos::ServiceTypeRepository::_tc_ValueTypeRedefinition;
#endif

#ifdef HAVE_EXPLICIT_STRUCT_OPS
CosTradingRepos::ServiceTypeRepository::ValueTypeRedefinition::ValueTypeRedefinition()
{
}

CosTradingRepos::ServiceTypeRepository::ValueTypeRedefinition::ValueTypeRedefinition( const ValueTypeRedefinition& _s )
{
  type_1 = ((ValueTypeRedefinition&)_s).type_1;
  definition_1 = ((ValueTypeRedefinition&)_s).definition_1;
  type_2 = ((ValueTypeRedefinition&)_s).type_2;
  definition_2 = ((ValueTypeRedefinition&)_s).definition_2;
}

CosTradingRepos::ServiceTypeRepository::ValueTypeRedefinition::~ValueTypeRedefinition()
{
}

CosTradingRepos::ServiceTypeRepository::ValueTypeRedefinition&
CosTradingRepos::ServiceTypeRepository::ValueTypeRedefinition::operator=( const ValueTypeRedefinition& _s )
{
  type_1 = ((ValueTypeRedefinition&)_s).type_1;
  definition_1 = ((ValueTypeRedefinition&)_s).definition_1;
  type_2 = ((ValueTypeRedefinition&)_s).type_2;
  definition_2 = ((ValueTypeRedefinition&)_s).definition_2;
  return *this;
}
#endif

CORBA::Boolean operator<<=( CORBA::Any &_a, const CosTradingRepos::ServiceTypeRepository::ValueTypeRedefinition &_e )
{
  _a.type( CosTradingRepos::ServiceTypeRepository::_tc_ValueTypeRedefinition );
  return (_a.except_put_begin( "IDL:omg.org/CosTradingRepos/ServiceTypeRepository/ValueTypeRedefinition:1.0" ) &&
    (_a <<= ((CosTradingRepos::ServiceTypeRepository::ValueTypeRedefinition&)_e).type_1) &&
    (_a <<= ((CosTradingRepos::ServiceTypeRepository::ValueTypeRedefinition&)_e).definition_1) &&
    (_a <<= ((CosTradingRepos::ServiceTypeRepository::ValueTypeRedefinition&)_e).type_2) &&
    (_a <<= ((CosTradingRepos::ServiceTypeRepository::ValueTypeRedefinition&)_e).definition_2) &&
    _a.except_put_end() );
}

CORBA::Boolean operator>>=( const CORBA::Any &_a, CosTradingRepos::ServiceTypeRepository::ValueTypeRedefinition &_e )
{
  CORBA::String_var _repoid;
  return (_a.except_get_begin( _repoid ) &&
    (_a >>= _e.type_1) &&
    (_a >>= _e.definition_1) &&
    (_a >>= _e.type_2) &&
    (_a >>= _e.definition_2) &&
    _a.except_get_end() );
}

void CosTradingRepos::ServiceTypeRepository::ValueTypeRedefinition::_throwit() const
{
  #ifdef HAVE_EXCEPTIONS
  throw ValueTypeRedefinition_var( (CosTradingRepos::ServiceTypeRepository::ValueTypeRedefinition*)_clone() );
  #else
  CORBA::Exception::_throw_failed( _clone() );
  #endif
}

const char *CosTradingRepos::ServiceTypeRepository::ValueTypeRedefinition::_repoid() const
{
  return "IDL:omg.org/CosTradingRepos/ServiceTypeRepository/ValueTypeRedefinition:1.0";
}

void CosTradingRepos::ServiceTypeRepository::ValueTypeRedefinition::_encode( CORBA::DataEncoder &_en ) const
{
  CORBA::Any _a;
  _a <<= *this;
  _a.marshal( _en );
}

CORBA::Exception *CosTradingRepos::ServiceTypeRepository::ValueTypeRedefinition::_clone() const
{
  return new ValueTypeRedefinition( *this );
}

CosTradingRepos::ServiceTypeRepository::ValueTypeRedefinition *CosTradingRepos::ServiceTypeRepository::ValueTypeRedefinition::_narrow( CORBA::Exception *_ex )
{
  if( _ex && !strcmp( _ex->_repoid(), "IDL:omg.org/CosTradingRepos/ServiceTypeRepository/ValueTypeRedefinition:1.0" ) )
    return (ValueTypeRedefinition *) _ex;
  return NULL;
}

#ifdef HAVE_NAMESPACE
namespace CosTradingRepos { CORBA::TypeCodeConst ServiceTypeRepository::_tc_DuplicateServiceTypeName; };
#else
CORBA::TypeCodeConst CosTradingRepos::ServiceTypeRepository::_tc_DuplicateServiceTypeName;
#endif

#ifdef HAVE_EXPLICIT_STRUCT_OPS
CosTradingRepos::ServiceTypeRepository::DuplicateServiceTypeName::DuplicateServiceTypeName()
{
}

CosTradingRepos::ServiceTypeRepository::DuplicateServiceTypeName::DuplicateServiceTypeName( const DuplicateServiceTypeName& _s )
{
  name = ((DuplicateServiceTypeName&)_s).name;
}

CosTradingRepos::ServiceTypeRepository::DuplicateServiceTypeName::~DuplicateServiceTypeName()
{
}

CosTradingRepos::ServiceTypeRepository::DuplicateServiceTypeName&
CosTradingRepos::ServiceTypeRepository::DuplicateServiceTypeName::operator=( const DuplicateServiceTypeName& _s )
{
  name = ((DuplicateServiceTypeName&)_s).name;
  return *this;
}
#endif

CORBA::Boolean operator<<=( CORBA::Any &_a, const CosTradingRepos::ServiceTypeRepository::DuplicateServiceTypeName &_e )
{
  _a.type( CosTradingRepos::ServiceTypeRepository::_tc_DuplicateServiceTypeName );
  return (_a.except_put_begin( "IDL:omg.org/CosTradingRepos/ServiceTypeRepository/DuplicateServiceTypeName:1.0" ) &&
    (_a <<= ((CosTradingRepos::ServiceTypeRepository::DuplicateServiceTypeName&)_e).name) &&
    _a.except_put_end() );
}

CORBA::Boolean operator>>=( const CORBA::Any &_a, CosTradingRepos::ServiceTypeRepository::DuplicateServiceTypeName &_e )
{
  CORBA::String_var _repoid;
  return (_a.except_get_begin( _repoid ) &&
    (_a >>= _e.name) &&
    _a.except_get_end() );
}

void CosTradingRepos::ServiceTypeRepository::DuplicateServiceTypeName::_throwit() const
{
  #ifdef HAVE_EXCEPTIONS
  throw DuplicateServiceTypeName_var( (CosTradingRepos::ServiceTypeRepository::DuplicateServiceTypeName*)_clone() );
  #else
  CORBA::Exception::_throw_failed( _clone() );
  #endif
}

const char *CosTradingRepos::ServiceTypeRepository::DuplicateServiceTypeName::_repoid() const
{
  return "IDL:omg.org/CosTradingRepos/ServiceTypeRepository/DuplicateServiceTypeName:1.0";
}

void CosTradingRepos::ServiceTypeRepository::DuplicateServiceTypeName::_encode( CORBA::DataEncoder &_en ) const
{
  CORBA::Any _a;
  _a <<= *this;
  _a.marshal( _en );
}

CORBA::Exception *CosTradingRepos::ServiceTypeRepository::DuplicateServiceTypeName::_clone() const
{
  return new DuplicateServiceTypeName( *this );
}

CosTradingRepos::ServiceTypeRepository::DuplicateServiceTypeName *CosTradingRepos::ServiceTypeRepository::DuplicateServiceTypeName::_narrow( CORBA::Exception *_ex )
{
  if( _ex && !strcmp( _ex->_repoid(), "IDL:omg.org/CosTradingRepos/ServiceTypeRepository/DuplicateServiceTypeName:1.0" ) )
    return (DuplicateServiceTypeName *) _ex;
  return NULL;
}


// Stub interface ServiceTypeRepository
CosTradingRepos::ServiceTypeRepository::~ServiceTypeRepository()
{
}

CosTradingRepos::ServiceTypeRepository_ptr CosTradingRepos::ServiceTypeRepository::_duplicate( ServiceTypeRepository_ptr _obj )
{
  if( !CORBA::is_nil( _obj ) )
    _obj->_ref();
  return _obj;
}

void *CosTradingRepos::ServiceTypeRepository::_narrow_helper( const char *_repoid )
{
  if( strcmp( _repoid, "IDL:omg.org/CosTradingRepos/ServiceTypeRepository:1.0" ) == 0 )
    return (void *)this;
  return NULL;
}

bool CosTradingRepos::ServiceTypeRepository::_narrow_helper2( CORBA::Object_ptr _obj )
{
  if( strcmp( _obj->_repoid(), "IDL:omg.org/CosTradingRepos/ServiceTypeRepository:1.0" ) == 0) {
    return true;
  }
  for( vector<CORBA::Narrow_proto>::size_type _i = 0;
       _narrow_helpers && _i < _narrow_helpers->size(); _i++ ) {
    bool _res = (*(*_narrow_helpers)[ _i ])( _obj );
    if( _res )
      return true;
  }
  return false;
}

CosTradingRepos::ServiceTypeRepository_ptr CosTradingRepos::ServiceTypeRepository::_narrow( CORBA::Object_ptr _obj )
{
  CosTradingRepos::ServiceTypeRepository_ptr _o;
  if( !CORBA::is_nil( _obj ) ) {
    void *_p;
    if( (_p = _obj->_narrow_helper( "IDL:omg.org/CosTradingRepos/ServiceTypeRepository:1.0" )))
      return _duplicate( (CosTradingRepos::ServiceTypeRepository_ptr) _p );
    if( _narrow_helper2( _obj ) ) {
      _o = new CosTradingRepos::ServiceTypeRepository_stub;
      _o->CORBA::Object::operator=( *_obj );
      return _o;
    }
  }
  return _nil();
}

CosTradingRepos::ServiceTypeRepository_ptr CosTradingRepos::ServiceTypeRepository::_nil()
{
  return NULL;
}

CosTradingRepos::ServiceTypeRepository::IncarnationNumber CosTradingRepos::ServiceTypeRepository_stub::incarnation()
{
  CORBA::Request_var _req = this->_request( "_get_incarnation" );
  _req->result()->value()->type( CosTradingRepos::ServiceTypeRepository::_tc_IncarnationNumber );
  _req->invoke();
  #ifdef HAVE_EXCEPTIONS
  if( _req->env()->exception() ) {
    CORBA::Exception *_ex = _req->env()->exception();
    CORBA::UnknownUserException *_uuex = CORBA::UnknownUserException::_narrow( _ex );
    if( _uuex ) {
      mico_throw( CORBA::UNKNOWN() );
    } else {
      mico_throw( *_ex );
    }
  }
  #else
  {
    CORBA::Exception *_ex;
    if( (_ex = _req->env()->exception()) )
      CORBA::Exception::_throw_failed( _ex );
  }
  #endif
  CosTradingRepos::ServiceTypeRepository::IncarnationNumber _res;
  *_req->result()->value() >>= _res;
  return _res;
}


CosTradingRepos::ServiceTypeRepository_stub::~ServiceTypeRepository_stub()
{
}

CosTradingRepos::ServiceTypeRepository::IncarnationNumber CosTradingRepos::ServiceTypeRepository_stub::add_type( const char* name, const char* if_name, const CosTradingRepos::ServiceTypeRepository::PropStructSeq& props, const CosTradingRepos::ServiceTypeRepository::ServiceTypeNameSeq& super_types, const CosTradingRepos::ServiceTypeRepository::PropertySeq& values )
{
  CORBA::Request_var _req = this->_request( "add_type" );
  _req->add_in_arg( "name" ) <<= CORBA::Any::from_string( (char *) name, 0 );
  _req->add_in_arg( "if_name" ) <<= CORBA::Any::from_string( (char *) if_name, 0 );
  _req->add_in_arg( "props" ) <<= props;
  _req->add_in_arg( "super_types" ) <<= super_types;
  _req->add_in_arg( "values" ) <<= values;
  _req->result()->value()->type( CosTradingRepos::ServiceTypeRepository::_tc_IncarnationNumber );
  _req->invoke();
  #ifdef HAVE_EXCEPTIONS
  if( _req->env()->exception() ) {
    CORBA::Exception *_ex = _req->env()->exception();
    CORBA::UnknownUserException *_uuex = CORBA::UnknownUserException::_narrow( _ex );
    if( _uuex ) {
      if( !strcmp( _uuex->_except_repoid(), "IDL:omg.org/CosTrading/IllegalServiceType:1.0" ) ) {
        CORBA::Any &_a = _uuex->exception( ::CosTrading::_tc_IllegalServiceType );
        ::CosTrading::IllegalServiceType _user_ex;
        _a >>= _user_ex;
        mico_throw( _user_ex );
      }
      if( !strcmp( _uuex->_except_repoid(), "IDL:omg.org/CosTradingRepos/ServiceTypeRepository/ServiceTypeExists:1.0" ) ) {
        CORBA::Any &_a = _uuex->exception( ::CosTradingRepos::ServiceTypeRepository::_tc_ServiceTypeExists );
        ::CosTradingRepos::ServiceTypeRepository::ServiceTypeExists _user_ex;
        _a >>= _user_ex;
        mico_throw( _user_ex );
      }
      if( !strcmp( _uuex->_except_repoid(), "IDL:omg.org/CosTradingRepos/ServiceTypeRepository/InterfaceTypeMismatch:1.0" ) ) {
        CORBA::Any &_a = _uuex->exception( ::CosTradingRepos::ServiceTypeRepository::_tc_InterfaceTypeMismatch );
        ::CosTradingRepos::ServiceTypeRepository::InterfaceTypeMismatch _user_ex;
        _a >>= _user_ex;
        mico_throw( _user_ex );
      }
      if( !strcmp( _uuex->_except_repoid(), "IDL:omg.org/CosTrading/IllegalPropertyName:1.0" ) ) {
        CORBA::Any &_a = _uuex->exception( ::CosTrading::_tc_IllegalPropertyName );
        ::CosTrading::IllegalPropertyName _user_ex;
        _a >>= _user_ex;
        mico_throw( _user_ex );
      }
      if( !strcmp( _uuex->_except_repoid(), "IDL:omg.org/CosTrading/DuplicatePropertyName:1.0" ) ) {
        CORBA::Any &_a = _uuex->exception( ::CosTrading::_tc_DuplicatePropertyName );
        ::CosTrading::DuplicatePropertyName _user_ex;
        _a >>= _user_ex;
        mico_throw( _user_ex );
      }
      if( !strcmp( _uuex->_except_repoid(), "IDL:omg.org/CosTradingRepos/ServiceTypeRepository/ValueTypeRedefinition:1.0" ) ) {
        CORBA::Any &_a = _uuex->exception( ::CosTradingRepos::ServiceTypeRepository::_tc_ValueTypeRedefinition );
        ::CosTradingRepos::ServiceTypeRepository::ValueTypeRedefinition _user_ex;
        _a >>= _user_ex;
        mico_throw( _user_ex );
      }
      if( !strcmp( _uuex->_except_repoid(), "IDL:omg.org/CosTrading/UnknownServiceType:1.0" ) ) {
        CORBA::Any &_a = _uuex->exception( ::CosTrading::_tc_UnknownServiceType );
        ::CosTrading::UnknownServiceType _user_ex;
        _a >>= _user_ex;
        mico_throw( _user_ex );
      }
      if( !strcmp( _uuex->_except_repoid(), "IDL:omg.org/CosTradingRepos/ServiceTypeRepository/DuplicateServiceTypeName:1.0" ) ) {
        CORBA::Any &_a = _uuex->exception( ::CosTradingRepos::ServiceTypeRepository::_tc_DuplicateServiceTypeName );
        ::CosTradingRepos::ServiceTypeRepository::DuplicateServiceTypeName _user_ex;
        _a >>= _user_ex;
        mico_throw( _user_ex );
      }
      mico_throw( CORBA::UNKNOWN() );
    } else {
      mico_throw( *_ex );
    }
  }
  #else
  {
    CORBA::Exception *_ex;
    if( (_ex = _req->env()->exception()) )
      CORBA::Exception::_throw_failed( _ex );
  }
  #endif
  CosTradingRepos::ServiceTypeRepository::IncarnationNumber _res;
  *_req->result()->value() >>= _res;
  return _res;
}


void CosTradingRepos::ServiceTypeRepository_stub::remove_type( const char* name )
{
  CORBA::Request_var _req = this->_request( "remove_type" );
  _req->add_in_arg( "name" ) <<= CORBA::Any::from_string( (char *) name, 0 );
  _req->result()->value()->type( CORBA::_tc_void );
  _req->invoke();
  #ifdef HAVE_EXCEPTIONS
  if( _req->env()->exception() ) {
    CORBA::Exception *_ex = _req->env()->exception();
    CORBA::UnknownUserException *_uuex = CORBA::UnknownUserException::_narrow( _ex );
    if( _uuex ) {
      if( !strcmp( _uuex->_except_repoid(), "IDL:omg.org/CosTrading/IllegalServiceType:1.0" ) ) {
        CORBA::Any &_a = _uuex->exception( ::CosTrading::_tc_IllegalServiceType );
        ::CosTrading::IllegalServiceType _user_ex;
        _a >>= _user_ex;
        mico_throw( _user_ex );
      }
      if( !strcmp( _uuex->_except_repoid(), "IDL:omg.org/CosTrading/UnknownServiceType:1.0" ) ) {
        CORBA::Any &_a = _uuex->exception( ::CosTrading::_tc_UnknownServiceType );
        ::CosTrading::UnknownServiceType _user_ex;
        _a >>= _user_ex;
        mico_throw( _user_ex );
      }
      if( !strcmp( _uuex->_except_repoid(), "IDL:omg.org/CosTradingRepos/ServiceTypeRepository/HasSubTypes:1.0" ) ) {
        CORBA::Any &_a = _uuex->exception( ::CosTradingRepos::ServiceTypeRepository::_tc_HasSubTypes );
        ::CosTradingRepos::ServiceTypeRepository::HasSubTypes _user_ex;
        _a >>= _user_ex;
        mico_throw( _user_ex );
      }
      mico_throw( CORBA::UNKNOWN() );
    } else {
      mico_throw( *_ex );
    }
  }
  #else
  {
    CORBA::Exception *_ex;
    if( (_ex = _req->env()->exception()) )
      CORBA::Exception::_throw_failed( _ex );
  }
  #endif
}


CosTradingRepos::ServiceTypeRepository::ServiceTypeNameSeq* CosTradingRepos::ServiceTypeRepository_stub::list_types( const CosTradingRepos::ServiceTypeRepository::SpecifiedServiceTypes& which_types )
{
  CORBA::Request_var _req = this->_request( "list_types" );
  _req->add_in_arg( "which_types" ) <<= which_types;
  _req->result()->value()->type( CosTradingRepos::ServiceTypeRepository::_tc_ServiceTypeNameSeq );
  _req->invoke();
  #ifdef HAVE_EXCEPTIONS
  if( _req->env()->exception() ) {
    CORBA::Exception *_ex = _req->env()->exception();
    CORBA::UnknownUserException *_uuex = CORBA::UnknownUserException::_narrow( _ex );
    if( _uuex ) {
      mico_throw( CORBA::UNKNOWN() );
    } else {
      mico_throw( *_ex );
    }
  }
  #else
  {
    CORBA::Exception *_ex;
    if( (_ex = _req->env()->exception()) )
      CORBA::Exception::_throw_failed( _ex );
  }
  #endif
  CosTradingRepos::ServiceTypeRepository::ServiceTypeNameSeq* _res = new ::CosTradingRepos::ServiceTypeRepository::ServiceTypeNameSeq;
  *_req->result()->value() >>= *_res;
  return _res;
}


CosTradingRepos::ServiceTypeRepository::TypeStruct* CosTradingRepos::ServiceTypeRepository_stub::describe_type( const char* name )
{
  CORBA::Request_var _req = this->_request( "describe_type" );
  _req->add_in_arg( "name" ) <<= CORBA::Any::from_string( (char *) name, 0 );
  _req->result()->value()->type( CosTradingRepos::ServiceTypeRepository::_tc_TypeStruct );
  _req->invoke();
  #ifdef HAVE_EXCEPTIONS
  if( _req->env()->exception() ) {
    CORBA::Exception *_ex = _req->env()->exception();
    CORBA::UnknownUserException *_uuex = CORBA::UnknownUserException::_narrow( _ex );
    if( _uuex ) {
      if( !strcmp( _uuex->_except_repoid(), "IDL:omg.org/CosTrading/IllegalServiceType:1.0" ) ) {
        CORBA::Any &_a = _uuex->exception( ::CosTrading::_tc_IllegalServiceType );
        ::CosTrading::IllegalServiceType _user_ex;
        _a >>= _user_ex;
        mico_throw( _user_ex );
      }
      if( !strcmp( _uuex->_except_repoid(), "IDL:omg.org/CosTrading/UnknownServiceType:1.0" ) ) {
        CORBA::Any &_a = _uuex->exception( ::CosTrading::_tc_UnknownServiceType );
        ::CosTrading::UnknownServiceType _user_ex;
        _a >>= _user_ex;
        mico_throw( _user_ex );
      }
      mico_throw( CORBA::UNKNOWN() );
    } else {
      mico_throw( *_ex );
    }
  }
  #else
  {
    CORBA::Exception *_ex;
    if( (_ex = _req->env()->exception()) )
      CORBA::Exception::_throw_failed( _ex );
  }
  #endif
  CosTradingRepos::ServiceTypeRepository::TypeStruct* _res = new ::CosTradingRepos::ServiceTypeRepository::TypeStruct;
  *_req->result()->value() >>= *_res;
  return _res;
}


CosTradingRepos::ServiceTypeRepository::TypeStruct* CosTradingRepos::ServiceTypeRepository_stub::fully_describe_type( const char* name )
{
  CORBA::Request_var _req = this->_request( "fully_describe_type" );
  _req->add_in_arg( "name" ) <<= CORBA::Any::from_string( (char *) name, 0 );
  _req->result()->value()->type( CosTradingRepos::ServiceTypeRepository::_tc_TypeStruct );
  _req->invoke();
  #ifdef HAVE_EXCEPTIONS
  if( _req->env()->exception() ) {
    CORBA::Exception *_ex = _req->env()->exception();
    CORBA::UnknownUserException *_uuex = CORBA::UnknownUserException::_narrow( _ex );
    if( _uuex ) {
      if( !strcmp( _uuex->_except_repoid(), "IDL:omg.org/CosTrading/IllegalServiceType:1.0" ) ) {
        CORBA::Any &_a = _uuex->exception( ::CosTrading::_tc_IllegalServiceType );
        ::CosTrading::IllegalServiceType _user_ex;
        _a >>= _user_ex;
        mico_throw( _user_ex );
      }
      if( !strcmp( _uuex->_except_repoid(), "IDL:omg.org/CosTrading/UnknownServiceType:1.0" ) ) {
        CORBA::Any &_a = _uuex->exception( ::CosTrading::_tc_UnknownServiceType );
        ::CosTrading::UnknownServiceType _user_ex;
        _a >>= _user_ex;
        mico_throw( _user_ex );
      }
      mico_throw( CORBA::UNKNOWN() );
    } else {
      mico_throw( *_ex );
    }
  }
  #else
  {
    CORBA::Exception *_ex;
    if( (_ex = _req->env()->exception()) )
      CORBA::Exception::_throw_failed( _ex );
  }
  #endif
  CosTradingRepos::ServiceTypeRepository::TypeStruct* _res = new ::CosTradingRepos::ServiceTypeRepository::TypeStruct;
  *_req->result()->value() >>= *_res;
  return _res;
}


void CosTradingRepos::ServiceTypeRepository_stub::mask_type( const char* name )
{
  CORBA::Request_var _req = this->_request( "mask_type" );
  _req->add_in_arg( "name" ) <<= CORBA::Any::from_string( (char *) name, 0 );
  _req->result()->value()->type( CORBA::_tc_void );
  _req->invoke();
  #ifdef HAVE_EXCEPTIONS
  if( _req->env()->exception() ) {
    CORBA::Exception *_ex = _req->env()->exception();
    CORBA::UnknownUserException *_uuex = CORBA::UnknownUserException::_narrow( _ex );
    if( _uuex ) {
      if( !strcmp( _uuex->_except_repoid(), "IDL:omg.org/CosTrading/IllegalServiceType:1.0" ) ) {
        CORBA::Any &_a = _uuex->exception( ::CosTrading::_tc_IllegalServiceType );
        ::CosTrading::IllegalServiceType _user_ex;
        _a >>= _user_ex;
        mico_throw( _user_ex );
      }
      if( !strcmp( _uuex->_except_repoid(), "IDL:omg.org/CosTrading/UnknownServiceType:1.0" ) ) {
        CORBA::Any &_a = _uuex->exception( ::CosTrading::_tc_UnknownServiceType );
        ::CosTrading::UnknownServiceType _user_ex;
        _a >>= _user_ex;
        mico_throw( _user_ex );
      }
      if( !strcmp( _uuex->_except_repoid(), "IDL:omg.org/CosTradingRepos/ServiceTypeRepository/AlreadyMasked:1.0" ) ) {
        CORBA::Any &_a = _uuex->exception( ::CosTradingRepos::ServiceTypeRepository::_tc_AlreadyMasked );
        ::CosTradingRepos::ServiceTypeRepository::AlreadyMasked _user_ex;
        _a >>= _user_ex;
        mico_throw( _user_ex );
      }
      mico_throw( CORBA::UNKNOWN() );
    } else {
      mico_throw( *_ex );
    }
  }
  #else
  {
    CORBA::Exception *_ex;
    if( (_ex = _req->env()->exception()) )
      CORBA::Exception::_throw_failed( _ex );
  }
  #endif
}


void CosTradingRepos::ServiceTypeRepository_stub::unmask_type( const char* name )
{
  CORBA::Request_var _req = this->_request( "unmask_type" );
  _req->add_in_arg( "name" ) <<= CORBA::Any::from_string( (char *) name, 0 );
  _req->result()->value()->type( CORBA::_tc_void );
  _req->invoke();
  #ifdef HAVE_EXCEPTIONS
  if( _req->env()->exception() ) {
    CORBA::Exception *_ex = _req->env()->exception();
    CORBA::UnknownUserException *_uuex = CORBA::UnknownUserException::_narrow( _ex );
    if( _uuex ) {
      if( !strcmp( _uuex->_except_repoid(), "IDL:omg.org/CosTrading/IllegalServiceType:1.0" ) ) {
        CORBA::Any &_a = _uuex->exception( ::CosTrading::_tc_IllegalServiceType );
        ::CosTrading::IllegalServiceType _user_ex;
        _a >>= _user_ex;
        mico_throw( _user_ex );
      }
      if( !strcmp( _uuex->_except_repoid(), "IDL:omg.org/CosTrading/UnknownServiceType:1.0" ) ) {
        CORBA::Any &_a = _uuex->exception( ::CosTrading::_tc_UnknownServiceType );
        ::CosTrading::UnknownServiceType _user_ex;
        _a >>= _user_ex;
        mico_throw( _user_ex );
      }
      if( !strcmp( _uuex->_except_repoid(), "IDL:omg.org/CosTradingRepos/ServiceTypeRepository/NotMasked:1.0" ) ) {
        CORBA::Any &_a = _uuex->exception( ::CosTradingRepos::ServiceTypeRepository::_tc_NotMasked );
        ::CosTradingRepos::ServiceTypeRepository::NotMasked _user_ex;
        _a >>= _user_ex;
        mico_throw( _user_ex );
      }
      mico_throw( CORBA::UNKNOWN() );
    } else {
      mico_throw( *_ex );
    }
  }
  #else
  {
    CORBA::Exception *_ex;
    if( (_ex = _req->env()->exception()) )
      CORBA::Exception::_throw_failed( _ex );
  }
  #endif
}


#ifdef HAVE_NAMESPACE
namespace CosTradingRepos { vector<CORBA::Narrow_proto> * ServiceTypeRepository::_narrow_helpers; };
#else
vector<CORBA::Narrow_proto> * CosTradingRepos::ServiceTypeRepository::_narrow_helpers;
#endif
#ifdef HAVE_NAMESPACE
namespace CosTradingRepos { CORBA::TypeCodeConst _tc_ServiceTypeRepository; };
#else
CORBA::TypeCodeConst CosTradingRepos::_tc_ServiceTypeRepository;
#endif

CORBA::Boolean
operator<<=( CORBA::Any &_a, const CosTradingRepos::ServiceTypeRepository_ptr _obj )
{
  return (_a <<= CORBA::Any::from_object( _obj, "ServiceTypeRepository" ));
}

CORBA::Boolean
operator>>=( const CORBA::Any &_a, CosTradingRepos::ServiceTypeRepository_ptr &_obj )
{
  CORBA::Object_ptr _o;
  if( !(_a >>= CORBA::Any::to_object( _o )) )
    return FALSE;
  if( CORBA::is_nil( _o ) ) {
    _obj = ::CosTradingRepos::ServiceTypeRepository::_nil();
    return TRUE;
  }
  _obj = ::CosTradingRepos::ServiceTypeRepository::_narrow( _o );
  CORBA::release( _o );
  return TRUE;
}

CORBA::Boolean operator<<=( CORBA::Any &_a, const SequenceTmpl<CosTradingRepos::ServiceTypeRepository::PropStruct> &_s )
{
  static CORBA::TypeCodeConst _tc =
    "0100000013000000f0010000010000000f000000e0010000010000004100"
    "000049444c3a6f6d672e6f72672f436f7354726164696e675265706f732f"
    "53657276696365547970655265706f7369746f72792f50726f7053747275"
    "63743a312e30000000000b00000050726f70537472756374000003000000"
    "050000006e616d6500000000150000008c00000001000000280000004944"
    "4c3a6f6d672e6f72672f436f7354726164696e672f50726f70657274794e"
    "616d653a312e30000d00000050726f70657274794e616d65000000001500"
    "000040000000010000002300000049444c3a6f6d672e6f72672f436f7354"
    "726164696e672f49737472696e673a312e3000000800000049737472696e"
    "670012000000000000000b00000076616c75655f7479706500000c000000"
    "050000006d6f64650000000011000000b800000001000000430000004944"
    "4c3a6f6d672e6f72672f436f7354726164696e675265706f732f53657276"
    "696365547970655265706f7369746f72792f50726f70657274794d6f6465"
    "3a312e3000000d00000050726f70657274794d6f64650000000004000000"
    "0c00000050524f505f4e4f524d414c000e00000050524f505f524541444f"
    "4e4c590000000f00000050524f505f4d414e4441544f5259000018000000"
    "50524f505f4d414e4441544f52595f524541444f4e4c590000000000";
  _a.type( _tc );
  if( !_a.seq_put_begin( _s.length() ) )
    return FALSE;
  for( CORBA::ULong _i = 0; _i < _s.length(); _i++ )
    if( !(_a <<= ((SequenceTmpl<CosTradingRepos::ServiceTypeRepository::PropStruct>&)_s)[ _i ]) )
      return FALSE;
  return _a.seq_put_end();
}

CORBA::Boolean operator>>=( const CORBA::Any &_a, SequenceTmpl<CosTradingRepos::ServiceTypeRepository::PropStruct> &_s )
{
  CORBA::ULong _len;

  if( !_a.seq_get_begin( _len ) )
    return FALSE;
  _s.length( _len );
  for( CORBA::ULong _i = 0; _i < _len; _i++ )
    if( !(_a >>= _s[ _i ]) )
      return FALSE;
  return _a.seq_get_end();
}


CORBA::Boolean operator<<=( CORBA::Any &_a, const SequenceTmpl<CosTradingRepos::ServiceTypeRepository::Property> &_s )
{
  static CORBA::TypeCodeConst _tc =
    "0100000013000000e0010000010000000f000000d0010000010000003f00"
    "000049444c3a6f6d672e6f72672f436f7354726164696e675265706f732f"
    "53657276696365547970655265706f7369746f72792f50726f7065727479"
    "3a312e3000000900000050726f706572747900000000040000000b000000"
    "76616c75655f7479706500000c0000000800000069735f66696c65000800"
    "0000050000006e616d650000000015000000c40000000100000043000000"
    "49444c3a6f6d672e6f72672f436f7354726164696e675265706f732f5365"
    "7276696365547970655265706f7369746f72792f50726f70657274794e61"
    "6d653a312e3000000d00000050726f70657274794e616d65000000001500"
    "00005c000000010000003e00000049444c3a6f6d672e6f72672f436f7354"
    "726164696e675265706f732f53657276696365547970655265706f736974"
    "6f72792f49737472696e673a312e300000000800000049737472696e6700"
    "12000000000000000600000076616c756500000015000000640000000100"
    "00004400000049444c3a6f6d672e6f72672f436f7354726164696e675265"
    "706f732f53657276696365547970655265706f7369746f72792f50726f70"
    "6572747956616c75653a312e30000e00000050726f706572747956616c75"
    "650000000b00000000000000";
  _a.type( _tc );
  if( !_a.seq_put_begin( _s.length() ) )
    return FALSE;
  for( CORBA::ULong _i = 0; _i < _s.length(); _i++ )
    if( !(_a <<= ((SequenceTmpl<CosTradingRepos::ServiceTypeRepository::Property>&)_s)[ _i ]) )
      return FALSE;
  return _a.seq_put_end();
}

CORBA::Boolean operator>>=( const CORBA::Any &_a, SequenceTmpl<CosTradingRepos::ServiceTypeRepository::Property> &_s )
{
  CORBA::ULong _len;

  if( !_a.seq_get_begin( _len ) )
    return FALSE;
  _s.length( _len );
  for( CORBA::ULong _i = 0; _i < _len; _i++ )
    if( !(_a >>= _s[ _i ]) )
      return FALSE;
  return _a.seq_get_end();
}


struct __tc_init_TYPEREPO {
  __tc_init_TYPEREPO()
  {
    CosTradingRepos::ServiceTypeRepository::_tc_ServiceTypeNameSeq = "010000001500000014010000010000004900000049444c3a6f6d672e6f72"
    "672f436f7354726164696e675265706f732f536572766963655479706552"
    "65706f7369746f72792f53657276696365547970654e616d655365713a31"
    "2e30000000001300000053657276696365547970654e616d655365710000"
    "13000000a0000000010000001500000090000000010000002b0000004944"
    "4c3a6f6d672e6f72672f436f7354726164696e672f536572766963655479"
    "70654e616d653a312e3000001000000053657276696365547970654e616d"
    "65001500000040000000010000002300000049444c3a6f6d672e6f72672f"
    "436f7354726164696e672f49737472696e673a312e300000080000004973"
    "7472696e6700120000000000000000000000";
    CosTradingRepos::ServiceTypeRepository::_tc_PropertyMode = "0100000011000000b8000000010000004300000049444c3a6f6d672e6f72"
    "672f436f7354726164696e675265706f732f536572766963655479706552"
    "65706f7369746f72792f50726f70657274794d6f64653a312e3000000d00"
    "000050726f70657274794d6f646500000000040000000c00000050524f50"
    "5f4e4f524d414c000e00000050524f505f524541444f4e4c590000000f00"
    "000050524f505f4d414e4441544f525900001800000050524f505f4d414e"
    "4441544f52595f524541444f4e4c5900";
    CosTradingRepos::ServiceTypeRepository::_tc_PropStruct = "010000000f000000e0010000010000004100000049444c3a6f6d672e6f72"
    "672f436f7354726164696e675265706f732f536572766963655479706552"
    "65706f7369746f72792f50726f705374727563743a312e30000000000b00"
    "000050726f70537472756374000003000000050000006e616d6500000000"
    "150000008c000000010000002800000049444c3a6f6d672e6f72672f436f"
    "7354726164696e672f50726f70657274794e616d653a312e30000d000000"
    "50726f70657274794e616d65000000001500000040000000010000002300"
    "000049444c3a6f6d672e6f72672f436f7354726164696e672f4973747269"
    "6e673a312e3000000800000049737472696e670012000000000000000b00"
    "000076616c75655f7479706500000c000000050000006d6f646500000000"
    "11000000b8000000010000004300000049444c3a6f6d672e6f72672f436f"
    "7354726164696e675265706f732f53657276696365547970655265706f73"
    "69746f72792f50726f70657274794d6f64653a312e3000000d0000005072"
    "6f70657274794d6f646500000000040000000c00000050524f505f4e4f52"
    "4d414c000e00000050524f505f524541444f4e4c590000000f0000005052"
    "4f505f4d414e4441544f525900001800000050524f505f4d414e4441544f"
    "52595f524541444f4e4c5900";
    CosTradingRepos::ServiceTypeRepository::_tc_PropStructSeq = "010000001500000058020000010000004400000049444c3a6f6d672e6f72"
    "672f436f7354726164696e675265706f732f536572766963655479706552"
    "65706f7369746f72792f50726f705374727563745365713a312e30000e00"
    "000050726f7053747275637453657100000013000000f001000001000000"
    "0f000000e0010000010000004100000049444c3a6f6d672e6f72672f436f"
    "7354726164696e675265706f732f53657276696365547970655265706f73"
    "69746f72792f50726f705374727563743a312e30000000000b0000005072"
    "6f70537472756374000003000000050000006e616d650000000015000000"
    "8c000000010000002800000049444c3a6f6d672e6f72672f436f73547261"
    "64696e672f50726f70657274794e616d653a312e30000d00000050726f70"
    "657274794e616d6500000000150000004000000001000000230000004944"
    "4c3a6f6d672e6f72672f436f7354726164696e672f49737472696e673a31"
    "2e3000000800000049737472696e670012000000000000000b0000007661"
    "6c75655f7479706500000c000000050000006d6f64650000000011000000"
    "b8000000010000004300000049444c3a6f6d672e6f72672f436f73547261"
    "64696e675265706f732f53657276696365547970655265706f7369746f72"
    "792f50726f70657274794d6f64653a312e3000000d00000050726f706572"
    "74794d6f646500000000040000000c00000050524f505f4e4f524d414c00"
    "0e00000050524f505f524541444f4e4c590000000f00000050524f505f4d"
    "414e4441544f525900001800000050524f505f4d414e4441544f52595f52"
    "4541444f4e4c590000000000";
    CosTradingRepos::ServiceTypeRepository::_tc_Istring = "01000000150000005c000000010000003e00000049444c3a6f6d672e6f72"
    "672f436f7354726164696e675265706f732f536572766963655479706552"
    "65706f7369746f72792f49737472696e673a312e30000000080000004973"
    "7472696e67001200000000000000";
    CosTradingRepos::ServiceTypeRepository::_tc_PropertyName = "0100000015000000c4000000010000004300000049444c3a6f6d672e6f72"
    "672f436f7354726164696e675265706f732f536572766963655479706552"
    "65706f7369746f72792f50726f70657274794e616d653a312e3000000d00"
    "000050726f70657274794e616d6500000000150000005c00000001000000"
    "3e00000049444c3a6f6d672e6f72672f436f7354726164696e675265706f"
    "732f53657276696365547970655265706f7369746f72792f49737472696e"
    "673a312e300000000800000049737472696e67001200000000000000";
    CosTradingRepos::ServiceTypeRepository::_tc_PropertyValue = "010000001500000064000000010000004400000049444c3a6f6d672e6f72"
    "672f436f7354726164696e675265706f732f536572766963655479706552"
    "65706f7369746f72792f50726f706572747956616c75653a312e30000e00"
    "000050726f706572747956616c75650000000b000000";
    CosTradingRepos::ServiceTypeRepository::_tc_Property = "010000000f000000d0010000010000003f00000049444c3a6f6d672e6f72"
    "672f436f7354726164696e675265706f732f536572766963655479706552"
    "65706f7369746f72792f50726f70657274793a312e300000090000005072"
    "6f706572747900000000040000000b00000076616c75655f747970650000"
    "0c0000000800000069735f66696c650008000000050000006e616d650000"
    "000015000000c4000000010000004300000049444c3a6f6d672e6f72672f"
    "436f7354726164696e675265706f732f5365727669636554797065526570"
    "6f7369746f72792f50726f70657274794e616d653a312e3000000d000000"
    "50726f70657274794e616d6500000000150000005c000000010000003e00"
    "000049444c3a6f6d672e6f72672f436f7354726164696e675265706f732f"
    "53657276696365547970655265706f7369746f72792f49737472696e673a"
    "312e300000000800000049737472696e6700120000000000000006000000"
    "76616c75650000001500000064000000010000004400000049444c3a6f6d"
    "672e6f72672f436f7354726164696e675265706f732f5365727669636554"
    "7970655265706f7369746f72792f50726f706572747956616c75653a312e"
    "30000e00000050726f706572747956616c75650000000b000000";
    CosTradingRepos::ServiceTypeRepository::_tc_PropertySeq = "010000001500000044020000010000004200000049444c3a6f6d672e6f72"
    "672f436f7354726164696e675265706f732f536572766963655479706552"
    "65706f7369746f72792f50726f70657274795365713a312e300000000c00"
    "000050726f70657274795365710013000000e0010000010000000f000000"
    "d0010000010000003f00000049444c3a6f6d672e6f72672f436f73547261"
    "64696e675265706f732f53657276696365547970655265706f7369746f72"
    "792f50726f70657274793a312e3000000900000050726f70657274790000"
    "0000040000000b00000076616c75655f7479706500000c00000008000000"
    "69735f66696c650008000000050000006e616d650000000015000000c400"
    "0000010000004300000049444c3a6f6d672e6f72672f436f735472616469"
    "6e675265706f732f53657276696365547970655265706f7369746f72792f"
    "50726f70657274794e616d653a312e3000000d00000050726f7065727479"
    "4e616d6500000000150000005c000000010000003e00000049444c3a6f6d"
    "672e6f72672f436f7354726164696e675265706f732f5365727669636554"
    "7970655265706f7369746f72792f49737472696e673a312e300000000800"
    "000049737472696e670012000000000000000600000076616c7565000000"
    "1500000064000000010000004400000049444c3a6f6d672e6f72672f436f"
    "7354726164696e675265706f732f53657276696365547970655265706f73"
    "69746f72792f50726f706572747956616c75653a312e30000e0000005072"
    "6f706572747956616c75650000000b00000000000000";
    CosTradingRepos::ServiceTypeRepository::_tc_Identifier = "0100000015000000a4000000010000004100000049444c3a6f6d672e6f72"
    "672f436f7354726164696e675265706f732f536572766963655479706552"
    "65706f7369746f72792f4964656e7469666965723a312e30000000000b00"
    "00004964656e746966696572000015000000400000000100000023000000"
    "49444c3a6f6d672e6f72672f436f7354726164696e672f49737472696e67"
    "3a312e3000000800000049737472696e67001200000000000000";
    CosTradingRepos::ServiceTypeRepository::_tc_IncarnationNumber = "010000000f00000088000000010000004800000049444c3a6f6d672e6f72"
    "672f436f7354726164696e675265706f732f536572766963655479706552"
    "65706f7369746f72792f496e6361726e6174696f6e4e756d6265723a312e"
    "300012000000496e6361726e6174696f6e4e756d62657200000002000000"
    "05000000686967680000000005000000040000006c6f770005000000";
    CosTradingRepos::ServiceTypeRepository::_tc_TypeStruct = "010000000f000000b8070000010000004100000049444c3a6f6d672e6f72"
    "672f436f7354726164696e675265706f732f536572766963655479706552"
    "65706f7369746f72792f547970655374727563743a312e30000000000b00"
    "0000547970655374727563740000060000000800000069665f6e616d6500"
    "15000000a4000000010000004100000049444c3a6f6d672e6f72672f436f"
    "7354726164696e675265706f732f53657276696365547970655265706f73"
    "69746f72792f4964656e7469666965723a312e30000000000b0000004964"
    "656e74696669657200001500000040000000010000002300000049444c3a"
    "6f6d672e6f72672f436f7354726164696e672f49737472696e673a312e30"
    "00000800000049737472696e670012000000000000000600000070726f70"
    "730000001500000058020000010000004400000049444c3a6f6d672e6f72"
    "672f436f7354726164696e675265706f732f536572766963655479706552"
    "65706f7369746f72792f50726f705374727563745365713a312e30000e00"
    "000050726f7053747275637453657100000013000000f001000001000000"
    "0f000000e0010000010000004100000049444c3a6f6d672e6f72672f436f"
    "7354726164696e675265706f732f53657276696365547970655265706f73"
    "69746f72792f50726f705374727563743a312e30000000000b0000005072"
    "6f70537472756374000003000000050000006e616d650000000015000000"
    "8c000000010000002800000049444c3a6f6d672e6f72672f436f73547261"
    "64696e672f50726f70657274794e616d653a312e30000d00000050726f70"
    "657274794e616d6500000000150000004000000001000000230000004944"
    "4c3a6f6d672e6f72672f436f7354726164696e672f49737472696e673a31"
    "2e3000000800000049737472696e670012000000000000000b0000007661"
    "6c75655f7479706500000c000000050000006d6f64650000000011000000"
    "b8000000010000004300000049444c3a6f6d672e6f72672f436f73547261"
    "64696e675265706f732f53657276696365547970655265706f7369746f72"
    "792f50726f70657274794d6f64653a312e3000000d00000050726f706572"
    "74794d6f646500000000040000000c00000050524f505f4e4f524d414c00"
    "0e00000050524f505f524541444f4e4c590000000f00000050524f505f4d"
    "414e4441544f525900001800000050524f505f4d414e4441544f52595f52"
    "4541444f4e4c5900000000000c00000073757065725f7479706573001500"
    "000014010000010000004900000049444c3a6f6d672e6f72672f436f7354"
    "726164696e675265706f732f53657276696365547970655265706f736974"
    "6f72792f53657276696365547970654e616d655365713a312e3000000000"
    "1300000053657276696365547970654e616d65536571000013000000a000"
    "0000010000001500000090000000010000002b00000049444c3a6f6d672e"
    "6f72672f436f7354726164696e672f53657276696365547970654e616d65"
    "3a312e3000001000000053657276696365547970654e616d650015000000"
    "40000000010000002300000049444c3a6f6d672e6f72672f436f73547261"
    "64696e672f49737472696e673a312e3000000800000049737472696e6700"
    "1200000000000000000000000700000076616c7565730000150000004402"
    "0000010000004200000049444c3a6f6d672e6f72672f436f735472616469"
    "6e675265706f732f53657276696365547970655265706f7369746f72792f"
    "50726f70657274795365713a312e300000000c00000050726f7065727479"
    "5365710013000000e0010000010000000f000000d0010000010000003f00"
    "000049444c3a6f6d672e6f72672f436f7354726164696e675265706f732f"
    "53657276696365547970655265706f7369746f72792f50726f7065727479"
    "3a312e3000000900000050726f706572747900000000040000000b000000"
    "76616c75655f7479706500000c0000000800000069735f66696c65000800"
    "0000050000006e616d650000000015000000c40000000100000043000000"
    "49444c3a6f6d672e6f72672f436f7354726164696e675265706f732f5365"
    "7276696365547970655265706f7369746f72792f50726f70657274794e61"
    "6d653a312e3000000d00000050726f70657274794e616d65000000001500"
    "00005c000000010000003e00000049444c3a6f6d672e6f72672f436f7354"
    "726164696e675265706f732f53657276696365547970655265706f736974"
    "6f72792f49737472696e673a312e300000000800000049737472696e6700"
    "12000000000000000600000076616c756500000015000000640000000100"
    "00004400000049444c3a6f6d672e6f72672f436f7354726164696e675265"
    "706f732f53657276696365547970655265706f7369746f72792f50726f70"
    "6572747956616c75653a312e30000e00000050726f706572747956616c75"
    "650000000b00000000000000070000006d61736b65640000080000000c00"
    "0000696e6361726e6174696f6e000f000000880000000100000048000000"
    "49444c3a6f6d672e6f72672f436f7354726164696e675265706f732f5365"
    "7276696365547970655265706f7369746f72792f496e6361726e6174696f"
    "6e4e756d6265723a312e300012000000496e6361726e6174696f6e4e756d"
    "626572000000020000000500000068696768000000000500000004000000"
    "6c6f770005000000";
    CosTradingRepos::ServiceTypeRepository::_tc_ListOption = "010000001100000072000000010000004100000049444c3a6f6d672e6f72"
    "672f436f7354726164696e675265706f732f536572766963655479706552"
    "65706f7369746f72792f4c6973744f7074696f6e3a312e30000000000b00"
    "00004c6973744f7074696f6e00000200000004000000616c6c0006000000"
    "73696e636500";
    CosTradingRepos::ServiceTypeRepository::_tc_SpecifiedServiceTypes = "010000001000000098010000010000004c00000049444c3a6f6d672e6f72"
    "672f436f7354726164696e675265706f732f536572766963655479706552"
    "65706f7369746f72792f5370656369666965645365727669636554797065"
    "733a312e3000160000005370656369666965645365727669636554797065"
    "730000001100000072000000010000004100000049444c3a6f6d672e6f72"
    "672f436f7354726164696e675265706f732f536572766963655479706552"
    "65706f7369746f72792f4c6973744f7074696f6e3a312e30000000000b00"
    "00004c6973744f7074696f6e00000200000004000000616c6c0006000000"
    "73696e6365000000ffffffff01000000010000000c000000696e6361726e"
    "6174696f6e000f00000088000000010000004800000049444c3a6f6d672e"
    "6f72672f436f7354726164696e675265706f732f53657276696365547970"
    "655265706f7369746f72792f496e6361726e6174696f6e4e756d6265723a"
    "312e300012000000496e6361726e6174696f6e4e756d6265720000000200"
    "000005000000686967680000000005000000040000006c6f770005000000"
    ;
    CosTradingRepos::ServiceTypeRepository::_tc_ServiceTypeExists = "010000001600000010010000010000004800000049444c3a6f6d672e6f72"
    "672f436f7354726164696e675265706f732f536572766963655479706552"
    "65706f7369746f72792f53657276696365547970654578697374733a312e"
    "300012000000536572766963655479706545786973747300000001000000"
    "050000006e616d65000000001500000090000000010000002b0000004944"
    "4c3a6f6d672e6f72672f436f7354726164696e672f536572766963655479"
    "70654e616d653a312e3000001000000053657276696365547970654e616d"
    "65001500000040000000010000002300000049444c3a6f6d672e6f72672f"
    "436f7354726164696e672f49737472696e673a312e300000080000004973"
    "7472696e67001200000000000000";
    CosTradingRepos::ServiceTypeRepository::_tc_InterfaceTypeMismatch = "010000001600000040030000010000004c00000049444c3a6f6d672e6f72"
    "672f436f7354726164696e675265706f732f536572766963655479706552"
    "65706f7369746f72792f496e74657266616365547970654d69736d617463"
    "683a312e300016000000496e74657266616365547970654d69736d617463"
    "68000000040000000d000000626173655f73657276696365000000001500"
    "000090000000010000002b00000049444c3a6f6d672e6f72672f436f7354"
    "726164696e672f53657276696365547970654e616d653a312e3000001000"
    "000053657276696365547970654e616d6500150000004000000001000000"
    "2300000049444c3a6f6d672e6f72672f436f7354726164696e672f497374"
    "72696e673a312e3000000800000049737472696e67001200000000000000"
    "08000000626173655f69660015000000a400000001000000410000004944"
    "4c3a6f6d672e6f72672f436f7354726164696e675265706f732f53657276"
    "696365547970655265706f7369746f72792f4964656e7469666965723a31"
    "2e30000000000b0000004964656e74696669657200001500000040000000"
    "010000002300000049444c3a6f6d672e6f72672f436f7354726164696e67"
    "2f49737472696e673a312e3000000800000049737472696e670012000000"
    "0000000010000000646572697665645f7365727669636500150000009000"
    "0000010000002b00000049444c3a6f6d672e6f72672f436f735472616469"
    "6e672f53657276696365547970654e616d653a312e300000100000005365"
    "7276696365547970654e616d650015000000400000000100000023000000"
    "49444c3a6f6d672e6f72672f436f7354726164696e672f49737472696e67"
    "3a312e3000000800000049737472696e670012000000000000000b000000"
    "646572697665645f6966000015000000a400000001000000410000004944"
    "4c3a6f6d672e6f72672f436f7354726164696e675265706f732f53657276"
    "696365547970655265706f7369746f72792f4964656e7469666965723a31"
    "2e30000000000b0000004964656e74696669657200001500000040000000"
    "010000002300000049444c3a6f6d672e6f72672f436f7354726164696e67"
    "2f49737472696e673a312e3000000800000049737472696e670012000000"
    "00000000";
    CosTradingRepos::ServiceTypeRepository::_tc_HasSubTypes = "0100000016000000b0010000010000004200000049444c3a6f6d672e6f72"
    "672f436f7354726164696e675265706f732f536572766963655479706552"
    "65706f7369746f72792f48617353756254797065733a312e300000000c00"
    "000048617353756254797065730002000000090000007468655f74797065"
    "000000001500000090000000010000002b00000049444c3a6f6d672e6f72"
    "672f436f7354726164696e672f53657276696365547970654e616d653a31"
    "2e3000001000000053657276696365547970654e616d6500150000004000"
    "0000010000002300000049444c3a6f6d672e6f72672f436f735472616469"
    "6e672f49737472696e673a312e3000000800000049737472696e67001200"
    "000000000000090000007375625f74797065000000001500000090000000"
    "010000002b00000049444c3a6f6d672e6f72672f436f7354726164696e67"
    "2f53657276696365547970654e616d653a312e3000001000000053657276"
    "696365547970654e616d6500150000004000000001000000230000004944"
    "4c3a6f6d672e6f72672f436f7354726164696e672f49737472696e673a31"
    "2e3000000800000049737472696e67001200000000000000";
    CosTradingRepos::ServiceTypeRepository::_tc_AlreadyMasked = "010000001600000008010000010000004400000049444c3a6f6d672e6f72"
    "672f436f7354726164696e675265706f732f536572766963655479706552"
    "65706f7369746f72792f416c72656164794d61736b65643a312e30000e00"
    "0000416c72656164794d61736b656400000001000000050000006e616d65"
    "000000001500000090000000010000002b00000049444c3a6f6d672e6f72"
    "672f436f7354726164696e672f53657276696365547970654e616d653a31"
    "2e3000001000000053657276696365547970654e616d6500150000004000"
    "0000010000002300000049444c3a6f6d672e6f72672f436f735472616469"
    "6e672f49737472696e673a312e3000000800000049737472696e67001200"
    "000000000000";
    CosTradingRepos::ServiceTypeRepository::_tc_NotMasked = "010000001600000000010000010000004000000049444c3a6f6d672e6f72"
    "672f436f7354726164696e675265706f732f536572766963655479706552"
    "65706f7369746f72792f4e6f744d61736b65643a312e30000a0000004e6f"
    "744d61736b656400000001000000050000006e616d650000000015000000"
    "90000000010000002b00000049444c3a6f6d672e6f72672f436f73547261"
    "64696e672f53657276696365547970654e616d653a312e30000010000000"
    "53657276696365547970654e616d65001500000040000000010000002300"
    "000049444c3a6f6d672e6f72672f436f7354726164696e672f4973747269"
    "6e673a312e3000000800000049737472696e67001200000000000000";
    CosTradingRepos::ServiceTypeRepository::_tc_ValueTypeRedefinition = "0100000016000000b4050000010000004c00000049444c3a6f6d672e6f72"
    "672f436f7354726164696e675265706f732f536572766963655479706552"
    "65706f7369746f72792f56616c7565547970655265646566696e6974696f"
    "6e3a312e30001600000056616c7565547970655265646566696e6974696f"
    "6e0000000400000007000000747970655f31000015000000900000000100"
    "00002b00000049444c3a6f6d672e6f72672f436f7354726164696e672f53"
    "657276696365547970654e616d653a312e30000010000000536572766963"
    "65547970654e616d65001500000040000000010000002300000049444c3a"
    "6f6d672e6f72672f436f7354726164696e672f49737472696e673a312e30"
    "00000800000049737472696e670012000000000000000d00000064656669"
    "6e6974696f6e5f31000000000f000000e001000001000000410000004944"
    "4c3a6f6d672e6f72672f436f7354726164696e675265706f732f53657276"
    "696365547970655265706f7369746f72792f50726f705374727563743a31"
    "2e30000000000b00000050726f7053747275637400000300000005000000"
    "6e616d6500000000150000008c000000010000002800000049444c3a6f6d"
    "672e6f72672f436f7354726164696e672f50726f70657274794e616d653a"
    "312e30000d00000050726f70657274794e616d6500000000150000004000"
    "0000010000002300000049444c3a6f6d672e6f72672f436f735472616469"
    "6e672f49737472696e673a312e3000000800000049737472696e67001200"
    "0000000000000b00000076616c75655f7479706500000c00000005000000"
    "6d6f64650000000011000000b8000000010000004300000049444c3a6f6d"
    "672e6f72672f436f7354726164696e675265706f732f5365727669636554"
    "7970655265706f7369746f72792f50726f70657274794d6f64653a312e30"
    "00000d00000050726f70657274794d6f646500000000040000000c000000"
    "50524f505f4e4f524d414c000e00000050524f505f524541444f4e4c5900"
    "00000f00000050524f505f4d414e4441544f525900001800000050524f50"
    "5f4d414e4441544f52595f524541444f4e4c590007000000747970655f32"
    "00001500000090000000010000002b00000049444c3a6f6d672e6f72672f"
    "436f7354726164696e672f53657276696365547970654e616d653a312e30"
    "00001000000053657276696365547970654e616d65001500000040000000"
    "010000002300000049444c3a6f6d672e6f72672f436f7354726164696e67"
    "2f49737472696e673a312e3000000800000049737472696e670012000000"
    "000000000d000000646566696e6974696f6e5f32000000000f000000e001"
    "0000010000004100000049444c3a6f6d672e6f72672f436f735472616469"
    "6e675265706f732f53657276696365547970655265706f7369746f72792f"
    "50726f705374727563743a312e30000000000b00000050726f7053747275"
    "6374000003000000050000006e616d6500000000150000008c0000000100"
    "00002800000049444c3a6f6d672e6f72672f436f7354726164696e672f50"
    "726f70657274794e616d653a312e30000d00000050726f70657274794e61"
    "6d65000000001500000040000000010000002300000049444c3a6f6d672e"
    "6f72672f436f7354726164696e672f49737472696e673a312e3000000800"
    "000049737472696e670012000000000000000b00000076616c75655f7479"
    "706500000c000000050000006d6f64650000000011000000b80000000100"
    "00004300000049444c3a6f6d672e6f72672f436f7354726164696e675265"
    "706f732f53657276696365547970655265706f7369746f72792f50726f70"
    "657274794d6f64653a312e3000000d00000050726f70657274794d6f6465"
    "00000000040000000c00000050524f505f4e4f524d414c000e0000005052"
    "4f505f524541444f4e4c590000000f00000050524f505f4d414e4441544f"
    "525900001800000050524f505f4d414e4441544f52595f524541444f4e4c"
    "5900";
    CosTradingRepos::ServiceTypeRepository::_tc_DuplicateServiceTypeName = "010000001600000020010000010000004f00000049444c3a6f6d672e6f72"
    "672f436f7354726164696e675265706f732f536572766963655479706552"
    "65706f7369746f72792f4475706c69636174655365727669636554797065"
    "4e616d653a312e300000190000004475706c696361746553657276696365"
    "547970654e616d650000000001000000050000006e616d65000000001500"
    "000090000000010000002b00000049444c3a6f6d672e6f72672f436f7354"
    "726164696e672f53657276696365547970654e616d653a312e3000001000"
    "000053657276696365547970654e616d6500150000004000000001000000"
    "2300000049444c3a6f6d672e6f72672f436f7354726164696e672f497374"
    "72696e673a312e3000000800000049737472696e67001200000000000000"
    ;
    CosTradingRepos::_tc_ServiceTypeRepository = "010000000e0000005a000000010000003600000049444c3a6f6d672e6f72"
    "672f436f7354726164696e675265706f732f536572766963655479706552"
    "65706f7369746f72793a312e300000001600000053657276696365547970"
    "655265706f7369746f727900";
  }
};

static __tc_init_TYPEREPO __init_TYPEREPO;

//--------------------------------------------------------
//  Implementation of skeletons
//--------------------------------------------------------

// Dynamic Implementation Routine for interface ServiceTypeRepository
CosTradingRepos::ServiceTypeRepository_skel::ServiceTypeRepository_skel( const CORBA::BOA::ReferenceData &_id )
{
  CORBA::ImplementationDef_var _impl =
    _find_impl( "IDL:omg.org/CosTradingRepos/ServiceTypeRepository:1.0", "ServiceTypeRepository" );
  assert( !CORBA::is_nil( _impl ) );
  _create_ref( _id,
    CORBA::InterfaceDef::_nil(),
    _impl,
    "IDL:omg.org/CosTradingRepos/ServiceTypeRepository:1.0" );
  register_dispatcher( new InterfaceDispatcherWrapper<ServiceTypeRepository_skel>( this ) );
}

CosTradingRepos::ServiceTypeRepository_skel::ServiceTypeRepository_skel( CORBA::Object_ptr _obj )
{
  CORBA::ImplementationDef_var _impl =
    _find_impl( "IDL:omg.org/CosTradingRepos/ServiceTypeRepository:1.0", "ServiceTypeRepository" );
  assert( !CORBA::is_nil( _impl ) );
  _restore_ref( _obj,
    CORBA::BOA::ReferenceData(),
    CORBA::InterfaceDef::_nil(),
    _impl );
  register_dispatcher( new InterfaceDispatcherWrapper<ServiceTypeRepository_skel>( this ) );
}

CosTradingRepos::ServiceTypeRepository_skel::~ServiceTypeRepository_skel()
{
}

bool CosTradingRepos::ServiceTypeRepository_skel::dispatch( CORBA::ServerRequest_ptr _req, CORBA::Environment & /*_env*/ )
{
  if( strcmp( _req->op_name(), "_get_incarnation" ) == 0 ) {
    CORBA::NVList_ptr _args = new CORBA::NVList (0);

    if (!_req->params( _args ))
      return true;

    IncarnationNumber _res;
    #ifdef HAVE_EXCEPTIONS
    try {
    #endif
      _res = incarnation();
    #ifdef HAVE_EXCEPTIONS
    } catch( CORBA::SystemException_var &_ex ) {
      _req->exception( _ex->_clone() );
      return true;
    } catch( ... ) {
      assert( 0 );
      return true;
    }
    #endif
    CORBA::Any *_any_res = new CORBA::Any;
    *_any_res <<= _res;
    _req->result( _any_res );
    return true;
  }
  if( strcmp( _req->op_name(), "add_type" ) == 0 ) {
    CosTrading::ServiceTypeName_var name;
    Identifier_var if_name;
    PropStructSeq props;
    ServiceTypeNameSeq super_types;
    PropertySeq values;

    CORBA::NVList_ptr _args = new CORBA::NVList (5);
    _args->add( CORBA::ARG_IN );
    _args->item( 0 )->value()->type( CosTrading::_tc_ServiceTypeName );
    _args->add( CORBA::ARG_IN );
    _args->item( 1 )->value()->type( CosTradingRepos::ServiceTypeRepository::_tc_Identifier );
    _args->add( CORBA::ARG_IN );
    _args->item( 2 )->value()->type( CosTradingRepos::ServiceTypeRepository::_tc_PropStructSeq );
    _args->add( CORBA::ARG_IN );
    _args->item( 3 )->value()->type( CosTradingRepos::ServiceTypeRepository::_tc_ServiceTypeNameSeq );
    _args->add( CORBA::ARG_IN );
    _args->item( 4 )->value()->type( CosTradingRepos::ServiceTypeRepository::_tc_PropertySeq );

    if (!_req->params( _args ))
      return true;

    *_args->item( 0 )->value() >>= CORBA::Any::to_string( name, 0 );
    *_args->item( 1 )->value() >>= CORBA::Any::to_string( if_name, 0 );
    *_args->item( 2 )->value() >>= props;
    *_args->item( 3 )->value() >>= super_types;
    *_args->item( 4 )->value() >>= values;
    IncarnationNumber _res;
    #ifdef HAVE_EXCEPTIONS
    try {
    #endif
      _res = add_type( name, if_name, props, super_types, values );
    #ifdef HAVE_EXCEPTIONS
    } catch( ::CosTrading::IllegalServiceType_var &_ex ) {
      _req->exception( _ex->_clone() );
      return true;
    } catch( ::CosTradingRepos::ServiceTypeRepository::ServiceTypeExists_var &_ex ) {
      _req->exception( _ex->_clone() );
      return true;
    } catch( ::CosTradingRepos::ServiceTypeRepository::InterfaceTypeMismatch_var &_ex ) {
      _req->exception( _ex->_clone() );
      return true;
    } catch( ::CosTrading::IllegalPropertyName_var &_ex ) {
      _req->exception( _ex->_clone() );
      return true;
    } catch( ::CosTrading::DuplicatePropertyName_var &_ex ) {
      _req->exception( _ex->_clone() );
      return true;
    } catch( ::CosTradingRepos::ServiceTypeRepository::ValueTypeRedefinition_var &_ex ) {
      _req->exception( _ex->_clone() );
      return true;
    } catch( ::CosTrading::UnknownServiceType_var &_ex ) {
      _req->exception( _ex->_clone() );
      return true;
    } catch( ::CosTradingRepos::ServiceTypeRepository::DuplicateServiceTypeName_var &_ex ) {
      _req->exception( _ex->_clone() );
      return true;
    } catch( CORBA::SystemException_var &_ex ) {
      _req->exception( _ex->_clone() );
      return true;
    } catch( ... ) {
      assert( 0 );
      return true;
    }
    #endif
    CORBA::Any *_any_res = new CORBA::Any;
    *_any_res <<= _res;
    _req->result( _any_res );
    return true;
  }
  if( strcmp( _req->op_name(), "remove_type" ) == 0 ) {
    CosTrading::ServiceTypeName_var name;

    CORBA::NVList_ptr _args = new CORBA::NVList (1);
    _args->add( CORBA::ARG_IN );
    _args->item( 0 )->value()->type( CosTrading::_tc_ServiceTypeName );

    if (!_req->params( _args ))
      return true;

    *_args->item( 0 )->value() >>= CORBA::Any::to_string( name, 0 );
    #ifdef HAVE_EXCEPTIONS
    try {
    #endif
      remove_type( name );
    #ifdef HAVE_EXCEPTIONS
    } catch( ::CosTrading::IllegalServiceType_var &_ex ) {
      _req->exception( _ex->_clone() );
      return true;
    } catch( ::CosTrading::UnknownServiceType_var &_ex ) {
      _req->exception( _ex->_clone() );
      return true;
    } catch( ::CosTradingRepos::ServiceTypeRepository::HasSubTypes_var &_ex ) {
      _req->exception( _ex->_clone() );
      return true;
    } catch( CORBA::SystemException_var &_ex ) {
      _req->exception( _ex->_clone() );
      return true;
    } catch( ... ) {
      assert( 0 );
      return true;
    }
    #endif
    return true;
  }
  if( strcmp( _req->op_name(), "list_types" ) == 0 ) {
    SpecifiedServiceTypes which_types;

    CORBA::NVList_ptr _args = new CORBA::NVList (1);
    _args->add( CORBA::ARG_IN );
    _args->item( 0 )->value()->type( CosTradingRepos::ServiceTypeRepository::_tc_SpecifiedServiceTypes );

    if (!_req->params( _args ))
      return true;

    *_args->item( 0 )->value() >>= which_types;
    ServiceTypeNameSeq* _res;
    #ifdef HAVE_EXCEPTIONS
    try {
    #endif
      _res = list_types( which_types );
    #ifdef HAVE_EXCEPTIONS
    } catch( CORBA::SystemException_var &_ex ) {
      _req->exception( _ex->_clone() );
      return true;
    } catch( ... ) {
      assert( 0 );
      return true;
    }
    #endif
    CORBA::Any *_any_res = new CORBA::Any;
    *_any_res <<= *_res;
    delete _res;
    _req->result( _any_res );
    return true;
  }
  if( strcmp( _req->op_name(), "describe_type" ) == 0 ) {
    CosTrading::ServiceTypeName_var name;

    CORBA::NVList_ptr _args = new CORBA::NVList (1);
    _args->add( CORBA::ARG_IN );
    _args->item( 0 )->value()->type( CosTrading::_tc_ServiceTypeName );

    if (!_req->params( _args ))
      return true;

    *_args->item( 0 )->value() >>= CORBA::Any::to_string( name, 0 );
    TypeStruct* _res;
    #ifdef HAVE_EXCEPTIONS
    try {
    #endif
      _res = describe_type( name );
    #ifdef HAVE_EXCEPTIONS
    } catch( ::CosTrading::IllegalServiceType_var &_ex ) {
      _req->exception( _ex->_clone() );
      return true;
    } catch( ::CosTrading::UnknownServiceType_var &_ex ) {
      _req->exception( _ex->_clone() );
      return true;
    } catch( CORBA::SystemException_var &_ex ) {
      _req->exception( _ex->_clone() );
      return true;
    } catch( ... ) {
      assert( 0 );
      return true;
    }
    #endif
    CORBA::Any *_any_res = new CORBA::Any;
    *_any_res <<= *_res;
    delete _res;
    _req->result( _any_res );
    return true;
  }
  if( strcmp( _req->op_name(), "fully_describe_type" ) == 0 ) {
    CosTrading::ServiceTypeName_var name;

    CORBA::NVList_ptr _args = new CORBA::NVList (1);
    _args->add( CORBA::ARG_IN );
    _args->item( 0 )->value()->type( CosTrading::_tc_ServiceTypeName );

    if (!_req->params( _args ))
      return true;

    *_args->item( 0 )->value() >>= CORBA::Any::to_string( name, 0 );
    TypeStruct* _res;
    #ifdef HAVE_EXCEPTIONS
    try {
    #endif
      _res = fully_describe_type( name );
    #ifdef HAVE_EXCEPTIONS
    } catch( ::CosTrading::IllegalServiceType_var &_ex ) {
      _req->exception( _ex->_clone() );
      return true;
    } catch( ::CosTrading::UnknownServiceType_var &_ex ) {
      _req->exception( _ex->_clone() );
      return true;
    } catch( CORBA::SystemException_var &_ex ) {
      _req->exception( _ex->_clone() );
      return true;
    } catch( ... ) {
      assert( 0 );
      return true;
    }
    #endif
    CORBA::Any *_any_res = new CORBA::Any;
    *_any_res <<= *_res;
    delete _res;
    _req->result( _any_res );
    return true;
  }
  if( strcmp( _req->op_name(), "mask_type" ) == 0 ) {
    CosTrading::ServiceTypeName_var name;

    CORBA::NVList_ptr _args = new CORBA::NVList (1);
    _args->add( CORBA::ARG_IN );
    _args->item( 0 )->value()->type( CosTrading::_tc_ServiceTypeName );

    if (!_req->params( _args ))
      return true;

    *_args->item( 0 )->value() >>= CORBA::Any::to_string( name, 0 );
    #ifdef HAVE_EXCEPTIONS
    try {
    #endif
      mask_type( name );
    #ifdef HAVE_EXCEPTIONS
    } catch( ::CosTrading::IllegalServiceType_var &_ex ) {
      _req->exception( _ex->_clone() );
      return true;
    } catch( ::CosTrading::UnknownServiceType_var &_ex ) {
      _req->exception( _ex->_clone() );
      return true;
    } catch( ::CosTradingRepos::ServiceTypeRepository::AlreadyMasked_var &_ex ) {
      _req->exception( _ex->_clone() );
      return true;
    } catch( CORBA::SystemException_var &_ex ) {
      _req->exception( _ex->_clone() );
      return true;
    } catch( ... ) {
      assert( 0 );
      return true;
    }
    #endif
    return true;
  }
  if( strcmp( _req->op_name(), "unmask_type" ) == 0 ) {
    CosTrading::ServiceTypeName_var name;

    CORBA::NVList_ptr _args = new CORBA::NVList (1);
    _args->add( CORBA::ARG_IN );
    _args->item( 0 )->value()->type( CosTrading::_tc_ServiceTypeName );

    if (!_req->params( _args ))
      return true;

    *_args->item( 0 )->value() >>= CORBA::Any::to_string( name, 0 );
    #ifdef HAVE_EXCEPTIONS
    try {
    #endif
      unmask_type( name );
    #ifdef HAVE_EXCEPTIONS
    } catch( ::CosTrading::IllegalServiceType_var &_ex ) {
      _req->exception( _ex->_clone() );
      return true;
    } catch( ::CosTrading::UnknownServiceType_var &_ex ) {
      _req->exception( _ex->_clone() );
      return true;
    } catch( ::CosTradingRepos::ServiceTypeRepository::NotMasked_var &_ex ) {
      _req->exception( _ex->_clone() );
      return true;
    } catch( CORBA::SystemException_var &_ex ) {
      _req->exception( _ex->_clone() );
      return true;
    } catch( ... ) {
      assert( 0 );
      return true;
    }
    #endif
    return true;
  }
  return false;
}

CosTradingRepos::ServiceTypeRepository_ptr CosTradingRepos::ServiceTypeRepository_skel::_this()
{
  return CosTradingRepos::ServiceTypeRepository::_duplicate( this );
}

