/*
  _____     ___ ____ 
   ____|   |    ____|      PS2FTP
  |     ___|   |____       (C)2003, John 'Warren' Kelley (warren@halifrag.com)
  ----------------------------------------------------------------------------
  ps2ftp.c
			   Simple single threaded FTP server for the IOP.
*/

#include "ps2ftp.h"
#define cmdPORT 21

void serverThread (void *arg);
void HandleClient (int cs);
void process_commands (int com_sock);
enum command_types get_command (int com_sock, char *buf, int bufsize);
void retrieve (char *filename, struct sockaddr_in *data_address,
	       int data_length, int com_sock, char *buf);
u8 buffer[BUFFER_SIZE] __attribute__ ((aligned (64)));
char currentDir[1024];
char *months[12] =
  { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct",
  "Nov", "Dec"
};



/*********************************************************************
	SUPPORT FUNCTIONS
*********************************************************************/
static inline int
isdigit (int c)
{
  return (c >= '0' && c <= '9');
}

static inline int
isAlphaNum (int c)
{
  return (c >= 48 && c <= 57) || (c >= 65 && c <= 90) || (c >= 97
							  && c <= 122);
}

static inline int
isPrintable (int c)
{
  return (c >= 33 && c <= 126);
}

int
atoi (const char *s)
{
  int sign = 1;
  int result = 0;
  int digit;

  if (*s == '-') {
    /* Leading minus sign. */
    sign = -1;
    s++;
  } else if (*s == '+') {
    /* Leading plus sign. */
    s++;
  }

  /* Stop at end of string. */
  while (*s != '\0') {
    /* Pass character to isdigit as an int,
       to preserve stack alignment. */
    if (!isdigit ((int) *s)) {
      /* Also stop if non-digit found. */
      break;
    }

    /* What value does this digit have?
     * This works because in ASCII, '0' (48) to
     * '9' (57) are contiguous. */
    digit = *s - '0';

    /* Append this digit to our running total. */
    result = result * 10 + digit;

    /* Next character. */
    s++;
  }

  /* Return result multiplied by sign. */
  return result * sign;
}

int
GetFileSize (char *file)
{
  int ret;

  ret = mc_getdir (0, 0, file, 0, ARRAY_ENTRIES - 10, mcDir);
  //mcSync(0, NULL, &ret);
  //if file doesn't exist
  if (ret != 1)
    return -1;
  else
    //if file is actually a directory
  if (mcDir[0].attrFile & MC_ATTR_SUBDIR)
    return -2;
  else
    //if everything is fine
    return mcDir[0].fileSizeByte;
}


/*********************************************************************
	END OF SUPPORT FUNCTIONS
*********************************************************************/



/* Initiate a connection to a tcp/ip port in the given address 
   Return a socket to the connected port.
   Return -1 if an error occurs */
int
connect_tcpip (struct sockaddr_in *data_address, int data_length, char *buf,
	       int com_sock)
{
  int sock;
  sock = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sock < 0) {
    reply (com_sock, -550, "connect_tcpip:sock() failed", buf);
    perror ("connect_tcpip:sock() failed %d", sock);
    return -1;
  }
  if (connect (sock, (struct sockaddr *) data_address, data_length) < 0) {
    reply (com_sock, -550, "connect_tcpip:connect() failed", buf);
    perror ("connect_tcpip:connect() failed");
    return -1;
  }
  return sock;
}

int
reply (int sock, int code, char *msg)
{
  static char replybuf[256];
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

int
_start (int argc, char **argv)
{
  struct t_thread t;
  int tid, ret;

  strcpy (currentDir, "/");
  perror ("PS2FTP: Module Loaded\n");
  perror ("mcInit returned: %d\n", mcInit ());

  t.type = TH_C;
  t.unknown = 0;
  t.function = serverThread;
  t.stackSize = 0x2800;
  t.priority = 0x1e;
  tid = CreateThread (&t);
  if (tid >= 0)
    StartThread (tid, NULL);
  else
    perror ("PS2FTP: Server thread failed to start. %i\n", tid);

  return 0;
}

void
serverThread (void *arg)
{
  int sh;
  int command_socket;
  struct sockaddr_in echoServAddr;
  struct sockaddr_in echoClntAddr;
  int clntLen;
  int rc;

  perror ("PS2FTP: Server Thread Started.\n");

  sh = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sh < 0) {
    perror ("PS2FTP: Socket failed to create.\n");
    SleepThread ();
  }
  perror ("PS2FTP: Got socket.. %i\n", sh);


  memset (&echoServAddr, 0, sizeof (echoServAddr));
  echoServAddr.sin_family = AF_INET;
  echoServAddr.sin_addr.s_addr = htonl (INADDR_ANY);
  echoServAddr.sin_port = htons (cmdPORT);

  rc = bind (sh, (struct sockaddr *) &echoServAddr, sizeof (echoServAddr));
  if (rc < 0) {
    perror ("PS2FTP: Socket failed to bind.\n");
    SleepThread ();
  }

  perror ("PS2FTP: bind returned %i\n", rc);


  rc = listen (sh, 2);
  if (rc < 0) {
    perror ("PS2FTP: listen failed.\n");
    SleepThread ();
  }

  perror ("PS2FTP: listen returned %i\n", rc);

  while (1) {
    clntLen = sizeof (echoClntAddr);
    command_socket = accept (sh, (struct sockaddr *) &echoClntAddr, &clntLen);
    if (command_socket < 0) {
      perror ("PS2FTP: accept failed.\n");
      SleepThread ();
    }

    perror ("PS2FTP: accept returned %i.\n", command_socket);

    HandleClient (command_socket);
  }
}


void
HandleClient (int command_socket)
{
  int rcvSize, sntSize;

  //start ftp processing here
  process_commands (command_socket);
  // disconnect the socket.
  disconnect (command_socket);
}



/*
  process_commands() provides a partial implementation of the FTP
  (File Transfer Protocol).  This routine executes until the user
  issues a QUIT command.  At that point, the connection is closed and
  control is returned to the caller.

  Note: This is not a complete implementation, many commands have been
  left out.  This is sufficient to login and retrieve a file.
*/
void
process_commands (int com_sock)
{
  /* Create buffered file descriptors for reading and writing */
  /* Data buffer for reading FTP command codes */
  char filename[256];
  char buf[512];
  char buff[256];
  char *buf2;

  int ret, numEntries, i, ls_sock, fd, sock, bytes;
  /* Address of the data port used to transfer files */
  struct sockaddr_in data_address;
  int data_length = sizeof (data_address);

  /* The initial command mode */
  enum command_modes command_mode = LOGIN;

  /* Get the default data address for the transfer */
  if (getsockname (com_sock, (struct sockaddr *) &data_address, &data_length)
      < 0) {
    perror ("process_commands():getsockname failed");
    reply (com_sock, 500, "getsockname failed");
    return;
  }

  /* Initiate connection by indicating that FTP server is ready to accept
     commands */
  reply (com_sock, 220, "PS2FTP Server v0.1 Beta Ready");
  while (command_mode != EXIT) {	/* Execute commands until QUIT issued */
    /* Get FTP command from input stream */
    enum command_types ftpcommand = get_command (com_sock, buf, sizeof (buf));

    /* Temporary Storage for tcp/ip address */
    int a1, a2, a3, a4, p1, p2;

    switch (command_mode) {
    case LOGIN:		/* In login mode, only the USER command accepted */
      if (ftpcommand == USER) {
	if (strcmp (buf, "anonymous") == 0 | strcmp (buf, "ftp") == 0) {
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
    case PASSWORD:		/* In password mode, only the PASS command accepted */
      if (ftpcommand == PASS) {
	command_mode = COMMANDS;
	reply (com_sock, 230, "user logged in");
      } else {
	reply (com_sock, 530, "user not logged in");
	command_mode = LOGIN;
      }
      break;
    case COMMANDS:		/* In the command mode, accept all other commands */
      switch (ftpcommand) {	/* Check commands */
      case EOFC:		/* End Of File detected on command stream */
      case QUIT:
	command_mode = EXIT;
	break;
      case SYST:		/* System command, return something but not much */
	reply (com_sock, 215, "UNIX Type: L8");
	break;
      case PWD:		/* Print working directory */
	sprintf (buff, "\"%s\" is the current directory.", currentDir);
	reply (com_sock, 257, buff);
	break;
      case CWD:		/* Change working directory */
	//reply (com_sock, -250, buf);
	if (isDir (buf)) {	// If we have an absolute path
	  if (buf[0] == '/') {	//keep /dirname pattern
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
	  //sprintf (buff, "CWD command successful. %s", currentDir);
	  reply (com_sock, 250, "CWD command successful.");
	} else {		//else assume that its a relative path
	  sprintf (buff, "%s%s", currentDir, buf);
	  if (isDir (buff)) {
	    //strcat (currentDir, buf);
	    //strcpy (currentDir, buff);
	    sprintf (currentDir, "%s%s", currentDir, buf);
	    reply (com_sock, 250, "CWD command successful 2.");
	  } else
	    reply (com_sock, 550, "No such directory");
	}
	break;
      case CDUP:
	i = strlen (currentDir);
	if (i < 5) {
	  currentDir[0] = '/';
	  currentDir[1] = '\0';
	  reply (com_sock, 250, "CDUP command successful. 1");
	} else {
	  //i = strlen (currentDir) - 1;
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
	}

	break;
      case TYPE:		/* Accept file TYPE commands, but always translate to ASCII */
	reply (com_sock, 200, "Type set to I.");
	break;
      case LIST:
	//open socket to client
	ls_sock = connect_tcpip (&data_address, data_length, buf, com_sock);
	if (ls_sock < 0) {
	  reply (com_sock, 500, "data connection failed");
	} else {
	  //get dir listing from mc
	  sprintf (buff, "%s/*", currentDir);
	  numEntries = mc_getdir (0, 0, buff, 0, ARRAY_ENTRIES - 10, mcDir);
	  perror ("getdir returned %d entries\n", numEntries);
	  reply (com_sock, 150,
		 "Opening BINARY mode data connection for /bin/ls.");
	  //do listing
	  for (i = 0; i < numEntries; i++) {
	    if (mcDir[i].attrFile & MC_ATTR_SUBDIR)
	      sprintf (buf,
		       "drw-rw-rw-   1 ps2ftp   ps2ftp   %8d %s %d %02d:%02d %s\r\n",
		       mcDir[i].fileSizeByte,
		       months[mcDir[i]._modify.month - 1],
		       mcDir[i]._modify.day, mcDir[i]._modify.hour,
		       mcDir[i]._modify.min, mcDir[i].name);
	    else
	      sprintf (buf,
		       "-rw-rw-rw-   1 ps2ftp   ps2ftp   %8d %s %2d %02d:%02d %s\r\n",
		       mcDir[i].fileSizeByte,
		       months[mcDir[i]._modify.month - 1],
		       mcDir[i]._modify.day, mcDir[i]._modify.hour,
		       mcDir[i]._modify.min, mcDir[i].name);
	    ret = send (ls_sock, buf, strlen (buf), 0);
	    if (ret < 0) {
	      perror ("Error sending dir listing: %d\n", ret);
	      reply (com_sock, 500, "Error Sending directory listing");
	    }
	  }
	  reply (com_sock, 226, "Transfer complete.");
	}
	//close socket
	disconnect (ls_sock);
	break;
      case NOOP:		/* Acknowledge NOOP command */
	reply (com_sock, 200, "OK");
	break;
      case PORT:		/* Set the tcp/ip address based on the given argument */
	/* Grab data port address from command argument */
	buf2 = strtok (buf, ",");
	a1 = atoi (buf2);
	buf2 = strtok (NULL, ",");
	a2 = atoi (buf2);
	buf2 = strtok (NULL, ",");
	a3 = atoi (buf2);
	buf2 = strtok (NULL, ",");
	a4 = atoi (buf2);
	perror ("addr: %d.%d.%d.%d\n", a1, a2, a3, a4);
	IP4_ADDR (&(data_address.sin_addr), a1, a2, a3, a4);
	buf2 = strtok (NULL, ",");
	p1 = atoi (buf2);
	buf2 = strtok (NULL, ",");
	p2 = atoi (buf2);

	perror ("port: %d\n", p1 * 256 + p2);
	data_address.sin_port = htons ((p1 * 256) + p2);
	/* Reply, everything OK */
	reply (com_sock, 200, "PORT command successful");	//
	break;
      case RETR:		/* Retrieve File and transfer it to data address */
	sprintf (filename, "%s%s", currentDir, buf);

	if ((fd = mc_open (0, 0, filename, O_RDONLY)) < 0) {
	  //if ((fd = open (filename, O_RDONLY)) < 0) {
	  sprintf (buff, "File Does Not Exist: '%s'", filename);
	  reply (com_sock, 550, buff);
	  break;
	} else
	  perror ("Opened %s", filename);

	/* Everything OK, so make data connection */
	sprintf (buff, "Opening data connection for '%s'", filename);
	reply (com_sock, 150, buff);

	/* Create tcp/ip connection for data link */
	sock = connect_tcpip (&data_address, data_length, buff, com_sock);
	if (sock < 0) {
	  reply (com_sock, 550, "data connection failed");
	  return;
	} else {

	  /* Transfer file using BINARY mode translation */
	  memset (buffer, '\0', BUFFER_SIZE);
	  while ((bytes = mc_read (fd, buffer, BUFFER_SIZE)) != 0) {
	    if (send (sock, buffer, bytes, 0) < 0)
	      break;
	  }
	  if (bytes != 0)
	    reply (com_sock, 550, "error transfering file");
	  else
	    reply (com_sock, 226, "Transfer Complete");

	  /* Clean up */
	  disconnect (sock);
	  mc_close (fd);
	}
	break;
      case STOR:
	if (currentDir[strlen (currentDir) - 1] == '/')
	  currentDir[strlen (currentDir) - 1] = '\0';
	if (currentDir[0] == '/')
	  strcpy (buff, &currentDir[1]);
	else
	  strcpy (buff, currentDir);
	sprintf (filename, "mc0:%s/%s", buff, buf);
	perror ("filename: %s\n", filename);
	perror ("Before open\n");
	if ((fd = open (filename, O_WRONLY | O_CREAT)) < 0) {
	  perror ("permission denied\n");
	  sprintf (buff, "Permission denied. Could not create file: '%s'",
		   filename);
	  reply (com_sock, 553, buff);
	  break;
	} else {
	  perror ("opened\n");
	}
	/* Everything OK, so make data connection */
	sprintf (buff, "Opening BINARY mode data connection for '%s'",
		 filename);
	reply (com_sock, 150, buff);

	/* Create tcp/ip connection for data link */
	sock = connect_tcpip (&data_address, data_length, buff, com_sock);
	if (sock < 0) {
	  reply (com_sock, 550, "data connection failed");
	  return;
	} else {

	  /* Transfer file using BINARY mode translation */
	  memset (buffer, '\0', BUFFER_SIZE);
	  while ((bytes = recv (sock, buffer, BUFFER_SIZE, 0)) > 0) {
	    perror ("read: %d bytes\n");
	    if (write (fd, buffer, bytes) < 1)
	      break;
	  }

	  if (bytes != 0)
	    reply (com_sock, 550, "error transfering file");
	  else
	    reply (com_sock, 226, "Transfer Complete");

	  /* Clean up */
	  disconnect (sock);
	  close (fd);
	}
	break;
      case DELE:
	sprintf (buff, "mc0:%s%s", currentDir, buf);
	perror ("deleting: %s\n", buff);
	ret = remove (buff);
	if (ret == 0)
	  reply (com_sock, 250, "DELE command successful.");
	else {
	  sprintf (buf, "Error %d returned from remove(%s).", ret, buff);
	  reply (com_sock, 550, buf);
	}
	break;
      case RMD:
	sprintf (buff, "mc0:%s", buf);
	ret = rmdir (buff);
	if (ret == 0)
	  reply (com_sock, 250, "RMD command successful.");
	else {
	  sprintf (buf, "Error %d returned from rmdir(%s).", ret, buff);
	  reply (com_sock, 550, buf);
	}
	break;
      case MKD:

	if (buf[0] == '/')
	  sprintf (buff, "mc0:%s", buf);
	else
	  sprintf (buff, "mc0:%s%s", currentDir, buf);

	perror ("MKD: %s\n", buff);
	ret = mkdir (buff);
	if (ret < 0) {
	  sprintf (buf, "Could not create directory \"%s\" (%d)\n", buff,
		   ret);
	  reply (com_sock, 550, buf);
	} else {
	  sprintf (buf, "\"%s\" created.\n", buff);
	  reply (com_sock, 257, buf);
	}
	/*case SIZE:

	   sprintf (buff, "%s%s", currentDir, buf);
	   bytes = GetFileSize (buff);
	   if (bytes < 0) {
	   perror ("SIZE: %s: No such file or directory\n", buff);
	   sprintf (buff, "%s: No such file or is a directory", buff);
	   reply (com_sock, 550, buff);
	   } else {
	   perror ("SIZE: %s is %d\n", buff, bytes);
	   sprintf (buff, "%d", bytes);
	   reply (com_sock, 250, buff);
	   }

	   break; */
      default:			/* Any command not implemented, return not recognized response. */
	reply (com_sock, 500, "command not recognized");
	break;
      }
      break;
    }
  };
}


/* Get a ftp command from the command stream, convert to command code and
   parse command argument */
enum command_types
get_command (int com_sock, char *buf, int bufsize)
{
  char cmd[5];
  int i, j, rcvSize;

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
