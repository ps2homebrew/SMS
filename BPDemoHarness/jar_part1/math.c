/*
Cephes Math Library Release 2.2:  June, 1992
Copyright 1984, 1987, 1988, 1992 by Stephen L. Moshier
Direct inquiries to 30 Frost Street, Cambridge, MA 02140

Slight modifications by jar to make it compile

*/

#include "math.h"

#define SQRTHF		0.707106781186547524f
#define PIF			3.141592653589793238f
#define PIO2F		1.5707963267948966192f
#define PIO4F	 	0.7853981633974483096f
#define MACHEPF		5.9604644775390625E-8f

/* 2^(-i/16)
* The decimal values are rounded to 24-bit precision
*/
static float A[] = {
	1.00000000000000000000E0,
	9.57603275775909423828125E-1,
	9.17004048824310302734375E-1,
	8.78126084804534912109375E-1,
	8.40896427631378173828125E-1,
	8.05245161056518554687500E-1,
	7.71105408668518066406250E-1,
	7.38413095474243164062500E-1,
	7.07106769084930419921875E-1,
	6.77127778530120849609375E-1,
	6.48419797420501708984375E-1,
	6.20928883552551269531250E-1,
	5.94603538513183593750000E-1,
	5.69394290447235107421875E-1,
	5.45253872871398925781250E-1,
	5.22136867046356201171875E-1,
	5.00000000000000000000E-1
};
/* continuation, for even i only
* 2^(i/16)  =  A[i] + B[i/2]
*/
static float B[] = {
	0.00000000000000000000E0,
	-5.61963907099083340520586E-9,
	-1.23776636307969995237668E-8,
	4.03545234539989593104537E-9,
	1.21016171044789693621048E-8,
	-2.00949968760174979411038E-8,
	1.89881769396087499852802E-8,
	-6.53877009617774467211965E-9,
	0.00000000000000000000E0
};

/* 1 / A[i]
* The decimal values are full precision
*/
static float Ainv[] = {
	1.00000000000000000000000E0,
	1.04427378242741384032197E0,
	1.09050773266525765920701E0,
	1.13878863475669165370383E0,
	1.18920711500272106671750E0,
	1.24185781207348404859368E0,
	1.29683955465100966593375E0,
	1.35425554693689272829801E0,
	1.41421356237309504880169E0,
	1.47682614593949931138691E0,
	1.54221082540794082361229E0,
	1.61049033194925430817952E0,
	1.68179283050742908606225E0,
	1.75625216037329948311216E0,
	1.83400808640934246348708E0,
	1.91520656139714729387261E0,
	2.00000000000000000000000E0
};

#ifdef DEC
#define MEXP 2032.0
#define MNEXP -2032.0
#else
#define MEXP 2048.0
#define MNEXP -2400.0
#endif

/* log2(e) - 1 */
#define LOG2EA 0.44269504088896340736F
#define LOGE2F  0.693147180559945309F
#define MAXNUMF 3.4028234663852885981170418348451692544e38
#define MAXLOGF 88.72283905206835
#define MINLOGF -103.278929903431851103
#define LOG2EF 1.44269504088896341

#define F W
#define Fa Wa
#define Fb Wb
#define G W
#define Ga Wa
#define Gb u
#define H W
#define Ha Wb
#define Hb Wb

/* Find a multiple of 1/16 that is within 1/16 of x. */
#define reduc(x)  0.0625 * floorf( 16 * (x) )
#define floorf(x) ((float)(int)(x))

float ldexpf( float x, int pw2 )
{
	union
	{
		float y;
		unsigned short i[2];
	} u;
	short *q;
	int e;

#ifdef UNK
	printf( "%s\n", unkmsg );
	return(0.0);
#endif

	u.y = x;
	q = &u.i[1];
	while( (e = ( *q >> 7) & 0xff) == 0 )
	{
		if( u.y == (float )0.0 )
		{
			return( 0.0 );
		}
		/* Input is denormal. */
		if( pw2 > 0 )
		{
			u.y *= 2.0;
			pw2 -= 1;
		}
		if( pw2 < 0 )
		{
			if( pw2 < -24 )
				return( 0.0 );
			u.y *= 0.5;
			pw2 += 1;
		}
		if( pw2 == 0 )
			return(u.y);
	}

	e += pw2;

	/* Handle overflow */
	if( e > MEXP )
	{
		return( MAXNUMF );
	}

	*q &= 0x807f;

	/* Handle denormalized results */
	if( e < 1 )
	{
#if DENORMAL
		if( e < -24 )
			return( 0.0 );
		*q |= 0x80; /* Set LSB of exponent. */
		/* For denormals, significant bits may be lost even
		when dividing by 2.  Construct 2^-(1-e) so the result
		is obtained with only one multiplication.  */
		u.y *= ldexpf(1.0f, e - 1);
		return(u.y);
#else
		return( 0.0 );
#endif
	}
	*q |= (e & 0xff) << 7;
	return(u.y);
}

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

	q = &u.i[1];

	/* find the exponent (power of 2) */

	i  = ( *q >> 7) & 0xff;
	if( i == 0 )
	{
		if( u.y == 0.0 )
		{
			*pw2 = 0;
			return(0.0);
		}
		/* Number is denormal or zero */
#if DENORMAL
		/* Handle denormal number. */
		do
		{
			u.y *= 2.0;
			i -= 1;
			k  = ( *q >> 7) & 0xff;
		}
		while( k == 0 );
		i = i + k;
#else
		*pw2 = 0;
		return( 0.0 );
#endif /* DENORMAL */
	}
	i -= 0x7e;
	*pw2 = i;
	*q &= 0x807f;	/* strip all exponent bits */
	*q |= 0x3f00;	/* mantissa between 0.5 and 1 */
	return( u.y );
}

float powif( float x, int nn )
{
	int n, e, sign, asign, lx;
	float w, y, s;

	if( x == 0.0 )
	{
		if( nn == 0 )
			return( 1.0 );
		else if( nn < 0 )
			return( MAXNUMF );
		else
			return( 0.0 );
	}

	if( nn == 0 )
		return( 1.0 );


	if( x < 0.0 )
	{
		asign = -1;
		x = -x;
	}
	else
		asign = 0;


	if( nn < 0 )
	{
		sign = -1;
		n = -nn;
		/*
		x = 1.0/x;
		*/
	}
	else
	{
		sign = 0;
		n = nn;
	}

	/* Overflow detection */

	/* Calculate approximate logarithm of answer */
	s = frexpf( x, &lx );
	e = (lx - 1)*n;
	if( (e == 0) || (e > 64) || (e < -64) )
	{
		s = (s - 7.0710678118654752e-1) / (s +  7.0710678118654752e-1);
		s = (2.9142135623730950 * s - 0.5 + lx) * nn * LOGE2F;
	}
	else
	{
		s = LOGE2F * e;
	}

	if( s > MAXLOGF )
	{
		y = MAXNUMF;
		goto done;
	}

	if( s < MINLOGF )
		return(0.0);

	/* Handle tiny denormal answer, but with less accuracy
	* since roundoff error in 1.0/x will be amplified.
	* The precise demarcation should be the gradual underflow threshold.
	*/
	if( s < (-MAXLOGF+2.0) )
	{
		x = 1.0/x;
		sign = 0;
	}

	/* First bit of the power */
	if( n & 1 )
		y = x;

	else
	{
		y = 1.0;
		asign = 0;
	}

	w = x;
	n >>= 1;
	while( n )
	{
		w = w * w;	/* arg to the 2-to-the-kth power */
		if( n & 1 )	/* if that bit is set, then include in product */
			y *= w;
		n >>= 1;
	}


done:

	if( asign )
		y = -y; /* odd power of negative number */
	if( sign )
		y = 1.0/y;
	return(y);
}



float pow( float x, float y )
{
	float u, w, z, W, Wa, Wb, ya, yb;
	/* float F, Fa, Fb, G, Ga, Gb, H, Ha, Hb */
	int e, i, nflg;


	nflg = 0;	/* flag = 1 if x<0 raised to integer power */
	w = floorf(y);
	if( w < 0 )
		z = -w;
	else
		z = w;
	if( (w == y) && (z < 32768.0) )
	{
		i = w;
		w = powif( x, i );
		return( w );
	}


	if( x <= 0.0F )
	{
		if( x == 0.0 )
		{
			if( y == 0.0 )
				return( 1.0 );  /*   0**0   */
			else  
				return( 0.0 );  /*   0**y   */
		}
		else
		{
			if( w != y )
			{
				return(0.0);
			}
			nflg = 1;
			if( x < 0 )
				x = -x;
		}
	}

	/* separate significand from exponent */
	x = frexpf( x, &e );

	/* find significand in antilog table A[] */
	i = 1;
	if( x <= A[9] )
		i = 9;
	if( x <= A[i+4] )
		i += 4;
	if( x <= A[i+2] )
		i += 2;
	if( x >= A[1] )
		i = -1;
	i += 1;


	/* Find (x - A[i])/A[i]
	* in order to compute log(x/A[i]):
	*
	* log(x) = log( a x/a ) = log(a) + log(x/a)
	*
	* log(x/a) = log(1+v),  v = x/a - 1 = (x-a)/a
	*/
	x -= A[i];
	x -= B[ i >> 1 ];
	x *= Ainv[i];


	/* rational approximation for log(1+v):
	*
	* log(1+v)  =  v  -  0.5 v^2  +  v^3 P(v)
	* Theoretical relative error of the approximation is 3.5e-11
	* on the interval 2^(1/16) - 1  > v > 2^(-1/16) - 1
	*/
	z = x*x;
	w = (((-0.1663883081054895  * x
		+ 0.2003770364206271) * x
		- 0.2500006373383951) * x
		+ 0.3333331095506474) * x * z;
	w -= 0.5 * z;

	/* Convert to base 2 logarithm:
	* multiply by log2(e)
	*/
	w = w + LOG2EA * w;
	/* Note x was not yet added in
	* to above rational approximation,
	* so do it now, while multiplying
	* by log2(e).
	*/
	z = w + LOG2EA * x;
	z = z + x;

	/* Compute exponent term of the base 2 logarithm. */
	w = -i;
	w *= 0.0625;  /* divide by 16 */
	w += e;
	/* Now base 2 log of x is w + z. */

	/* Multiply base 2 log by y, in extended precision. */

	/* separate y into large part ya
	* and small part yb less than 1/16
	*/
	ya = reduc(y);
	yb = y - ya;


	F = z * y  +  w * yb;
	Fa = reduc(F);
	Fb = F - Fa;

	G = Fa + w * ya;
	Ga = reduc(G);
	Gb = G - Ga;

	H = Fb + Gb;
	Ha = reduc(H);
	w = 16 * (Ga + Ha);

	/* Test the power of 2 for overflow */
	if( w > MEXP )
	{
		return( MAXNUMF );
	}

	if( w < MNEXP )
	{
		return( 0.0 );
	}

	e = w;
	Hb = H - Ha;

	if( Hb > 0.0 )
	{
		e += 1;
		Hb -= 0.0625;
	}

	/* Now the product y * log2(x)  =  Hb + e/16.0.
	*
	* Compute base 2 exponential of Hb,
	* where -0.0625 <= Hb <= 0.
	* Theoretical relative error of the approximation is 2.8e-12.
	*/
	/*  z  =  2**Hb - 1    */
	z = ((( 9.416993633606397E-003 * Hb
		+ 5.549356188719141E-002) * Hb
		+ 2.402262883964191E-001) * Hb
		+ 6.931471791490764E-001) * Hb;

	/* Express e/16 as an integer plus a negative number of 16ths.
	* Find lookup table entry for the fractional power of 2.
	*/
	if( e < 0 )
		i = -( -e >> 4 );
	else
		i = (e >> 4) + 1;
	e = (i << 4) - e;
	w = A[e];
	z = w + w * z;      /*    2**-e * ( 1 + (2**Hb-1) )    */
	z = ldexpf( z, i );  /* multiply by integer power of 2 */

	if( nflg )
	{
		/* For negative x,
		* find out if the integer exponent
		* is odd or even.
		*/
		w = 2 * floorf( (float) 0.5 * w );
		if( w != y )
			z = -z; /* odd exponent */
	}

	return( z );
}

float sinh(float xx)
{
register float z;
float x;

x = xx;
if( xx < 0 )
	z = -x;
else
	z = x;

if( z > MAXLOGF )
	{
	if( x > 0 )
		return( MAXNUMF );
	else
		return( -MAXNUMF );
	}
if( z > 1.0 )
	{
		float exp(float);
	z = exp(z);
	z = 0.5*z - (0.5/z);
	if( x < 0 )
		z = -z;
	}
else
	{
	z = x * x;
	z =
	(( 2.03721912945E-4 * z
	  + 8.33028376239E-3) * z
	  + 1.66667160211E-1) * z * x
	  + x;
	}
return( z );
}


#define C1 0.693359375
#define C2 -2.12194440e-4

float exp(float xx)
{
float x, z;
int n;

x = xx;


if( x > MAXLOGF)
	{
	return( MAXNUMF );
	}

if( x < MINLOGF )
	{
	return(0.0);
	}

/* Express e**x = e**g 2**n
 *   = e**g e**( n loge(2) )
 *   = e**( g + n loge(2) )
 */
z = floorf( LOG2EF * x + 0.5 ); /* floor() truncates toward -infinity. */
x -= z * C1;
x -= z * C2;
n = z;

z = x * x;
/* Theoretical peak relative error in [-0.5, +0.5] is 4.2e-9. */
z =
((((( 1.9875691500E-4  * x
   + 1.3981999507E-3) * x
   + 8.3334519073E-3) * x
   + 4.1665795894E-2) * x
   + 1.6666665459E-1) * x
   + 5.0000001201E-1) * z
   + x
   + 1.0;

/* multiply by power of 2 */
x = ldexpf( z, n );

return( x );
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

