/* Name: commonfunc.cpp
            
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

#include <stdio.h>
//#include <kmsgbox.h>
#include "commonfunc.h"
#include "pageOne.h"
#include "pageTwo.h"
#include "pageThree.h"
#include "pageFour.h"
#include "pageFive.h"
#include "rename.h"
#define IM1 "/usr/X11R6/share/icons/print_wiz1.xpm"
#define IM2 "/usr/X11R6/share/icons/print_wiz2.xpm"

void setPrinterObject(const QString printerName,
												KPrinterObject * aPrinter)
{
 // fprintf(stderr,"\n\nCommonfunc---setPrinterObject\n");
//  fprintf(stderr,"printerName = %s\n", (char*)printerName);
  Aps_PrinterHandle printer;
	Aps_Result result;
	char *manufacture;
	char *model;
	char *location;
	Aps_ConnectionType connectionType;
	QString location1;
	Aps_JobAttrHandle jobAttributes;
	QString printerTypeName;
  char text[100];

  if (!aPrinter ||! printerName)
    return ;

	aPrinter->setPrinterName(printerName);
	if ((result = Aps_OpenPrinter((const char*)printerName
#ifdef QT_20
  .latin1()
#endif
                                                        , &printer)) == APS_SUCCESS)
	{
		//get info of connection & location
		if ((result = Aps_PrinterGetConnectInfo(printer, &connectionType,
																				&location)) == APS_SUCCESS)
		{
			//fprintf(stderr,"\nOK, Aps_PrinterGetConnectInfo()= %d\n", result);
      QString loc;
			loc.sprintf("%s",location);
			aPrinter->setLocation(loc);
			//Aps_ReleaseBuffer(location);
			if (connectionType == APS_CONNECT_LOCAL)
				aPrinter->setPrinterType(PRINTER_TYPE_LOCAL);
			else if (connectionType == APS_CONNECT_NETWORK_LPD)
			{
				aPrinter->setPrinterType(PRINTER_TYPE_UNIX);
				QString queue(location);
				queue.remove(0,1);
				int i = queue.find('/');
				queue.truncate(i);
				aPrinter->setQueue(queue);
			}
			else
				aPrinter->setPrinterType(PRINTER_TYPE_SMB);

		}
		else
		{
			 //fprintf(stderr, "\nErr, Aps_PrinterGetConnectInfo() = %d", result);
		}

		//get info of model and manufacture
  	if ((result = Aps_PrinterGetModel( printer,&manufacture, &model))
				 ==	APS_SUCCESS)
		{
			QString manuf;
			QString mod;
      manuf.sprintf("%s", manufacture);
			mod.sprintf("%s", model);
			aPrinter->setManufacture(manuf);
			aPrinter->setModel(mod);

			Aps_ReleaseBuffer(manufacture);
			Aps_ReleaseBuffer(model);
    }
		else
		{
			//fprintf(stderr,"\nErr, Aps_printerGetModel() = %d", result);
		}

		// get info is default
		int def;
		if ((result = Aps_PrinterIsDefault(printer,&def)) == APS_SUCCESS)
		{
			aPrinter->setDefaultPrinter((bool)def);
		}
		else
		{
      //fprintf(stderr,"\nErr, Aps_printerIsDefault() = %d", result);
		}

		// get file limit
		int size;
		if ((result = Aps_PrinterGetMaxJobSize(printer,&size))
																						== APS_SUCCESS)
		{
			aPrinter->setFileLimit(size/1000);
		}
		else
		{
       //fprintf(stderr,"\nErr, Aps_PrinterGetMaxJobSize() = %d\n", result);
		}

		//get info of an attributes
		char *setting;
		if ((result = Aps_PrinterGetDefAttr( printer, &jobAttributes))
					== APS_SUCCESS)
		{
			int colorDevice;
      if ((result = Aps_AttrQuickIsColorDevice(jobAttributes,
							                                 &colorDevice))
																							== APS_SUCCESS)
			{
				aPrinter->setIsColor(colorDevice>0? true:false);
			}
			else
			{
        //fprintf(stderr,"\n Err,Aps_AttrQuickIsColorDevice() = %d", result);
        Aps_GetResultText(result, text,100);
				//fprintf(stderr, "\n Err= %s",text);
			}

			// get colordepth or gray out the option
      if((result = Aps_AttrGetSetting(jobAttributes, "colorrendering",
																			&setting)) == APS_SUCCESS)
 			{
        aPrinter->setColorDepth(QString(setting));
				Aps_ReleaseBuffer(setting);
			}
			else
			{
				//fprintf(stderr, "\n Err, Aps_AttrGetSetting(ColorDepth) = %d", result);
				aPrinter->setColorDepth("");
        Aps_GetResultText(result, text,100);
				//fprintf(stderr, "\n Err= %s",text);
			}
			// get resolution or gray out the option
			Aps_Resolution ressetting;

      if ((result = Aps_AttrQuickGetRes(jobAttributes, &ressetting))
                                      == APS_SUCCESS)
			{
				QString res;
				res.sprintf("%dx%ddpi",(int)ressetting.horizontalRes,
																				(int)ressetting.verticalRes);

				aPrinter->setResolution(res);
			}
			else
			{
				//fprintf(stderr, "\n Err, Aps_AttrQuickGetRes(Resolution) = %d", result);
        Aps_GetResultText(result, text,100);
				//fprintf(stderr, "\n Err= %s",text);
        aPrinter->setResolution("");
			}

			// get paper size
			Aps_PageSize *pageSize;
			if ((result = Aps_AttrQuickGetPageSize(jobAttributes, &pageSize))
                                      == APS_SUCCESS)
			{
				QString name;
				name = QString (pageSize->id);
				//fprintf(stderr, "\npageSize->id = %s",(char*)name);
 				aPrinter->setPaperSize(name);
				if ((result = Aps_ReleaseBuffer(pageSize)) == APS_SUCCESS)
				{
					//fprintf(stderr, "\n OK, Aps_ReleaseBuffer(pageSize)");
				}
				else
				{
          //fprintf(stderr, "\n Err, Aps_ReleaseBuffer(pageSize) = %d", result);
 				}
			}
			else
			{
				//fprintf(stderr, "\n Err, Aps_AttrQuickGetPageSize() = %d",result);
        Aps_GetResultText(result, text,100);
				//fprintf(stderr, "\n Err= %s",text);
 				aPrinter->setPaperSize("");
			}

			// get pageformat
			char *setting1;
      if ((result = Aps_AttrGetSetting(jobAttributes, "n-up", &setting1))
                                       == APS_SUCCESS)
			{
				QString fp;
				fp.sprintf("%s",setting1);
        aPrinter->setFormatPages(setting1);
				Aps_ReleaseBuffer(setting1);
			}
			else
			{
				//fprintf(stderr, "\n Err, Aps_AttrGetSetting(n-up) = %d", result);
        Aps_GetResultText(result, text,100);
				//fprintf(stderr, "\n Err=%s",text);
				aPrinter->setFormatPages("");
			}

			// get margins
     // if ((result = Aps_AttrGetSetting(jobAttributes, "LeftMargin",
			//																 &setting)) == APS_SUCCESS)
      if ((result = Aps_AttrGetSetting(jobAttributes, "RightMargin",
																				&setting)) == APS_SUCCESS)
			{
        aPrinter->setHorizontalMargin(QString(setting));
				Aps_ReleaseBuffer(setting);
			}
			else
			{
				//fprintf(stderr, "\n Err, Aps_AttrGetSetting(Rightmargin) = %d", result);
        Aps_GetResultText(result, text,100);
				//fprintf(stderr, "\n Err=%s",text);
				aPrinter->setHorizontalMargin("");
			}
//      if ((result = Aps_AttrGetSetting(jobAttributes, "TopMargin",
//																				&setting)) == APS_SUCCESS)
      if ((result = Aps_AttrGetSetting(jobAttributes, "BottomMargin",
																							&setting)) == APS_SUCCESS)
			{
        aPrinter->setVerticalMargin(QString(setting));
				Aps_ReleaseBuffer(setting);
			}
			else
			{
				//fprintf(stderr, "\n Err, Aps_AttrGetSetting(Bottommargin) = %d", result);
        Aps_GetResultText(result, text,100);
				//fprintf(stderr, "\n Err=%s",text);
				aPrinter->setVerticalMargin("");
			}

			// get GhostScript options
			 if ((result = Aps_AttrGetSetting(jobAttributes, "gsoptions",
																													&setting))
								 																			== APS_SUCCESS)
			{

				QString gh(setting);
        aPrinter->setGhostScript(gh);
				if (setting)
					Aps_ReleaseBuffer(setting);
			}
			else
			{
				//fprintf(stderr, "\n Err, Aps_AttrGetSetting(GhostScript) = %d", result);
        Aps_GetResultText(result, text,100);
				//fprintf(stderr, "\n Err=%s",text);
				aPrinter->setGhostScript("");
			}

			// get flag's value
			long int configFlag;
      if ((result = Aps_PrinterGetConfigFlags(printer, &configFlag))
																						 == APS_SUCCESS)
			{
        if (configFlag & APS_CONFIG_EOF_AT_END)
				{
					aPrinter->setSendEof(true);
				}
				else
				{
					aPrinter->setSendEof(false);
        }

				if (configFlag & APS_CONFIG_ADD_CR)
				{
					aPrinter->setFixStair(true);
				}
				else
				{
          aPrinter->setFixStair(false);
				}

				if (configFlag & APS_CONFIG_TEXT_AS_TEXT)
				{
          aPrinter->setFastTextPrinting(true);
				}
				else
				{
          aPrinter->setFastTextPrinting(false);
				}

				if (configFlag & APS_CONFIG_HEADER_PAGE)
				{
          aPrinter->setHeaderPage(true);
				}
				else
				{
          aPrinter->setHeaderPage(false);
				}
			}
			else
			{
       	//fprintf(stderr, "\n Err, Aps_PrinterGetConfigFlags() = %d", result);
			}
			Aps_ReleaseHandle(jobAttributes);
		}
		else
		{
			//fprintf(stderr,"\nErr, Aps_printerGetDefAttr() = %d\n", result);

		}
		Aps_ReleaseHandle(printer);
	} //end of openprinter
	else
	{
		//fprintf(stderr,"\nErr, Aps_OpenPrinter() = %d\n", result);
	}//end of Aps_OpenPrinter()
}

bool administrator()
{
  if (getuid () == 0)
		return true;
  else
		return false;

}

bool init(CPrinterWizard* pw)
{
	bool bRet = true;
	static int i = 0;
	char str[80];

	sprintf(str,(const char *)SET_P_TYPE
#ifdef QT_20
  .latin1()
#endif
  );
	pageOne* pg1 = new pageOne( i++, false, -1, -1, IM1, str, true, pw );
	pw->addPage(pg1, eButtonNext | eButtonHelp | eButtonCancel,
								   eButtonNext | eButtonHelp | eButtonCancel,
 									 true);

 	sprintf(str, (const char *)SET_P_ATTRIBUTE
#ifdef QT_20
  .latin1()
#endif
                                                  );
	pageTwo* pg2 = new pageTwo( i++, false, -1, -1, IM2, str, true, pw );
	pw->addPage(pg2,eButtonPrev | eButtonNext | eButtonHelp | eButtonCancel,
                  eButtonPrev | eButtonNext | eButtonHelp | eButtonCancel,
 									false);

	sprintf(str, (const char *)SET_P_MODE
#ifdef QT_20
  .latin1()
#endif
  );
///////////////modify here for adding page four//////////////
//	pageThree* pg3 = new pageThree( i++, true, -1, -1, IM2, str, true, pw );
	pageThree* pg3 = new pageThree( i++, false, -1, -1, IM2, str, true, pw );
	pw->addPage(pg3,eButtonPrev | eButtonNext | eButtonHelp |eButtonCancel,
                  eButtonPrev | eButtonNext | eButtonHelp | eButtonCancel,
									false);

 //// if printer type is local, set share or not share////////////
/*	sprintf(str, "Set the sharebility");
	pageFive* pg5 = new pageFive( i++, false, -1, -1, IM2, str, true, pw );
	pw->addPage(pg5,eButtonPrev | eButtonNext | eButtonHelp|eButtonCancel,false);
 */
//disabled because has not finished
/////////////////////////////////////////////////////////////////
	sprintf(str,(const char *)SET_P_TEST
#ifdef QT_20
  .latin1()
#endif
  );
	pageFour* pg4 = new pageFour( i++, true, -1, -1, IM2, str,true, pw );
	pw->addPage(pg4,eButtonPrev | eButtonNext | eButtonHelp|eButtonCancel|eButtonDone ,
                  eButtonPrev | eButtonNext | eButtonHelp|eButtonCancel|eButtonDone ,
 									false);
 /* pw->addPage(pg4,eButtonPrev | eButtonDone | eButtonHelp|eButtonCancel,
                  eButtonPrev | eButtonDone | eButtonHelp|eButtonCancel,
									false);   */
//printf("\n getcount = %d\n\n", pw->getCount());
	return bRet;
}


void addAPrinter()
{
  displayID();  //just for testing
	KPrinterObject *aNewPrinter;
  CPrinterWizard *wiz_dlg;

  wiz_dlg = new CPrinterWizard( eButtonPrev, eButtonNext,eButtonCancel,
 										eButtonDone,eButtonHelp, false, 0, "New printer" );
	init(wiz_dlg);
 // fprintf(stderr, "\n before wiz_dlg->exec()");
	wiz_dlg->setCaption(CAPTION);
	wiz_dlg->setDefaultButton(eButtonNext);
	wiz_dlg->updatePrinterCombos();

  if ( wiz_dlg->exec())
  {
		aNewPrinter = new KPrinterObject();
		switch(wiz_dlg->getPrinterType())
		{
			case  0:
    		aNewPrinter->setPrinterType(PRINTER_TYPE_LOCAL);
        break;
      case 1:
    		aNewPrinter->setPrinterType(PRINTER_TYPE_UNIX);
  	    break;
      case 2:
    		aNewPrinter->setPrinterType(PRINTER_TYPE_SMB);
  	    break;
		}

    wiz_setPrinterInfo(wiz_dlg, aNewPrinter );
	}
}

void wiz_setPrinterInfo(CPrinterWizard *wiz_dlg, KPrinterObject * aPrinter)
{
//fprintf(stderr, "\n get into wiz_setPrinterInfo");
	Aps_PrinterHandle printer;
//	Aps_JobHandle jobHandle;
	Aps_JobAttrHandle jobAttributeHandle;
	Aps_Result result;
	Aps_Resolution **resolutionArray;
	Aps_AttrOption **options;
	Aps_PageSize **pageSizes;
	int numPageSizes;
	int numOptions;
	double minSetting;
	double maxSetting;

  char name[50];
	memset(name, '\0',50);
	if ( !wiz_dlg || !aPrinter )
    return;

	aPrinter->setPrinterName( wiz_dlg->getNickName() );
	aPrinter->setFileLimit( 0 );
	aPrinter->setPrintingTestPage(wiz_dlg->getPrintTestPage());
	strcpy(name, (const char*)aPrinter->getPrinterName()
#ifdef QT_20
  .latin1()
#endif
                                                          );
 	if ((result = Aps_AddPrinter(name, &printer))== APS_SUCCESS)
	{
  	switch( aPrinter->getPrinterType() )
  	{
  		case PRINTER_TYPE_LOCAL :
  		{
        //STRTRACE("Entered Printer type local\n")

  	    aPrinter->setLocation(wiz_dlg->getLocation() );
				if ((result = Aps_PrinterSetConnectInfo(printer, APS_CONNECT_LOCAL,
												(const char*)wiz_dlg->getLocation()
#ifdef QT_20
  .latin1()
#endif
                                                            ))	!= APS_SUCCESS)
				{
          //fprintf(stderr, "\nErr,Aps_PrinterSetConnectInfo = %d",result);
				}
	 		}
   		break;
  		case PRINTER_TYPE_UNIX:
			{

  	    // Unix specific options
				QString location(wiz_dlg->getLocation());
			  for (uint i = 0; i<location.length();i++)
			 	{
   				if (location.at(i) == '\\')
						location.replace(i,1,"/");
					//printf("\nlocation = %s", (char*)location);
			  }
  	    aPrinter->setLocation(location );
			  if ((result = Aps_PrinterSetConnectInfo(printer,
																							 APS_CONNECT_NETWORK_LPD,
				                               (const char*)aPrinter->getLocation()
#ifdef QT_20
  .latin1()
#endif
                                                                           ))
																								!= APS_SUCCESS)
				{
          //fprintf(stderr, "\nErr,Aps_PrinterSetConnectInfo = %d",result);
				}
				aPrinter->setQueue(wiz_dlg->getQueueName());
  		}
  		break;
  		case PRINTER_TYPE_SMB:
  		{
       	QString location(wiz_dlg->getLocation());
			 	for (uint i = 0; i<location.length();i++)
			 	{
   				if (location.at(i) == '\\')
						location.replace(i,1,"/");
					//fprintf(stderr,"\nlocation = %s", (char*)location);
			 	}
  	   	aPrinter->setLocation(location );
				if ((result = Aps_PrinterSetConnectInfo(printer,
																							 APS_CONNECT_NETWORK_SMB,
				                                (const char*)aPrinter->getLocation()
#ifdef QT_20
  .latin1()
#endif
                                                                            ))
																								!= APS_SUCCESS)
				{
          //fprintf(stderr, "\nErr,Aps_PrinterSetConnectInfo = %d",result);
				}
  		}
  		break;
		  default:
	  	return;
		}
		if ((result = Aps_PrinterSetMaxJobSize(printer, 0))
																								!= APS_SUCCESS)
		{

      //fprintf(stderr, "\nErr,Aps_PrinterSetMaxJobSize = %d",result);
		}
		aPrinter->setManufacture(wiz_dlg->getManufacture());
		aPrinter->setModel(wiz_dlg->getModel());
		if ((result = Aps_PrinterSetModel(printer,
																(const char*)wiz_dlg->getManufacture()
#ifdef QT_20
  .latin1()
#endif
                                                                      ,
																(const char*)wiz_dlg->getModel()
#ifdef QT_20
  .latin1()
#endif
                                                                          ))
																								!= APS_SUCCESS)
		{

       //fprintf(stderr, "\nErr,Aps_PrinterSetModel = %d",result);
		}
		QString type(wiz_dlg->getManufacture());
		type +=	" ";
		type += wiz_dlg->getModel();

		if (aPrinter->printingTestPage())
		{
			if((result = Aps_PrinterSendTestPage(printer,NULL))// &jobHandle))//NULL))
						!= APS_SUCCESS)
			{
					//fprintf(stderr, "\nErr, Aps_PrinterSendTestPage() = %d", result);
			}
    }

		//set default flags for a new printer, turn off all flags
		if ((result = Aps_PrinterSetConfigFlags(printer,0,
		APS_CONFIG_EOF_AT_END|APS_CONFIG_ADD_CR
		|APS_CONFIG_TEXT_AS_TEXT|APS_CONFIG_HEADER_PAGE))
									!= APS_SUCCESS)
		{

      //fprintf(stderr,"\n Err,Aps_PrinterSetConfigFlags =%d",result);
		}


		if ((result = Aps_PrinterGetDefAttr(printer, &jobAttributeHandle))
																				== APS_SUCCESS)
		{
			int colorDevice;
      // check is color device?
			if ((result = Aps_AttrQuickIsColorDevice(jobAttributeHandle,
							                                 &colorDevice))
																							== APS_SUCCESS)
			{
				aPrinter->setIsColor(colorDevice == 1? true:false);
			}
			else
			{
        //fprintf(stderr,"\n Err,Aps_AttrQuickIsColorDevice() = %d", result);
			}

			///check is there colordepth and resolutions available
			///set the first one as default if available
      if ((result = Aps_AttrQuickGetResOptions(jobAttributeHandle,
																										&resolutionArray,
																										&numOptions))
 																									== APS_SUCCESS)
 			{

        if (numOptions>0)
				{
					QString res;
					res.sprintf("%dx%ddpi",(int)resolutionArray[0]->horizontalRes,
                               (int)resolutionArray[0]->verticalRes);
					aPrinter->setResolution(res);
//modify for setting printer's attributes
        Aps_AttrSetSetting(jobAttributeHandle,"*Resolution",(const char*)res
#ifdef QT_20
  .latin1()
#endif
                                                                                );
				}
				if (resolutionArray)
        	Aps_ReleaseBuffer(resolutionArray);

			}
			else
	    {
			  aPrinter->setResolution("");
			}

			if ((result = Aps_AttrGetOptions(jobAttributeHandle,
																									"colorrendering",
                                                  &numOptions,
																									&options))
																									== APS_SUCCESS)
			{
				QString opt;
				if (numOptions>0)
				{
					opt.sprintf("%s",options[0]->optionID);
					aPrinter->setColorDepth(opt);
				}
        if (options)
        	Aps_ReleaseBuffer(options);
//modify for setting printer's attributes
        Aps_AttrSetSetting(jobAttributeHandle,"colorrendering",
																								(const char*)opt
#ifdef QT_20
  .latin1()
#endif
                                                                );
 			}
			else
					aPrinter->setColorDepth("");
			// 3- getPaperSize options
	    if ((result = Aps_AttrQuickGetPageSizeOptions(jobAttributeHandle,
								 																				&pageSizes,
																										&numPageSizes))
																										== APS_SUCCESS)
			{

				if (numPageSizes>0)
				{
					QString ps;
					ps.sprintf("%s",pageSizes[0]->id);
					aPrinter->setPaperSize(ps);
				}
				if (pageSizes)
					Aps_ReleaseBuffer(pageSizes);
      }
      else
					aPrinter->setPaperSize("");
			// 4--- pageFormat
      if ((result = Aps_AttrGetOptions(jobAttributeHandle, "n-up",
																				&numOptions,
																				&options))
																				== APS_SUCCESS)
			{

				if (numOptions>0)
				{
					QString fmt;
					fmt.sprintf("%s",options[0]->optionID);
          //fmt.sprintf("%s [%s]",options[0]->optionID,
					//	options[0]->translatedName);
						aPrinter->setFormatPages(fmt);
//modify for setting printer's attributes
        Aps_AttrSetSetting(jobAttributeHandle,"n-up", (const char*)fmt
#ifdef QT_20
  .latin1()
#endif
                                                                        );
 				}
				if (options)
					Aps_ReleaseBuffer(options);

			}
			else
      	aPrinter->setFormatPages("");
			// obtain the margin limit
			if ((result = Aps_AttrGetRange(jobAttributeHandle, "LeftMargin",
																				&minSetting, &maxSetting))
																				== APS_SUCCESS)
			{
					QString margin;
					margin.sprintf("%f",maxSetting);
	//fprintf(stderr, "\nmaxSetting = %f", maxSetting);
					aPrinter->setHorizontalMargin(margin);

					if ((result = Aps_AttrGetRange(jobAttributeHandle, "TopMargin",
																				&minSetting, &maxSetting))
																				== APS_SUCCESS)
					{
						QString margint;
						margint.sprintf("%f",maxSetting);
						aPrinter->setVerticalMargin(margint);
         	}
//modify for setting printer's attributes
        Aps_AttrSetSetting(jobAttributeHandle,"LeftMargin",
																					(const char*)margin
#ifdef QT_20
  .latin1()
#endif
                                                               );
        Aps_AttrSetSetting(jobAttributeHandle,"RightMargin",
																					(const char*)margin
#ifdef QT_20
  .latin1()
#endif
                                                               );
        Aps_AttrSetSetting(jobAttributeHandle,"TopMargin",
																					(const char*)margin
#ifdef QT_20
  .latin1()
#endif
                                                              );
        Aps_AttrSetSetting(jobAttributeHandle,"BottomMargin",
																					(const char*)margin
#ifdef QT_20
  .latin1()
#endif
                                                                );

    	 }
			 else
			 {
				 aPrinter->setHorizontalMargin("");
         aPrinter->setVerticalMargin("");
			 }
			// get the ghostscript options
			char *settings;
			if ((result = Aps_AttrGetSetting(jobAttributeHandle, "gsoptions",
																				&settings))
																				== APS_SUCCESS)
			{

				QString ghost;
				ghost.sprintf("%s",settings);
				aPrinter->setGhostScript(ghost);
				if (settings)
					Aps_ReleaseBuffer(settings);
			}
    	else
			  aPrinter->setGhostScript("");

			/////////////////////end of set default value///////////////////
			if ((result = Aps_PrinterSetDefAttr(printer, jobAttributeHandle))
																				!= APS_SUCCESS)
			{
	      //fprintf(stderr, "\nErr, Aps_PrinterSetDefAttr = %d",result);
			}
	  	Aps_ReleaseHandle(jobAttributeHandle);
		}
		else
		{
      //fprintf(stderr,"\n Err,Aps_PrinterGetDefAttr() = %d", result);
		}

		if ((result = Aps_ReleaseHandle(printer)) != APS_SUCCESS)
		{
			//fprintf(stderr,"\n Err,Aps_ReleaseHandle(printer) = %d", result);
		}
	}
	else
	{
		//fprintf(stderr, "\nErr, Aps_AddPrinter() = %d", result);
	}
}

void deleteAPrinter(const char *printerName)
{
  displayID();//just for testing
  Aps_PrinterHandle printer;
	Aps_Result result;
	if ((result = Aps_OpenPrinter(printerName,
																			&printer)) == APS_SUCCESS)
	{
		if ((result = Aps_PrinterRemove(printer))
						== APS_SUCCESS)
		{
			;
		}
		else
		{
			//fprintf(stderr, "\n Err, Aps_PrinterRemove(printer) = %d", result);
		}
	}
	else
	{
		//fprintf(stderr, "\n deleteAPriner:Err, Aps_OpenPrinter() = %d", result);
	}
}

// Get the filenames from the diretory provided by path matching
// a given pattern. Buffer has to be provided by user.
// buffer_size is the number of lines that can be saved in the buffer

int GetCommandOutput(char *buffer[], int buffer_size, bool newLineTermination, char* tool,
																char* path, char *options)
{
	char cmdLine[CMD_WIDTH];
	char dataLine[LINE_WIDTH];
	FILE* fp;
	if((buffer == NULL) || (buffer_size < 1))
	{
		// do some clean up if required and bail out
		//fprintf(stderr,"GetCommandOutput -- NULL buffer pointer");
    return -1;
	}
	if ( tool == NULL )
		tool = LIST_TOOL; // defaulting to ls
	//if path is not null then change directory to that
	if( path != NULL )
	{
		if(chdir(path))
		{
			//fprintf(stderr,"GetCommandOutput -- Unable to change directory");
		}
	}
	// prepare the command string to be given to popen
	if( options != NULL )
		sprintf( cmdLine,"%s %s", tool, options);
	else
		sprintf( cmdLine,"%s ", tool);
	if ( (fp = popen( cmdLine, "r")) == NULL ) // we just read the directory
	{
		//fprintf(stderr,"GetDirectoryContentbyPattern -- Unable to open pipe");
		return -1;
	}
	int count = 0;
	while( (fgets(dataLine, LINE_WIDTH, fp) != NULL) )
	{
		// copy the data to provided buffer
		int i = strlen(dataLine);
		if(!newLineTermination)
	   dataLine[i-1] = '\0';
		char* temp = (char*) malloc(i*2);
		memset( temp, 0, i);
		strncpy( temp, dataLine, i);
		buffer[count++] = temp;
	}
	pclose(fp);
	return count;
}

QStrList allPrintersName()
{
	QStrList printersnames;
	printersnames.clear();
  char **printerNames;
	int size;
	int count;
	Aps_Result result;

	if ((result = Aps_GetPrinters(&printerNames, &size)) == APS_SUCCESS)
	{
  	printf("Size = %d\n", size);

		for (count = 0; count < size; count++)
		{
			printersnames.append(printerNames[count]);
		}
		Aps_ReleaseBuffer(printerNames);
	}
	//else
	//	fprintf(stderr, "\n Aps_GetPrinters() = %d",result);

	return printersnames;
}

bool uniquePrinterName(const char *newName)
{
  bool bUniqueName = TRUE;
	int flag = 1;
	QStrList allNames = allPrintersName();

	for (uint i = 0; i< allNames.count(); i++)
  {
	  if (!strcmp((char*)allNames.at(i), newName))
		{
      bUniqueName = FALSE;
			flag = 0;
     // break;
    }
		if (flag == 0)
			break;
	}
  return bUniqueName;
}


bool uniqueNewName(const char *newName, const char* oldName)
{
  bool bUniqueName = TRUE;
	int flag = 1;
	QStrList allNames = allPrintersName();

	for (uint i = 0; i< allNames.count(); i++)
  {
	  if ( strcmp((char*)allNames.at(i), oldName) &&
				!strcmp((char*)allNames.at(i), newName))
		{
      bUniqueName = FALSE;
			flag = 0;
     // break;
    }
		if (flag == 0)
			break;
	}
  return bUniqueName;
}

bool validFileName( const QString& strNameToValidate )
{
  bool bValidString = TRUE;
  static QString strTempValidate;
  if ( !strNameToValidate.isEmpty() )
  {
    strTempValidate = strNameToValidate.lower();
    //if ( ( strTempValidate[0] >='a' ) && ( strTempValidate[0] <='z' ) )
#ifndef QT_20
		if ( isalpha(strTempValidate[0])|| isdigit(strTempValidate[0]) )
    {
      int nStringLength = strTempValidate.length();
      for ( int i=1; i < nStringLength; i++ )
      {
        if ( !isalpha(strTempValidate[i]) && !isdigit( strTempValidate[i]) &&
             ( strTempValidate[i] != '_' ) && ( strTempValidate[i] != '-' ))
        {
          bValidString = FALSE;
          break;
        }
      }
    }
    else
      bValidString = FALSE;
//commented by alexandrm
#endif
  }
  else
    bValidString = FALSE;
  return bValidString;
}

void keepOldInfo(KPrinterObject * aPrinter)
{

	if (!aPrinter)
    return;

  // setting the General info
  Aps_PrinterHandle printerHandle;
	Aps_JobAttrHandle jobAttributeHandle;
  Aps_Result result;
  QString loc;
	QString printerName;
	char *oldPrinterName;
	oldPrinterName = new char[(aPrinter->getPrinterName()).length()+1];
	strcpy(oldPrinterName, (const char*)aPrinter->getPrinterName()
#ifdef QT_20
  .latin1()
#endif
    );
	//fprintf(stderr,"\nprinterName = %s",oldPrinterName);
	if ((result = Aps_OpenPrinter(oldPrinterName, &printerHandle))
															== APS_SUCCESS)
	{

				if ((result = Aps_PrinterGetDefAttr(printerHandle, &jobAttributeHandle))
																					== APS_SUCCESS)
				{

						if ((result = Aps_AttrSetSetting(jobAttributeHandle,
																					"colorrendering",
																					(const char*)aPrinter->getColorDepth()
#ifdef QT_20
  .latin1()
#endif
                                          ))
																					!= APS_SUCCESS)
						{
            	//fprintf(stderr, "\n Err, Aps_AttrSetSetting(ColorDepth) = %d", result);
 						}


						if ((result = Aps_AttrSetSetting(jobAttributeHandle,
																					"*Resolution",
																					(const char*)aPrinter->getResolution()
#ifdef QT_20
  .latin1()
#endif
                                          ))
																					!= APS_SUCCESS)
						{
            	//fprintf(stderr, "\n Err, Aps_AttrSetSetting(ColorDepth) = %d", result);
 						}


						if ((result = Aps_AttrSetSetting(jobAttributeHandle,
																					"n-up",
																					(const char*)aPrinter->getFormatPages()
#ifdef QT_20
  .latin1()
#endif
                                          ))
																					!= APS_SUCCESS)
						{
            	//fprintf(stderr, "\n Err, Aps_AttrSetSetting(PageFormat) = %d", result);
 						}

					// set page size

						if ((result = Aps_AttrSetSetting(jobAttributeHandle,
																					"*PageSize",
																					(const char*)aPrinter->getPaperSize()
#ifdef QT_20
  .latin1()
#endif
                                          ))
																					!= APS_SUCCESS)
						{

            	//fprintf(stderr, "\n Err, Aps_AttrSetSetting(PageSize) = %d", result);
 						}


					// set margin

						if ((result = Aps_AttrSetSetting(jobAttributeHandle,
																					"TopMargin",
																					(const char*)aPrinter->getVerticalMargin()
#ifdef QT_20
  .latin1()
#endif
                                          ))
																					!= APS_SUCCESS)
						{
            	//fprintf(stderr,"\n Err,Aps_AttrSetSetting(VerticalMargin) =%d", result);
 						}
          	if ((result = Aps_AttrSetSetting(jobAttributeHandle,
																					"BottomMargin",
																					(const char*)aPrinter->getVerticalMargin()
#ifdef QT_20
  .latin1()
#endif
                                          ))
																					!= APS_SUCCESS)
						{
            	//fprintf(stderr, "\n Err,Aps_AttrSetSetting(VerticalMargin)=%d",result);
						}
						if ((result = Aps_AttrSetSetting(jobAttributeHandle,
																					"LeftMargin",
																					(const char*)aPrinter->getHorizontalMargin()
#ifdef QT_20
  .latin1()
#endif
                                          ))
																					!= APS_SUCCESS)
						{
            	//fprintf(stderr,"\n Err,Aps_AttrSetSetting(HorizontalMargin)=%d", result);
  					}
 						if ((result = Aps_AttrSetSetting(jobAttributeHandle,
																					"RightMargin",
																					(const char*)aPrinter->getHorizontalMargin()
#ifdef QT_20
  .latin1()
#endif
                                          ))
																					!= APS_SUCCESS)
						{
            	//fprintf(stderr, "\nErr,Aps_AttrSetSetting(HorizontalMargin)=%d", result);
						}

          // GhostScript

    				if ((result = Aps_AttrSetSetting(jobAttributeHandle,
																				 "gsoptions",
																					(const char*)aPrinter->getGhostScript()
#ifdef QT_20
  .latin1()
#endif
                                          ))
																					!= APS_SUCCESS)
						{
          		//fprintf(stderr,"\nErr,Aps_AttrSetSetting(GhostScript)=%d", result);
 						}
					//set the default value
						if ((result = Aps_PrinterSetDefAttr(printerHandle, jobAttributeHandle))
																				!= APS_SUCCESS)
						{
	      			//fprintf(stderr, "\nErr, Aps_PrinterSetDefAttr = %d",result);
						}

						if (jobAttributeHandle)
	  					Aps_ReleaseHandle(jobAttributeHandle);
				}
				else
				{
          //fprintf(stderr, "\n Err, Aps_PrinterGetDefAttr() = %d", result);
				}

			}//end of Aps_OpenPrinter()
			if (printerHandle)
				Aps_ReleaseHandle(printerHandle);
}


void setAsDefaultPrinter(const char* printerName)
{
	//displayID();//just for testing
  Aps_PrinterHandle printer;
	Aps_Result result;
	if ((result = Aps_OpenPrinter(printerName,
																&printer)) == APS_SUCCESS)
	{
		if ((result = Aps_PrinterSetAsDefault(printer))
						!= APS_SUCCESS)
		{
			//fprintf(stderr, "\n Err, Aps_PrinterSetAsDefault(printer) = %d", result);
		}
	}
	else
	{
		//fprintf(stderr, "\n setAsDefault: Err, Aps_OpenPrinter() = %d", result);
	}
  if (printer)
				Aps_ReleaseHandle(printer);
}

void displayID()
{
 //fprintf(stderr,"\n\n=====>uid = %d, euid = %d",getuid(), geteuid());
}

bool varifyNickName(const QString& strName)
{
//	fprintf(stderr, "\n varifyNickName");
	static QString fieldValue;
  static QString vError(WARNING_CAPTION);
  static QString tmpError;
	int j;
 // KPrinterObjects *allprinters = pr->getAllPrinters();
 /////////////this part need reimplementation
  if (!validFileName(strName))
  {
  	tmpError.sprintf((const char *)i18n("The name for the printer contains illegal characters.\n Please try again.")
#ifdef QT_20
  .latin1()
#endif

    );
#ifndef QT_20
    KMsgBox::message(0, vError.data(), WARNING1, KMsgBox::EXCLAMATION,OK_BUTTON);
// commented by alexandrm
#endif

    return false;
  }

  if(!uniquePrinterName((const char*)strName
#ifdef QT_20
  .latin1()
#endif
                                            ))
  {
#ifndef QT_20
   	KMsgBox::message(0, vError.data(),WARNING2, KMsgBox::EXCLAMATION,OK_BUTTON);
// commented by alexandrm
#endif

     return false;
  }

 	char array[7][5] = {"lp", "lpr", "lpq", "lprm", "lp0", "lp1", "lp2"};
  for (j =0; j < 7; j++)
  {
  	if (!strcmp((const char *)strName
#ifdef QT_20
  .latin1()
#endif
    , array[j]))
  	{
  		tmpError.sprintf((const char *)i18n("The name for the printer can not be lp\\lpq\\lpr\\lprm\\lp0\\lp1\\lp2.\n Please try again.")
#ifdef QT_20
  .latin1()
#endif
      );
#ifndef QT_20
    	KMsgBox::message(0, vError.data(), WARNING3, KMsgBox::EXCLAMATION, OK_BUTTON);

// commented by alexandrm
#endif
   	 return false;
   	 }
  }

  return true;

}

void rename_printer(const QString & oldName)
{
  Crenamedlg *rn = new Crenamedlg(NULL, "rename", oldName);
  rn->exec();
	//if (rn->exec())
	//{
	//}
}

