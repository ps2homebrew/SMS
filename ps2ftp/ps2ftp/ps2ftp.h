#ifndef ps2ftp_h
#define ps2ftp_h
/* define DEBUG if you want to print status msgs via the 'perror' command
   ie if running under naplink or ps2link*/
//#define DEBUG
#define ARRAY_ENTRIES 64


#include <kernel.h>
#include <ps2ip.h>
#include <loadfile.h>
#include <sifrpc.h>
#include <libmc.h>
#include <fileio.h>
#include <string.h>
#include "FSMan.h"

/* 'perror' definition for printf */
#ifdef DEBUG
#define perror(FMT, __VA_LIST__...) printf(FMT, __VA_LIST__)
#else
#define perror(FMT, __VA_LIST__...) {}
#endif

//hton[l,s] fix
#define ntohl(x) htonl(x)
#define ntohs(x) htons(x)
#define BUFFER_SIZE 1024
#define EOF (-1)

#define verMajor 0
#define verMinor 2
#define cmdPORT 21
#define isAlphaNum(x) isalnum(x)
#define isPrintable(x) isprint(x)
/* FTP Command tokens */
enum command_types
{ USER, PASS, ACCT, CWD, CDUP, SMNT, REIN, QUIT, PORT, PASV, TYPE, STRU, MODE,
  RETR, STOR, STOU, APPE, ALLO, REST, RNFR, RNTO, ABOR, DELE, RMD, MKD, PWD,
  LIST, NLST, SITE, SIZE, SYST, STAT, HELP, NOOP, EOFC, ERROR
};

/* struct used to build a table mapping command character sequences to
   their command token listed above */
struct command_list
{
  char *command;
  enum command_types command_code;
};

/* Lookup table for FTP Commands */
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
  {"NOOP", NOOP}, {"EOFC", EOFC}
};

enum command_modes {LOGIN, PASSWORD, COMMANDS, EXIT};

#endif
