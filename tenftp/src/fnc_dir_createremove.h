

/*! \brief Handle MkDir request.
 *  \ingroup tenftp 
 */
int fd_mkdir( iop_io_file_t *f, const char *iname )
{
	int ret;
	char NameBuf[512];
	strcpy( NameBuf, iname );
	ret = TenFtp_MkRmDir( NameBuf, 0 );
	if( ret < 0 )
	{
		FtpClose( LastFtp );
		LastFtp=-1;
		return ret;
	}
	return 1;
}



/*! \brief Handle RmDir request.
 *  \ingroup tenftp 
 */
int fd_rmdir( iop_io_file_t *f, const char *iname )
{
	int ret;
	char NameBuf[512];
	strcpy( NameBuf, iname );
	ret = TenFtp_MkRmDir( NameBuf, 1 );
	if( ret < 0 )
	{
		FtpClose( LastFtp );
		LastFtp=-1;
		return ret;
	}
	return 1;
}



