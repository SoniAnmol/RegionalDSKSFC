 # include <math.h>
extern double ran1(long *idum); /* defined elsewhere */

 double gasdev(long *idum)

/* Returns a normally distributed deviate with zero mean and unit variance, using ran1(idum)
   as the source of uniform deviates. From Numerical Recipes */

 {
	 
	 //double ran1(long *idum);
	 static int iset=0;
	 static double gset;
	 double fac,rsq,v1,v2;

	 if (*idum < 0) iset=0;						// Reinitialize
	 if (iset==0)								// We don't have an extra deviate, so
	 {
		 do
		 {
			 v1=2.0*ran1(idum)-1.0;				// pick two uniform numbers in the square
			 v2=2.0*ran1(idum)-1.0;				// extending from -1.0 to +1.0 in each
			 rsq=v1*v1+v2*v2;					// direction, 
		 }										// see if they are in the unit circle
		 while (rsq >= 1.0 || rsq == 0.0);		// and if they are not, try again.
		 fac=sqrt(-2.0*log(rsq)/rsq);
		 /* Now makes the Box-muller transformation to get two normal deviates. 
		    Return one and save the other for next time */
		 gset=v1*fac;
		 iset=1;								// set flag
		 return v2*fac;
	 }
	 else										// We have an extra deviate handy, so
	 {											
		 iset=0;								// so unset the flag,
		 return gset;							// and return it.
	 }

 }
