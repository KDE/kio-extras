/* Name: usermang.h
            

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

#ifndef USERMANG_H
#define USERMANG_H

#include <sys/types.h>

/* Stuff to use the library with C++ programs */
#undef __BEGIN_DECLS
#undef __END_DECLS
#ifdef __cplusplus
# define __BEGIN_DECLS extern "C" {
# define __END_DECLS }
#else
# define __BEGIN_DECLS /* empty */
# define __END_DECLS /* empty */
#endif

/* Stuff for non-ANSI compilers */
#undef __P
#if defined (__STDC__) || defined (_AIX) \
	|| (defined (__mips) && defined (_SYSTYPE_SVR4)) \
	|| defined (WIN32) || defined (__cplusplus)
# define __P(protos) protos
#else
# define __(protos) ()
#endif


__BEGIN_DECLS
int getnewid __P((void));
int addUserToGroup __P((const char *user, const char *group));
int is_gshadow __P((void));
int addGroup __P((const char *groupname, int new_gid));
int getNewGid __P((const char *groupname));
int is_shadow __P((void));
int createUser __P((const char *name, const char *group, const char *gecos, const char *home_dir, const char *shell));
int removeUser __P((const char *username));
int createHomeDir __P((const char *user));
int copy_dir __P((const char *src, const char *dest, uid_t uid, gid_t gid));
int renameUser __P((const char *olduser, const char *newuser));
int checkUser __P((const char *username));
__END_DECLS

#endif /* USERMANG_H */
