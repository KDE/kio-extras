/* Name: pageThree.cpp
            
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

#include <qpixmap.h>
#include <qlayout.h>
#include <kfiledialog.h>

#include "constText.h"
#define Inherited CWizardPage

#include "pageThree.h"
#include <qlabel.h>
#include <qcombobox.h>
#include <aps.h>
#define TRACE printf("file:%s, ------ line:%d\n", __FILE__, __LINE__);

#include <kfiledialog.h>
#include <qfiledialog.h>

pageThree::pageThree
(
	int nPage, bool bFinalPage = false, int nNextPage = -1, int nPrevPage = -1,
	const char* pszImage = 0, const char* pszTitle = 0, bool bCheckPage = true,
	QWidget *parent=0, const char *name=0

)
	:
	Inherited( nPage, bFinalPage,
		nNextPage, nPrevPage, pszImage, pszTitle, bCheckPage,
		parent, name )
{
	// General page description

	m_pDescriptionLabel = new QLabel(this, "Label_1");
	m_pDescriptionLabel->setGeometry(5, 3, 360, 62 );
	m_pDescriptionLabel->setMinimumSize( 0, 0 );
	m_pDescriptionLabel->setMaximumSize( 32767, 32767 );
	m_pDescriptionLabel->setFocusPolicy( QWidget::NoFocus );
	m_pDescriptionLabel->setBackgroundMode( QWidget::PaletteBackground );
	m_pDescriptionLabel->setFontPropagation( QWidget::NoChildren );
	m_pDescriptionLabel->setPalettePropagation( QWidget::NoChildren );
	m_pDescriptionLabel->setFrameStyle( 0 );
	m_pDescriptionLabel->setLineWidth( 1 );
	m_pDescriptionLabel->setMidLineWidth( 0 );
	m_pDescriptionLabel->QFrame::setMargin( 0 );
	m_pDescriptionLabel->setText( LABEL3);
	m_pDescriptionLabel->setAlignment( 1289 );
	m_pDescriptionLabel->setMargin( -1 );

  // Manufacturer

	m_pManufacturerLabel = new QLabel( this, "Label_2" );
	m_pManufacturerLabel->setMinimumSize( 0, 0 );
	m_pManufacturerLabel->setMaximumSize( 32767, 32767 );
	m_pManufacturerLabel->setFocusPolicy( QWidget::NoFocus );
	m_pManufacturerLabel->setBackgroundMode( QWidget::PaletteBackground );
	m_pManufacturerLabel->setFontPropagation( QWidget::NoChildren );
	m_pManufacturerLabel->setPalettePropagation( QWidget::NoChildren );
	m_pManufacturerLabel->setFrameStyle( 0 );
	m_pManufacturerLabel->setLineWidth( 1 );
	m_pManufacturerLabel->setMidLineWidth( 0 );
	m_pManufacturerLabel->QFrame::setMargin( 0 );
	m_pManufacturerLabel->setText( MANUFACTURER );
	m_pManufacturerLabel->setMargin( -1 );

	m_pManufacturer = new QComboBox( FALSE, this, "ComboBox_1" );
	m_pManufacturer->setMinimumSize( 0, 0 );
	m_pManufacturer->setMaximumSize( 32767, 32767 );
	m_pManufacturer->setFocusPolicy( QWidget::StrongFocus );
	m_pManufacturer->setBackgroundMode( QWidget::PaletteBackground );
	m_pManufacturer->setFontPropagation( QWidget::AllChildren );
	m_pManufacturer->setPalettePropagation( QWidget::AllChildren );
	m_pManufacturer->setSizeLimit( 10 );
	m_pManufacturer->setAutoResize( FALSE );
	m_pManufacturer->setMaxCount( 2147483647 );
	m_pManufacturer->setAutoCompletion( FALSE );
  m_pManufacturerLabel->setBuddy(m_pManufacturer);
  connect( m_pManufacturer, SIGNAL(activated(const char*)), this, SLOT(updateList(const char*)));

	// Model

	m_pModelLabel = new QLabel( this, "Label_3" );
	m_pModelLabel->setMinimumSize( 0, 0 );
	m_pModelLabel->setMaximumSize( 32767, 32767 );
	m_pModelLabel->setFocusPolicy( QWidget::NoFocus );
	m_pModelLabel->setBackgroundMode( QWidget::PaletteBackground );
	m_pModelLabel->setFontPropagation( QWidget::NoChildren );
	m_pModelLabel->setPalettePropagation( QWidget::NoChildren );
	m_pModelLabel->setFrameStyle( 0 );
	m_pModelLabel->setLineWidth( 1 );
	m_pModelLabel->setMidLineWidth( 0 );
	m_pModelLabel->QFrame::setMargin( 0 );
	m_pModelLabel->setText( PRINTER_MODE );
	m_pModelLabel->setMargin( -1 );

	m_pModel = new QComboBox( FALSE, this, "ComboBox_2" );
	m_pModel->setMinimumSize( 0, 0 );
	m_pModel->setMaximumSize( 32767, 32767 );
	m_pModel->setFocusPolicy( QWidget::StrongFocus );
	m_pModel->setBackgroundMode( QWidget::PaletteBackground );
	m_pModel->setFontPropagation( QWidget::AllChildren );
	m_pModel->setPalettePropagation( QWidget::AllChildren );
	m_pModel->setSizeLimit( 10 );
	m_pModel->setAutoResize( FALSE );
	m_pModel->setMaxCount( 2147483647 );
	m_pModel->setAutoCompletion( FALSE );
	m_pModelLabel->setBuddy(m_pModel);

	// Adjust sizes

	int nLabelWidth = m_pManufacturerLabel->sizeHint().width();

	if (nLabelWidth < m_pModelLabel->sizeHint().width())
		nLabelWidth = m_pModelLabel->sizeHint().width();

  m_pManufacturerLabel->setGeometry(5, 70, nLabelWidth, 25);
	m_pManufacturer->setGeometry(15+nLabelWidth, 70, 305 - nLabelWidth, 25);

	m_pModelLabel->setGeometry(5, 110, nLabelWidth, 25);
	m_pModel->setGeometry(15+nLabelWidth, 110, 305 - nLabelWidth, 25);

	init_comboboxes();
	updateList((const char *)m_pManufacturer->currentText()
#ifdef QT_20
  .latin1()
#endif
  );

	m_DiskButton = new QPushButton(this, "button");
  m_DiskButton->setMinimumSize( 0, 0 );
  m_DiskButton->setMaximumSize( 32767, 32767 );
  m_DiskButton->setFocusPolicy( QWidget::TabFocus );
  m_DiskButton->setBackgroundMode( QWidget::PaletteBackground );
  m_DiskButton->setFontPropagation( QWidget::NoChildren );
  m_DiskButton->setPalettePropagation( QWidget::NoChildren );
  m_DiskButton->setText(HAVE_DISK);
  m_DiskButton->setAutoRepeat( FALSE );
  m_DiskButton->setAutoResize( FALSE );
  m_DiskButton->setToggleButton( FALSE );
  m_DiskButton->setDefault( FALSE );
  m_DiskButton->setAutoDefault( FALSE );
  m_DiskButton->setIsMenuButton( FALSE );

	m_DiskButton->setGeometry(320 - m_DiskButton->sizeHint().width(), 150, m_DiskButton->sizeHint().width(), m_DiskButton->sizeHint().height());

	connect(m_DiskButton, SIGNAL(clicked()), this, SLOT(diskButtonClicked()));

	resize(400, 300);

	setMinimumSize( 0, 0 );
	setMaximumSize( 32767, 32767 );
}


pageThree::~pageThree()
{

    delete m_pManufacturer;
    delete m_pModel;
    //delete pd;
    delete m_pDescriptionLabel;
    delete m_pManufacturerLabel;
    delete m_pModelLabel;

// KMsgBox::message(this, "pageThree","Distructor", 0);
}

void pageThree::updateList(const char* pszManufacturer)
{
	m_pModel->clear();

	char **modelNames;
	int numModels;

	if (APS_SUCCESS == Aps_GetKnownModels(pszManufacturer, &modelNames, &numModels))
	{
		for (int i=0; i < numModels; i++)
		{
			m_pModel->insertItem(modelNames[i]);
		}

		Aps_ReleaseBuffer(modelNames);
	}
}

int pageThree::getManufacture(char* name)
{
		//if (m_pManufacturer->currentText())
    {
			strcpy( name, (const char *)m_pManufacturer->currentText()
#ifdef QT_20
  .latin1()
#endif
      );
		}
		//else
		//	name = NULL;
    return 0;
}

int pageThree::getPrinterModel(char* name)
{
	//if (m_pModel->currentText())
  {
	  strcpy( name, (const char *)m_pModel->currentText()
#ifdef QT_20
  .latin1()
#endif
    );
	}
	//else
	//	name = NULL;
    return 0;
}

int pageThree::getMagicFilter(char* name)
{
 /*   pd->getMagicFilterName(name, m_pManufacturer->currentText(),
                           m_pModel->currentText());   */
    return 0;
}

void pageThree::init_comboboxes()
{
	m_pManufacturer->clear();

	char **manNames;
	int numMans;

	if (APS_SUCCESS == Aps_GetKnownManufacturers(&manNames, &numMans))
	{
		QStrList list;

		for (int i = 0; i < numMans; i++)
			list.inSort(manNames[i]);

		m_pManufacturer->insertStrList(&list);

		Aps_ReleaseBuffer(manNames);
	}
}

int pageThree::indexOfItem(const char *item, QComboBox *list)
{
  // Simple linear search because the list should be small
  for (int i = 0; i < list->count(); i++)
  {
    if (strcmp(item, (const char *)list->text(i)
#ifdef QT_20
  .latin1()
#endif
                                    ) == 0)
       return i;
  }
  return -1;
}

void pageThree::diskButtonClicked()
{

 	Aps_ModelHandle model;
	char *modelName;
	char *manufactureName;
	Aps_Result result;

	QString strStartDir = getenv("HOME");
	if (strStartDir.isEmpty() == true)
	{
		strStartDir = "/";
	}

  QString pathAndFilenameOfPPD = KFileDialog::getOpenFileName(strStartDir, "*.[pP][pP][dD]|Printer driver files (*.ppd, *.PPD)", this);

  if(pathAndFilenameOfPPD.isEmpty())
 	return;


	if ((result = Aps_AddModelFromPPD(
								(const char*)pathAndFilenameOfPPD
#ifdef QT_20
  .latin1()
#endif
                                                  ,
								&model))== APS_SUCCESS)
 	{
	//	fprintf(stderr, "\n OK, Aps_AddModelFromPPD() = %d",result);
    if ((result = Aps_GetPropertyString(model, "manufacturer",
					&manufactureName))
																			 == APS_SUCCESS)
		{
			/* Refresh the list of manufacturer and model names,
		 * and select the model
     * identified by the manufacturerName and modelName strings.
     */
  //    fprintf(stderr, "\n OK, Aps_GetPropertyString(maf) = %d",result);

			if ((result = Aps_GetPropertyString(model, "model", &modelName))
																				== APS_SUCCESS)
			{
        fprintf(stderr, "\n OK, Aps_GetPropertyString(model) = %d",result);
			}
			else
			{
        fprintf(stderr, "\n Err, Aps_GetPropertyString(model) = %d",result);
			}

    }
		else
		{
      fprintf(stderr, "\n Err, Aps_GetPropertyString(maf) = %d",result);
		}

		init_comboboxes();

		m_pManufacturer->setCurrentItem(indexOfItem(manufactureName,
																			m_pManufacturer));
		updateList((const char *)m_pManufacturer->currentText()
#ifdef QT_20
  .latin1()
#endif
    );
     m_pModel->setCurrentItem(indexOfItem(modelName,
																			m_pModel));
		//m_pModel->setCurrentItem(0);

		Aps_ReleaseBuffer(manufactureName);
    Aps_ReleaseBuffer(modelName);
		Aps_ReleaseHandle(model);
	}
	else
	{
    fprintf(stderr, "\n Err, Aps_AddModelFromPPD() = %d",result);
	}
}

void pageThree::show()
{
	CWizardPage::show();
	m_pManufacturer->setFocus();
}
