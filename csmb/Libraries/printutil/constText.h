/* Name: constText.h
            
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

//define the const label/message text with i18n()
#include <kapp.h>
#include <klocale.h>

#define CAPTION					i18n("Add Printer")
/// defined for print.cpp file ////
#define PRINTERNAME			i18n("Printer Name")
#define HOSTNAME				i18n("Host Name")
#define INSTALLEDPRINT  i18n("&Installed Printers:")
#define PROPERTY				i18n("P&roperties")
#define ADD             i18n("A&dd..."
#define REMOVE					i18n("Re&move")
#define SETDEFAULT			i18n("&Set as Default")
///defined for property dialog --GeneralDlg ///
#define GENERAL					i18n("&General")
#define NAME            i18n("&Name:")
#define HOSTNAME1				i18n("&Host Name:")
#define QUEUE           i18n("&Queue:")
#define LIMIT						i18n("Limit printed files to under")
#define DEVICES					i18n("De&vice:")
#define PRINTER1				i18n("&Printer:")
#define USER2						i18n("&User:")
#define PASSWD1					i18n("Pass&word:")
///defined for property dialog --OutputDlg ///
#define OUTPUT					i18n("O&utput")
#define PRINTER_TYPE_1		i18n("Printer &type:")
#define COLOR_DEPTH_1			i18n("&Color depth:")
#define RESOLUTION_1			i18n("&Resolution:")
#define PRINT_HEADER_1		i18n("&Print header page")
#define PAGE_SETUP_1			i18n("Page Setup")
#define PAPER_SIZE_1			i18n("Paper &size:")
#define FORMAT_PAGES_1		i18n("For&mat pages:")
#define MARGINS					i18n("Margins")
#define HORIZONTAL_1			i18n("&Horizontal:")
#define VERTICAL_1				i18n("&Vertical:")
#define INCHES					i18n("inches")
#define PIXEL						i18n("pixels")
///defined for property dialog --AdvancedDlg ///
#define ADVANCED				i18n("&Advanced")
///defined for property dialog --sharedDlg ///
#define SHARED					i18n("&Shared")
#define SEND_EOF				i18n("&Send EOF at end of print job")
#define FIX_STAIR				i18n("&Fix stair-stepping text")
#define FAST_TEXT_PRINT	i18n("Fast &text printing")
#define GHOSTSCRIPT			i18n("G&hostscript options:")
///defined for property dialog --jobQueueDlg ///
#define JOBQUEUE				i18n("&Job Queue")
#define OWNER						i18n("Owner")
#define JOB							i18n("Job Name")
#define FILE_1						i18n("File")
#define TIME						i18n("Time")
#define SIZE_1					i18n("Size")
#define REMOVE_BUTTON		i18n("&Remove")
///defined for wizard --pageOne///
#define SET_P_TYPE			i18n(" Set Printer Type")
#define LABEL1					i18n("You have chosen to add a printer.\nHow would you like this printer to be managed?")
#define LOCALLY					i18n("&Locally on my Computer")
#define REMOTELY				i18n("&Remotely on the Network")
///defined for wizard --pageTwo --local///
#define SET_P_ATTRIBUTE	i18n(" Set Printer Attributes")	
#define LABEL21					i18n("Give your printer name and select a port for output to be sent through to the printer." )
#define LABEL2					i18n("Provide a nickname for the printer.\nType, or browse for the hostname.")
#define NICK_NAME				i18n("N&ickname:")
#define DEVICE_OR_PORT	i18n("Device or Port:")
///defined for wizard --pageTwo --remote///
#define HOSTNAME2				i18n("H&ostname:")
#define NETWORK_TYPE		i18n("Network type")
#define WINDOWS					i18n("&Windows")
#define UNIX1						i18n("&Unix")     										
#define BROWSE					i18n("&Browse...")
#define WARNING_CAPTION i18n("Validation Error")
#define WARNING1				i18n("The name for the printer contains illegal characters\nor is empty. Please try again.")
#define WARNING2				i18n("The name for the printer must be Unique.\n Please type a different name.")
#define WARNING3				i18n("The name for the printer can not be lp\\lpq\\lpr\\lprm\\lp0\\lp1\\lp2.\n Please try again.")
#define WARNING4				i18n("Please specify a valid device name.\n ex: '/dev/lp1'.")
#define WARNING5 				i18n("Please specify a Host name for the remote printer.")
#define WARNING6 				i18n(" REMOTE PRINTER NAME CAN NOT BE EMPTY")
#define WARNING_Q1				i18n("Please specify a valid name for the remote \
printer queue.")
#define WARNING_Q2			  i18n("The name for the remote queue contains illegal \
characters. \n Please try again.")
#define WARNINGH				i18n("Please specify a valid host name.")
#define OK_BUTTON		i18n("OK")
#define HAVE_DISK				i18n("&Have PPD...")
#define QUEUENAME				i18n("&Queue: ")
///defined for wizard --pageThree ///
#define SET_P_MODE			i18n(" Set Printer Model")
#define LABEL3					i18n("Choose the printer manufacturer and model.\nIf your printer is not listed, \
consult your printer \ndocumentation for a compatible printer.")
#define MANUFACTURER		i18n("&Manufacturer:")
#define PRINTER_MODE		i18n("M&odel:")
#define PRINTER_FILTER  i18n("Printer filter:")
///defined for wizard -- button ////
#define IDS_PREV1				i18n("<Back")
#define IDS_NEXT1				i18n("Next>")
#define IDS_HELP1				i18n("Help")
#define IDS_CANCEL1			i18n("Cancel")
#define IDS_DONE1				i18n("Finish")
///defined for wizard --pageFour ///
#define SET_P_TEST			i18n(" Print test page")
#define LABEL4					i18n("Would you like to print a test page for this printer?")
#define YESPRINT				i18n("Yes")
#define NOPRINT					i18n("No")
#define PAPERSIZE1			i18n("Paper size")
///defined for wizard --pageFive ///
#define LABEL5					i18n("Do you like to share this printer with other network\
\nusers? If yes, please give it a share name.")
#define SHARE						i18n("Share")
#define NOSHARE					i18n("Not share")
#define SHARENAME				i18n("Share Name:")
#define	WARNING7				i18n("Warning")
#define WARNING8				i18n("By some reasons, you may not have '/var/spool/lpd' directory.\n \
So, you can't add any printer.")

#define DEFAULT_PRINTER_SYMBOL i18n("DEFAULT")
#define MSG_DEFAULT			i18n("Default")
#define MSG_BROWSE_FOR_NETWORK_PRINTER i18n("Browse For Network Printer")
