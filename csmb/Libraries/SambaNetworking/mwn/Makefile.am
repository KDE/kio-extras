INCLUDES = $(KDE_INCLUDES) $(QT_INCLUDES) $(X_INCLUDES) -I../../ccui
EXTRA_DIST = 
lib_LTLIBRARIES = libmwn.la

include_HEADERS = 

#mainwindow.h:	qt2compat/mainwindow.h
#	ln -s qt2compat/mainwindow.h mainwindow.h


noinst_HEADERS = \
	acttask.h \
	array.h \
	clientparser.h \
	common.h \
	copymove.h \
	CorelFileDialog.h \
	CorelFileDialogData.h \
	credentials.h \
	exports.h \
	ExportsEntryDialog.h \
	ExportsEntryDialogData.h \
	filejob.h \
	FileReplaceDialog.h \
	FileReplaceDialogData.h \
	fileUNC.h \
	ftpsite.h \
	ftpsession.h \
	filesystem.h \
	ftpfile.h \
	inifile.h \
	labeledit.h \
	localfile.h \
	mainwindow.h \
	mapdialog.h \
	mapdialogdata.h \
	mapper.h \
	msroot.h \
	mycomputer.h \
	NewShareDialog.h \
	NewShareDialogData.h \
	nfsroot.h \
	nfsserver.h \
	nfsutils.h \
	nfsshare.h \
	notifier.h \
	plaincombo.h \
	PasswordDlg.h \
	PasswordDlgData.h \
	PermissionsDlg.h \
	PermissionsDlgData.h \
	PermissionsAddDlg.h \
	PermissionsAddDlgData.h \
	pixmapcombo.h \
	PrinterSelectionDialog.h \
	PrinterSelectionDialogData.h \
	PromptDialog.h \
	PromptDialogData.h \
	resource.h \
	ShareLevelAccessDialog.h \
	ShareLevelAccessDialogData.h \
	SharingPage.h \
	SharingPageData.h \
	SharingPageNFS.h \
	SharingPageNFSData.h \
	shell.h \
	smbfile.h \
	smbserver.h \
	smbshare.h \
	smbutil.h \
	smbworkgroup.h \
	trashentry.h \
	treeitem.h \
	wintree.h


mainwindow.cpp:	qt2compat/mainwindow.cpp
	ln -s qt2compat/mainwindow.cpp mainwindow.cpp

libmwn_la_SOURCES = \
		acttask.cpp \
		clientparser.cpp \
		common.cpp \
		copymove.cpp \
		CorelFileDialog.cpp \
		CorelFileDialogData.cpp \
		exports.cpp \
		ExportsEntryDialog.cpp \
		ExportsEntryDialogData.cpp \
		filejob.cpp \
		FileReplaceDialog.cpp \
		FileReplaceDialogData.cpp \
		filesystem.cpp \
		fileUNC.cpp \
		ftpfile.cpp \
		ftpsession.cpp \
		ftpsite.cpp \
		inifile.cpp \
		labeledit.cpp \
		localfile.cpp \
		mainwindow.cpp \
		mapdialog.cpp \
		mapdialogdata.cpp \
		mapper.cpp \
		MessageBoxFour.cpp \
		MessageBoxFourData.cpp \
		msroot.cpp \
		mycomputer.cpp \
		NewShareDialog.cpp \
		NewShareDialogData.cpp \
		nfsroot.cpp \
		nfsserver.cpp \
		nfsshare.cpp \
		nfsutils.cpp \
		PasswordDlg.cpp \
		PasswordDlgData.cpp \
		PermissionsDlg.cpp \
		PermissionsDlgData.cpp \
		PermissionsAddDlg.cpp \
		PermissionsAddDlgData.cpp \
		pixmapcombo.cpp \
		plaincombo.cpp \
		PrinterSelectionDialog.cpp \
		PrinterSelectionDialogData.cpp \
		PromptDialog.cpp \
		PromptDialogData.cpp \
		ReadConfig.cpp \
		resource.cpp \
		ShareLevelAccessDialog.cpp \
		ShareLevelAccessDialogData.cpp \
		SharingPage.cpp \
		SharingPageData.cpp \
		SharingPageNFS.cpp \
		SharingPageNFSData.cpp \
		shell.cpp \
		smbfile.cpp \
		smbserver.cpp \
		smbshare.cpp \
		smbutil.cpp \
		smbworkgroup.cpp \
		trashentry.cpp \
		treeitem.cpp \
		wintree.cpp 

libmwn_la_LDFLAGS = -version-info 2:0 --enable-shared
libmwn_la_LIBADD = -lccui

libmwn_la_METASOURCES = USE_AUTOMOC

enable_shared=yes

messages:	stringres_mwn.h resourcegen.cpp
	g++ -o resourcegen resourcegen.cpp
	./resourcegen > ../../po/mwn.pot
	rm -f ./resourcegen

