/*
  _____     ___ ____ 
   ____|   |    ____|      PS2FTP
  |     ___|   |____       (C)2003, John Kelley (warren@halifrag.com)
  ---------------------------------------------------------------------
  ps2ftp.c
            Simple single threaded FTP server for the EE.
*/

#include "ps2ftp.h"

/* function prototypes */
void serverThread ();
void HandleClient (const int);
enum command_types get_command (const int com_sock, const char *buf, const int bufsize);
void processCommand(const int com_sock, const enum command_types ftpcommand, const char *cmd_buf, enum command_modes *command_mode);

/* Variables */
FSMan *vfs;
static char *months[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
bool isPASV;
int PASVport;
int PASVSock;
/*
 *	Send a FTP reply to the client
 *  If code is negative it means that the message is coninued
 *  and the client will simple print it out and wait for the 
 *  next, allows multiline status msgs to be returned to client
 */
int
reply (const int sock, const int code, const char *msg)
{
	char replybuf[256] __attribute__ ((aligned (64)));
	int ret;

	//let - #'s for code indicate that there is another line to send
	if (code < 0)
		snprintf (replybuf, 256, "%d- %s\r\n", (code * -1), msg);
	else
		snprintf (replybuf, 256, "%d %s\r\n", code, msg);
	ret = send (sock, replybuf, strlen (replybuf), 0);
	if (ret < 0) {
		perror ("PS2FTP: send error %d\n", ret);
		return -1;
	} else {
		return ret;
	}
}

/*
	Initiate a connection to a tcp/ip port in the given address 
	Return a socket to the connected port.
	Return -1 if an error occurs
	NOTE: had to make it static inline or the sock would seem
	      to open and send data but it would not happen *shrug*
		  possibly same problem as sjeep has in pgen in loading
		  hdd modules
*/
static inline int 
connect_tcpip (const struct sockaddr_in *data_address, const int data_length, const int com_sock)
{
	int sock;

	perror("Creating new socket...\n");
	sock = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock < 0) {
		reply (com_sock, 550, "connect_tcpip:sock() failed");
		printf ("connect_tcpip:sock() failed %d", sock);
		return -1;
	}
	perror("Connecting socket to remote host...\n");
	if (connect(sock, (struct sockaddr *) data_address, data_length) < 0) {
		reply (com_sock, 550, "connect_tcpip:connect() failed");
		printf ("FATAL: connect_tcpip:connect() failed\n");
		return -1;
	}
	printf("Connected to remote host, socket is #%d\n", sock);
	return sock;
}

/* Get a ftp command from the command stream, convert to command code and
parse command argument */
enum command_types
get_command (const int com_sock, char *buf, const unsigned int bufsize)
{
	char cmd[5];
	unsigned int i, j, rcvSize;

	rcvSize = recv (com_sock, buf, bufsize, 0);

	//Parse out the 3-4 letter FTP command
	memcpy (cmd, buf, 5);
	for (i = 0; i < 5; i++)
		if (!isAlphaNum (cmd[i]))
			cmd[i] = '\0';

	//advance i to the start of the command argument
	i = strlen (cmd);
	while (i < rcvSize && !isPrintable(buf[i]) || buf[i] == 0x20)
		i++;

	//copy the command argument into buf stopping at
	// a non printable char or when we hit the size 
	// of our buffer
	j = 0;
	while (i < rcvSize && isPrintable (buf[i])) {
		buf[j] = buf[i];
		i++;
		j++;
	}

	//terminate string
	buf[j] = '\0';
	// Fix Buffer Overflow
	buf[bufsize - 1] = '\0';

	//Find FTP Command and return its enum
	for (i = 0; i < sizeof (command_lookup) / sizeof (command_lookup[0]); ++i)
		if (strncmp (cmd, command_lookup[i].command, 4) == 0)
			return command_lookup[i].command_code;

	//Return error as we don't recognize that command
	return ERROR;
}

void setupIP() {
	int i;
	scr_printf("Net settings: %s %s %s\n", ip, netmask, gw);
	memset(if_conf, 0x00, sizeof(if_conf));
	i = 0;
	strncpy(&if_conf[i], ip, 15);
	i += strlen(ip) + 1;

	strncpy(&if_conf[i], netmask, 15);
	i += strlen(netmask) + 1;

	strncpy(&if_conf[i], gw, 15);
	i += strlen(gw) + 1;

	if_conf_len = i;
}

int
main (int argc, char **argv)
{
	int ret;
	bool underPS2Link = false;
	
	//causes an exception *shrug*
	//printf("argv0: %s\n", argv[0]);
	//Test to see if we're running under PS2Link
	//f (strncmp(argv[0], "host", 4) == 0)
	//	underPS2Link = true;
	isPASV = false;
	PASVport = 200;

	init_scr();
	printf("PS2FTP v%d.%d starting...\n", verMajor, verMinor);
	
	if (!underPS2Link) {
		SifExitRpc();
		SifIopReset("rom0:UDNL rom0:EELOADCNF", 0);
		while(!SifIopSync());
	}
	SifInitRpc(0);

	//load dev9 and smap here
	printf("Loading Network Adapter drivers...\n");
	if (!underPS2Link) {
		setupIP();
		SifExecModuleBuffer(&iomanx_irx, size_iomanx_irx, 0, NULL, &ret);
		printf("\tLoaded IOMANX: %d\n", ret);
		SifExecModuleBuffer(&ps2dev9_irx, size_ps2dev9_irx, 0, NULL, &ret);
		printf("\tLoaded PS2DEV9: %d\n", ret);
		SifExecModuleBuffer(&ps2ip_irx, size_ps2ip_irx, 0, NULL, &ret);
		printf("\tLoaded PS2IP: %d\n", ret);
		SifExecModuleBuffer(&ps2ips_irx, size_ps2ips_irx, 0, NULL, &ret);
		printf("\tLoaded PS2IPS: %d\n", ret);
		SifExecModuleBuffer(&ps2smap_irx, size_ps2smap_irx, if_conf_len, &if_conf[0], &ret);
		printf("\tLoaded PS2SMAP: %d\n", ret);
	}
	
	printf("Initialising IP stack...\n");
	ps2ip_init();

	printf("Initialising filesystems:\n");
	vfs = new FSMan(true /*mc*/, false /*cdvd*/, false /*hdd*/);

	serverThread ();
	return 0;
}

void
serverThread ()
{
	int sh, command_socket, clntLen, rc;
	struct sockaddr_in ftpServAddr __attribute__ ((aligned (64)));
	struct sockaddr_in ftpClntAddr __attribute__ ((aligned (64)));

	printf("Server started.\n");

	if ((sh = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		printf("FATAL: Failed to create socket: %d\n", sh);
		SleepThread ();
	}

	memset (&ftpServAddr, 0, sizeof (ftpServAddr));
	ftpServAddr.sin_family = AF_INET;
	ftpServAddr.sin_addr.s_addr = htonl (INADDR_ANY);
	ftpServAddr.sin_port = htons (cmdPORT);

	rc = bind (sh, (struct sockaddr *) &ftpServAddr, sizeof (ftpServAddr));
	if (rc < 0) {
		perror ("FATAL: Failed to bind socket!\n");
		SleepThread ();
	}

	rc = listen (sh, 2);
	if (rc < 0) {
		perror ("FATAL: Could not listen on socket!\n");
		SleepThread ();
	}
	printf("entering accept loop\n");
	while (1) {
		clntLen = sizeof (ftpClntAddr);
		command_socket = accept (sh, (struct sockaddr *) &ftpClntAddr, &clntLen);
		if (command_socket < 0) {
			perror ("FATAL: Could not accept connection!\n");
			SleepThread ();
		}
	printf("I got a client\n");
		HandleClient (command_socket);
	}
}

bool setupPASV() {
	int rc;
	struct sockaddr_in ftpPASVAddr;

	if ((PASVSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		printf("PASV: Failed to create socket: %d\n", PASVSock);
		return false;
	}

	memset (&ftpPASVAddr, 0, sizeof (ftpPASVAddr));
	ftpPASVAddr.sin_family = AF_INET;
	ftpPASVAddr.sin_addr.s_addr = htonl (INADDR_ANY);
	ftpPASVAddr.sin_port = htons (PASVport);

	rc = bind (PASVSock, (struct sockaddr *) &ftpPASVAddr, sizeof (ftpPASVAddr));
	if (rc < 0) {
		perror ("PASV: Failed to bind socket!\n");
		return false;
	}

	rc = listen (PASVSock, 2);
	if (rc < 0) {
		perror ("PASV: Could not listen on socket!\n");
		return false;
	}

	return true;
}

void
HandleClient (const int com_sock)
{
	/* Create buffered file descriptors for reading and writing */
	/* Data buffer for reading FTP command codes */
	static char buf[256] __attribute__ ((aligned (64)));

	/* The initial command mode */
	enum command_modes command_mode = LOGIN;

	/* Initiate connection by indicating that FTP server is ready to accept
	commands */
	sprintf(buf, "PS2FTP Server v%d.%d Ready", verMajor, verMinor);
	reply(com_sock, 220, buf);
	while(command_mode != EXIT) { /* Execute commands until QUIT issued */
		/* Get FTP command from input stream */
		enum command_types ftpcommand = get_command(com_sock, buf, sizeof(buf));

		switch (command_mode) {
			case LOGIN:  /* In login mode, only the USER command accepted */
				if (ftpcommand == USER) {
					if (strcmp (buf, "anonymous") == 0 || strcmp (buf, "ftp") == 0) {
						command_mode = PASSWORD;
						reply (com_sock, 331, "anonymous login accepted, enter email as password");
					} else {
						reply (com_sock, 504, "Invalid username");
					}
				} else
					reply (com_sock, 530, "user not logged in");
				break;
			case PASSWORD:  /* In password mode, only the PASS command accepted */
				if (ftpcommand == PASS) {
					command_mode = COMMANDS;
					reply (com_sock, 230, "user logged in");
				} else {
					reply (com_sock, 530, "user not logged in");
					command_mode = LOGIN;
				}
				break;
			case COMMANDS:  /* In the command mode, accept all other commands */
				processCommand(com_sock, ftpcommand, buf, &command_mode);
				break;
			case EXIT:
				break;
		}
	}
	//Disconnect
	disconnect (com_sock);
}

void 
processCommand(const int com_sock, const enum command_types ftpcommand, const char *cmd_buf, enum command_modes *command_mode) {
//	static char filename[256];
	char buf[256] __attribute__ ((aligned (64)));
	//static char buf2[256];
	char *tok; //for strtok in PORT
	//static t_aioDent fileEntry; //for SIZE command
	int ret=0, i, ls_sock;//, fd, bytes;
	
	//PASV stuff
	struct sockaddr_in PASVClntAddr;
	int clntLen;
	
	/* Address of the data port used to transfer files */
	struct sockaddr_in data_address;
	int data_length = sizeof (data_address);

	/* Temporary Storage for tcp/ip address (PORT command)*/
	int a1, a2, a3, a4, p1, p2;

	switch (ftpcommand) { /* Check commands */
		case EOFC:  /* End Of File detected on command stream */

		case QUIT:
			*command_mode = EXIT;
			break;
		case SYST:  /* System command, return something but not much */
			reply (com_sock, 215, "UNIX Type: L8");
			break;
		case PWD:  /* Print working directory */
			sprintf (buf, "\"%s\" is the current directory.", vfs->currentDir);
			reply (com_sock, 257, buf);
			break;
		case CWD:  /* Change working directory */
			/*
			if (isDir (buf)) { // If we have an absolute path
			if (buf[0] == '/') { //keep /dirname pattern
			strcpy (currentDir, buf);
			perror ("last char is: %c\n", buf[strlen (buf) - 1]);
			if (buf[strlen (buf) - 1] != '/')
			strcat (currentDir, "/");
			perror ("currentdir now: %s\n", currentDir);
			} else {
			//sprintf (currentDir, "/%s", buf);
			strcpy (currentDir, "/");
			strcat (currentDir, buf);
			perror ("last char 2 is: %c\n", buf[strlen (buf) - 1]);
			if (buf[strlen (buf) - 1] != '/')
			strcat (currentDir, "/");
			perror ("currentdir 2 now: %s\n", currentDir);
			}
			reply (com_sock, 250, "CWD command successful.");
			} else {  //else assume that its a relative path
			sprintf (buf2, "%s%s", currentDir, buf);
			if (isDir (buf2)) {
			sprintf (currentDir, "%s%s", currentDir, buf);
			reply (com_sock, 250, "CWD command successful 2.");
			} else*/
			reply (com_sock, 550, "No such directory");
			//}
			break;
		case CDUP:
			/*i = strlen (currentDir);
			if (i < 5) {
			currentDir[0] = '/';
			currentDir[1] = '\0';
			reply (com_sock, 250, "CDUP command successful. 1");
			} else {
			i--;
			if (currentDir[i] == '/')
			currentDir[i] = '\0';
			i--;
			while (i >= 0 && currentDir[i] != '/')
			i--;
			if (i == 0) {
			currentDir[0] = '/';
			currentDir[1] = '\0';
			} else {
			currentDir[i] = '\0';
			}*/
			reply (com_sock, 250, "CDUP command successful.");
			//}
			break;
		case TYPE:  /* Accept file TYPE commands, but always translate to BIN */
			reply (com_sock, 200, "Type set to I.");
			break;
		case LIST:

			perror("In List\n");
			//open socket to client
			if (isPASV) {
				printf("Handling PASV transfer\n");
				clntLen = sizeof (PASVClntAddr);
				ls_sock = accept (PASVSock, (struct sockaddr *) &PASVClntAddr, &clntLen);
				if (ls_sock < 0) {
					perror ("PASV: Could not accept connection!\n");
					break;
				}
			} else {
				printf("Handling PORT transfer\n");
				reply(com_sock, -150, "I don't know how you're here, PORT based xfers are not supported");
				reply(com_sock, -150, "due to a bug in ps2ip(s)'s connect function");
				//NOTICE: THIS AREA IS SCREWING UP, there is some kind of stack
				//        weirdness or something going on, any help is appreciated
				//        please only compile with -O0 right now, it seems to give
				//        the best results. Basically, the socket is being created
				//        and its reported that its connected to the client but it
				//        never actually does, check it with a packet sniffer. If
				//        you want to help with code please concentrate _only_ on
				//        helping figure out what's going on and getting dir listing
				//        to work. CWD and other commands depend on FSMan which I
				//        have not finished writing (I wanted to haev this working
				//        to test it with.
				ls_sock = connect_tcpip (&data_address, data_length, com_sock);
				perror("connection opened\n");
				if (ls_sock < 0) {
					reply (com_sock, 500, "PORT data connection failed");
					break;
				}
			} //end of PASV or PORT setup
			//get dir listing
			reply (com_sock, 150, "Opening BINARY mode data connection for /bin/ls.");
			i = 0;
			while (vfs->dirlisting[i].name[0] != '\0') {
				if (vfs->dirlisting[i].attrib && AIO_ATTRIB_DIR)
					sprintf(buf, "drw-rw-rw-  1 ps2ftp ps2ftp  %8d %s %d %02d:%02d %s\r\n",
					vfs->dirlisting[i].size,
					months[0], //month
					1, //day
					0, //hour
					1, //min
					vfs->dirlisting[i].name);	
				else
					sprintf(buf, "-rw-rw-rw-  1 ps2ftp ps2ftp  %8d %s %2d %02d:%02d %s\r\n",
					vfs->dirlisting[i].size,
					months[0], //month
					1, //day
					0, //hour
					1, //min
					vfs->dirlisting[i].name);

				perror("%s", buf);
				ret = send (ls_sock, buf, strlen (buf), 0);
				printf("sent %d bytes\n", ret);
				if (ret < 0) {
					perror ("Error sending dir listing: %d\n", ret);
					reply (com_sock, 500, "Error Sending directory listing");
					break;
				}
				i++;
			}
			disconnect (ls_sock);
			if (ret >= 0)
				reply (com_sock, 226, "Transfer complete.");
			break;
		case NOOP:  /* Acknowledge NOOP command */
			reply (com_sock, 200, "OK");
			break;
		case PASV:
			if (setupPASV()) {
				isPASV = true;
				tok = strtok (ip, ".");
				a1 = atoi (tok);
				tok = strtok (NULL, ".");
				a2 = atoi (tok);
				tok = strtok (NULL, ".");
				a3 = atoi (tok);
				tok = strtok (NULL, ".");
				a4 = atoi (tok);
				sprintf(buf, "Entering Passive Mode (%d,%d,%d,%d,%d,%d)", a1, a2, a3, a4, PASVport/256, PASVport - (PASVport/256));
				reply(com_sock, 227, buf);
			} else {
				isPASV = false;
				reply(com_sock, 550, "PASV Command Failed");
			}
			break;
		case PORT:  /* Set the tcp/ip address based on the given argument */
			
			//disable this as ps2ip(s)'s connect doesn't like to work
			/*
			isPASV = false;
			disconnect (PASVSock);
			// Grab data port address from command argument
			strncpy(buf, cmd_buf, 256);
			tok = strtok (buf, ",");
			a1 = atoi (tok);
			tok = strtok (NULL, ",");
			a2 = atoi (tok);
			tok = strtok (NULL, ",");
			a3 = atoi (tok);
			tok = strtok (NULL, ",");
			a4 = atoi (tok);
			perror ("addr: %d.%d.%d.%d\n", a1, a2, a3, a4);
			IP4_ADDR(&(data_address.sin_addr), a1, a2, a3, a4);
			tok = strtok (NULL, ",");
			p1 = atoi (tok);
			tok = strtok (NULL, ",");
			p2 = atoi (tok);

			perror ("port: %d\n", p1 * 256 + p2);
			data_address.sin_port = htons ((p1 * 256) + p2);

			reply (com_sock, 200, "PORT command successful");
			*/
			reply(com_sock, 550, "PORT command not supported due to ps2ip(s) bug. Use Passive transfers");
			break;
		case RETR:  /* Retrieve File and transfer it to data address */
			//TODO: STRIP OFF VFS DEVICE DIRECTORY
			/*sprintf (filename, "%s%s", currentDir, cmd_buf);

			if ((fd = vfs->currentDevice->open(filename, O_RDONLY)) < 0) {
				sprintf (buf, "File Does Not Exist: '%s'", filename);
			*/	reply (com_sock, 550, "File does not exist");//buf);
				break;
			/*} else
				perror ("Opened %s", filename);

			// Everything OK, so make data connection 
			sprintf (buf, "Opening data connection for '%s'", filename);
			reply (com_sock, 150, buf);

			// Create tcp/ip connection for data link 
			sock = connect_tcpip (&data_address, data_length, com_sock);
			if (sock < 0) {
				reply (com_sock, 550, "data connection failed");
				return;
			} else {
				// Transfer file using BINARY mode translation
				memset (buffer, '\0', BUFFER_SIZE);
				while ((bytes = vfs->currentDevice->read(fd, buffer, BUFFER_SIZE)) != 0) {
					if (send (sock, buffer, bytes, 0) < 0)
						break;
				}
				if (bytes != 0)
					reply (com_sock, 550, "error transfering file");
				else
					reply (com_sock, 226, "Transfer Complete");

				// Clean up 
				disconnect (sock);
				vfs->currentDevice->close(fd);
			}*/
			break;
		case STOR:
			//TODO: STRIP OFF VFS DEVICE DIRECTORY
			/*if (currentDir[strlen (currentDir) - 1] == '/')
				currentDir[strlen (currentDir) - 1] = '\0';
			if (currentDir[0] == '/')
				strcpy (buf2, &currentDir[1]);
			else
				strcpy (buf2, currentDir);
			sprintf (filename, "mc0:%s/%s", buf2, cmd_buf);
			perror ("filename: %s\n", filename);
			perror ("Before open\n");

			if ((fd = vfs->currentDevice->open(filename, O_WRONLY | O_CREAT)) < 0) {
				perror ("permission denied\n");
				sprintf (buf2, "Permission denied. Could not create file: '%s'",
					filename);
			*/	reply (com_sock, 553, "Permission denied."); //buf2);
				break;
			/*} else {
				perror ("opened\n");
			}
			// Everything OK, so make data connection 
			sprintf (buf2, "Opening BINARY mode data connection for '%s'", filename);
			reply (com_sock, 150, buf2);

			// Create tcp/ip connection for data link
			sock = connect_tcpip (&data_address, data_length, com_sock);
			if (sock < 0) {
				reply (com_sock, 550, "data connection failed");
				return;
			} else {

				// Transfer file using BINARY mode translation
				memset (buffer, '\0', BUFFER_SIZE);
				while ((bytes = recv (sock, buffer, BUFFER_SIZE, 0)) > 0) {
					perror ("read: %d bytes\n");
					if (vfs->currentDevice->write(fd, buffer, bytes) < 1)
						break;
				}

				if (bytes != 0)
					reply (com_sock, 550, "error transfering file");
				else
					reply (com_sock, 226, "Transfer Complete");

				// Clean up
				disconnect (sock);
				vfs->currentDevice->close(fd);
			}*/
			break;
		case DELE:
			//TODO: STRIP OFF VFS DEVICE DIRECTORY
			/*perror ("deleting: %s\n", buf);
			ret = vfs->currentDevice->remove(buf);
			if (ret == 0)
				reply (com_sock, 250, "DELE command successful.");
			else {
				sprintf (buf2, "Error %d returned from remove(%s).", ret, buf);
				reply (com_sock, 550, buf2);
			}*/
			reply (com_sock, 250, "Error executing DELE command");
			break;
		case RMD:
			//TODO: STRIP OFF VFS DEVICE DIRECTORY
			/*ret = vfs->currentDevice->rmdir(buf);
			if (ret == 0)
				reply (com_sock, 250, "RMD command successful.");
			else {
				sprintf (buf2, "Error %d returned from rmdir(%s).", ret, buf);
				reply (com_sock, 550, buf2);
			}*/
			reply (com_sock, 250, "Error executing RMD command");
			break;
		case MKD:
			//TODO: STRIP OFF VFS DEVICE DIRECTORY
			/*if (buf[0] == '/')
				sprintf (buf2, "%s", buf);
			else
				sprintf (buf2, "%s%s", currentDir, buf);

			perror ("MKD: %s\n", buf2);
			ret = vfs->currentDevice->mkdir(buf2);
			if (ret < 0) {
				sprintf (buf, "Could not create directory \"%s\" (%d)\n", buf2, ret);
				reply (com_sock, 550, buf);
			} else {
				sprintf (buf, "\"%s\" created.\n", buf2);
				reply (com_sock, 257, buf);
			}*/
			reply (com_sock, 250, "Error executing MKDIR command");
			break;
		case SIZE:
			//TODO: STRIP OFF VFS DEVICE DIRECTORY
			//sprintf (buf2, "%s%s", currentDir, buf);
			/*vfs->currentDevice->getstat(buf2, &fileEntry);
			bytes = fileEntry.size;
			if (bytes < 0) {
				perror ("SIZE: %s: No such file or directory\n", buf2);
				sprintf (buf2, "%s: No such file or is a directory", buf2);
				reply (com_sock, 550, buf2);
			} else {
				perror ("SIZE: %s is %d\n", buf2, bytes);
				sprintf (buf2, "%d", bytes);
				reply (com_sock, 250, buf2);
			}*/
			reply (com_sock, 250, "Error executing SIZE command");
			break;
		case PLNK: /*command to (re)load ps2link*/
			disconnect (com_sock);
			LoadExecPS2(PS2LINK_PATH, 0, NULL);
			break;
		default: /* Any command not implemented, return not recognized response. */
			reply(com_sock, 500, "command not recognized");
			break;
	}
}