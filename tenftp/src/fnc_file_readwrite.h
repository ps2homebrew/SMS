
/*! \brief Handle read request.
 *  \ingroup tenftp 
 *
 *  \param f       Pointer to io_device structure.
 *  \param buffer  Pointer to read buffer.
 *  \param size    Size of buffer.
 *  \return Status (as for fileio read).
 *
 */
int fd_read( iop_io_file_t *f, char * buffer, int size )
{
	int ret,sConn = realfd(f);

#if DEBUG_LEVEL>=1
	printf( "read: %p %d\n", buffer, size );
#endif

	ret = TenFtp_CheckDataConn( sConn );
	if( ret < 0 )
		return ret;

	if( !IsDataFromFtpDataConnAvail( sConn ) )
	{
		DelayThread( 500 );
		if( !IsDataFromFtpDataConnAvail( sConn ) )
			return 0;
	}

	ret = GetDataFromFtpDataConn( sConn, buffer, size );
	if( ret < 0 )
	{
		printf( "! GetDataFromFtpDataConn\n" );
		return -4;
	}

#if DEBUG_LEVEL>=1
	printf( "read res: %d\n", ret );
#endif
	return ret; 
}


/*! \brief Handle write request.
 *  \ingroup tenftp 
 *
 *  \param f       Pointer to io_device structure.
 *  \param buffer  Pointer to read buffer.
 *  \param size    Size of buffer.
 *  \return Status (as for fileio write).
 *
 */
int fd_write( iop_io_file_t *f, void *buffer, int size )
{
	int ret,sConn = realfd(f);

#if DEBUG_LEVEL>=1
	printf( "write: %p %d %d\n", buffer, size, realbWr(f) );
#endif

	if( realbWr(f)==0 )
		return -99;

	ret = TenFtp_CheckDataConn( sConn );
	if( ret < 0 )
		return ret;

	if( !IsDataSendToFtpDataConnAvail( sConn ) )
	{
		DelayThread( 500 );
		if( !IsDataSendToFtpDataConnAvail( sConn ) )
			return 0;
	}

	ret = SendDataToFtpDataConn( sConn, buffer, size );
	if( ret < 0 )
	{
		printf( "! SendDataToFtpDataConn\n" );
		return -4;
	}

#if DEBUG_LEVEL>=1
	printf( "write res: %d\n", ret );
#endif
	return ret; 
}
