
/*! \brief Handle dopen request.
 *  \ingroup tenftp 
 */
int fd_dopen( iop_io_file_t *f, const char *name )
{
	int ret;

	char NameBuf[512];
	strcpy( NameBuf, name );

#if DEBUG_LEVEL>=1
	printf( "tenftp: open %i %s\n", f->unit, name );
#endif

	ret = TenFtp_Open( NameBuf, 1, 0, 0 );
	if( ret < 0 )
	{
		FtpClose( LastFtp );
		LastFtp=-1;
		return ret;
	}

	lFtpState[ret].DirReadCacheData=(char *)malloc(512);
	lFtpState[ret].DirReadCacheData[0]=0;

	return fd_save( ret, 0, f );
}

/*! \brief Handle dclose request.
 *  \ingroup tenftp 
 *
 *  \param f     Pointer to io_device structure.
 *  \return Status (as for fileio close).
 *
 */
int fd_dclose( iop_io_file_t *f )
{
	int sConn = realfd(f);

	if( lFtpState[sConn].DirReadCacheData )
		free( lFtpState[sConn].DirReadCacheData );
	lFtpState[sConn].DirReadCacheData=0;

	return TenFtp_Close( sConn );
}


/*! \brief Handle dread request.
 *  \ingroup tenftp 
 *
 *  \param f       Pointer to io_device structure.
 *  \param buffer  Pointer to read buffer.
 *  \param size    Size of buffer.
 *  \return Status (as for fileio read).
 *
 */
int fd_dread( iop_io_file_t *f, fio_dirent_t *buf )
{
	int ret,sConn = realfd(f);
	char tmp[520], *tmpstor, *c, *c2, *c3;

#if DEBUG_LEVEL>=1
	printf( "dread: %p\n", buf );
#endif

	tmpstor = lFtpState[sConn].DirReadCacheData;
	if( !tmpstor )
		return -1;

fd_dread_try_again:
	c = ten_strchr(tmpstor,'\r');
	if( !c )
		c = ten_strchr(tmpstor,'\n');

	if( !c )
	{//we need data
		#if DEBUG_LEVEL>=1
			printf( "need data\n" );
		#endif

		if( strlen( tmpstor ) >200 )
			return -2; //too much, so we arent reading a listing file

		ret=fd_read( f, tmp, 256 );
		if( ret <= 0 )
			return ret;
		tmp[ret]=0;
		strcat( tmpstor, tmp );

		printf( "got data\n" );
		goto fd_dread_try_again;
	}

	printf( "found line end\n" );

	c[0]=0;
	c++;
	if( c[0]=='\n' )
	{
		c[0]=0;
		c++;
	}
	//now exactly one complete line is in tmpstor
	memset( buf, 0, sizeof(fio_dirent_t) );

	//drwxr-xr-x   2 tentacle tentacle     4096 Oct 26 13:33 [AonE]_Bakuretsu_Tenshi_01_24.torrent.

//drwxr-xr-x
	if( tmpstor[0]=='d' )
		buf->stat.mode |= FIO_SO_IFDIR;
	else
		buf->stat.mode |= FIO_SO_IFREG;

	if( tmpstor[1]=='r' )
		buf->stat.mode |= FIO_SO_IROTH;
	if( tmpstor[2]=='w' )
		buf->stat.mode |= FIO_SO_IWOTH;
	if( tmpstor[3]=='x' )
		buf->stat.mode |= FIO_SO_IXOTH;

	c2=tmpstor;

printf( "2\n" );
// 2 
	c2=ten_strchr( c2, ' ' );
	if( !c2 )
		return -3;
	while( c2[0]==' ' )
		c2++;

printf( "tentacle\n" );
// tentacle 
	c2=ten_strchr( c2, ' ' );
	if( !c2 )
		return -4;
	while( c2[0]==' ' )
		c2++;

printf( "tentacle\n" );
// tentacle 
	c2=ten_strchr( c2, ' ' );
	if( !c2 )
		return -5;
	while( c2[0]==' ' )
		c2++;

printf( "size\n" );
// size 
	c2=ten_strchr( c2, ' ' );
	if( !c2 )
		return -6;
	while( c2[0]==' ' )
		c2++;

printf( "c3\n" );
	c3=ten_strchr( c2, ' ' );
	if( !c3 )
		return -7;

	c3[0]=0;

	buf->stat.size = atoi( c2 );
	c2=c3+1;

printf( "Oct 26 13:33\n" );
//Oct 26 13:33

	//skip date + space after
	c2+=12 + 1;

printf( "name\n" );
	strcpy( buf->name, c2 );

	strcpy( tmp, c );
	strcpy( tmpstor, tmp );


#if DEBUG_LEVEL>=1
	printf( "dread res: 1\n" );
#endif
	return 1; 
}

