// -*- C++ -*-                                                                  
////////////////////////////////////////////////////////////////////////////////
///                                                                             
/// \file   mkWD99disc.cc                                                       
///                                                                             
/// \brief  creates N-body initial conditions far a galactic disc based on the  
///         distribution function f_new of Dehnen (1999).                       
///                                                                             
/// \author Paul McMillan                                                       
/// \author Walter Dehnen                                                       
/// \date   2005-2007                                                           
///                                                                             
////////////////////////////////////////////////////////////////////////////////
//                                                                              
// Copyright (C) 2005-2007 Paul McMillan, Walter Dehnen                         
//                                                                              
// This program is free software; you can redistribute it and/or modify         
// it under the terms of the GNU General Public License as published by         
// the Free Software Foundation; either version 2 of the License, or (at        
// your option) any later version.                                              
//                                                                              
// This program is distributed in the hope that it will be useful, but          
// WITHOUT ANY WARRANTY; without even the implied warranty of                   
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU            
// General Public License for more details.                                     
//                                                                              
// You should have received a copy of the GNU General Public License            
// along with this program; if not, write to the Free Software                  
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.                    
//                                                                              
////////////////////////////////////////////////////////////////////////////////
//                                                                              
// history:                                                                     
//                                                                              
// v 1.0   24/06/2005   PJM started the process of making this                  
// v 1.1      09/2005   PJM added iteration of the density profile              
// v 1.2      01/2006   PJM added iteration of the velocity distribution        
// v 1.3   30/04/2007   WD  making ready for release                            
// v 1.4   13/09/2007   WD  some changes in parameter names                     
// v 2.0   17/09/2007   WD  using DiscPot (new) for disc's own potential        
////////////////////////////////////////////////////////////////////////////////
#define falcON_VERSION   "1.4"
#define falcON_VERSION_D "11-sep-2007 Paul McMillan & Walter Dehnen          "
//-----------------------------------------------------------------------------+
#ifndef falcON_NEMO                                // this is a NEMO program    
#  error You need NEMO to compile mkWD99disc
#endif
//-----------------------------------------------------------------------------+
#include <body.h>                                  // N-body bodies             
#include <public/io.h>                             // my I/O utilities          
#include <public/WD99disc.h>                       // my disc models            
#include <fstream>                                 // C++ file I/O              
#include <iomanip>                                 // C++ I/O formatting        
#include <main.h>                                  // main & NEMO stuff         
////////////////////////////////////////////////////////////////////////////////
string defv[] = {
  "out=???\n          output file                                        ",
  "nbody=???\n        number of bodies                                   ",
  "nbpero=100\n       number of bodies per orbit                         ",
  "R_d=1\n            scale radius: Surface density=Sig_0*(e^(-R/R_d))   ",
  "Sig_0=???\n        central surface density, see above                 ",
  "R_sig=0\n          vel. disp. scale radius, sigr propto e^(-R/R_sig)  ",
  "Q=???\n            Toomre's Q, const if R_sig=0, else Q(R_sig)        ",
  "z_d=0.1\n          vertical scale height                              ",
  "eps=0\n            particle smoothing length (undefined if 0)         ",
  "seed=0\n           seed for the randum number generator               ",
  "q-ran=f\n          use quasi- instead of pseudo-random numbers        ",
  "time=0\n           simulation time of snapshot                        ",
  "ni=3\n             no. iterations of disc surf dens & vel disp (min 1)",
  "giveF=f\n          give distribution function in aux data?            ",
//   "WD_units=f\n       input:  kpc, km/s\n"
//   "                   output: kpc, kpc/Gyr, G=1 (-> mass unit)           ",
  "accname=\n         name of any external acceleration field            ",
  "accpars=\n         parameters of external acceleration field          ",
  "accfile=\n         file required by external acceleration field       ",
  falcON_DEFV, NULL };
//------------------------------------------------------------------------------
string usage =
"mkWD99disc: construct a disc according to Dehnen (1999)\n"
"            optionally immersed in an external gravitational potential\n"
"\n"
"    Surface density: Sigma(R) = Sig_0 * exp(R/R_d)\n"
"\n"
"    Radial velocity dispersion:\n"
"    if R_sig=0: such that Toomre's (kappa(R) is epicycle frequency)\n"
"\n"
"                           sigma(R) * kappa(R)\n"
"                     Q =   -------------------\n"
"                           3.36 * G * Sigma(R)\n"
"\n"
"    otherwise (R_sig>0): sigma(R) = sigma_0 * exp(R/R_sig)\n"
"                         with sigma_0 such that Q(R_sig)=Q\n"
"\n"
"    z-component is an isothermal sheet with rho_z propto sech^2(z/z_d)\n"
"\n"
" BEWARE: known \"feature\". On occasion there is an error report from\n"
" \"find\". Can be avoided by reducing ni, or increasing nbpero \n";
//------------------------------------------------------------------------------
void falcON::main() falcON_THROWING
{
//   const double vf = 0.977775320024919;             // km/s  in WD_units         
  const double mf = 2.2228847e5;                   // M_sun in WD_units         
  //----------------------------------------------------------------------------
  // 1. set some parameters                                                     
  //----------------------------------------------------------------------------
  if (getiparam("ni") < 1) error("Code requires at least one iteration");

//   const bool   WD (getbparam("WD_units"));         // using WD_units?           
  const Random Ran(getparam("seed"),8);
  const fieldset data( 
    (hasvalue ("eps")   ? fieldset::e : fieldset::o) |
    (getbparam("giveF") ? fieldset::y : fieldset::o) |
    fieldset::basic );
  // add disc potential using external potential DiscPot
  char accname[256]={0};
  bool extacc = hasvalue("accname");
  if(extacc) {
    strcpy(accname,getparam("accname"));
    strcat(accname,"+DiscPot");
  } else
    strcpy(accname,"DiscPot");
  char accpars[1024]={0};
  if(extacc) {
    strcpy(accpars,getparam_z("accpars"));
    strcat(accpars,";");
  }
  char accpar[128];
  sprintf(accpar,"0,%g",getdparam("Sig_0")*mf); strcat(accpars,accpar);
  sprintf(accpar,",%g",getdparam("R_d")); strcat(accpars,accpar);
  sprintf(accpar,",%g",-0.5*abs(getdparam("z_d"))); strcat(accpars,accpar);
  char accfile[1024]={0};
  if(extacc) {
    strcpy(accfile,getparam_z("accfile"));
    strcat(accfile,";");
  }
  nemo_acc Aex(accname,              //   initialize external  
	       accpars,              //   accelerations        
	       accfile);
  // now sample velocities ...
  WD99disc DM(getiparam("nbpero"),
	      getdparam("R_d"),
	      getdparam("Sig_0"),
	      getdparam("R_sig"),
	      getdparam("Q"),
	      getdparam("z_d"),
	      getdparam("eps"),&Aex);
  snapshot shot(getdparam("time"),getiparam("nbody"), data);
  if(getdparam("Q"))    
    DM.sample(shot,getiparam("ni"),getbparam("q-ran"),Ran,getbparam("giveF"));
  else
    DM.coldsample(shot,getbparam("q-ran"),Ran);

  //----------------------------------------------------------------------------
  // 3. output of snapshot                                                      
  //----------------------------------------------------------------------------
  nemo_out out(getparam("out"));
  shot.write_nemo(out,data);
}
