/*
 *  betadev.cpp
 *  double betadev(double alfa, double beta, long *idum)
 *  
 *  Returns a Beta(alfa,beta) distributed deviate, using ran1(idum)
 *  as the source of uniform deviates.
 *
 */

#include <math.h>

double betadev(double alfa, double beta, long *idum)

{
	double ran1(long *idum);
	double U,V,den;
	
	den=0;
	U=ran1(idum);
	V=ran1(idum);
	den=pow(U,(1/alfa))+pow(V,(1/beta));
	
	while (den<=0 || den >1)
	{
		den=0;
		U=ran1(idum);
		V=ran1(idum);
		den=pow(U,(1/alfa))+pow(V,(1/beta));
	}
	return pow(U,(1/alfa))/den;
}


