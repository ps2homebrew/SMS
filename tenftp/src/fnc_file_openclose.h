

/*! \brief Handle open request.
 *  \ingroup tenftp 
 */
int fd_open( iop_io_file_t *f, const char *name, int mode)
{
	int ret;
	int bWr=0;

	char NameBuf[512];
	strcpy( NameBuf, name );

#if DEBUG_LEVEL>=1
	printf( "tenftp: open %i %s\n", f->unit, name );
#endif

	if( mode&O_WRONLY )
		bWr=1;

	ret = TenFtp_Open( NameBuf, 0, bWr, 0 );
	if( ret < 0 )
	{
		FtpClose( LastFtp );
		LastFtp=-1;
		return ret;
	}

	return fd_save( ret, bWr, f );
}

/*! \brief Handle close request.
 *  \ingroup tenftp 
 *
 *  \param f     Pointer to io_device structure.
 *  \return Status (as for fileio close).
 *
 */
int fd_close( iop_io_file_t *f )
{
	int sConn = realfd(f);

	return TenFtp_Close( sConn );
}

