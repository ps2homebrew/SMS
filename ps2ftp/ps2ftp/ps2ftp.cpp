/*
  _____     ___ ____ 
   ____|   |    ____|      PS2FTP
  |      __|   |____       (C)2003, John Kelley (warren@halifrag.com)
  ---------------------------------------------------------------------
  ps2ftp.c
            Simple single threaded FTP server for the EE.
*/

#include "ps2ftp.h"


void serverThread ();
void HandleClient (int, struct sockaddr_in *);

enum command_types get_command (int com_sock, char *buf, int bufsize);
void retrieve (char *filename, struct sockaddr_in *data_address,
			   int data_length, int com_sock, char *buf);

u8 buffer[BUFFER_SIZE] __attribute__ ((aligned (64))); //for sending files
FSMan *vfs;
char currentDir[1024];
char *months[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

int
reply (int sock, int code, char *msg)
{
	static char replybuf[256] __attribute__ ((aligned (64)));
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
*/
int
connect_tcpip (struct sockaddr_in *data_address, int data_length, char *buf,
			   int com_sock)
{
	int sock;
	sock = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock < 0) {
		reply (com_sock, -550, "connect_tcpip:sock() failed");
		perror ("connect_tcpip:sock() failed %d", sock);
		return -1;
	}
	if (connect (sock, (struct sockaddr *) data_address, data_length) < 0) {
		reply (com_sock, -550, "connect_tcpip:connect() failed");
		printf ("FATAL: connect_tcpip:connect() failed\n");
		return -1;
	}
	return sock;
}

/* Get a ftp command from the command stream, convert to command code and
parse command argument */
enum command_types
get_command (int com_sock, char *buf, unsigned int bufsize)
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
	while (!isPrintable (buf[i]))
		i++;

	//copy the command argument into buf stopping at
	// a non printable char or when we hit the size 
	// of our buffer
	j = 0;
	while (isPrintable (buf[i]) && i < bufsize - 1) {
		buf[j] = buf[i];
		i++;
		j++;
	}
	//terminate string
	buf[j] = '\0';
	// Fix Buffer Overflow
	buf[bufsize - 1] = '\0';

	//printf("CMD='%s', ARG='%s'\n", cmd, buf);
	//Find FTP Command and return its enum
	for (i = 0; i < sizeof (command_lookup) / sizeof (command_lookup[0]); ++i)
		if (strncmp (cmd, command_lookup[i].command, 4) == 0)
			return command_lookup[i].command_code;

	//Return error as we don't recognize that command
	//printf ("UNKNOWN COMMAND: %s\n", cmd);
	return ERROR;
}

int
main (int argc, char **argv)
{
	printf("PS2FTP v%d.%d starting...\n", verMajor, verMinor);
	SifInitRpc (0);

	//load dev9 and smap here
	printf("Loading Network Adapter drivers...\n");
	
	printf("Initialising IP stack...\n");
	ps2ip_init ();

	printf("Initialising filesystems:\n");
	vfs = new FSMan(true /*mc*/, false /*cdvd*/, false /*hdd*/);

	serverThread ();
	return 0;
}

void
serverThread ()
{
	int sh;
	int command_socket;
	struct sockaddr_in ftpServAddr;
	struct sockaddr_in ftpClntAddr;
	int clntLen;
	int rc;

	printf("Server started.\n");

	sh = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sh < 0) {
		printf("FATAL: Failed to create socket!\n");
		SleepThread ();
	}
	//perror ("PS2FTP: Got socket.. %i\n", sh);


	memset (&ftpServAddr, 0, sizeof (ftpServAddr));
	ftpServAddr.sin_family = AF_INET;
	ftpServAddr.sin_addr.s_addr = htonl (INADDR_ANY);
	ftpServAddr.sin_port = htons (cmdPORT);

	rc = bind (sh, (struct sockaddr *) &ftpServAddr, sizeof (ftpServAddr));
	if (rc < 0) {
		perror ("FATAL: Failed to bind socket!\n");
		SleepThread ();
	}
	//perror ("PS2FTP: bind returned %i\n", rc);

	rc = listen (sh, 2);
	if (rc < 0) {
		perror ("FATAL: Could not listen on socket!\n");
		SleepThread ();
	}
	//perror ("PS2FTP: listen returned %i\n", rc);

	while (1) {
		clntLen = sizeof (ftpClntAddr);
		command_socket = accept (sh, (struct sockaddr *) &ftpClntAddr, &clntLen);
		if (command_socket < 0) {
			perror ("FATAL: Could not accept connection!\n");
			SleepThread ();
		}
		//perror ("PS2FTP: accept returned %i.\n", command_socket);

		HandleClient (command_socket, &ftpClntAddr);
	}
}


void
HandleClient (int com_sock, struct sockaddr_in *ftpClntAddr)
{
	/* Create buffered file descriptors for reading and writing */
	/* Data buffer for reading FTP command codes */
	char filename[256];
	char buf[512];
	char buf2[512];
	char *tok; //for strtok in PORT
	t_aioDent fileEntry; //for SIZE command

	int ret=0, i, ls_sock, fd, sock, bytes;
	/* Address of the data port used to transfer files */
	struct sockaddr_in data_address;
	int data_length = sizeof (data_address);

	/* The initial command mode */
	enum command_modes command_mode = LOGIN;

	/* Get the default data address for the transfer */

	/* Initiate connection by indicating that FTP server is ready to accept
	commands */
	sprintf(buf, "PS2FTP Server v%d.%d Ready", verMajor, verMinor);
	reply(com_sock, 220, buf);
	while(command_mode != EXIT) { /* Execute commands until QUIT issued */
		/* Get FTP command from input stream */
		enum command_types ftpcommand = get_command(com_sock, buf, sizeof(buf));

		/* Temporary Storage for tcp/ip address */
		int a1, a2, a3, a4, p1, p2;

		switch (command_mode) {
			case LOGIN:  /* In login mode, only the USER command accepted */
				if (ftpcommand == USER) {
					if (strcmp (buf, "anonymous") == 0 || strcmp (buf, "ftp") == 0) {
						command_mode = PASSWORD;
						reply (com_sock, 331,
							"anonymous login accepted, enter email as password");
					} else {
						perror ("user: >%s<\n", buf);
						reply (com_sock, 504, buf);
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
				switch (ftpcommand) { /* Check commands */
			case EOFC:  /* End Of File detected on command stream */
			case QUIT:
				command_mode = EXIT;
				break;
			case SYST:  /* System command, return something but not much */
				reply (com_sock, 215, "UNIX Type: L8");
				break;
			case PWD:  /* Print working directory */
				sprintf (buf2, "\"%s\" is the current directory.", vfs->currentDir);
				reply (com_sock, 257, buf2);
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
					} else
						reply (com_sock, 550, "No such directory");
				}*/
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
					}
					reply (com_sock, 250, "CDUP command successful.");
				}*/

				break;
			case TYPE:  /* Accept file TYPE commands, but always translate to BIN */
				reply (com_sock, 200, "Type set to I.");
				break;
			case LIST:
				//open socket to client
				ls_sock = connect_tcpip (&data_address, data_length, buf, com_sock);
				if (ls_sock < 0) {
					reply (com_sock, 500, "data connection failed");
				} else {
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

						ret = send (ls_sock, buf, strlen (buf), 0);
						if (ret < 0) {
							perror ("Error sending dir listing: %d\n", ret);
							reply (com_sock, 500, "Error Sending directory listing");
							break;
						}
						i++;
					}
					if (ret >= 0)
						reply (com_sock, 226, "Transfer complete.");
				}
				//close socket
				disconnect (ls_sock);
				break;
			case NOOP:  /* Acknowledge NOOP command */
				reply (com_sock, 200, "OK");
				break;
			case PORT:  /* Set the tcp/ip address based on the given argument */
				/* Grab data port address from command argument */
				tok = strtok (buf, ",");
				a1 = atoi (tok);
				tok = strtok (NULL, ",");
				a2 = atoi (tok);
				tok = strtok (NULL, ",");
				a3 = atoi (tok);
				tok = strtok (NULL, ",");
				a4 = atoi (tok);
				perror ("addr: %d.%d.%d.%d\n", a1, a2, a3, a4);
				IP4_ADDR (&(data_address.sin_addr), a1, a2, a3, a4);
				tok = strtok (NULL, ",");
				p1 = atoi (tok);
				tok = strtok (NULL, ",");
				p2 = atoi (tok);

				perror ("port: %d\n", p1 * 256 + p2);
				data_address.sin_port = htons ((p1 * 256) + p2);

				reply (com_sock, 200, "PORT command successful"); //
				break;
			case RETR:  /* Retrieve File and transfer it to data address */
				//TODO: STRIP OFF VFS DEVICE DIRECTORY
				sprintf (filename, "%s%s", currentDir, buf);

				if ((fd = vfs->currentDevice->open(filename, O_RDONLY)) < 0) {
					sprintf (buf2, "File Does Not Exist: '%s'", filename);
					reply (com_sock, 550, buf2);
					break;
				} else
					perror ("Opened %s", filename);

				/* Everything OK, so make data connection */
				sprintf (buf2, "Opening data connection for '%s'", filename);
				reply (com_sock, 150, buf2);

				/* Create tcp/ip connection for data link */
				sock = connect_tcpip (&data_address, data_length, buf2, com_sock);
				if (sock < 0) {
					reply (com_sock, 550, "data connection failed");
					return;
				} else {
					/* Transfer file using BINARY mode translation */
					memset (buffer, '\0', BUFFER_SIZE);
					while ((bytes = vfs->currentDevice->read(fd, buffer, BUFFER_SIZE)) != 0) {
						if (send (sock, buffer, bytes, 0) < 0)
							break;
					}
					if (bytes != 0)
						reply (com_sock, 550, "error transfering file");
					else
						reply (com_sock, 226, "Transfer Complete");

					/* Clean up */
					disconnect (sock);
					vfs->currentDevice->close(fd);
				}
				break;
			case STOR:
				//TODO: STRIP OFF VFS DEVICE DIRECTORY
				if (currentDir[strlen (currentDir) - 1] == '/')
					currentDir[strlen (currentDir) - 1] = '\0';
				if (currentDir[0] == '/')
					strcpy (buf2, &currentDir[1]);
				else
					strcpy (buf2, currentDir);
				sprintf (filename, "mc0:%s/%s", buf2, buf);
				perror ("filename: %s\n", filename);
				perror ("Before open\n");
				
				if ((fd = vfs->currentDevice->open(filename, O_WRONLY | O_CREAT)) < 0) {
					perror ("permission denied\n");
					sprintf (buf2, "Permission denied. Could not create file: '%s'",
						filename);
					reply (com_sock, 553, buf2);
					break;
				} else {
					perror ("opened\n");
				}
				/* Everything OK, so make data connection */
				sprintf (buf2, "Opening BINARY mode data connection for '%s'", filename);
				reply (com_sock, 150, buf2);

				/* Create tcp/ip connection for data link */
				sock = connect_tcpip (&data_address, data_length, buf2, com_sock);
				if (sock < 0) {
					reply (com_sock, 550, "data connection failed");
					return;
				} else {

					/* Transfer file using BINARY mode translation */
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

					/* Clean up */
					disconnect (sock);
					vfs->currentDevice->close(fd);
				}
				break;
			case DELE:
				//TODO: STRIP OFF VFS DEVICE DIRECTORY
				perror ("deleting: %s\n", buf);
				ret = vfs->currentDevice->remove(buf);
				if (ret == 0)
					reply (com_sock, 250, "DELE command successful.");
				else {
					sprintf (buf2, "Error %d returned from remove(%s).", ret, buf);
					reply (com_sock, 550, buf2);
				}
				break;
			case RMD:
				//TODO: STRIP OFF VFS DEVICE DIRECTORY
				ret = vfs->currentDevice->rmdir(buf);
				if (ret == 0)
					reply (com_sock, 250, "RMD command successful.");
				else {
					sprintf (buf2, "Error %d returned from rmdir(%s).", ret, buf);
					reply (com_sock, 550, buf2);
				}
				break;
			case MKD:
				//TODO: STRIP OFF VFS DEVICE DIRECTORY
				if (buf[0] == '/')
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
				}
				break;
			case SIZE:
				//TODO: STRIP OFF VFS DEVICE DIRECTORY
				//sprintf (buf2, "%s%s", currentDir, buf);
				vfs->currentDevice->getstat(buf2, &fileEntry);
				bytes = fileEntry.size;
				if (bytes < 0) {
					perror ("SIZE: %s: No such file or directory\n", buf2);
					sprintf (buf2, "%s: No such file or is a directory", buf2);
					reply (com_sock, 550, buf2);
				} else {
					perror ("SIZE: %s is %d\n", buf2, bytes);
					sprintf (buf2, "%d", bytes);
					reply (com_sock, 250, buf2);
				}
				break;
			default: /* Any command not implemented, return not recognized response. */
				reply(com_sock, 500, "command not recognized");
				break;
				}
				break;
			case EXIT:
				break;
		}
	};
	//Disconnect
	disconnect (com_sock);
}
