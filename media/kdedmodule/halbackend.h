/* This file is part of the KDE Project
   Copyright (c) 2004 Jérôme Lodewyck <lodewyck@clipper.ens.fr>

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

/**
* This is a media:/ backend for the freedesktop Hardware Abstraction Layer
* Usage : create an instance of HALBackend, then call InitHal(). A false
* result from the later function means that something went wrong and that
* the backend shall not be used.
*
* @author Jerome Lodewyck <lodewyck@clipper.ens.fr>
* @short media:/ backend for the HAL
*/

#ifndef _HALBACKEND_H_
#define _HALBACKEND_H_

#include "backendbase.h"

#include <qobject.h>
#include <qstringlist.h>
#include <qstring.h>

/* We acknowledge the the dbus API is unstable */
#define DBUS_API_SUBJECT_TO_CHANGE
/* DBus-Qt bindings */
#include <dbus/connection.h>
/* HAL libraries */
#include <libhal.h>
#include <libhal-storage.h>

/**
* A handy function to query a hal string
*
*  @param ctx                 HAL Context
*  @param udi                 Unique Device Id
*  @param key                 Name of the property that has changed
*/
QString hal_device_get_property_QString(LibHalContext *ctx, const char *udi, const char *key);


class HALBackend : public QObject, public BackendBase
{
Q_OBJECT

public:
	/**
	* Constructor
	*/
	HALBackend(MediaList &list, QObject* parent);

	/**
	* Destructor
	*/
	~HALBackend();

	/**
	* Perform HAL initialization.
	*
	* @return true if succeded. If not, rely on some other backend
	*/
	bool InitHal();

	/**
	* List all devices and append them to the media device list (called only once, at startup).
	*
	* @return true if succeded, false otherwise
	*/
	bool ListDevices();

private:
	/**
	* Append a device in the media list. This function will check if the device
	* is worth listing.
	*
	*  @param udi                 Universal Device Id
	*/
	void AddDevice(const char* udi);

	/**
	* Remove a device from the device list
	*
	*  @param udi                 Universal Device Id
	*/
	void RemoveDevice(const char* udi);

	/**
	* A device has changed, update it
	*
	*  @param udi                 Universal Device Id
	*/
	void ModifyDevice(const char *udi, const char* key);

	/**
	* HAL informed that a special action has occured
	* (e.g. device unplugged without unmounting)
	*
	*  @param udi                 Universal Device Id
	*/
	void DeviceCondition(const char *udi, const char *condition);

	/**
	* Integrate the DBus connection within qt main loop
	*/
	void MainLoopIntegration(DBusConnection *dbusConnection);

/* Set media properties */
private:
	/**
	* Reset properties for the given medium
	*/
	void ResetProperties(const char* MediumUdi);

	/**
	* Find the medium that is concerned with device udi
	*/
	const char* findMediumUdiFromUdi(const char* udi);

	void setVolumeProperties(Medium* medium);
	void setFloppyProperties(Medium* medium);
	void setCameraProperties(Medium* medium);
	QString generateName(const QString &devNode);

/* Hal call-backs -- from gvm*/
public:
	/** Invoked by libhal for integration with our mainloop.
	*
	*  @param  ctx                 LibHal context
	*  @param  dbus_connection     D-BUS connection to integrate
	*/
	static void hal_main_loop_integration(LibHalContext *ctx, DBusConnection *dbus_connection);

	/** Invoked when a device is added to the Global Device List.
	*
	*  @param  ctx                 LibHal context
	*  @param  udi                 Universal Device Id
	*/
	static void hal_device_added(LibHalContext *ctx, const char *udi);

	/** Invoked when a device is removed from the Global Device List.
	*
	*  @param  ctx                 LibHal context
	*  @param  udi                 Universal Device Id
	*/
	static void hal_device_removed(LibHalContext *ctx, const char *udi);

	/** Invoked when a property of a device in the Global Device List is
	*  changed, and we have we have subscribed to changes for that device.
	*
	*  @param  ctx                 LibHal context
	*  @param  udi                 Univerisal Device Id
	*  @param  key                 Key of property
	*/
	static void hal_device_property_modified(LibHalContext *ctx, const char *udi, const char *key,
				dbus_bool_t is_removed, dbus_bool_t is_added);

	/** Type for callback when a non-continuos condition occurs on a device
	*
	*  @param  udi                 Univerisal Device Id
	*  @param  condition_name      Name of the condition
	*  @param  message             D-BUS message with variable parameters depending on condition
	*/
	static void hal_device_condition(LibHalContext *ctx, const char *udi,
				const char *condition_name, DBusMessage *message);

/* HAL and DBus structures */
private:
	/**
	* The HAL context connecting the whole application to the HAL
	*/
	LibHalContext*		m_halContext;

	/**
	* Structure defining the hal callback function for devices events
	*/
	LibHalFunctions 	m_halFunctions;

	/**
	* libhal-storage HAL policy, e.g. for icon names
	*/
	HalStoragePolicy*	m_halStoragePolicy;

	/**
	* The DBus-Qt bindings connection for mainloop integration
	*/
	DBusQt::Connection*	m_dBusQtConnection;

	/**
	* Object for the kded module
	*/
	QObject* m_parent;
};

#endif /* _HALBACKEND_H_ */
