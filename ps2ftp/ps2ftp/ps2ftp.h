#ifndef ps2ftp_h
#define ps2ftp_h

#include <kernel.h>
#include <ps2ip.h>
#include <loadfile.h>
#include <sifrpc.h>
#include <libmc.h>
#include <fileio.h>
#include <string.h>
#include "FSMan.h"
#include <iopcontrol.h>
#include <tamtypes.h>

//Set this to fit your setup so the PLNK command works
#define PS2LINK_PATH "mc0:/BOOT/PS2LINK/PS2LINK.ELF"

/* define DEBUG if you want to print status msgs via the 'perror' command
ie if running under naplink or ps2link*/
#define DEBUG

#define printf scr_printf
/* 'perror' definition for printf */
#ifdef DEBUG
#define perror scr_printf
#else
#define perror(FMT, __VA_LIST__...) {}
#endif

//hton[l,s] fix
#define ntohl(x) htonl(x)
#define ntohs(x) htons(x)
#define BUFFER_SIZE 1024
#define EOF (-1)

#define verMajor 0
#define verMinor 5
#define cmdPORT 21
#define isAlphaNum(x) isalnum(x)
#define isPrintable(x) isprint(x)

/* Vars for loading modules embedded in the elf */
extern u8 *iomanx_irx;
extern int size_iomanx_irx;
extern u8 *ps2dev9_irx;
extern int size_ps2dev9_irx;
extern u8 *ps2ip_irx;
extern int size_ps2ip_irx;
extern u8 *ps2ips_irx;
extern int size_ps2ips_irx;
extern u8 *ps2smap_irx;
extern int size_ps2smap_irx;

/* Default IP stuff */
char if_conf[3*16];
int if_conf_len;
char ip[16] = "192.168.0.10";
char netmask[16] = "255.255.255.0";
char gw[16] = "192.168.0.1";

/* FTP Command tokens */
enum command_types
{ USER, PASS, ACCT, CWD, CDUP, SMNT, REIN, QUIT, PORT, PASV, TYPE, STRU, MODE,
  RETR, STOR, STOU, APPE, ALLO, REST, RNFR, RNTO, ABOR, DELE, RMD, MKD, PWD,
  LIST, NLST, SITE, SIZE, SYST, STAT, HELP, NOOP, EOFC, ERROR, PLNK,
};

/* struct used to build a table mapping command character sequences to
   their command token listed above */
struct command_list
{
  char *command;
  enum command_types command_code;
};

/* Lookup table for FTP Commands
   NOTE: PLNK is a custom command to reload PS2LINK
         since we kick it off the IOP
*/
struct command_list command_lookup[] = {
  {"USER", USER}, {"PASS", PASS}, {"ACCT", ACCT}, {"CWD", CWD},
  {"CDUP", CDUP}, {"SMNT", SMNT}, {"REIN", REIN}, {"QUIT", QUIT},
  {"PORT", PORT}, {"PASV", PASV}, {"TYPE", TYPE}, {"STRU", STRU},
  {"MODE", MODE}, {"RETR", RETR}, {"STOR", STOR}, {"STOU", STOU},
  {"APPE", APPE}, {"ALLO", ALLO}, {"REST", REST}, {"RNFR", RNFR},
  {"RNTO", RNTO}, {"ABOR", ABOR}, {"DELE", DELE}, {"RMD", RMD},
  {"MKD", MKD}, {"XMKD", MKD}, {"XRMD", RMD}, {"PWD", PWD},
  {"XPWD", PWD}, {"LIST", LIST}, {"NLST", NLST}, {"SITE", SITE},
  {"SIZE", SIZE}, {"SYST", SYST}, {"STAT", STAT}, {"HELP", HELP},
  {"NOOP", NOOP}, {"EOFC", EOFC}, {"PLNK", PLNK},
};

enum command_modes {LOGIN, PASSWORD, COMMANDS, EXIT};

#endif
