/*
# $Id$
# EE TSR to init debug printf in ffx v1.0
#
*/

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


#define TEN_STACK_A 0x00ffff0
#define TEN_STACK_S 0x00ffffc
#define TEN_STACK_R 0x00ffff8

#define STR_TEN_STACK_A "0x00ffff0"
#define STR_TEN_STACK_S "0x00ffffc"
#define STR_TEN_STACK_R "0x00ffff8"

#define ten_switch_on	{	asm volatile (	".set noat;" \
	"li $1, "	STR_TEN_STACK_S "\n" \
	"sw $29, 0 ( $1 )\n" \
	"li $29, "	STR_TEN_STACK_A "\n"  \
); }

#define ten_jump( target )	{	asm volatile (	".set noat;"  \
	"li $1, "	STR_TEN_STACK_R "\n" \
	"sw $31, 0 ( $1 )\n" \
	"jal %0\n"  \
	"li $1, "	STR_TEN_STACK_R "\n" \
	"lw $31, 0 ( $1 )\n" \
:: "i" ( ((u32)target) )  ); }

#define ten_gate( target )	{	asm volatile (	  \
	"nop\n" \
	"nop\n" \
	"j %0\n"  \
:: "i" ( ((u32)target) )  ); }


#define ten_switch_off	{	asm volatile (	".set noat;" \
	"li $1, "	STR_TEN_STACK_S "\n" \
	"lw $29, 0 ( $1 )\n" \
); }



//00119250
#define TEN_START 0x001e0000
extern void *_ten_end;

#define inject( c, func )  ((( (0x08000000+c*0x04000000) + (		(	( ((u32)func)-TEN_START) + (TEN_START) ) >>2) )))

#define tnop asm volatile (	"nop\n" );


typedef void (*funcptr_ten)();

void TenWait( int anz );
void TenLoadNet(u8 bReset);
void TenShowModlist();
void TenModlistDiffInit();
void TenModlistDiffShow();


void nprintf_tests()
{
	nprintf( "t1\n" );
	tnop
		tnop

	nprintf( "t2 %s\n", "str t2" );
	tnop
		tnop

	nprintf( "t3 %d\n", 56 );
	tnop
		tnop

}

char *lCount[26];
int nCount=0;
void InjDbgOut( const char * str )
{	
	if( nCount < 25 )
	{
		lCount[nCount]=malloc( strlen(str)+3 );
		strcpy(lCount[nCount],str);
		nCount++;
		return;
	}

	TenLoadNet(0);
	TenWait(1);

	if( nCount==25 )
	{
		int i;
		for( i=0;i<nCount;i++ )
		{
			nprintf( ">> %s\n",  lCount[i] );
			free( lCount[i] );
		}
		nCount++;
	}

	nprintf( ">> %s\n",  str );
}

void _InjDbgOut( const char * str )
{

	ten_switch_on

	ten_jump( &InjDbgOut );

	ten_switch_off
}

void _InjDbgPrintf2( const char * str )
{

	ten_switch_on

	ten_jump( &nprintf );

	ten_switch_off
}

void _InjDbgPrintf3( const char * str )
{

	ten_switch_on

	ten_jump( &nprintf );

	ten_switch_off

	asm volatile ( "addiu $29, $29, 0xff70\n" );

	ten_gate( 0x002ec2ec );
}


void _tenfunc() 
{//this is called when browser/game gets loaded, after reboot is completed

	//DbgOut j
	*((u32*)0xa0125550)=inject( 0, _InjDbgOut );

	//DbgPrintf2 j
	*((u32*)0xa0120108)=inject( 0, _InjDbgPrintf2 );

	//DbgPrintf3 j
// wont work so far, causes black screen hang before ten net init
	// *((u32*)0xa02ec2e8)=inject_inside( 0, _InjDbgPrintf3 );

}

void _ten_tsr_load_addr( u32 *_tenfunc_store ) 
{
	*_tenfunc_store = (u32)_tenfunc;
}

