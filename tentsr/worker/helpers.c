#define NO_DISP
//#define MODLIST_SUCK_MEM
#define NAP_PRINTF


#include <tamtypes.h>
#include <kernel.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <fileio.h>
#include <malloc.h>
#include <libmc.h>
#include <debug.h>
#include <string.h>
#include <debug.h>
#include <stdio.h>


#include <sbv_patches.h>
#include <smod.h>
#include <smem.h>


void TenWait( int anz )
{
	int j,i;
	for( j=0;j<anz;j++ )
	{
		for( i=0;i<1000*1000;i++ )
		{ 
			asm volatile (	"nop\n" );
			asm volatile (	"nop\n" );
			asm volatile (	"nop\n" );
			asm volatile (	"nop\n" );
			asm volatile (	"nop\n" );
			asm volatile (	"nop\n" );
			asm volatile (	"nop\n" );
			asm volatile (	"nop\n" );
			asm volatile (	"nop\n" );
			asm volatile (	"nop\n" );
		}
	}
}





#ifdef NO_DISP

#define init_scr() {asm volatile (	"nop\n" );};
#define scr_printf(arg...) {asm volatile (	"nop\n" );};

#endif

int bRun=0;
int bRun2=0;

void TenLoadNet(u8 bReset)
{
	int ret;
	if( !bRun2 || bReset )
	{
		smod_mod_info_t cur;
		memset(&cur, 0, sizeof(smod_mod_info_t));

#ifndef NO_DISP
		SifInitRpc(0);
		init_scr();
#endif

		if( !bRun )
		{
			scr_printf("patching...\n");
			SifInitRpc(0);
			sbv_patch_enable_lmb();		
			sbv_patch_disable_prefix_check();

			bRun=1;
		}

		TenWait(30);
		scr_printf("loading...\n");

		TenWait(30);

		scr_printf("SIO2MAN\n");
		if( !smod_get_mod_by_name( "sio2man", &cur ) )
		{
			scr_printf("not loaded\n");
			if( (ret=SifLoadModule( "rom0:SIO2MAN", 0, 0 ) ) < 0 )
				scr_printf("error %d.\n",ret);
		}
		else scr_printf("skipped.\n");

		scr_printf("MCMAN\n");
		if( !smod_get_mod_by_name( "mcman", &cur ) )
		{
			if( (ret=SifLoadModule( "rom0:MCMAN", 0, 0 ) ) < 0 )
				scr_printf("error %d.\n",ret);
		}
		else scr_printf("skipped.\n");

		scr_printf("MCSERV\n");
		if( !smod_get_mod_by_name( "mcserv", &cur ) )
		{
			if( (ret=SifLoadModule( "rom0:MCSERV", 0, 0 ) ) < 0 )
				scr_printf("error %d.\n",ret);
		}
		else scr_printf("skipped.\n");

		scr_printf("IO/File_Manager\n");
		if( !smod_get_mod_by_name( "IO/File_Manager", &cur ) )
		{
			if( (ret=SifLoadModule( "rom0:IOMAN", 0, 0 ) ) < 0 )
				scr_printf("error %d.\n",ret);
		}
		else scr_printf("skipped.\n");

//		if( bReset )
		{
			scr_printf("dev9_driver\n");
			if( !smod_get_mod_by_name( "dev9_driver", &cur ) )
			{
				if( (ret=SifLoadModule( "mc0:/ten/ps2dev9_noX.irx", 0, 0 ) ) < 0 )
					scr_printf("error %d.\n",ret);
			}
			else scr_printf("skipped.\n");
		}

		scr_printf("udptty\n");
		if( !smod_get_mod_by_name( "udptty", &cur ) )
		{
			if( (ret=SifLoadModule( "mc0:/ten/udptty_ten.irx", 0, 0 ) ) < 0 )
				scr_printf("error %d.\n",ret);
		}
		else scr_printf("skipped.\n");



#ifdef NAP_PRINTF
#ifndef NO_DISP
		scr_printf("npm_puts_init...\n");
		TenWait(30);
#endif
		npm_puts_init();
#endif

#ifndef NO_DISP
		scr_printf("Testing...\n");
		nprintf( "Test OK.\n");
#endif

		nprintf( "done.\n");
		scr_printf("done.\n");


		bRun2=1-bReset;
	}
}



#ifdef MODLIST_SUCK_MEM


void TenShowModlist()
{
	SifInitRpc(0);
	init_scr();

	smod_mod_info_t cur;

	memset(&cur, 0, sizeof(smod_mod_info_t));
	if( !smod_get_next_mod( 0, &cur ) )
		scr_printf( "!smod_get_next_mod\n" );

	do{
		char curname [60];
		if( !smem_read(cur.name, curname, sizeof( curname) ) )
			scr_printf( "!smem_read\n" );	
		scr_printf( curname );
		scr_printf( "\n" );
		TenWait(10);
	}while( smod_get_next_mod( &cur, &cur ) );
}


char lMods[256][256];
int nMods=0;

void TenModlistDiffInit()
{
	smod_mod_info_t cur;

	nMods=0;

	memset(&cur, 0, sizeof(smod_mod_info_t));
	if( !smod_get_next_mod( 0, &cur ) )
		scr_printf( "!smod_get_next_mod\n" );

	do{
		char curname [60];
		if( !smem_read(cur.name, curname, sizeof( curname) ) )
			scr_printf( "!smem_read\n" );
		strcpy( lMods[nMods], curname );
		nMods++;
	}while( smod_get_next_mod( &cur, &cur ) );
}


void TenModlistDiffShow()
{
	smod_mod_info_t cur;
		int i;

	memset(&cur, 0, sizeof(smod_mod_info_t));
	if( !smod_get_next_mod( 0, &cur ) )
		scr_printf( "!smod_get_next_mod\n" );

	do{
		char curname [60];
		if( !smem_read(cur.name, curname, sizeof( curname) ) )
			scr_printf( "!smem_read\n" );
		for( i=0;i<nMods;i++ )	
		{
			if( !strcmp( lMods[i], curname ) )
			{
				lMods[i][0]=0;
				break;
			}
		}
		if( i==nMods )
		{
			scr_printf( "+ " );
			scr_printf( curname );
			scr_printf( " \n" );
		}
	}while( smod_get_next_mod( &cur, &cur ) );

	for( i=0;i<nMods;i++ )	
	{
		if( lMods[i][0] )
		{
			scr_printf( "- " );
			scr_printf( lMods[i] );
			scr_printf( " \n" );
		}
	}

	nMods=0;
}



#endif










#ifdef NAP_PRINTF

#define NPM_RPC_SERVER	0x14d704e
#define NPM_RPC_PUTS	1

static int npm_puts_sema = -1;
static int init = 0;
static SifRpcClientData_t npm_cd;

static int npm_puts_init()
{
	int res;
	ee_sema_t sema;

	sema.init_count = 0;
	sema.max_count  = 1;
	sema.option     = 0;
	if ((npm_puts_sema = CreateSema(&sema)) < 0)
		return -1;

	SifInitRpc(0);

	while (((res = SifBindRpc(&npm_cd, NPM_RPC_SERVER, 0)) >= 0) &&
			(npm_cd.server == NULL))
		nopdelay();

	if (res < 0)
		return res;

	SignalSema(npm_puts_sema);

	init = 1;
	return 0;
}

int npmPuts(const char *buf)
{
	u8 puts_buf[512]; /* Implicitly aligned. */
	void *p = puts_buf;

	if (!init && npm_puts_init() < 0)
		return -E_LIB_API_INIT;

	WaitSema(npm_puts_sema);

	/* If the buffer is already 16-byte aligned, no need to copy it.  */
	if (((u32)buf & 15) == 0)
		p = (void *)buf;
	else
		strncpy(p, buf, 512);

	if (SifCallRpc(&npm_cd, NPM_RPC_PUTS, 0, p, 512, NULL, 0, NULL, NULL) < 0)
		return -E_SIF_RPC_CALL;

	SignalSema(npm_puts_sema);

	return 1;
}

int nprintf(const char *format, ...)
{
	char buf[PS2LIB_STR_MAX];
	va_list args;
	int ret;

	va_start(args, format);
	ret = vsnprintf(buf, PS2LIB_STR_MAX, format, args);
	va_end(args);

	npmPuts(buf);
	return ret;
}

int vnprintf(const char *format, va_list args)
{
	char buf[PS2LIB_STR_MAX];
	int ret;

	ret = vsnprintf(buf, PS2LIB_STR_MAX, format, args);
	npmPuts(buf);

	return ret;
}

#endif
