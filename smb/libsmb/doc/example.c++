#include <iostream.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <smb.h>

/*
 This function will be a mean for SMBIO to retreive data
 used when passwords are needed for example
 it should display "message" and return the user answer
 If you don't provide one, null passwords will always be used,
 If you do provide one, null passwords will be tested as well
*/
char *getFunc(const char* message)
{
	cout<<message<<"\n";
	char *tmp=getpass("");
	cout<<"\n";
	char* ret=new char[strlen(tmp)+1];
	strcpy(ret,tmp);
	return ret;
}

main(int argc, char **argv)
{

// Simple name resolver commented out


	NMBIO nio;
	// Will do a NMB lookup of the first argument
	NBHostEnt *list=nio.gethostbyname(argc>1?argv[1]:"cthulhu");
	if (!list) cout<<"Null list !\n";
	else
	{
		// Print corresponding IP
		NBHostEnt *sav=list;
		while (list)
		{
			printf("%d.%d.%d.%d\n",(list->ip>>24)&0xFF,(list->ip>>16)&0xFF,(list->ip>>8)&0xFF,list->ip&0xFF);
			list=list->next;
		}
		delete sav;
	}
    return 0;

	
	SMBIO io(getFunc);
//	io.setDefaultBrowseServer("cthulhu");
	io.setDefaultUser("brodu");

	// process command-line argument as a URL
	if (argc>=2) {
		struct stat statbuf;
		if (io.stat(argv[1],&statbuf)==-1) {cout<<"error stat\n"; return 0;}
		printf("=> mode=%o, size=%lu\n",statbuf.st_mode,statbuf.st_size);
		
		// Test mode : directory => list
		if (statbuf.st_mode&040000) {
			int dd=io.opendir(argv[1]);
			if (dd==-1) {cout<<"error, dd=-1\n"; return 0;}
			SMBdirent *dent;
			while ((dent=io.readdir(dd))) {
				printf("=> mode=%o, size=%u, name=%s\n",dent->st_mode,dent->st_size,dent->d_name);
			}
			io.closedir(dd);
			return 0;
		}
		
		// And file => download
		else {
			int fd;
			if ((fd=io.open(argv[1],O_RDONLY))==-1) {cout<<"error open\n"; return -1;}
			char *name=io.getName(fd);
			int locfd=open(name, O_WRONLY | O_CREAT, 0644);
			delete name;
			/* You can use any buffer size */
			int count, total=0, size=123456;
			char *buffer=new char[size];
			do {
				count=io.read(fd, buffer, size);
				if (count==-1) {cout<<"io.read error\n"; break;}
				write(locfd,buffer,count);
				total+=count;
			} while (count>0);
			cout<<total<<" byte read.\n";
			delete buffer;
			close(locfd);
			io.close(fd);
			return 0;
		}
	} // endif argc>=2

}



