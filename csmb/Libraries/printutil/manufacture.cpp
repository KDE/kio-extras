/* Name: manufacture.cpp
            
    Description: This file is a part of the printutil shared library.

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

////////////////////////////////////////////////////////////////////////////////
//implementation of class manufacture

#include "manufacture.h"
#include <stdio.h>
#include <string.h>

////////////////////////////////////////////////////////////////////////////////

KManufacture::KManufacture(QString name)
{
	setMFName(name);
	char **modelNames;
	int numModels;
	int mod;
	int k;
	int result;

	if ((result = Aps_GetKnownModels((const char*)name
#ifdef QT_20
  .latin1()
#endif
  , &modelNames, &numModels))
            == APS_SUCCESS)
	{
		m_numOfModels = numModels;

		// sort the Arrays now according the model name
		// and insert to the m_models array

		for (mod = 0; mod < numModels; mod++)
		{
			int index = mod;
			for (k = index +1; k < numModels;k++)
			{
       	if (strcmp(modelNames[index], modelNames[k])>0)
				{
				 	char *temp;
					temp = modelNames[index];
					modelNames[index] = modelNames[k];
					modelNames[k]= temp;
				}
			}

			m_models.append((const char *)((QString)modelNames[mod])
#ifdef QT_20
  .latin1()
#endif

      );
    }

		if((result = Aps_ReleaseBuffer(modelNames)) != APS_SUCCESS)
			fprintf(stderr, "\n Err, Aps_ReleaseBuffer(modleNames) = %d", result);
  }
	else
   	fprintf(stderr, "\nErr, Aps_GetKnownModels() = %d\n", result);
}

////////////////////////////////////////////////////////////////////////////////

KManufacture::~KManufacture()
{
  m_numOfModels = 0;
	m_models.clear();
}

////////////////////////////////////////////////////////////////////////////////

QStrList KManufacture::getModels()
{
  return m_models;
}

////////////////////////////////////////////////////////////////////////////////

int KManufacture::getNumOfModels()
{
  return m_numOfModels;
}

////////////////////////////////////////////////////////////////////////////////

const QString KManufacture::getMFName()
{
  return m_manufactureName;
}

////////////////////////////////////////////////////////////////////////////////

void KManufacture::setMFName(const QString name)
{
  m_manufactureName = name;
}

////////////////////////////////////////////////////////////////////////////////

void KManufacture::insertModel(const QString name)
{
  m_models.append((const char *)name
#ifdef QT_20
  .latin1()
#endif
                          );
}

////////////////////////////////////////////////////////////////////////////////
//implementation of KManufactures

KManufactures::KManufactures()
{
 	m_manufactures.clear();
	char **manNames;
	int numMans;
	int man;
	int manf;

	int result;

	if ((result = Aps_GetKnownManufacturers(&manNames, &numMans)) == APS_SUCCESS)
	{
	  for (man = 0; man < numMans; ++man)
		{
		  for (manf = man+1; manf<numMans; ++manf)
			{
      	if (strcmp(manNames[man], manNames[manf])>0)
				{
         	char *temp;
					temp = manNames[man];
					manNames[man] = manNames[manf];
					manNames[manf]= temp;
				}
			}
			KManufacture *km;
			km = new KManufacture(manNames[man]);
			insertManufacture(km);
    }

    Aps_ReleaseBuffer(manNames);
	}
	else
    fprintf(stderr,"\nErr, Aps_GetKnownManufacturers = %d",result);
}

////////////////////////////////////////////////////////////////////////////////

KManufactures::~KManufactures()
{
	m_manufactures.clear();
}

////////////////////////////////////////////////////////////////////////////////

void KManufactures::insertManufacture(KManufacture *km)
{
	KManufacture *akm;

	for (uint i = 0; i< m_manufactures.count();i++)
	{
		akm = m_manufactures.at(i);

		if (km->getMFName() ==akm->getMFName())
			m_manufactures.remove(akm);
	}

	m_manufactures.append(km);
}

////////////////////////////////////////////////////////////////////////////////

KManufacture* KManufactures::getManufacture1(const QString name)
{
	KManufacture *km;

	for (uint i = 0; i< m_manufactures.count();i++)
	{
		km = m_manufactures.at(i);

		if (km->getMFName() == name)
			return km;
	}

	return NULL;
}

////////////////////////////////////////////////////////////////////////////////

KManufacture* KManufactures::getManufacture2(int k)
{
	if (k < numberOfManufactures())
		return m_manufactures.at(k);
	else
		return NULL;
}

////////////////////////////////////////////////////////////////////////////////

int KManufactures::numberOfManufactures()
{
  return m_manufactures.count();
}

////////////////////////////////////////////////////////////////////////////////
