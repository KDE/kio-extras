/* This file is part of the KDE Project
   Copyright (c) 2004-2005 Jérôme Lodewyck <jerome dot lodewyck at normalesup dot org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

/**
* This is a media:/ backend for the freedesktop Hardware Abstraction Layer
* Usage : create an instance of HALBackend, then call InitHal(). A false
* result from the later function means that something went wrong and that
* the backend shall not be used.
*
* @author Jérôme Lodewyck <jerome dot lodewyck at normalesup dot org>
* @short media:/ backend for the HAL
*/

#ifndef _HALBACKEND_H_
#define _HALBACKEND_H_

#include "backendbase.h"

#include <QObject>

/* We acknowledge the the dbus API is unstable */
#define DBUS_API_SUBJECT_TO_CHANGE
/* DBus-Qt bindings */
#include <dbus/connection.h>
/* HAL libraries */
#include <libhal.h>
#include <libhal-storage.h>

/* The HAL API changed between 0.4 and 0.5 series.
These defines enable backward compatibility */
#ifdef HAL_0_4
	// libhal-storage 0.4 API
	#define LibHalStoragePolicy				HalStoragePolicy
	#define LibHalDrive						HalDrive
	#define LibHalVolume					HalVolume
	#define LibHalVolumeDiscType			HalVolumeDiscType
	#define libhal_storage_policy_free		hal_storage_policy_free
	#define libhal_storage_policy_new		hal_storage_policy_new
	#define libhal_drive_from_udi			hal_drive_from_udi
	#define libhal_drive_find_all_volumes	hal_drive_find_all_volumes
	#define libhal_drive_get_type			hal_drive_get_type
	#define libhal_drive_get_device_file	hal_drive_get_device_file
	#define libhal_drive_free				hal_drive_free
	#define libhal_drive_policy_compute_display_name	hal_drive_policy_compute_display_name
	#define libhal_drive_is_hotpluggable	hal_drive_is_hotpluggable
	#define libhal_drive_get_physical_device_udi hal_drive_get_physical_device_udi
	#define libhal_volume_from_udi			hal_volume_from_udi
	#define libhal_volume_get_device_file	hal_volume_get_device_file
	#define libhal_volume_get_mount_point	hal_volume_get_mount_point
	#define libhal_volume_get_fstype		hal_volume_get_fstype
	#define libhal_volume_is_mounted		hal_volume_is_mounted
	#define libhal_volume_get_disc_type		hal_volume_get_disc_type
	#define libhal_volume_free				hal_volume_free
	#define libhal_volume_policy_compute_display_name	hal_volume_policy_compute_display_name
	#define libhal_volume_disc_has_data		hal_volume_disc_has_data
	#define libhal_volume_disc_has_audio	hal_volume_disc_has_audio
	#define libhal_volume_disc_is_blank		hal_volume_disc_is_blank
	#define libhal_volume_is_disc			hal_volume_is_disc
	#define libhal_volume_get_storage_device_udi	hal_volume_get_storage_device_udi
	#define LIBHAL_VOLUME_DISC_TYPE_CDROM		HAL_VOLUME_DISC_TYPE_CDROM
	#define LIBHAL_VOLUME_DISC_TYPE_CDR			HAL_VOLUME_DISC_TYPE_CDR
	#define LIBHAL_VOLUME_DISC_TYPE_CDRW		HAL_VOLUME_DISC_TYPE_CDRW
	#define LIBHAL_VOLUME_DISC_TYPE_DVDROM		HAL_VOLUME_DISC_TYPE_DVDROM
	#define LIBHAL_VOLUME_DISC_TYPE_DVDRAM		HAL_VOLUME_DISC_TYPE_DVDRAM
	#define LIBHAL_VOLUME_DISC_TYPE_DVDR		HAL_VOLUME_DISC_TYPE_DVDR
	#define LIBHAL_VOLUME_DISC_TYPE_DVDRW		HAL_VOLUME_DISC_TYPE_DVDRW
	#define LIBHAL_VOLUME_DISC_TYPE_DVDPLUSR	HAL_VOLUME_DISC_TYPE_DVDPLUSR
	#define LIBHAL_VOLUME_DISC_TYPE_DVDPLUSRW	HAL_VOLUME_DISC_TYPE_DVDPLUSRW
	#define LIBHAL_DRIVE_TYPE_COMPACT_FLASH			HAL_DRIVE_TYPE_COMPACT_FLASH
	#define LIBHAL_DRIVE_TYPE_MEMORY_STICK			HAL_DRIVE_TYPE_MEMORY_STICK
	#define LIBHAL_DRIVE_TYPE_SMART_MEDIA 			HAL_DRIVE_TYPE_SMART_MEDIA
	#define LIBHAL_DRIVE_TYPE_SD_MMC				HAL_DRIVE_TYPE_SD_MMC
	#define LIBHAL_DRIVE_TYPE_PORTABLE_AUDIO_PLAYER	HAL_DRIVE_TYPE_PORTABLE_AUDIO_PLAYER
	#define LIBHAL_DRIVE_TYPE_CAMERA				HAL_DRIVE_TYPE_CAMERA
	#define LIBHAL_DRIVE_TYPE_TAPE					HAL_DRIVE_TYPE_TAPE

	// libhal 0.4 API
	#define libhal_free_string hal_free_string
	#define libhal_device_exists(ctx, udi, error) hal_device_exists(ctx, udi)
	#define libhal_device_property_watch_all(ctx, error) hal_device_property_watch_all(ctx)
	#define libhal_get_all_devices(ctx, num_devices, error) hal_get_all_devices(ctx, num_devices)
	#define libhal_device_property_exists(ctx, udi, key, error) hal_device_property_exists(ctx, udi, key)
	#define libhal_device_get_property_bool(ctx, udi, key, error) hal_device_get_property_bool(ctx, udi, key)
	#define libhal_device_get_property_string(ctx, udi, key, error) hal_device_get_property_string(ctx, udi, key)
	#define libhal_device_query_capability(ctx, udi, capability, error) hal_device_query_capability(ctx, udi, capability)
#endif


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
	* @return true if succeeded. If not, rely on some other backend
	*/
	bool InitHal();

	/**
	* List all devices and append them to the media device list (called only once, at startup).
	*
	* @return true if succeeded, false otherwise
	*/
	bool ListDevices();

private:
	/**
	* Append a device in the media list. This function will check if the device
	* is worth listing.
	*
	*  @param udi                 Universal Device Id
	*  @param allowNotification   Indicates if this event will be notified to the user
	*/
	void AddDevice(const char* udi, bool allowNotification=true);

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
	* HAL informed that a special action has occurred
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
#ifdef HAL_0_4
	/** Invoked by libhal for integration with our mainloop.
	*
	*  @param  ctx                 LibHal context
	*  @param  dbus_connection     D-BUS connection to integrate
	*/
	static void hal_main_loop_integration(LibHalContext *ctx, DBusConnection *dbus_connection);
#endif

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
				const char *condition_name,
				#ifdef HAL_0_4
				DBusMessage *message
				#else
				const char* message
				#endif
				);

/* HAL and DBus structures */
private:
	/**
	* The HAL context connecting the whole application to the HAL
	*/
	LibHalContext*		m_halContext;

#ifdef HAL_0_4
	/**
	* Structure defining the hal callback function for devices events
	*/
	LibHalFunctions 	m_halFunctions;
#endif

	/**
	* libhal-storage HAL policy, e.g. for icon names
	*/
	LibHalStoragePolicy*	m_halStoragePolicy;

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
