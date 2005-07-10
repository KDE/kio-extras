/* This file is part of the KDE Project
   Copyright (c) 2004 - 2005 Jérôme Lodewyck <lodewyck@clipper.ens.fr>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "halbackend.h"
#include "linuxcdpolling.h"

#include <stdlib.h>

#include <klocale.h>
#include <kurl.h>
#include <kdebug.h>

#define MOUNT_SUFFIX	(libhal_volume_is_mounted(halVolume) ? QString("_mounted") : QString("_unmounted"))

/* Static instance of this class, for static HAL callbacks */
static HALBackend* s_HALBackend;

/* A macro function to convert HAL string properties to QString */
QString hal_device_get_property_QString(LibHalContext *ctx, const char* udi, const char *key)
{
	char*   _ppt_string;
	QString _ppt_QString;
	_ppt_string = libhal_device_get_property_string(ctx, udi, key, NULL);
	_ppt_QString = QString(_ppt_string ? _ppt_string : "");
	libhal_free_string(_ppt_string);
	return _ppt_QString;
}

/* Constructor */
HALBackend::HALBackend(MediaList &list, QObject* parent)
	: QObject()
	, BackendBase(list)
	, m_halContext(NULL)
	, m_halStoragePolicy(NULL)
	, m_parent(parent)
{
	s_HALBackend = this;
}

/* Destructor */
HALBackend::~HALBackend()
{
	/* Close HAL connection */
	if (m_halContext)
	{
		#ifdef HAL_0_4
		hal_shutdown(m_halContext);
		#else
		libhal_ctx_shutdown(m_halContext, NULL);
		libhal_ctx_free(m_halContext);
		#endif
	}
	if (m_halStoragePolicy)
		libhal_storage_policy_free(m_halStoragePolicy);

	/** @todo empty media list ? */
}

/* Connect to the HAL */
bool HALBackend::InitHal()
{
#ifdef HAL_0_4 /* HAL API 0.4 */
	/* libhal initialization */
	m_halFunctions.main_loop_integration	= HALBackend::hal_main_loop_integration;
	m_halFunctions.device_added				= HALBackend::hal_device_added;
	m_halFunctions.device_removed			= HALBackend::hal_device_removed;
	m_halFunctions.device_new_capability	= NULL;
	m_halFunctions.device_lost_capability	= NULL;
	m_halFunctions.device_property_modified	= HALBackend::hal_device_property_modified;
	m_halFunctions.device_condition			= HALBackend::hal_device_condition;

	m_halContext = hal_initialize(&m_halFunctions, FALSE);
	if (!m_halContext)
	{
		kdDebug(1219) << "Failed to initialize HAL!" << endl;
		return false;
	}

	/** @todo customize watch policy */
	kdDebug(1219) << "Watch properties" << endl;
	if (libhal_device_property_watch_all(m_halContext, NULL))
	{
		kdDebug(1219) << "Failed to watch HAL properties!" << endl;
		return false;
	}
#else /* HAL API >= 0.5 */
	kdDebug(1219) << "Context new" << endl;
	m_halContext = libhal_ctx_new();
	if (!m_halContext)
	{
		kdDebug(1219) << "Failed to initialize HAL!" << endl;
		return false;
	}

	// Main loop integration
	kdDebug(1219) << "Main loop integration" << endl;
	DBusError error;
	dbus_error_init(&error);
	DBusConnection *dbus_connection = dbus_bus_get(DBUS_BUS_SYSTEM, &error);
	if (dbus_error_is_set(&error)) {
		dbus_error_free(&error);
		libhal_ctx_free(m_halContext);
		m_halContext = NULL;
		return false;
	}
	MainLoopIntegration(dbus_connection);
	libhal_ctx_set_dbus_connection(m_halContext, dbus_connection);

	// HAL callback functions
	kdDebug(1219) << "Callback functions" << endl;
	libhal_ctx_set_device_added(m_halContext, HALBackend::hal_device_added);
	libhal_ctx_set_device_removed(m_halContext, HALBackend::hal_device_removed);
	libhal_ctx_set_device_new_capability (m_halContext, NULL);
	libhal_ctx_set_device_lost_capability (m_halContext, NULL);
	libhal_ctx_set_device_property_modified (m_halContext, HALBackend::hal_device_property_modified);
	libhal_ctx_set_device_condition(m_halContext, HALBackend::hal_device_condition);

	kdDebug(1219) << "Context Init" << endl;
	if (!libhal_ctx_init(m_halContext, &error))
	{
		if (dbus_error_is_set(&error))
			dbus_error_free(&error);
		libhal_ctx_free(m_halContext);
		m_halContext = NULL;
		kdDebug(1219) << "Failed to init HAL context!" << endl;
		return false;
	}

	/** @todo customize watch policy */
	kdDebug(1219) << "Watch properties" << endl;
	if (!libhal_device_property_watch_all(m_halContext, &error))
	{
		kdDebug(1219) << "Failed to watch HAL properties!" << endl;
		return false;
	}
#endif

	/* libhal-storage initialization */
	kdDebug(1219) << "Storage Policy" << endl;
	m_halStoragePolicy = libhal_storage_policy_new();
	/** @todo define libhal-storage icon policy */

	/* List devices at startup */
	return ListDevices();
}

/* List devices (at startup)*/
bool HALBackend::ListDevices()
{
	kdDebug(1219) << "ListDevices" << endl;

	int numDevices;
	char** halDeviceList = libhal_get_all_devices(m_halContext, &numDevices, NULL);

	if (!halDeviceList)
		return false;

	kdDebug(1219) << "HALBackend::ListDevices : " << numDevices << " devices found" << endl;
	for (int i = 0; i < numDevices; i++)
		AddDevice(halDeviceList[i]);

	return true;
}

/* Create a media instance for the HAL device "udi".
This functions checks whether the device is worth listing */
void HALBackend::AddDevice(const char *udi)
{
	/* We don't deal with devices that do not expose their capabilities.
	If we don't check this, we will get a lot of warning messages from libhal */
	if (!libhal_device_property_exists(m_halContext, udi, "info.capabilities", NULL))
		return;

	/* If the device is already listed, do not process.
	This should not happen, but who knows... */
	/** @todo : refresh properties instead ? */
	if (m_mediaList.findById(udi))
		return;

	/* Add volume block devices */
	if (libhal_device_query_capability(m_halContext, udi, "volume", NULL))
	{
		/* We only list volume that have a filesystem or volume that have an audio track*/
		if ( (hal_device_get_property_QString(m_halContext, udi, "volume.fsusage") != "filesystem") &&
		     (!libhal_device_get_property_bool(m_halContext, udi, "volume.disc.has_audio", NULL)) )
			return;
		/* Query drive udi */
		QString driveUdi = hal_device_get_property_QString(m_halContext, udi, "block.storage_device");
		/* We don't list floppy volumes because we list floppy drives */
		if ((hal_device_get_property_QString(m_halContext, driveUdi.ascii(), "storage.drive_type") == "floppy") ||
		    (hal_device_get_property_QString(m_halContext, driveUdi.ascii(), "storage.drive_type") == "zip") ||
		    (hal_device_get_property_QString(m_halContext, driveUdi.ascii(), "storage.drive_type") == "jaz"))
			return;

		/** @todo check exclusion list **/

		/* Create medium */
		Medium* medium = new Medium(udi, "");
		setVolumeProperties(medium);
		m_mediaList.addMedium(medium);

		return;
	}

	/* Floppy & zip drives */
	if (libhal_device_query_capability(m_halContext, udi, "storage", NULL))
		if ((hal_device_get_property_QString(m_halContext, udi, "storage.drive_type") == "floppy") ||
		    (hal_device_get_property_QString(m_halContext, udi, "storage.drive_type") == "zip") ||
		    (hal_device_get_property_QString(m_halContext, udi, "storage.drive_type") == "jaz"))
		{
			/* Create medium */
			Medium* medium = new Medium(udi, "");
			setFloppyProperties(medium);
			m_mediaList.addMedium(medium);
			return;
		}

	/* Camera handled by gphoto2*/
	if (libhal_device_query_capability(m_halContext, udi, "camera", NULL))

		{
			/* Create medium */
			Medium* medium = new Medium(udi, "");
			setCameraProperties(medium);
			m_mediaList.addMedium(medium);
			return;
		}
}

void HALBackend::RemoveDevice(const char *udi)
{
	m_mediaList.removeMedium(udi);
}

void HALBackend::ModifyDevice(const char *udi, const char* key)
{
	Q_UNUSED(key);
	const char* mediumUdi = findMediumUdiFromUdi(udi);
	if (!mediumUdi)
		return;
	ResetProperties(mediumUdi);
}

void HALBackend::DeviceCondition(const char* udi, const char* condition)
{
	const char* mediumUdi = findMediumUdiFromUdi(udi);
	if (!mediumUdi)
		return;

	QString conditionName = QString(condition);
	kdDebug(1219) << "Processing device condition " << conditionName << " for " << udi << endl;

	/* TODO: Warn the user that (s)he should unmount devices before unplugging */
	if (conditionName == "VolumeUnmountForced")
		ResetProperties(mediumUdi);

	/* Reset properties after mounting */
	if (conditionName == "VolumeMount")
		ResetProperties(mediumUdi);

	/* Reset properties after unmounting */
	if (conditionName == "VolumeUnmount")
		ResetProperties(mediumUdi);
}

void HALBackend::MainLoopIntegration(DBusConnection *dbusConnection)
{
	m_dBusQtConnection = new DBusQt::Connection(m_parent);
	m_dBusQtConnection->dbus_connection_setup_with_qt_main(dbusConnection);
}

/******************************************
** Properties attribution                **
******************************************/

/* Return the medium udi that should be updated when recieving a call for
device udi */
const char* HALBackend::findMediumUdiFromUdi(const char* udi)
{
	/* Easy part : this Udi is already registered as a device */
	const Medium* medium = m_mediaList.findById(udi);
	if (medium)
		return medium->id().ascii();

	/* Hard part : this is a volume whose drive is registered */
	if (libhal_device_property_exists(m_halContext, udi, "info.capabilities", NULL))
		if (libhal_device_query_capability(m_halContext, udi, "volume", NULL))
		{
			QString driveUdi = hal_device_get_property_QString(m_halContext, udi, "block.storage_device");
			return findMediumUdiFromUdi(driveUdi.ascii());
		}

	return NULL;
}

void HALBackend::ResetProperties(const char* mediumUdi)
{
	kdDebug(1219) << "HALBackend::setProperties" << endl;

	Medium* m = new Medium(mediumUdi, "");
	if (libhal_device_query_capability(m_halContext, mediumUdi, "volume", NULL))
		setVolumeProperties(m);
	if (libhal_device_query_capability(m_halContext, mediumUdi, "storage", NULL))
		setFloppyProperties(m);
	if (libhal_device_query_capability(m_halContext, mediumUdi, "camera", NULL))
		setCameraProperties(m);

	m_mediaList.changeMediumState(*m);

	delete m;
}

void HALBackend::setVolumeProperties(Medium* medium)
{
	kdDebug(1219) << "HALBackend::setVolumeProperties for " << medium->id() << endl;

	const char* udi = medium->id().ascii();
	/* Check if the device still exists */
	if (!libhal_device_exists(m_halContext, udi, NULL))
			return;

	/* Get device information from libhal-storage */
	LibHalVolume* halVolume = libhal_volume_from_udi(m_halContext, udi);
	if (!halVolume)
		return;
	QString driveUdi = libhal_volume_get_storage_device_udi(halVolume);
	LibHalDrive*  halDrive  = libhal_drive_from_udi(m_halContext, driveUdi.ascii());

	medium->setName(
		generateName(libhal_volume_get_device_file(halVolume)) );

	medium->mountableState(
		libhal_volume_get_device_file(halVolume),		/* Device node */
		libhal_volume_get_mount_point(halVolume),		/* Mount point */
		libhal_volume_get_fstype(halVolume),			/* Filesystem type */
		libhal_volume_is_mounted(halVolume) );			/* Mounted ? */

	QString mimeType;
	if (libhal_volume_is_disc(halVolume))
	{
		mimeType = "media/cdrom" + MOUNT_SUFFIX;

		LibHalVolumeDiscType discType = libhal_volume_get_disc_type(halVolume);
		if ((discType == LIBHAL_VOLUME_DISC_TYPE_CDROM) ||
		    (discType == LIBHAL_VOLUME_DISC_TYPE_CDR) ||
			(discType == LIBHAL_VOLUME_DISC_TYPE_CDRW))
			if (libhal_volume_disc_is_blank(halVolume))
			{
				mimeType = "media/blankcd";
				medium->unmountableState("");
			}
			else
				mimeType = "media/cdwriter" + MOUNT_SUFFIX;

		if ((discType == LIBHAL_VOLUME_DISC_TYPE_DVDROM) || (discType == LIBHAL_VOLUME_DISC_TYPE_DVDRAM) ||
			(discType == LIBHAL_VOLUME_DISC_TYPE_DVDR) || (discType == LIBHAL_VOLUME_DISC_TYPE_DVDRW) ||
			(discType == LIBHAL_VOLUME_DISC_TYPE_DVDPLUSR) || (discType == LIBHAL_VOLUME_DISC_TYPE_DVDPLUSRW) )
			if (libhal_volume_disc_is_blank(halVolume))
			{
				mimeType = "media/blankdvd";
				medium->unmountableState("");
			}
			else
				mimeType = "media/dvd" + MOUNT_SUFFIX;

		if (libhal_volume_disc_has_audio(halVolume) && !libhal_volume_disc_has_data(halVolume))
		{
			mimeType = "media/audiocd";
			medium->unmountableState( "audiocd:/?device=" + QString(libhal_volume_get_device_file(halVolume)) );
		}

		medium->setIconName(QString::null);

		/* check if the disc id a vcd or a video dvd */
		DiscType type = LinuxCDPolling::identifyDiscType(libhal_volume_get_device_file(halVolume));
		switch (type)
		{
		  case DiscType::VCD:
		    mimeType = "media/vcd";
		    break;
		  case DiscType::SVCD:
		    mimeType = "media/svcd";
		    break;
		  case DiscType::DVD:
		    mimeType = "media/dvdvideo";
		    break;
		}
	}
	else
	{
		mimeType = "media/hdd" + MOUNT_SUFFIX;
		if (libhal_drive_is_hotpluggable(halDrive))
		{
			mimeType = "media/removable" + MOUNT_SUFFIX;
			medium->needMounting();
			switch (libhal_drive_get_type(halDrive)) {
			case LIBHAL_DRIVE_TYPE_COMPACT_FLASH:
				medium->setIconName("compact_flash" + MOUNT_SUFFIX);
				break;
			case LIBHAL_DRIVE_TYPE_MEMORY_STICK:
				medium->setIconName("memory_stick" + MOUNT_SUFFIX);
				break;
			case LIBHAL_DRIVE_TYPE_SMART_MEDIA:
				medium->setIconName("smart_media" + MOUNT_SUFFIX);
				break;
			case LIBHAL_DRIVE_TYPE_SD_MMC:
				medium->setIconName("sd_mmc" + MOUNT_SUFFIX);
				break;
			case LIBHAL_DRIVE_TYPE_PORTABLE_AUDIO_PLAYER:
				medium->setIconName(QString::null); //FIXME need icon
				break;
			case LIBHAL_DRIVE_TYPE_CAMERA:
				medium->setIconName("camera" + MOUNT_SUFFIX);
				break;
			case LIBHAL_DRIVE_TYPE_TAPE:
				medium->setIconName(QString::null); //FIXME need icon
				break;
			default:
				medium->setIconName(QString::null);
			};
		};
	}
	medium->setMimeType(mimeType);

	char* name = libhal_volume_policy_compute_display_name(halDrive, halVolume, m_halStoragePolicy);
	//char* name = libhal_drive_policy_compute_display_name(halDrive, halVolume, m_halStoragePolicy);
	QString volume_name = QString::fromUtf8(name);
	QString media_name = volume_name;
	medium->setLabel(media_name);
	free(name);

	libhal_drive_free(halDrive);
	libhal_volume_free(halVolume);
}

// Handle floppies and zip drives
void HALBackend::setFloppyProperties(Medium* medium)
{
	kdDebug(1219) << "HALBackend::setFloppyProperties for " << medium->id() << endl;

	const char* udi = medium->id().ascii();
	/* Check if the device still exists */
	if (!libhal_device_exists(m_halContext, udi, NULL))
		return;

	LibHalDrive*  halDrive  = libhal_drive_from_udi(m_halContext, udi);
	if (!halDrive)
		return;
	int numVolumes;
	char** volumes = libhal_drive_find_all_volumes(m_halContext, halDrive, &numVolumes);
	LibHalVolume* halVolume = NULL;
	kdDebug(1219) << " found " << numVolumes << " volumes" << endl;
	if (numVolumes)
		halVolume = libhal_volume_from_udi(m_halContext, volumes[0]);

	medium->setName(
		generateName(libhal_drive_get_device_file(halDrive)) );

	if (halVolume)
	{
		medium->mountableState(
			libhal_volume_get_device_file(halVolume),		/* Device node */
			libhal_volume_get_mount_point(halVolume),		/* Mount point */
			libhal_volume_get_fstype(halVolume),			/* Filesystem type */
			libhal_volume_is_mounted(halVolume) );			/* Mounted ? */
	}
	else
	{
		medium->mountableState(
			libhal_drive_get_device_file(halDrive),		/* Device node */
			"",											/* Mount point */
			"",											/* Filesystem type */
			false );									/* Mounted ? */
	}

	if (hal_device_get_property_QString(m_halContext, udi, "storage.drive_type") == "floppy")
	{
		if (halVolume)
			medium->setMimeType("media/floppy" + MOUNT_SUFFIX);
		else
			medium->setMimeType("media/floppy_unmounted");
	}

	if (hal_device_get_property_QString(m_halContext, udi, "storage.drive_type") == "zip")
	{
		if (halVolume)
			medium->setMimeType("media/zip" + MOUNT_SUFFIX);
		else
			medium->setMimeType("media/zip_unmounted");
	}

	/** @todo And mimtype for JAZ drives ? */

	medium->setIconName(QString::null);

	QString media_name;
	if (halVolume)
	{
		char* name = libhal_drive_policy_compute_display_name(halDrive, halVolume, m_halStoragePolicy);
		QString volume_name = QString::fromUtf8(name);
		media_name = volume_name;
		free(name);
	}
	else
	{
		char* name = libhal_drive_policy_compute_display_name(halDrive, halVolume, m_halStoragePolicy);
		QString drive_name =  QString::fromUtf8(name);
		media_name = drive_name;
		free(name);
	}
	medium->setLabel(media_name);

	free(volumes);
	libhal_drive_free(halDrive);
	libhal_volume_free(halVolume);
}

void HALBackend::setCameraProperties(Medium* medium)
{
	kdDebug(1219) << "HALBackend::setCameraProperties for " << medium->id() << endl;

	const char* udi = medium->id().ascii();
	/* Check if the device still exists */
	if (!libhal_device_exists(m_halContext, udi, NULL))
		return;

	/** @todo find name */
	medium->setName("camera");
	/** @todo find the rest of this URL */
	medium->unmountableState("camera:/");
	medium->setMimeType("media/gphoto2camera");
	medium->setIconName(QString::null);
	/** @todo find label */
	medium->setLabel("Camera");
}

QString HALBackend::generateName(const QString &devNode)
{
	return KURL(devNode).fileName();
}

/******************************************
** HAL CALL-BACKS                        **
******************************************/

#ifdef HAL_0_4
void HALBackend::hal_main_loop_integration(LibHalContext *ctx,
			DBusConnection *dbus_connection)
{
	kdDebug(1219) << "HALBackend::hal_main_loop_integration" << endl;
	Q_UNUSED(ctx);
	s_HALBackend->MainLoopIntegration(dbus_connection);
}
#endif

void HALBackend::hal_device_added(LibHalContext *ctx, const char *udi)
{
	kdDebug(1219) << "HALBackend::hal_device_added " << udi <<  endl;
	Q_UNUSED(ctx);
	s_HALBackend->AddDevice(udi);
}

void HALBackend::hal_device_removed(LibHalContext *ctx, const char *udi)
{
	kdDebug(1219) << "HALBackend::hal_device_removed " << udi << endl;
	Q_UNUSED(ctx);
	s_HALBackend->RemoveDevice(udi);
}

void HALBackend::hal_device_property_modified(LibHalContext *ctx, const char *udi,
			const char *key, dbus_bool_t is_removed, dbus_bool_t is_added)
{
	kdDebug(1219) << "HALBackend::hal_property_modified " << udi << " -- " << key << endl;
	Q_UNUSED(ctx);
	Q_UNUSED(is_removed);
	Q_UNUSED(is_added);
	s_HALBackend->ModifyDevice(udi, key);
}

void HALBackend::hal_device_condition(LibHalContext *ctx, const char *udi,
			const char *condition_name,
			#ifdef HAL_0_4
			DBusMessage *message
			#else
			const char* message
			#endif
			)
{
	kdDebug(1219) << "HALBackend::hal_device_condition " << udi << " -- " << condition_name << endl;
	Q_UNUSED(ctx);
	Q_UNUSED(message);
	s_HALBackend->DeviceCondition(udi, condition_name);
}

#include "halbackend.moc"
