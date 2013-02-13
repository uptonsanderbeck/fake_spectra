"""Class to gather and analyse various metal line statistics"""

import numpy as np
import hdfsim
import halocat
import convert_cloudy
import line_data
import spectra

class MetalLines:
    """Generate metal line spectra from simulation snapshot"""
    def __init__(self,num, base, minpart = 400, cloudy_dir="/home/spb/codes/ArepoCoolingTables/tmp_spb/", nbins = 1024):
        self.num = num
        self.base = base
        f = hdfsim.get_file(num, base, 0)
        self.box = f["Header"].attrs["BoxSize"]
        self.hubble = f["Header"].attrs["HubbleParam"]
        self.atime = f["Header"].attrs["Time"]
        self.redshift = f["Header"].attrs["Redshift"]
        self.omegam = f["Header"].attrs["Omega0"]
        OmegaLambda = f["Header"].attrs["OmegaLambda"]
        self.npart=f["Header"].attrs["NumPart_Total"]+2**32*f["Header"].attrs["NumPart_Total_HighWord"]
        f.close()
        self.Hz = 100.0*self.hubble * np.sqrt(self.omegam/self.atime**3 + OmegaLambda)
        self.nbins = nbins
        self.xbins = np.arange(0,self.nbins)*self.box/self.nbins
        self.species = ['He', 'C', 'N', 'O', 'Ne', 'Mg', 'Si', 'Fe']
        #Load halo centers to push lines through them
        min_mass = self.min_halo_mass(minpart)
        (ind, self.sub_mass, self.sub_cofm, self.sub_radii) = halocat.find_wanted_halos(num, base, min_mass)
        self.NumLos = np.size(self.sub_mass)
        #Random integers from [1,2,3]
        self.axis = np.random.random_integers(3, size = self.NumLos)
        #Line data
        self.lines = line_data.LineData()
        #Generate cloudy tables
        self.cloudy = convert_cloudy.CloudyTable(cloudy_dir)

    def min_halo_mass(self, minpart = 400):
        """Min resolved halo mass in internal Gadget units (1e10 M_sun)"""
        #This is rho_c in units of h^-1 1e10 M_sun (kpc/h)^-3
        rhom = 2.78e+11* self.omegam / 1e10 / (1e3**3)
        #Mass of an SPH particle, in units of 1e10 M_sun, x omega_m/ omega_b.
        target_mass = self.box**3 * rhom / self.npart[0]
        min_mass = target_mass * minpart
        return min_mass

    def get_tau(self, elem, ion):
        """Get the optical depth for a particular element out of:
           (He, C, N, O, Ne, Mg, Si, Fe)
           and some ion number
           NOTE: May wish to special-case SiIII at some point
        """
        #generate metal and hydrogen spectral densities
        #Indexing is: rho_metals [ NSPECTRA, NBIN ]
        (rho_H, metals) = spectra.SPH_Interpolate_metals(self.num, self.base, self.sub_cofm, self.axis, self.nbins, elem, ion, self.cloudy)
        #Rescale metals
        for (key, value) in metals.iteritems():
            value.rescale_units(self.hubble, self.atime)

        species = metals[elem]
        line = self.lines.get_line(elem,ion)
        mass = self.lines.get_mass(elem)
        #Compute tau for this metal ion
        tau_metal=np.empty(np.shape(species.rho))
        for n in np.arange(0,self.NumLos):
            tau_metal[n,:] = spectra.compute_absorption(np.size(self.xbins), species.rho[n,:], species.vel[n,:], species.temp[n,:],line,self.Hz,self.hubble, self.box, self.atime,mass)
        return tau_metal

    def vel_width(self, tau):
        """Find the velocity width of a line"""
        #  Size of a single velocity bin
        dvbin = self.box / (1.*self.nbins) * self.Hz *self.atime /self.hubble / 1000 # velocity bin size (kms^-1)

        tot_tau = np.sum(tau,axis = 1)
        cum_tau = np.cumsum(tau,axis = 1)
        vel_width = np.zeros(np.shape(tot_tau))
        for ll in np.arange(0, np.shape(tau)[1]):
            ind_low = np.where(cum_tau[ll,:] > 0.05 * tot_tau[ll])
            low = ind_low[0][-1]
            ind_high = np.where(cum_tau[ll,:] > 0.95 * tot_tau[ll])
            high = ind_high[0][0]
            vel_width[ll] = dvbin*(high-low)
        #Return the width
        return vel_width
