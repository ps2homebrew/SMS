/*
Cephes Math Library Release 2.2:  June, 1992
Copyright 1984, 1987, 1988, 1992 by Stephen L. Moshier
Direct inquiries to 30 Frost Street, Cambridge, MA 02140

Slight modifications by jar to make it compile

*/


#define MAXNUMF 	1.7014117331926442990585209174225846272e38f
#define MAXLOGF 	88.02969187150841f
#define MINLOGF 	-88.7228391116729996f

#define LOG2EF		1.44269504088896341f
#define LOGE2F		0.693147180559945309f
#define SQRTHF		0.707106781186547524f
#define PIF			3.141592653589793238f
#define PIO2F		1.5707963267948966192f
#define PIO4F	 	0.7853981633974483096f
#define MACHEPF		5.9604644775390625E-8f

float frexpf( float x, int *pw2 )
{
	union
	{
		float y;
		unsigned short i[2];
	} u;
	int i;
	short *q;

	u.y = x;

	q = &u.i[1];	// &u.i[0]; on big-endian machines

	/* find the exponent (power of 2) */

	i  = ( *q >> 7) & 0xff;
	if( i == 0 )
	{
		if( u.y == 0.0 )
		{
			*pw2 = 0;
			return(0.0);
		}
		*pw2 = 0;
		return( 0.0 );
	}
	i -= 0x7e;
	*pw2 = i;
	*q &= 0x807f;	/* strip all exponent bits */
	*q |= 0x3f00;	/* mantissa between 0.5 and 1 */
	return( u.y );
}


float log( float xx )
{
	register float y;
	float x, z, fe;
	int e;
	
	x = xx;
	fe = 0.0;
	if( x <= 0.0 )
	{
		return( MINLOGF );
	}
	
	x = frexpf( x, &e );
	if( x < SQRTHF )
	{
		e -= 1;
		x = x + x - 1.0; /*  2x - 1  */
	}	
	else
	{
		x = x - 1.0;
	}
	z = x * x;

	y =
	(((((((( 7.0376836292E-2f * x
	- 1.1514610310E-1f) * x
	+ 1.1676998740E-1f) * x
	- 1.2420140846E-1f) * x
	+ 1.4249322787E-1f) * x
	- 1.6668057665E-1f) * x
	+ 2.0000714765E-1f) * x
	- 2.4999993993E-1f) * x
	+ 3.3333331174E-1f) * x * z;

	if( e )
	{
		fe = e;
		y += -2.12194440e-4f * fe;
	}

	y +=  -0.5 * z;  /* y - 0.5 x^2 */
	z = x + y;   /* ... + x  */

	if( e )
		z += 0.693359375f * fe;

	return( z );
}


float atan( float xx )
{
	float x, y, z;
	int sign;
	
	x = xx;
	
	/* make argument positive and save the sign */
	if( xx < 0.0f )
		{
		sign = -1;
		x = -xx;
		}
	else
		{
		sign = 1;
		x = xx;
		}
	/* range reduction */
	if( x > 2.414213562373095f )  /* tan 3pi/8 */
		{
		y = PIO2F;
		x = -( 1.0f/x );
		}
	
	else if( x > 0.4142135623730950f ) /* tan pi/8 */
		{
		y = PIO4F;
		x = (x-1.0)/(x+1.0);
		}
	else
		y = 0.0;
	
	z = x * x;
	y +=
	((( 8.05374449538e-2f * z
	  - 1.38776856032E-1f) * z
	  + 1.99777106478E-1f) * z
	  - 3.33329491539E-1f) * z * x
	  + x;
	
	if( sign < 0 )
		y = -y;
	
	return( y );
}




float atan2( float y, float x )
{
	float z, w;
	int code;
	
	
	code = 0;
	
	if( x < 0.0 )
		code = 2;
	if( y < 0.0 )
		code |= 1;
	
	if( x == 0.0 )
		{
		if( code & 1 )
			{
			return( -PIO2F );
			}
		if( y == 0.0 )
			return( 0.0 );
		return( PIO2F );
		}
	
	if( y == 0.0 )
		{
		if( code & 2 )
			return( PIF );
		return( 0.0 );
		}
	
	
	switch( code )
		{
		default:
		case 0:
		case 1: w = 0.0; break;
		case 2: w = PIF; break;
		case 3: w = -PIF; break;
		}
	
	z = atan( y/x );
	
	return( w + z );
}

