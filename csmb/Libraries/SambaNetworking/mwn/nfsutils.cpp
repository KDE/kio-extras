/* Name: nfsutils.cpp

   Description: This file is a part of the libmwn library.

   Author:	Oleg Noskov (olegn@corel.com)

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

#include "common.h"
#include "qlist.h"
#include "nfsserver.h"
#include "nfsshare.h"
#include "acttask.h"
#include "nfsutils.h"
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <rpc/rpc.h>
#include <rpc/pmap_prot.h>
#include <rpc/pmap_clnt.h>
#include <signal.h>
#include <ctype.h>
#include <getopt.h>
#include <unistd.h>
#include <string.h>
#include <netinet/tcp.h>
#include "qapplication.h"
#include <sys/wait.h> // for waitpid()

#define MAXHOSTLEN 256

static u_long	getprognum(char *arg);
//static void brdcst(int argc, char **argv);
//static u_long	getvers(char *arg);

static bool_t	reply_proc(void *res, struct sockaddr_in *who);

CNetworkTreeItem *gpParent;

/* 
 * reply_proc collects replies from the broadcast. 
 * to get a unique list of responses the output of rpcinfo should
 * be piped through sort(1) and then uniq(1).
 */

/* res: Nothing comes back */
/* who: Who sent us the reply */

static int gFD;

static bool_t reply_proc(void *res, struct sockaddr_in *who)
{
	(void)res;

	LPSTR addr = inet_ntoa(who->sin_addr);
	strcat(addr, "\n");
	write(gFD, addr, strlen(addr));
	return FALSE;
}

static u_long getprognum(char *arg)
{
	register struct rpcent *rpc;
	register u_long prognum;

	if (isalpha(*arg)) {
		rpc = getrpcbyname(arg);
		if (rpc == NULL) {
		  fprintf(stderr, "rpcinfo: %s is unknown service\n",
			    arg);
			exit(1);
		}
		prognum = rpc->r_number;
	} else {
		prognum = (u_long) atoi(arg);
	}

	return (prognum);
}

CSMBErrorCode GetNFSHostList(CNetworkTreeItem *pParent)
{
	gpParent = pParent;

	enum clnt_stat rpc_stat;
	u_long prognum, vers;

	prognum = getprognum("mountd");
	vers = 2;

	pid_t pid;
	int input[2];
	
	pipe(input);	

	if ((pid = fork()) < 0)
	{
		printf("Unable to fork\n");	
		return keNetworkError;
	}
	else
	{
		if (pid == 0)
		{
			/* child */
			gFD = input[1];
			close(input[0]);
			
			rpc_stat = clnt_broadcast(prognum, vers, NULLPROC, 
							(xdrproc_t) xdr_void, NULL, 
							(xdrproc_t) xdr_void, NULL, 
							(resultproc_t) reply_proc);
			
			exit(RPC_TIMEDOUT != rpc_stat && RPC_SUCCESS != rpc_stat);
		}
	}

	// parent
	
	gTasks.append(new CActiveTask(pid, NULL, NULL, 3)); // run for 3 seconds only!

	close(input[1]);	
	
	/* now just read from input[0] */
	/* and write to output[1] */

	FILE *f = fdopen(input[0], "r");
	
	if (NULL != f)
	{
		setbuf(f, NULL);

		while (!feof(f))
		{              
			if (!WaitWithMessageLoop(f))
				return keStoppedByUser;
	
			char buf[100];
			
			fgets(buf, sizeof(buf)-1, f);
			
			if (feof(f))
				break;
	
			buf[strlen(buf)-1] = '\0';
			QString IP = buf;
			
			QListIterator<CNFSServerInfo> it(gNFSHostList);
	
			for (it.toFirst(); it.current() != NULL;++it)
				if (it.current()->m_IP == IP)
					break;
	
			if (NULL == it.current())
			{
				CNFSServerInfo *pInfo = new CNFSServerInfo;
				
				pInfo->m_ServerName = IP;
				pInfo->m_IP = IP;
				
				gNFSHostList.append(pInfo);
	
				if (NULL != gpParent)
				{
					/*CNFSServerItem *pItem = */
					new CNFSServerItem(gpParent, pInfo);
					
					if (gpParent->childCount() == 1)
					{
						gpParent->setOpen(TRUE);
					}
		//				pItem->Fill();
				}
			}
		}

		fclose(f);
	}

	return keSuccess;
}

#define MOUNTPROG ((u_long)100005)
#define MOUNTVERS ((u_long)1)
#define MOUNTPROC_DUMP ((u_long)2)
#define MOUNTPROC_EXPORT ((u_long)5)
#define MNTPATHLEN 1024
#define MNTNAMLEN 255

bool_t
xdr_dirpath(XDR *xdrs, dirpath *objp)
{

	 register long *buf=buf;

	 if (!xdr_string(xdrs, objp, MNTPATHLEN)) {
		 return (FALSE);
	 }
	return (TRUE);
}

bool_t
xdr_name(XDR *xdrs, name *objp)
{

	 register long *buf=buf;

	 if (!xdr_string(xdrs, objp, MNTNAMLEN)) {
		 return (FALSE);
	 }
	return (TRUE);
}

bool_t
xdr_mountlist(XDR *xdrs, mountlist *objp)
{

	 register long *buf=buf;

	 if (!xdr_pointer(xdrs, (char **)objp, sizeof(struct mountbody), (xdrproc_t)xdr_mountbody)) {
		 return (FALSE);
	 }
	return (TRUE);
}

bool_t
xdr_mountbody(XDR *xdrs, mountbody *objp)
{

	 register long *buf=buf;

	 if (!xdr_name(xdrs, &objp->ml_hostname)) {
		 return (FALSE);
	 }
	 if (!xdr_dirpath(xdrs, &objp->ml_directory)) {
		 return (FALSE);
	 }
	 if (!xdr_mountlist(xdrs, &objp->ml_next)) {
		 return (FALSE);
	 }
	return (TRUE);
}

bool_t
xdr_groups(XDR *xdrs, groups *objp)
{

	 register long *buf=buf;

	 if (!xdr_pointer(xdrs, (char **)objp, sizeof(struct groupnode), (xdrproc_t)xdr_groupnode)) {
		 return (FALSE);
	 }
	return (TRUE);
}

bool_t
xdr_groupnode(XDR *xdrs, groupnode *objp)
{

	 register long *buf=buf;

	 if (!xdr_name(xdrs, &objp->gr_name)) {
		 return (FALSE);
	 }
	 if (!xdr_groups(xdrs, &objp->gr_next)) {
		 return (FALSE);
	 }
	return (TRUE);
}

bool_t
xdr_exports(XDR *xdrs, exports *objp)
{

	 register long *buf=buf;

	 if (!xdr_pointer(xdrs, (char **)objp, sizeof(struct exportnode), (xdrproc_t)xdr_exportnode)) {
		 return (FALSE);
	 }
	return (TRUE);
}

bool_t
xdr_exportnode(XDR *xdrs, exportnode *objp)
{

	 register long *buf=buf;

	 if (!xdr_dirpath(xdrs, &objp->ex_dir)) {
		 return (FALSE);
	 }
	 if (!xdr_groups(xdrs, &objp->ex_groups)) {
		 return (FALSE);
	 }
	 if (!xdr_exports(xdrs, &objp->ex_next)) {
		 return (FALSE);
	 }
	return (TRUE);
}

static BOOL TimedConnect(struct sockaddr_in addr, int Timeout)
{
  pid_t pid;
	int status = -1;

	if ((pid = fork()) < 0)
	{
		return FALSE; // Unable to fork()...
	}
	else
	{
		if (pid == 0)
		{
			/* child */
      int retcode = 0;
      int sockfd;

      if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) != -1)
      {
        addr.sin_port = htons(111);
          
        if (-1 != connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)))
          retcode = 1;
        
        close(sockfd);
      }
      
      exit(retcode);
    }
  }

	// parent
  time_t EndTime = time(NULL) + Timeout;

  do
  {
    if (waitpid(pid, &status, WNOHANG) > 0)
      return status;

    qApp->processEvents(500);
  }
  while (time(NULL) < EndTime);
  
  kill(pid, SIGKILL);
  
  waitpid(pid, &status, 0);
  return 0;
}

CSMBErrorCode GetNFSShareList(QString& Server, 
				  CNFSShareArray *pShareList)
{
	//char hostname_buf[MAXHOSTLEN];
	//char *hostname;
	enum clnt_stat clnt_stat;
	struct hostent *hp;
	struct sockaddr_in server_addr;
	int msock;
	struct timeval total_timeout;
	struct timeval pertry_timeout;
	CLIENT *mclient;
	//groups grouplist;
	exports exportlist;
	mountlist dumplist;
 	mountlist list;
	int n;
	
	hp = gethostbyname(Server);
  
  if (NULL == hp)
  {
    //printf("Unknown host %s!!\n", (LPCSTR)Server);
    return keUnknownHost;
  }

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr = *(struct in_addr *)(hp->h_addr);
	
  if (!TimedConnect(server_addr, 3))
  {
    return keUnknownHost;
  }

  hp = gethostbyaddr((char *) &server_addr.sin_addr, sizeof(server_addr.sin_addr),
				 AF_INET);

	if (NULL != hp)
	{
		//printf("Reverse-resolve %s to %s\n", (LPCSTR)Server, hp->h_name);
		Server = QString(hp->h_name);
	}

	/* create mount daemon client */

	server_addr.sin_port = 0;
	msock = RPC_ANYSOCK;
	
  ////////////////////////////////////////////////////////
	
  if ((mclient = clnttcp_create(&server_addr, MOUNTPROG, MOUNTVERS, &msock, 0, 0)) == NULL)
	{
		server_addr.sin_port = 0;
		msock = RPC_ANYSOCK;
		pertry_timeout.tv_sec = 3;
		pertry_timeout.tv_usec = 0;
		
		if ((mclient = clntudp_create(&server_addr,  MOUNTPROG, MOUNTVERS, pertry_timeout, &msock)) == NULL)
		{
			//printf("Unknown host!\n");
      return keUnknownHost;
		}
	}
	
	mclient->cl_auth = authunix_create_default();
	total_timeout.tv_sec = 20;
	total_timeout.tv_usec = 0;

	memset(&dumplist, '\0', sizeof(dumplist));
  
	clnt_stat = clnt_call(mclient, 
												MOUNTPROC_DUMP, 
												(xdrproc_t) xdr_void, 
												NULL, 
												(xdrproc_t) xdr_mountlist, 
												(char *)&dumplist, 
												total_timeout);
	
	if (clnt_stat != RPC_SUCCESS)
		return keNetworkError;

	n = 0;
	
	for (list = dumplist; list; list = list->ml_next)
	{
		CNFSShareInfo Info;

		if (Server[0] >= '0' && Server[0] <= '9')
		{
			//printf("Server was IP\n");

			if ((hp = gethostbyname(dumplist->ml_hostname)) != NULL)
			{
				
				in_addr a;
				memcpy(&a, hp->h_addr, hp->h_length);

				QString IP = inet_ntoa(a);
				
				//printf("Its IP = %s, my IP = %s\n", (LPCSTR)IP, (LPCSTR)Server);

				if (IP == Server)
					Server = dumplist->ml_hostname;
			}
		}

		Info.m_ShareName.sprintf("%s:%s", dumplist->ml_hostname, dumplist->ml_directory);

		int i;

		for (i=pShareList->count()-1; i >= 0; i--)
			if ((*pShareList)[i].m_ShareName == Info.m_ShareName)
				break;
		
		if (i < 0)
			pShareList->Add(Info);
	}
	
	
 	memset(&exportlist, '\0', sizeof(exportlist));
 	
	clnt_stat = clnt_call(mclient, MOUNTPROC_EXPORT,
			(xdrproc_t) xdr_void, NULL,
			(xdrproc_t) xdr_exports, 
			(char *)&exportlist,
			total_timeout);

  if (clnt_stat != RPC_SUCCESS)
		return keNetworkError;

	while (exportlist)
	{
		CNFSShareInfo Info;

		Info.m_ShareName.sprintf("%s:%s", (const char *)Server, exportlist->ex_dir);
		
		int i;

		for (i=pShareList->count()-1; i >= 0; i--)
			if ((*pShareList)[i].m_ShareName == Info.m_ShareName)
				break;
		
		if (i < 0)
			pShareList->Add(Info);
		
		/*grouplist = exportlist->ex_groups;
			
		if (grouplist)
				while (grouplist) {
					printf("%s%s", grouplist->gr_name,
						grouplist->gr_next ? "," : "");
					grouplist = grouplist->gr_next;
				}
			else
				printf("(everyone)");
			printf("\n");
		*/
		
		exportlist = exportlist->ex_next;
	}
	
	return keSuccess;
}

