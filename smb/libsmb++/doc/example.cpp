// This is a basic example for the library.
// Assuming smb-config is in your path, compile it with:
//     g++ example.cpp -o example `smb-config --cxxflags` smb-config --libs`

#include <iostream.h>
#include <unistd.h>
#include <string.h>
#include "smb++.h"

class MyCallback : public SmbAnswerCallback
{
protected:
	// Warning: don't use a fixed size buffer in a real application.
	// This is a security hazard.
	char buf[200];
public:
	char *getAnswer(int type, const char *optmessage) {
		switch (type) {
			case ANSWER_USER_NAME:
				cout<<"User name for host "<<optmessage<<": ";
				cin>>buf;
				break;
			case ANSWER_USER_PASSWORD:
				cout<<"Password for user "<<optmessage<<": ";
				cin>>buf;
				break;
			case ANSWER_SERVICE_PASSWORD:
				cout<<"Password for service "<<optmessage<<": ";
				cin>>buf;
				break;
		}
		return buf;
	}
} cb;


main(int argc, char **argv)
{
	SMB smb;
	smb.setPasswordCallback(&cb);
	
	int fd;
	if (argc>1) fd = smb.open(argv[1],O_RDONLY);
	else {
		cout<<"Specify a file as an argument."<<endl;
		cout<<"For example 'smb://myhost/myshare/myfile'"<<endl;
		exit(0);
	}
	cout<<"fd="<<fd<<endl;
	int ret;
	ret = smb.lseek(fd,0,SEEK_END);
	cout<<"total size="<<ret<<endl;
	ret = smb.lseek(fd,0,SEEK_SET);
	cout<<"position="<<ret<<endl;
	char buf[1000];
	int count;
	while ( (count=smb.read(fd, buf, 1000))>0 ) {
		write(1,buf,count);
		if (count<1000) break;
	}
	
	cout<<"last count="<<count<<endl;
	ret=smb.close(fd);
	cout<<"close returns "<<ret<<endl;

	int dd;
	if (argc>2) dd = smb.opendir(argv[2]);
	else {
		cout<<"You can specify a directory as a second argument."<<endl;
		cout<<"For example 'smb://myworkgroup'"<<endl;
		exit(0);
	}
	cout<<"dd="<<dd<<endl;
	SMBdirent *ret2;
	while ((ret2 = smb.readdir(dd))) {
		cout << ret2->d_name << " " << oct << ret2->st_mode << endl;
	}

	int closeres =smb.closedir(dd);
	cout<<"close returns "<<closeres<<endl;

}


