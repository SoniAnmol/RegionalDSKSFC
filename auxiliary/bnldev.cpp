#include <math.h>
#define PI 3.141592654

double bnldev(double pp, int n, long *idum)

/*Returns as a floating-point number an integer value that is a random deviate drawn from
a binomial distribution of n trials each of probability pp, using ran1(idum) as a source of
uniform random deviates.*/

{
	double gammln(double xx);
	double ran1(long *idum);
	int j;
	static int nold=(-1);
	double am,em,g,angle,p,bnl,sq,t,y;
	static double pold=(-1.0),pc,plog,pclog,en,oldg;
	
	p=(pp <= 0.5 ? pp : 1.0-pp);	
										//The binomial distribution is invariant under changing pp to 1-pp, if we also change the
										//answer to n minus itself; we�all remember to do this below.
	am=n*p;										//This is the mean of the deviate to be produced.
	if (n < 25) 
	{											//Use the direct method while n is not too large.
			bnl=0.0;							//This can require up to 25 calls to ran1.
			for (j=1;j<=n;j++)
				if (ran1(idum) < p) 
					++bnl;
	} 
	else if (am < 1.0) 
	{											// If fewer than one event is expected out of 25
												// or more trials, then the distribution is quite
												// accurately Poisson. Use direct Poisson method.
		g=exp(-am);
		t=1.0;
		for (j=0;j<=n;j++) 
		{
			t *= ran1(idum);
			if (t < g) 
				break;
		}
			bnl=(j <= n ? j : n);
	} 
	else 
	{											// Use the rejection method.
		if (n != nold) 
		{										// If n has changed, then compute useful quantities
			en=n;
			oldg=gammln(en+1.0);
			nold=n;
		} 
		if (p != pold) 
		{										// If p has changed, then compute useful quantities
			pc=1.0-p;
			plog=log(p);
			pclog=log(pc);
			pold=p;
		}
		sq=sqrt(2.0*am*pc);						// The following code should by now seem familiar:
												// rejection method with a Lorentzian comparison
												// function.
		do {
			do {
				angle=PI*ran1(idum);
				y=tan(angle);
				em=sq*y+am;
				} 
			while (em < 0.0 || em >= (en+1.0)); // Reject.
			em=floor(em);						// Trick for integer-valued distribution.
			t=1.2*sq*(1.0+y*y)*exp(oldg-gammln(em+1.0)-gammln(en-em+1.0)+em*plog+(en-em)*pclog);
		} 
		while (ran1(idum) > t);					// Reject. This happens about 1.5 times per deviate, on average.
		bnl=em; 
	}
	if (p != pp) 
		bnl=n-bnl;								// Remember to undo the symmetry transformation
	return bnl;
}



