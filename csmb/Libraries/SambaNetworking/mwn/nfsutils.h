/* Name: nfsutils.h

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

#ifndef __INC_NFSUTILS_H__
#define __INC_NFSUTILS_H__
#include <rpc/rpc.h>

typedef char *dirpath;
#ifdef __cplusplus 
extern "C" bool_t xdr_dirpath(XDR *, dirpath*);
#elif __STDC__ 
extern  bool_t xdr_dirpath(XDR *, dirpath*);
#else /* Old Style C */ 
bool_t xdr_dirpath();
#endif /* Old Style C */ 


typedef char *name;
#ifdef __cplusplus 
extern "C" bool_t xdr_name(XDR *, name*);
#elif __STDC__ 
extern  bool_t xdr_name(XDR *, name*);
#else /* Old Style C */ 
bool_t xdr_name();
#endif /* Old Style C */ 

typedef struct mountbody *mountlist;
#ifdef __cplusplus 
extern "C" bool_t xdr_mountlist(XDR *, mountlist*);
#elif __STDC__ 
extern  bool_t xdr_mountlist(XDR *, mountlist*);
#else /* Old Style C */ 
bool_t xdr_mountlist();
#endif /* Old Style C */ 


struct mountbody {
	name ml_hostname;
	dirpath ml_directory;
	mountlist ml_next;
};
typedef struct mountbody mountbody;
#ifdef __cplusplus 
extern "C" bool_t xdr_mountbody(XDR *, mountbody*);
#elif __STDC__ 
extern  bool_t xdr_mountbody(XDR *, mountbody*);
#else /* Old Style C */ 
bool_t xdr_mountbody();
#endif /* Old Style C */ 


typedef struct groupnode *groups;
#ifdef __cplusplus 
extern "C" bool_t xdr_groups(XDR *, groups*);
#elif __STDC__ 
extern  bool_t xdr_groups(XDR *, groups*);
#else /* Old Style C */ 
bool_t xdr_groups();
#endif /* Old Style C */ 


struct groupnode {
	name gr_name;
	groups gr_next;
};
typedef struct groupnode groupnode;
#ifdef __cplusplus 
extern "C" bool_t xdr_groupnode(XDR *, groupnode*);
#elif __STDC__ 
extern  bool_t xdr_groupnode(XDR *, groupnode*);
#else /* Old Style C */ 
bool_t xdr_groupnode();
#endif /* Old Style C */ 


typedef struct exportnode *exports;
#ifdef __cplusplus 
extern "C" bool_t xdr_exports(XDR *, exports*);
#elif __STDC__ 
extern  bool_t xdr_exports(XDR *, exports*);
#else /* Old Style C */ 
bool_t xdr_exports();
#endif /* Old Style C */ 


struct exportnode {
	dirpath ex_dir;
	groups ex_groups;
	exports ex_next;
};
typedef struct exportnode exportnode;
#ifdef __cplusplus 
extern "C" bool_t xdr_exportnode(XDR *, exportnode*);
#elif __STDC__ 
extern  bool_t xdr_exportnode(XDR *, exportnode*);
#else /* Old Style C */ 
bool_t xdr_exportnode();
#endif /* Old Style C */ 

CSMBErrorCode GetNFSHostList(CNetworkTreeItem *pParent);

CSMBErrorCode GetNFSShareList(QString& Server, 
				  CNFSShareArray *pShareList);

#endif /* __INC_NFSUTILS_H__ */


