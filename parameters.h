/* Snapshot information */
#define PARTTYPE 0/* Particle type required */
#define FILENUMBER 1 /* Number of sub-files */

/* Spectrum data set size     */ 
#define NBINS 1024 /* number of pixels */

/*Number of lines at a time to allocate to each thread*/
#define THREAD_ALLOC 10

/* Model parameters outwith header */
#define XH 0.76  /* hydrogen fraction by mass */
#define OMEGAB 0.0449 /* baryon fraction */
/*The value from 0711.1862 is (0.0023±0.0007) (1+z)^(3.65±0.21)*/
#define TAU_EFF 0.0023*pow(1.0+redshift,3.65)

/* Some useful numbers */
#define  GAMMA (5.0/3.0)

/* Physical constants, SI units */
#define  GRAVITY     6.67428e-11
#define  BOLTZMANN   1.3806504e-23
#define  C           2.99792458e8
#define  PROTONMASS  1.66053886e-27 /* 1 a.m.u */
#define  MPC 3.08568025e22
#define  KPC 3.08568025e19
#define  SIGMA_T 6.652458558e-29
#define  SOLAR_MASS 1.98892e30

/* Atomic data (from VPFIT) */
#define  LAMBDA_LYA_H1 1215.6701e-10
#define  LAMBDA_LYA_HE2 303.7822e-10
#define  FOSC_LYA 0.416400
#define  HMASS 1.00794   /* Hydrogen mass in a.m.u. */
#define  HEMASS 4.002602 /* Helium-4 mass in a.m.u. */
#define  GAMMA_LYA_H1 6.265e8
#define  GAMMA_LYA_HE2 6.27e8
