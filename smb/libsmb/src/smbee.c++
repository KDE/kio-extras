/*
	smbee.c++ : smb browser using curses
    Copyright (C) 1999  Nicolas Brodu
    brodu@iie.cnam.fr

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program, see the file COPYING; if not, write
    to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
    MA 02139, USA.
*/

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "SMBIO.h"
#include "cursesHacks.h"
#include <arpa/inet.h>

class DirList
{
public:
	DirList(SMBdirent *d)
	{
		dirent=*d;
		dirent.d_name=new char[strlen(d->d_name)+1];
		strcpy(dirent.d_name,d->d_name);
		mark=0;
		next=0;
	}
	~DirList()
	{
		if (next) delete next;
	}
	int mark;
	SMBdirent dirent;
	DirList *next;
};

// global parameters
int minx, miny, maxx, maxy;

// This function will be a mean for SMBIO to retreive data.
// Used when passwords are needed for example
// it should display "message" and return the user answer
// If you don't provide one, null passwords will always be used,
// If you do provide one, null passwords will be tested as well
char *getFunc(const char* message, bool doecho)
{
	if (!message) return 0;
	attrset(A_NORMAL);
	mvprintw(miny,minx,"%s : ",message);
	hline(' ',maxx-minx);
	attrset(A_NORMAL);
	mvprintw(maxy-1,maxx-1," ");
	move(maxy-1,maxx-1); // make cursor invisible
	refresh();
	if (doecho) echo();
	else noecho();
	char *tmpbuf=new char[100];
	int len=0;
	while (int c=getch()) {
		if ((c==13) || (c==KEY_ENTER)) break;
		if ((c==127) || (c==263) || (c==330)) { // backspace, delete
			if (len>0) tmpbuf[len--]=0;
		}
		if (len<99) tmpbuf[len++]=c;
		tmpbuf[len]=0;
		for (int i=0; i<(int)strlen(tmpbuf); i++) {
			mvprintw(miny, i+minx+strlen(message)+3, "*");
		}
		clrtoeol();
		move(maxy-1,maxx-1); // make cursor invisible
		refresh();
	}
	mvhline(miny,minx,' ',maxx-minx);
	attrset(A_NORMAL);
	mvprintw(maxy-1,maxx-1," ");
	move(maxy-1,maxx-1); // make cursor invisible
	refresh();
	return tmpbuf;
}


main(int argc, char **argv)
{
	char *defaultUser=0;
	char *browseServer=0;
	char *broadcast=0;
	char *tmpbuf;
	extern char *optarg;
	char opt;
	char url[1024]="smb://";    // see 'default:' for hard-coded 1024
	int urlLen=6;
	
	while ((opt=getopt(argc, argv,"u:s:B:h"))!=EOF) {
		switch (opt) {
			case 'u':
				defaultUser=new char[strlen(optarg)+1];
				strcpy(defaultUser,optarg);
				break;
			case 's':
				browseServer=new char[strlen(optarg)+1];
				strcpy(browseServer,optarg);
				break;
			case 'B':
				broadcast=new char[strlen(optarg)+1];
				strcpy(broadcast,optarg);
				break;
			default:
				printf("Usage :\n  smbee [-s browseServer] [-u user] [-B broadcast address] [URL]\n");
				exit(0);
				break;
		}
	}
	if (!browseServer) {
		char *tmp=getenv("SMB_BROWSE_SERVER");
		if (tmp) {
			browseServer=new char[strlen(tmp)+1];
			strcpy(browseServer,tmp);
		}
	}
	if (!defaultUser) {
		char *tmp=getenv("SMB_USER");
		if (tmp) {
			defaultUser=new char[strlen(tmp)+1];
			strcpy(defaultUser,tmp);
		}
	}

	for (int i=1; i<argc; i++) {
		if (argv[i][0]!='-') {
			if (((!defaultUser) || (strcmp(argv[i],defaultUser)))
				&& ((!browseServer) || (strcmp(argv[i],browseServer)))
				&& ((!broadcast) || (strcmp(argv[i],broadcast)))) {
				strcpy(url,argv[i]);
				break;
			}
		}
	}

// curses init
	initscr(); cbreak(); noecho(); raw();
	nonl(); intrflush(stdscr, FALSE); keypad(stdscr, TRUE);

// get params of the "screen"
	getbegyx(stdscr,miny,minx);
	getmaxyx(stdscr,maxy,maxx);

// set up the layout
	mvhline(miny,minx,' ',maxx-minx);
	mvprintw(miny,minx,"C-U : Default user :");
	mvprintw(miny,(minx+maxx)/2,"C-S : Browse server :");
//	mvhline(miny+1,minx,'-',maxx-minx);  // '-' OK for all terminals
	attrset(A_REVERSE);
	mvhline(miny+1,minx,' ',maxx-minx);
	mvhline(maxy-1,minx,' ',maxx-minx-1); // keep a space to make cursor invisible
	attrset(A_NORMAL);
	move(maxy-1,maxx-1); // make cursor "invisible"
//	mvhline(miny+3,minx,'-',maxx-minx);
	refresh();
//	int car=getch(); //debug purpose


// Declare state variables
	int c=0;
	SMBIO io(getFunc);
	DirList *list=0, *cur=0;
	SMBdirent *dirent;
	int dd,fd;
	int stop=0;
	int i; // most famous multi-purpose index
	int numEntries=0;
	int currentLine=0; // uses with up & down keys, 0 is relative to the listing area
	int scrollNum=0;
	int maxLine=maxy-1;
	int modified=0;    // URL has been modified "by hand"
	int charAvailable=0;    // next char available
	
	attrset(A_NORMAL);
	mvprintw(miny,minx,"C-U : Default user : %s",(defaultUser)?defaultUser:"");
	mvprintw(miny,(minx+maxx)/2,"C-S : Browse server : %s",(browseServer)?browseServer:"");
	attrset(A_REVERSE);
	mvprintw(miny+1,minx,"%s",url);
	hline(' ',maxx-minx);
	mvhline(maxy-1,minx,' ',maxx-minx-1); // keep a space to make cursor invisible
	attrset(A_NORMAL);
	refresh();

	if (defaultUser) io.setDefaultUser(defaultUser);
	if (browseServer) io.setDefaultBrowseServer(browseServer);
	char *tmp=getenv("SMB_BROADCAST_ADDRESS");
	uint32 addr;
	if (tmp) {
		addr=inet_addr(tmp);
		io.setNetworkBroadcastAddress(addr);
	}
	if (broadcast) {
		addr=inet_addr(broadcast);
		io.setNetworkBroadcastAddress(addr);
		delete broadcast;
	}
	
	
	// smb:// is considered to be a directory
	dd=io.opendir(url);
	while ((dirent=io.readdir(dd))) {
		if (!list) {
			list=new DirList(dirent);
			cur=list;
		} else {
			cur->next=new DirList(dirent);
			cur=cur->next;
		}
		numEntries++;
	}

// main loop	
	while (!stop) {
		mvprintw(miny,minx,"C-U : Default user : %s",(defaultUser)?defaultUser:"");
		mvprintw(miny,(minx+maxx)/2,"C-S : Browse server : %s",(browseServer)?browseServer:"");
		attrset(A_REVERSE);
		mvprintw(miny+1,minx,"%s",url);
		hline(' ',maxx-minx);
		mvhline(maxy-1,minx,' ',maxx-minx-1); // keep a space to make cursor invisible
//		mvprintw(maxy-1,minx,"maxLine %d, numEntries %d, currentLine %d, scrollNum %d ",maxLine, numEntries, currentLine, scrollNum);
		attrset(A_NORMAL);
		cur=list;
		i=0;
		while (cur) {
			if (i>=scrollNum) {
				if (i-scrollNum==currentLine) attrset(A_BOLD);
				mvprintw(miny+2+i-scrollNum,minx,"%s%s",
					(cur->dirent.st_mode&040000)?"=> ":(cur->mark)?" * ":"   ",
					cur->dirent.d_name);
				clrtoeol();
				if (i-scrollNum==currentLine) {
					attrset(A_REVERSE);
					if (cur->dirent.st_mode & 040000)
						mvprintw(maxy-1,minx,"Directory");
					else mvprintw(maxy-1,minx,"File size : %d",cur->dirent.st_size);
					hline(' ',maxx-minx);
					attrset(A_NORMAL);
				}
			}
			i++; cur=cur->next;
			if (miny+i-scrollNum+2>=maxLine) break;
		}
		while (miny+i-scrollNum+2<maxLine) {
			move(miny+i-scrollNum+2,minx); clrtoeol(); i++;
		}
		attrset(A_NORMAL);
		mvprintw(maxy-1,maxx-1," ");
		move(maxy-1,maxx-1); // make cursor invisible
		refresh();
		if (!charAvailable) c=getch();
		charAvailable=0;
		switch (c) {
			case KEY_ENTER:
			case 13:
				if ((!list) && (!modified)) break;
				cur=list; i=0;
				if (!modified) while (cur) {
					if ((cur->mark) && (!((i>=scrollNum) && (i-scrollNum==currentLine)))) {
						cur->mark=0;
						i=-1;
						if (char *tmpurl=io.append(url,cur->dirent.d_name)) {
							strcpy(url,tmpurl);
							delete tmpurl;
							urlLen=strlen(url);
						}
						break;
					}
					i++; cur=cur->next;
				}
				if (i!=-1) {       // no mark left
					cur=list; i=0;
					if (!modified) while (cur) {
						if ((i>=scrollNum) && (i-scrollNum==currentLine)) {
							cur->mark=0;
							if (char *tmpurl=io.append(url,cur->dirent.d_name)) {
								strcpy(url,tmpurl);
								delete tmpurl;
								urlLen=strlen(url);
							}
							break;
						}
						i++; cur=cur->next;
					}
					modified=0;
				} else {         // download all marked files first
					c=13;
					charAvailable=1;
				}
				if (dd!=-1) io.closedir(dd);
				struct stat s;
				io.stat(url,&s);
				if (s.st_mode & 040000) {	// directory
					if (list) delete list;
					cur=list=0;
					numEntries=0;
					dd=io.opendir(url);
					int upexists=0;
					while ((dirent=io.readdir(dd))) {
						if (!list) {
							list=new DirList(dirent);
							cur=list;
						} else {
							cur->next=new DirList(dirent);
							cur=cur->next;
						}
						numEntries++;
						if (!strcmp(cur->dirent.d_name,"..")) upexists=1;
					}
					if ((!upexists) && (strcmp(url,"")) && (strcmp(url,"smb://"))) { // then add one !
						dirent=new SMBdirent();
						dirent->d_name=new char[3];
						strcpy(dirent->d_name,"..");
						dirent->st_mode=040755;
						cur=new DirList(dirent);
						cur->next=list; // add it at the beginning
						list=cur;
						numEntries++;
					}
					currentLine=0;
					scrollNum=0;
				} else { // endif directory, now download file
					fd=io.open(url,O_RDONLY);
					char *name=io.getName(fd);
					int locfd=open(name, O_WRONLY | O_CREAT, 0644);
					if (fd!=-1) {
						int count, total=0, size=123456; // magic buffer size !
						char *buffer=new char[size];
						do {
							count=io.read(fd, buffer, size);
							total+=count;
							attrset(A_REVERSE);
							if (count==-1) mvprintw(maxy-1,minx,"read error at position %d",total);
							else mvprintw(maxy-1,minx,"%d/%d bytes read, %s",total, s.st_size,
								(count<=0)?"download complete !":name);
							hline(' ',maxx-minx);
							attrset(A_NORMAL);
							move(maxy-1,maxx-1); // make cursor invisible
							refresh();
							if (count==-1) break;
							write(locfd,buffer,count);
						} while (count>0);
						delete buffer;
						close(locfd);
						io.close(fd);
						if (!charAvailable) getch();
					}
					if (name) delete name;
					// remove the file name...
					if (char *tmpurl=io.append(url,"..")) {
						strcpy(url,tmpurl);
						delete tmpurl;
						urlLen=strlen(url);
					}
				}
				break;
			case KEY_DOWN:
				if (currentLine+scrollNum<numEntries-1) {
					if (currentLine+miny+3<maxLine) currentLine++;
					else scrollNum++;
				}
				break;
			case KEY_UP:
				if (currentLine>0) currentLine--;
				else if (scrollNum>0) scrollNum--;
				break;

			case 127:  // backspace and delete
			case 263:
			case 330:
				modified=1;
				if (urlLen>0) {
					url[--urlLen]=0;
				}
				break;

			case 21:    // CTRL-U
				attrset(A_NORMAL);
				mvprintw(miny,minx,"Default user : ");
				hline(' ',maxx-minx);
				move(maxy-1,maxx-1); // make cursor invisible
				refresh();
				echo();
				tmpbuf=new char[100];
				attrset(A_REVERSE);
				mvhline(maxy-1,minx,' ',maxx-minx);
				attrset(A_NORMAL);
				mvgetnstr(miny,minx+strlen("Default user : "),tmpbuf,100);
				attrset(A_REVERSE);
				mvhline(miny+1,minx,' ',maxx-minx);
				mvhline(maxy-1,minx,' ',maxx-minx-1); // keep a space to make cursor invisible
				attrset(A_NORMAL);
				mvhline(miny,minx,' ',maxx-minx);
				mvprintw(maxy-1,maxx-1," ");
				move(maxy-1,maxx-1); // make cursor invisible
				if (defaultUser) delete defaultUser;
				defaultUser=tmpbuf;
				io.setDefaultUser(defaultUser);
				noecho();
				c=13;
//				strcpy(url,"smb://");
				modified=1;
				charAvailable=1;
				break;

			case 19:    // CTRL-S
				attrset(A_NORMAL);
				mvprintw(miny,minx,"Browse server : ");
				hline(' ',maxx-minx);
				move(maxy-1,maxx-1); // make cursor invisible
				refresh();
				echo();
				tmpbuf=new char[100];
				attrset(A_REVERSE);
				mvhline(maxy-1,minx,' ',maxx-minx);
				attrset(A_NORMAL);
				mvgetnstr(miny,minx+strlen("Browse server : "),tmpbuf,100);
				attrset(A_REVERSE);
				mvhline(miny+1,minx,' ',maxx-minx);
				mvhline(maxy-1,minx,' ',maxx-minx-1); // keep a space to make cursor invisible
				attrset(A_NORMAL);
				mvhline(miny,minx,' ',maxx-minx);
				mvprintw(maxy-1,maxx-1," ");
				move(maxy-1,maxx-1); // make cursor invisible
				if (browseServer) delete browseServer;
				browseServer=tmpbuf;
				io.setDefaultBrowseServer(browseServer);
				noecho();
				c=13;
				strcpy(url,"smb://");
				modified=1;
				charAvailable=1;
				break;
			case 27:    // escape
			case 3:     // CTRL-C
			case 24:    // CTRL-X
			case 17:    // CTRL-Q
				stop=1;
				break;

#ifdef KEY_MARK // is in ncurses.h on my system (Linux) but not on alpha/OSF1
			case KEY_MARK:   // mark or un-mark a file
#endif
			case KEY_RIGHT:   // mark or un-mark a file
			case KEY_LEFT:   // mark or un-mark a file
				i=0; cur=list;
				while (cur) {
					if ((i>=scrollNum) && (i-scrollNum==currentLine)
					&& (!(cur->dirent.st_mode&040000))) {
						cur->mark=1-cur->mark;
						break;
					}
					i++; cur=cur->next;
				}
				break;
			
			case 2:     // CTRL-B
				attrset(A_NORMAL);
				mvprintw(miny,minx,"Network broadcast address : ");
				hline(' ',maxx-minx);
				move(maxy-1,maxx-1); // make cursor invisible
				refresh();
				echo();
				tmpbuf=new char[100];
				attrset(A_REVERSE);
				mvhline(maxy-1,minx,' ',maxx-minx);
				attrset(A_NORMAL);
				mvgetnstr(miny,minx+strlen("Network broadcast address : "),tmpbuf,100);
				attrset(A_REVERSE);
				mvhline(miny+1,minx,' ',maxx-minx);
				mvhline(maxy-1,minx,' ',maxx-minx-1); // keep a space to make cursor invisible
				attrset(A_NORMAL);
				mvhline(miny,minx,' ',maxx-minx);
				mvprintw(maxy-1,maxx-1," ");
				move(maxy-1,maxx-1); // make cursor invisible
				noecho();
				addr=inet_addr(tmp);
				io.setNetworkBroadcastAddress(addr);
				c=13;
				strcpy(url,"smb://");
				modified=1;
				charAvailable=1;
				break;
			
			default:
				modified=1;
				if (urlLen<1022) {
					url[urlLen++]=c;
					url[urlLen]=0;
				}
				break;
		}
	}

// terminate smb connection
	if (dd!=-1) io.closedir(dd);

// exit curses cleanly
	endwin();
//	printf("%d\n",car);
}

