/* Name: usermang.c


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

#ifdef HAVE_CONFIG_H
#include "../../config.h"
#endif

#include "usermang.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#ifdef HAVE_SHADOW
# include <shadow.h>
#endif

#define		PASSWDBACKUPFILE	"/etc/passwd.Backup"
#define		GROUPBACKUPFILE		"/etc/group.Backup"
#define 	GROUPTEMPFILE		"/etc/.tmpgroup.kdm" 
#define 	SHADOWGROUPTEMPFILE	"/etc/.tmpgshadow.kdm" 
#define		PASSWDTEMPFILE		"/etc/.tmppasswd.kdm"
#define		SHADOWTEMPFILE		"/etc/.tmpshadow.kdm"
#define 	GROUPFILE		"/etc/group"
#define		SHADOWGROUPFILE		"/etc/gshadow"
#define		PASSWDFILE		"/etc/passwd"
#define		SHADOWFILE		"/etc/shadow"
#define		SKELDIR			"/etc/skel"

#define 	MAXSTRINGLENGTH		255

extern char** environ;

/*
 * Get the next free UID, and -1 if none are available.
 */
int getnewid(void)
{ 
	struct passwd *pCheckedUser = NULL  ;
	int i, nUidFound = 0;

	for (i= 1000 ;  ; i++ )
	{
		pCheckedUser = getpwuid(i);  
		if (pCheckedUser == NULL)
		{ 
			nUidFound = 1; 
			break;  
		}
	}
	if (nUidFound == 1)
	{
		return (i);
	}
  else 	
	{
		return (-1);
	}
}

/*
 * Add the user 'user' to the group 'group'
 *
 * Returns 0 on success and -1 on error
 */
int addUserToGroup(const char *user, const char *group)
{
	char grentry[2048];
	FILE *fGroupFile, *fTempGroupFile;
	uid_t uid;
	gid_t gid;
	int len;
#ifdef HAVE_SHADOW
	char sgentry[2048];
	FILE *fShadowGroupFile, *fTempShadowGroupFile;
	int bgshadow;
#endif

	/* Do some preliminary tests */

	/* If the user does not exist, exit */
	if (getpwnam(user) == NULL)
		return -1;

	/* If the group does not exist, exit */
	if (getgrnam(group) == NULL)
		return -1;
	
	/* Open the files */
	if ((fGroupFile = fopen(GROUPFILE, "r")) == NULL)
	{
		return -1;
	}
	if ((fTempGroupFile = fopen(GROUPTEMPFILE, "w")) == NULL)
	{
		return -1;
	}
	else
	{
		/* Make sure the new group file has the correct permission/ownership */
		chmod(GROUPTEMPFILE, 0644);	/* /etc/group IS world readable */
		uid = 0;			/* Owned by root.root */
		gid = 0;
		chown(GROUPTEMPFILE, uid, gid);
	}

	/* Then do the work */
	while(1)
	{
		if (fgets(grentry, sizeof(grentry), fGroupFile) == NULL)
		{
			break;
		}
		if(strncmp(grentry, group, strlen(group)) == 0)
		{
			/* We found the group */
			len = strlen(grentry);
			if (grentry[len - 1] == '\n')
			{
				/* Remove the trailing '\n' if any */
				grentry[len - 1] = '\0';
				len--;
			}
			if (grentry[len - 1] != ':')
				strcat(grentry, ",");
			strcat(grentry, user);
			strcat(grentry, "\n");
		}
		fputs(grentry, fTempGroupFile);
	}
	fclose(fGroupFile);
	fclose(fTempGroupFile);
#ifdef HAVE_SHADOW
	bgshadow = is_gshadow();
	if(bgshadow)
	{
		/* Open the files */
		if ((fShadowGroupFile = fopen(SHADOWGROUPFILE, "r")) == NULL)
		{
			return -1;
		}
		if ((fTempShadowGroupFile = fopen(SHADOWGROUPTEMPFILE, "w")) == NULL)
		{
			return -1;
		}
		else
		{
			/* Make sure the new group file has the correct permission/ownership */
			chmod(SHADOWGROUPTEMPFILE, 0644);	/* /etc/group IS world readable */
			uid = 0;			/* Owned by root.root */
			gid = 0;
			chown(SHADOWGROUPTEMPFILE, uid, gid);
		}

		/* Then do the work */
		while(1)
		{
			if (fgets(sgentry, sizeof(sgentry), fShadowGroupFile) == NULL)
			{
				break;
			}
			if(strncmp(sgentry, group, strlen(group)) == 0)
			{
				/* We found the group */
				len = strlen(sgentry);
				if (sgentry[len - 1] == '\n')
				{
					/* Remove the trailing '\n' if any */
					sgentry[len - 1] = '\0';
					len--;
				}
				if (sgentry[len - 1] != ':')
					strcat(sgentry, ",");
				strcat(sgentry, user);
				strcat(sgentry, "\n");
			}
			fputs(sgentry, fTempShadowGroupFile);
		}
		fclose(fShadowGroupFile);
		fclose(fTempShadowGroupFile);
	}
#endif
	rename(GROUPTEMPFILE, GROUPFILE);
#ifdef HAVE_SHADOW
	if(bgshadow)
		rename(SHADOWGROUPTEMPFILE, SHADOWGROUPFILE);
#endif

	return 0;
}

/*
 * Check if shadow groups are enabled on the system
 */
int is_gshadow(void)
{
	return (access(SHADOWGROUPFILE, F_OK) == 0);
}

/*
 * Add the group 'groupname' to the system, with gid 'new_gid'
 *
 * Return 0 on success, and -1 one error.
 */
int addGroup(const char *groupname, int new_gid)
{
	uid_t uid;
	gid_t gid;
	char grentry[1024];
	FILE *fGroupFile, *fTempGroupFile;
#ifdef HAVE_SHADOW
	char sgentry[1024];
	FILE *fShadowGroupFile, *fTempShadowGroupFile;
	int bgshadow = 0;
	struct group *gr_shadow;
#endif
	int bnis = 0;

	/* Write the group entry.
	 * We must be careful to add this entry BEFORE
	 * a possible NIS entry (+::::). We therefore
	 * use fgets instead of getgrent to read the group
	 * file.
	 */
	if ((fGroupFile = fopen(GROUPFILE, "r")) == NULL)
	{
		return -1;
	}
	if ((fTempGroupFile = fopen(GROUPTEMPFILE, "w")) == NULL)
	{
		return -1;
	}
	else
	{
		/* Make sure the new group file has the correct permission/ownership */
		chmod(GROUPTEMPFILE, 0644);	/* /etc/group IS world readable */
		uid = 0;			/* Owned by root.root */
		gid = 0;
		chown(GROUPTEMPFILE, uid, gid);
	}
	while(1)
	{
		if (fgets(grentry, sizeof(grentry), fGroupFile) == NULL)
		{
			break;
		}
		if((grentry[0] == '+') || (grentry[0] == '-'))
		{
			bnis = 1;
			break;
		}
		fputs(grentry, fTempGroupFile);
	}
#ifdef HAVE_SHADOW
	bgshadow = is_gshadow();
	if (bgshadow)
	{
		fprintf(fTempGroupFile, "%s:x:%d:\n", groupname, new_gid);
	}
	else
	{
		fprintf(fTempGroupFile, "%s::%d:\n", groupname, new_gid);
	}
#else
	fprintf(fTempGroupFile, "%s::%d:\n", groupname, new_gid);
#endif
	/* If there are some NIS entries left in the group file,
	 * add them.
	 */
	if (bnis)
	{
		do
		{
			fputs(grentry, fTempGroupFile);
		} while (fgets(grentry, sizeof(grentry), fGroupFile) != NULL);
	}
	fclose(fGroupFile);
	fclose(fTempGroupFile);
#ifdef HAVE_SHADOW
	if (bgshadow)
	{
		bnis = 0;
		
		/* Do the same thing for shadow groups. */
		if ((fShadowGroupFile = fopen(SHADOWGROUPFILE, "r")) == NULL)
		{
			return -1;
		}
		if ((fTempShadowGroupFile = fopen(SHADOWGROUPTEMPFILE, "w")) == NULL)
		{
			return -1;
		}
		else
		{
			chmod(SHADOWGROUPTEMPFILE, 0640);	/* NOT world-readable				*/
			uid = 0;				/* Owned by root				*/
			gr_shadow = getgrnam("shadow"); 	/* The shadow group file should belong to	*/
			if (gr_shadow != NULL)			/* the group 'shadow', if such a group exists.	*/
				gid = gr_shadow->gr_gid;
			else
				gid = 0;
			chown(SHADOWGROUPTEMPFILE, uid, gid);
		}
		while(1)
		{
			if (fgets(sgentry, sizeof(sgentry), fShadowGroupFile) == NULL)
			{
				break;
			}
			if((sgentry[0] == '+') || (sgentry[0] == '-'))
			{
				bnis = 1;
				break;
			}
			fputs(sgentry, fTempShadowGroupFile);
		}
		fprintf(fTempShadowGroupFile, "%s:::\n", groupname);
		/* If there are some NIS entries left in the shadow group file,
		 * add them. */
		if (bnis)
		{
			do
			{
				fputs(sgentry, fTempShadowGroupFile);
			} while (fgets(sgentry, sizeof(sgentry), fShadowGroupFile) != NULL);
		}
		fclose(fShadowGroupFile);
		fclose(fTempShadowGroupFile);
	}
#endif

	rename(GROUPTEMPFILE, GROUPFILE);
#ifdef HAVE_SHADOW
	if (bgshadow)
		rename(SHADOWGROUPTEMPFILE, SHADOWGROUPFILE);
#endif

	return 0;
}

/*
 * Return the gid of the group 'groupname'.
 * 
 * NOTE: The group is created if it doesn't exist !
 *
 * Return -1 on error.
 */
int getNewGid (const char *groupname)
{
	struct group *pGroup = NULL ;
	int i, gidfound = 0;

	/* check if the group exist. */
	pGroup = getgrnam(groupname);
	if (pGroup)
		return pGroup->gr_gid;
	/* if the group does not exist get a new GID. */
	else
	{
		for (i = 1000 ;  ; i++ )
		{
			pGroup = getgrgid(i);
			if (pGroup == NULL)
			{
				gidfound = 1;
				break;
			}
		}
		if ( (gidfound == 1) && (addGroup (groupname, i) == 0) )
		{
			return (i);
		}
		else
		{
			return (-1);
		}
	}
}

/*
 * Check if shadow passwords are enabled on the system
 */
int is_shadow(void)
{
	return (access(SHADOWFILE, F_OK) == 0);
}

/* Create a new user. If it doesn't exist, it also creates the new user's
 * group. If you want to actually create his home directory, you will have
 * to call createHomeDir().
 *
 * Return 0 if the user was successfully created, and -1 if the user
 * already existed or if an error occured.
 */
int createUser(const char *name, const char *group, const char *gecos, const char *home_dir, const char *shell)
{
	FILE *fPassFile = NULL, *fTempPassFile = NULL;
	char pwentry[1024];
	struct passwd newuser, *pw;
	uid_t uid;
	gid_t gid;
#ifdef HAVE_SHADOW
	struct spwd sp_newuser;
	struct group *gr_shadow;
#endif
	int bnis = 0;
	int bshadow = 0;

	/* Check if the user already exists */
	pw = getpwnam(name);
	if (pw != NULL)
		return -1;
	
	/* Fill the password structure */
	newuser.pw_name = (char *) name;
	newuser.pw_uid = getnewid() ;
	newuser.pw_gid = getNewGid(group);
	newuser.pw_gecos = (char *) gecos;   
	newuser.pw_dir = (char *) home_dir;
	newuser.pw_shell = (char *) shell;

#ifdef HAVE_SHADOW
	bshadow = is_shadow();
	if (bshadow)
	{
		/* Fill the shadow password structure */
		sp_newuser.sp_namp = (char *) name;
		sp_newuser.sp_pwdp = "*";
		newuser.pw_passwd = "x";
		sp_newuser.sp_lstchg = time(NULL) / (60 * 60 * 24);
		sp_newuser.sp_min = 0;
		sp_newuser.sp_max = 99999;
		sp_newuser.sp_warn = 7;
		sp_newuser.sp_inact = -1;
		sp_newuser.sp_expire = -1;
		sp_newuser.sp_flag = (unsigned long int) -1;
	}
	else
	{
		newuser.pw_passwd = "*";
	}
#else
	newuser.pw_passwd = "*";
#endif

	if (newuser.pw_uid != -1 )
	{				
		/* Write the password entry
		 * We must be careful to add this entry BEFORE
		 * a possible NIS entry (+:::::). We therefore
		 * use fgets instead of getpwent to read the password
		 * file.
		 */
		if ((fPassFile = fopen(PASSWDFILE, "r")) == NULL)
		{
			return -1;
		}
		if ((fTempPassFile = fopen(PASSWDTEMPFILE, "w")) == NULL)
		{
			return -1;
		}
		else
		{
			/* Make sure the new passwd file has the correct permission/ownership	*/
			chmod(PASSWDTEMPFILE, 0644);	/* /etc/passwd IS world readable	*/
			uid = 0;			/* Owned by root.root 			*/
			gid = 0;
			chown(PASSWDTEMPFILE, uid, gid);
		}
		while(1)
		{
			if (fgets(pwentry, sizeof(pwentry), fPassFile) == NULL)
			{
				break;
			}
			if((pwentry[0] == '+') || (pwentry[0] == '-'))
			{
				bnis = 1;
				break;
			}
			fputs(pwentry, fTempPassFile);
		}
		putpwent(&newuser, fTempPassFile);
		/* If there are some NIS entries left in the password file,
		 * add them.
		 */
		if (bnis)
		{
			do
			{
				fputs(pwentry, fTempPassFile);
			} while (fgets(pwentry, sizeof(pwentry), fPassFile) != NULL);
		}
		fclose(fPassFile);
		fclose(fTempPassFile);
#ifdef HAVE_SHADOW
		if (bshadow)
		{
			bnis = 0;
			
			/* Do the same thing for shadow passwords. */
			if ((fPassFile = fopen(SHADOWFILE, "r")) == NULL)
			{
				return -1;
			}
			if ((fTempPassFile = fopen(SHADOWTEMPFILE, "w")) == NULL)
			{
				return -1;
			}
			else
			{
				/* Set the correct permission and ownership */

				if (chmod(SHADOWTEMPFILE, 0640) != 0)	/* NOT world-readable				*/
					return -1;
				uid = 0;				/* Owned by root				*/
				gr_shadow = getgrnam("shadow"); 	/* The shadow password file should belong to	*/
				if (gr_shadow != NULL)			/* the group 'shadow', if such a group exists.  */
					gid = gr_shadow->gr_gid;
				else
					gid = 0;
				if (chown(SHADOWTEMPFILE, uid, gid) != 0)
					return -1;
			}
			while(1)
			{
				if (fgets(pwentry, sizeof(pwentry), fPassFile) == NULL)
				{
					break;
				}
				if((pwentry[0] == '+') || (pwentry[0] == '-'))
				{
					bnis = 1;
					break;
				}
				fputs(pwentry, fTempPassFile);
			}
			putspent(&sp_newuser, fTempPassFile);
			/* If there are some NIS entries left in the password file,
			 * add them.
			 */
			if (bnis)
			{
				do
				{
					fputs(pwentry, fTempPassFile);
				} while (fgets(pwentry, sizeof(pwentry), fPassFile) != NULL);
			}
			fclose(fPassFile);
			fclose(fTempPassFile);
		}
#endif
	}	

	/* Replace the passwd and shadow files by the new ones */
	if (rename(PASSWDTEMPFILE, PASSWDFILE) != 0)
		return -1;
#ifdef HAVE_SHADOW
	if (bshadow)
	{
		if (rename(SHADOWTEMPFILE, SHADOWFILE) != 0)
			return -1;
	}
#endif
	return 0;
}

/* Remove the user 'username' from the password file(s).
 *
 * Return 0 on success, and -1 otherwise.
 */
int removeUser(const char *username)
{
	char pwentry[1024];
	struct passwd *pw;
	FILE *fTempPasswdFile;
	FILE *fPasswdFile;
	uid_t uid;
	gid_t gid;
#ifdef HAVE_SHADOW
	int bshadow;
	char spentry[1024];
	struct spwd *sp;
	FILE *fTempShadowFile;
	FILE *fShadowFile;
	struct group *gr_shadow;
#endif

	/* If the user does not exist, exit. */
	pw = getpwnam(username);
	if (pw == NULL)
		return -1;

	if ((fPasswdFile = fopen(PASSWDFILE, "r")) == NULL)
	{
		return -1;
	}
	if ((fTempPasswdFile = fopen(PASSWDTEMPFILE, "w")) == NULL)
	{
		fclose(fPasswdFile);
		return -1;
	}
	else
	{
		/* Make sure the new passwd file has the correct permission/ownership		*/
		if (chmod(PASSWDTEMPFILE, 0644) != 0)	/* /etc/passwd IS world readable	*/
			return -1;
		uid = 0;				/* Owned by root.root 			*/
		gid = 0;
		if (chown(PASSWDTEMPFILE, uid, gid) != 0)
			return -1;
	}

	/* Copy the passwd file to a temp file, without the entry we want to remove
	 * We can'to use [get|put]pwent() because of NIS.
	 */
	while(1)
	{
		if (fgets(pwentry, sizeof(pwentry), fPasswdFile) == NULL)
		{
			break;
		}
		if(strncmp(pwentry, username, strlen(username)) != 0)
		{
			fputs(pwentry, fTempPasswdFile);
		}
	}
	fclose(fPasswdFile);
	fclose(fTempPasswdFile);
	
#ifdef HAVE_SHADOW
	bshadow = is_shadow();
	if (bshadow)
	{
		/* Copy the shadow file to a temp file, without the entry we want to remove */
		sp = getspnam(username);
		if (sp == NULL)
			return -1;

		if ((fShadowFile = fopen(SHADOWFILE, "r")) == NULL)
		{
			return -1;
		}

		if ((fTempShadowFile = fopen(SHADOWTEMPFILE, "w")) == NULL)
		{
			fclose(fShadowFile);
			return -1;
		}
		else
		{
			/* Set the correct permission and ownership */

			if (chmod(SHADOWTEMPFILE, 0640) != 0) /* NOT world-readable			*/
				return -1;
			uid = 0;			/* Owned by root				*/
			gr_shadow = getgrnam("shadow"); /* The shadow password file should belong to	*/
			if (gr_shadow != NULL)		/* the group 'shadow', if such a group exists.	*/
				gid = gr_shadow->gr_gid;
			else
				gid = 0;
			if (chown(SHADOWTEMPFILE, uid, gid) != 0)
				return -1;
		}

		/* Copy the shadow file to a temp file, without the entry we want to remove
		 * We can'to use [get|put]spent() because of NIS.
		 */
		while(1)
		{
			if (fgets(spentry, sizeof(spentry), fShadowFile) == NULL)
			{
				break;
			}
			if(strncmp(spentry, username, strlen(username)) != 0)
			{
				fputs(spentry, fTempShadowFile);
			}
		}
		fclose(fShadowFile);
		fclose(fTempShadowFile);
	}
#endif

	/* Replace the passwd and shadow files by the new ones */
	if (rename(PASSWDTEMPFILE, PASSWDFILE) != 0)
		return -1;
#ifdef HAVE_SHADOW
	if (bshadow)
	{
		if (rename(SHADOWTEMPFILE, SHADOWFILE) != 0)
			return -1;
	}
#endif
	return 0;
}

/* Create the home directory for a user, as specified in the password file.
 * All directories along the path are created if necessary.
 *
 * Return 0 if successfully created, -1 otherwise.
 */
int createHomeDir(const char *user)
{
	struct passwd *pw;
	char tmp[NAME_MAX];
	char *homedir, *curptr;
	int length;

	pw = getpwnam(user);
	if(pw == NULL)
		return -1;
	
	homedir = pw->pw_dir;
	curptr = homedir;

	/* Create all the directories leading to the final directory,
	 * if necessary. */
	while (*++curptr)
	{
		if (*curptr != '/')
			continue;
		length = curptr - homedir;
		strncpy(tmp, homedir, length);
		tmp[length] = '\0';
		if (access(tmp, F_OK) != 0)
		{
			if (mkdir(tmp, 0755) != 0)
				return -1;
		}
	}
	/* If the final dir hasn't ben created in the previous loop,
	 * create it now. */
	if (access(homedir, F_OK) != 0)
		if (mkdir(homedir, 0755) != 0)
			return -1;

	/* The home directory is owned by the user */
	if (chown(homedir, pw->pw_uid, pw->pw_gid) != 0)
		return -1;
	
	/* Copy the /etc/skel directory into the user's home directory */
	if (copy_dir(SKELDIR, pw->pw_dir, pw->pw_uid, pw->pw_gid) != 0)
		return -1;

	return 0;
}

/* Copy the content of the src directory into the dest directory.
 * This function recurses through subdirs if any.
 * The files and directories created in dest belong to the user
 * identified by uid and gid.
 *
 * Return -1 if there was an error, 0 otherwise.
 */
int copy_dir(const char *src, const char *dest, uid_t uid, gid_t gid)
{
	DIR *directory;
	struct dirent *directory_entry;
	struct stat st;
	char filename_src[NAME_MAX];
	char filename_dest[NAME_MAX];
	int fd_src, fd_dest;
	char buf[1024];
	int nb_read;
	
	if ((access(src, F_OK) != 0) || (access(dest, F_OK) != 0))
		return -1;

	if ((directory = opendir(src)) == NULL)
		return -1;

	while ((directory_entry = readdir(directory)) != NULL)
	{
		if ((strcmp(directory_entry->d_name, ".") == 0) ||
				(strcmp(directory_entry->d_name, "..") == 0))
			continue;
		
		sprintf(filename_src, "%s/%s", src, directory_entry->d_name);
		sprintf(filename_dest, "%s/%s", dest, directory_entry->d_name);
		stat(filename_src, &st);
		if(S_ISDIR(st.st_mode))
		{
			/* Recursively copy the subdirectory */
			if (mkdir(filename_dest, st.st_mode & 0777) != 0)
				return -1;
			if (chown(filename_dest, uid, gid) != 0)
				return -1;
			if (copy_dir(filename_src, filename_dest, uid, gid) != 0)
				return -1;
		}
		else if (S_ISREG(st.st_mode) || S_ISLNK(st.st_mode))
		{
			/* Copy the file */
			if ((fd_src = open(filename_src, O_RDONLY)) == -1)
			{
				return -1;
			}
			if ((fd_dest = open(filename_dest, O_WRONLY | O_CREAT)) == -1)
			{
				return -1;
			}
			if (fchmod(fd_dest, st.st_mode & 0777) != 0)
				return -1;
			if (fchown(fd_dest, uid, gid) != 0)
				return -1;
			while ((nb_read = read(fd_src, (void *) buf, sizeof(buf))) > 0)
			{
				write(fd_dest, (void *) buf, nb_read);
			}
			if (nb_read == -1)
			{
				return -1;
			}
			close(fd_src);
			close(fd_dest);
		}
		else
		{
			/* We ignore any special file (fifo, socket...) */
		}
	}

	closedir(directory);
	return 0;
}

/*
 * Rename the olduser to newuser.
 *
 * Returns -1 if olduser doesn't exist, or if an error occured,
 * and 0 on success.
 */
int renameUser(const char *olduser, const char *newuser)
{
	FILE *fTempPasswdFile;
	FILE *fPasswdFile;
	uid_t uid;
	gid_t gid;
	char pwentry[1024];
	struct passwd *pw;
	struct passwd newpw;
#ifdef HAVE_SHADOW
	int bshadow;
	char spentry[1024];
	struct spwd *sp;
	struct spwd newsp;
	FILE *fTempShadowFile;
	FILE *fShadowFile;
	struct group *gr_shadow;
#endif

	/* If the user does not exist, we can't rename it... */
	if((pw = getpwnam(olduser)) == NULL)
	{
		return -1;
	}
	
	if ((fPasswdFile = fopen(PASSWDFILE, "r")) == NULL)
	{
		return -1;
	}
	if ((fTempPasswdFile = fopen(PASSWDTEMPFILE, "w")) == NULL)
	{
		fclose(fPasswdFile);
		return -1;
	}
	else
	{
		/* Make sure the new passwd file has the correct permission/ownership	 */
		if (chmod(PASSWDTEMPFILE, 0644) != 0)	/* /etc/passwd IS world readable */
			return -1;
		uid = 0;				/* Owned by root.root 		 */
		gid = 0;
		if (chown(PASSWDTEMPFILE, uid, gid) != 0)
			return -1;
	}

	/* Prepare the password entry */
	newpw.pw_name	= (char *) newuser;
	newpw.pw_passwd = pw->pw_passwd;
	newpw.pw_uid	= pw->pw_uid;
	newpw.pw_gid	= pw->pw_gid;
	newpw.pw_gecos	= pw->pw_gecos;
	newpw.pw_dir	= pw->pw_dir  ;
	newpw.pw_shell	= pw->pw_shell;
	
	while (1)
	{
		if (fgets(pwentry, sizeof(pwentry), fPasswdFile) == NULL)
			break;
		if (strncmp(pwentry, olduser, strlen(olduser)) == 0)
		{
			putpwent(&newpw, fTempPasswdFile);
		}
		else
		{
			fputs(pwentry, fTempPasswdFile);
		}
	}
	fclose(fPasswdFile);
	fclose(fTempPasswdFile);

#ifdef HAVE_SHADOW
	bshadow = is_shadow();
	if (bshadow)
	{
		/* Copy the shadow file to a temp file, without the entry we want to remove */
		sp = getspnam(olduser);
		if (sp == NULL)
			return -1;

		/* Prepare the new shadow password entry */
		newsp.sp_namp	= (char *) newuser;
		newsp.sp_pwdp	= sp->sp_pwdp;
		newsp.sp_lstchg	= sp->sp_lstchg;
		newsp.sp_min	= sp->sp_min;
		newsp.sp_max	= sp->sp_max;
		newsp.sp_warn	= sp->sp_warn;
		newsp.sp_inact	= sp->sp_inact;
		newsp.sp_expire	= sp->sp_expire;
		newsp.sp_flag	= sp->sp_flag;
			
		if ((fShadowFile = fopen(SHADOWFILE, "r")) == NULL)
		{
			return -1;
		}

		if ((fTempShadowFile = fopen(SHADOWTEMPFILE, "w")) == NULL)
		{
			fclose(fShadowFile);
			return -1;
		}
		else
		{
			/* Set the correct permission and ownership */

			if (chmod(SHADOWTEMPFILE, 0640) != 0) /* NOT world-readable			*/
				return -1;
			uid = 0;			/* Owned by root				*/
			gr_shadow = getgrnam("shadow"); /* The shadow password file should belong to	*/
			if (gr_shadow != NULL)		/* the group 'shadow', if such a group exists.	*/
				gid = gr_shadow->gr_gid;
			else
				gid = 0;
			if (chown(SHADOWTEMPFILE, uid, gid) != 0)
				return -1;
		}

		/* Copy the shadow file to a temp file, without the entry we want to remove
		 * We can'to use [get|put]spent() because of NIS.
		 */
		while(1)
		{
			if (fgets(spentry, sizeof(spentry), fShadowFile) == NULL)
			{
				break;
			}
			if(strncmp(spentry, olduser, strlen(olduser)) == 0)
			{
				putspent(&newsp, fTempShadowFile);
			}
			else
			{
				fputs(spentry, fTempShadowFile);
			}
		}
		fclose(fShadowFile);
		fclose(fTempShadowFile);
	}
#endif

	/* Replace the passwd and shadow files by the new ones */
	if (rename(PASSWDTEMPFILE, PASSWDFILE) != 0)
		return -1;
#ifdef HAVE_SHADOW
	if (bshadow)
	{
		if (rename(SHADOWTEMPFILE, SHADOWFILE) != 0)
			return -1;
	}
#endif
	return 0;
}

/*
 * This simple function just checks if the user exists.
 *
 * Returns 0 if it exists, -1 otherwise.
 */
int checkUser(const char *username)
{
	if (getpwnam(username) == NULL)
		return -1;
	else
		return 0;
}
