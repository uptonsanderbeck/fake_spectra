/* Copyright (c) 2009, Simeon Bird <spb41@cam.ac.uk>
 *               Based on code (c) 2005 by J. Bolton
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
//For getopt
#include <unistd.h>
//For strerror
#include <errno.h>
#include <string.h>
#include "global_vars.h"
#include "parameters.h"
#include "index_table.h"
#include <string>
#include <sstream>
#include <iostream>
#ifdef HDF5
#include <hdf5.h>

std::string find_first_hdf_file(const std::string& infname)
{
  /*Switch off error handling so that we can check whether a
   * file is HDF5 */
  /* Save old error handler */
  hid_t error_stack=0;
  herr_t (*old_func)(hid_t, void*);
  void *old_client_data;
  H5Eget_auto(error_stack, &old_func, &old_client_data);
  /* Turn off error handling */
  H5Eset_auto(error_stack, NULL, NULL);
  std::string fname = infname;

  /*Were we handed an HDF5 file?*/
  if(H5Fis_hdf5(fname.c_str()) <= 0){
     /*If we weren't, were we handed an HDF5 file without the suffix?*/
     fname = infname+std::string(".0.hdf5");
     if (H5Fis_hdf5(fname.c_str()) <= 0)
        fname = std::string();
  }

  /* Restore previous error handler */
  H5Eset_auto(error_stack, old_func, old_client_data);
  return fname;
}
#endif
#ifdef HELIUM
  #error "Helium no longer supported like this: Use the python module"
#endif

/*Open a file for reading to check it exists*/
int file_readable(const char * filename)
{
     FILE * file;
     if ((file = fopen(filename, "r"))){
          fclose(file);
          return 1;
     }
     return 0;
}


int main(int argc, char **argv)
{
  int64_t Npart;
  int NumLos=0;
  int64_t MaxRead=256*256*256,StartPart=0;

  FILE *output;
  los *los_table=NULL;
  char *ext_table=NULL;
  std::string outname;
  std::string indir;
  char c;
  int i;
#ifndef NO_HEADER
  int pad[32]={0};
#endif
  double  atime, redshift, Hz, box100, h100, omegab;
  struct particle_data P;
  double * rhoker_H=NULL;
  double * tau_H1=NULL;
  interp H1;
  const int nspecies = 1;
  /*Make sure stdout is line buffered even when not 
   * printing to a terminal but, eg, perl*/
  setlinebuf(stdout);
  while((c = getopt(argc, argv, "o:i:t:n:h")) !=-1)
  {
    switch(c)
      {
        case 'o':
           outname=std::string(optarg)+std::string("_spectra.dat");
           break;
        case 'i':
           indir=std::string(optarg);
           break;
        case 'n':
           NumLos=atoi(optarg);
           break;
        case 't':
           ext_table=optarg;
           break;
        case 'h':
        case '?':
           help();
        default:
           exit(1);
      }
  }
  if(NumLos <=0) {
          std::cerr<<"Need NUMLOS >0"<<std::endl;
          help();
          exit(99);
  }
  if(indir.empty()) {
         std::cerr<<"Specify input ("<<indir<<") directory."<<std::endl;
         help();
         exit(99);
  }
  los_table=(los *) malloc(NumLos*sizeof(los));
  if(!los_table){
          std::cerr<<"Error allocating memory for sightline table"<<std::endl;
          exit(2);
  }
  if(InitLOSMemory(&H1, NumLos, nspecies*NBINS) || 
      !(rhoker_H = (double *) calloc((NumLos * NBINS) , sizeof(double)))){
                  std::cerr<<"Error allocating LOS memory\n"<<std::endl;
                  exit(2);
  }
  #ifdef HDF5
    /*ffname is a copy of input filename for extension*/
    /*First open first file to get header properties*/
    std::string fname = find_first_hdf_file(indir);
    std::string ffname = fname;
    unsigned i_fileno=0;
    int fileno=0;
    if ( !fname.empty() && load_hdf5_header(fname.c_str(), &atime, &redshift, &Hz, &box100, &h100) == 0 ){
            /*See if we have been handed the first file of a set:
             * our method for dealing with this closely mirrors
             * HDF5s family mode, but we cannot use this, because
             * our files may not all be the same size.*/
	    i_fileno = fname.find(".0.hdf5")+1;
    }
    /*If not an HDF5 file, try opening as a gadget file*/
    else
#endif
     if(load_header(indir.c_str(),&atime, &redshift, &Hz, &box100, &h100) < 0){
                std::cerr<<"No data loaded\n";
                exit(2);
    }
    /*Setup the los tables*/
    populate_los_table(los_table,NumLos, ext_table, box100);
    IndexTable sort_los_table(los_table, NumLos, box100);
  if(!(output=fopen(outname.c_str(),"w")))
  {
          fprintf(stderr, "Error opening %s: %s\n",outname.c_str(), strerror(errno));
          exit(1);
  }
        /*Loop over files. Keep going until we run out, skipping over broken files.
         * The call to file_readable is an easy way to shut up HDF5's error message.*/
    while(1){
          /* P is allocated inside load_snapshot*/
#ifdef HDF5
          if(i_fileno){
            /*If we ran out of files, we're done*/
            if(!(file_readable(ffname.c_str()) && H5Fis_hdf5(ffname.c_str()) > 0))
                    break;
              Npart=load_hdf5_snapshot(ffname.c_str(), &P,&omegab,fileno);
          }
          else
#endif
              Npart=load_snapshot(indir.c_str(), StartPart,MaxRead,&P, &omegab);
          if(Npart > 0){
             /*Rescale neutral fraction to take account of hydrogen fraction*/
              for(int ii = 0; ii< Npart; ii++)
                P.fraction[ii]*=XH;
             /*Do the hard SPH interpolation*/
             SPH_Interpolation(rhoker_H,&H1, 1, NBINS, Npart, NumLos,box100, los_table,sort_los_table, &P);
          }
          /*Free the particle list once we don't need it*/
          if(Npart >= 0)
            free_parts(&P);
#ifdef HDF5
          if(i_fileno){
                fileno++;
                if(i_fileno != std::string::npos){
		  std::ostringstream convert;
		  convert<<fileno;
                  ffname = fname.replace(i_fileno, 1, convert.str());
		}
                else
                 break;
          }
          else
#endif
                StartPart+=Npart;
          /*If we haven't been able to read the maximum number of particles, 
           * signals we have reached the end of the snapshot set*/
	#ifdef HDF5
	  if(!i_fileno)
	#endif
          if(Npart != MaxRead)
                  break;
  }
  free(los_table);
  if(!(tau_H1 = (double *) calloc((NumLos * NBINS) , sizeof(double)))
                  ){
                  fprintf(stderr, "Error allocating memory for tau\n");
                  exit(2);
  }
  printf("Done interpolating, now calculating absorption\n");
#pragma omp parallel
  {
     #pragma omp for
     for(i=0; i<NumLos; i++){
       /*Make a bunch of pointers to the quantities for THIS LOS*/
       Rescale_Units(H1.rho+i*NBINS, H1.veloc+i*NBINS, H1.temp+i*NBINS, NBINS, h100, atime);
       Compute_Absorption(tau_H1+(i*NBINS), H1.rho+i*NBINS, H1.veloc+i*NBINS, H1.temp+i*NBINS,NBINS, Hz,h100, box100,atime, LAMBDA_LYA_H1, GAMMA_LYA_H1,FOSC_LYA,HMASS);
       Convert_Density(rhoker_H+(i*NBINS), H1.rho+i*NBINS, h100, atime, omegab);
     }
  }
  fwrite(&redshift,sizeof(double),1,output);
#ifndef NO_HEADER
  /*Write a bit of a header. */
  i=NBINS;
  fwrite(&box100,sizeof(double),1,output);
  fwrite(&i,sizeof(int),1,output);
  fwrite(&NumLos,sizeof(int),1,output);
  /*Write some space for future header data: total header size is
   * 128 bytes, with 24 full.*/
  fwrite(&pad,sizeof(int),32-6,output);
#endif
  fwrite(rhoker_H,sizeof(double),NBINS*NumLos,output);     /* gas overdensity */
  if(WriteLOSData(&H1, tau_H1,NumLos, output)){ 
     fprintf(stderr, "Error writing spectra to disk!\n");
  }
  fclose(output);
  /*Free other memory*/
  free(tau_H1);
  free(rhoker_H);
  FreeLOSMemory(&H1);
  return 0;
}
/**********************************************************************/

void help()
{
           fprintf(stderr, "Usage: ./extract -n NUMLOS -i filename (ie, without the .0) -o output_file (_flux_power.txt or _spectra.dat will be appended)\n"
                  "-t table_file will read line of sight coordinates from a table.\n");
           return;
}

int WriteLOSData(interp* species,double * tau, int NumLos,FILE * output)
{
  int items=0;
  items+=fwrite((*species).rho,sizeof(double),NBINS*NumLos,output);      /* n_HI/n_H */
  items+=fwrite((*species).temp,sizeof(double),NBINS*NumLos,output);   /* T [K], HI weighted */
  items+=fwrite((*species).veloc,sizeof(double),NBINS*NumLos,output);  /* v_pec [km s^-1], HI weighted */
  items+=fwrite(tau,sizeof(double),NBINS*NumLos,output);    /* HI optical depth */
  if(items !=4*NBINS*NumLos)
          return 1;
  return 0;
}
