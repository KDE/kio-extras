#ifndef SAMBA_EXTERNS_H
#define SAMBA_EXTERNS_H
#include "defines.h"
#ifdef USE_SAMBA

extern "C" {
BOOL cli_RNetShareEnum(struct cli_state *cli, void (*fn)(const char *, uint32, const char *));
void cli_shutdown(struct cli_state *cli);
struct cli_state *cli_initialise(struct cli_state *cli);
void make_nmb_name( struct nmb_name *n, const char *name, int type, const char *this_scope );
int cli_set_port(struct cli_state *cli, int port);
BOOL cli_establish_connection(struct cli_state *cli,
				const char *dest_host, struct in_addr *dest_ip,
				struct nmb_name *calling, struct nmb_name *called,
				char *service, char *service_type,
				BOOL do_shutdown, BOOL do_tcon);
BOOL cli_close(struct cli_state *cli, int fnum);
int cli_open(struct cli_state *cli, const char *fname, int flags, int share_mode);
BOOL cli_getattrE(struct cli_state *cli, int fd,
		  uint16 *attr, size_t *size,
		  time_t *c_time, time_t *a_time, time_t *m_time);
BOOL cli_getatr(struct cli_state *cli, char *fname,
		uint16 *attr, size_t *size, time_t *t);
size_t cli_read(struct cli_state *cli, int fnum, char *buf, off_t offset, size_t size, BOOL overlap);
ssize_t cli_write(struct cli_state *cli,
		  int fnum, uint16 write_mode,
		  char *buf, off_t offset, size_t size, size_t bytes_left);
BOOL cli_rename(struct cli_state *cli, char *fname_src, char *fname_dst);
BOOL cli_unlink(struct cli_state *cli, char *fname);
BOOL cli_mkdir(struct cli_state *cli, char *dname);
BOOL cli_rmdir(struct cli_state *cli, char *dname);
int cli_error(struct cli_state *cli, uint8 *eclass, uint32 *num);

void pwd_set_nullpwd(struct pwd_info *pwd);
void pwd_make_lm_nt_16(struct pwd_info *pwd, char *clr);
void pwd_init(struct pwd_info *pwd);

int cli_list(struct cli_state *cli,const char *Mask,uint16 attribute,
	     void (*fn)(file_info *, const char *));
BOOL cli_NetServerEnum(struct cli_state *cli, char *workgroup, uint32 stype,
		       void (*fn)(const char *, uint32, const char *));

BOOL get_any_dc_name(const char *domain, char *srv_name);
BOOL find_master_ip(char *group, struct in_addr *master_ip);

char *lp_workgroup(void);

char *safe_strcpy(char *dest,const char *src, size_t maxlength);
BOOL lp_client_ntlmv2(void);
BOOL get_myname(char *my_name,struct in_addr *ip);
BOOL lp_load(char *pszFname,BOOL global_only, BOOL save_defaults, BOOL add_ipc);
void TimeInit(void);
void charset_initialise(void);
void codepage_initialise(int client_codepage);
int lp_client_code_page(void);
void interpret_coding_system(char *str);
void load_interfaces(void);
char *lp_workgroup(void);

// variables in libsamba
BOOL in_client;
pstring global_myname;
pstring myhostname;
pstring scope;

// from kanji...
char *(*_dos_to_unix)(char *str, BOOL overwrite);
#define dos_to_unix(x,y) ((*_dos_to_unix)((x), (y)))
}

#endif
#endif