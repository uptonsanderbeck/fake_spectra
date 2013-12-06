# -*- coding: utf-8 -*-
"""Class to gather and analyse various metal line statistics"""

import numpy as np
import hdfsim
import spectra

class RandSpectra(spectra.Spectra):
    """Generate metal line spectra from simulation snapshot"""
    def __init__(self,num, base, numlos=5000, res = 1., cdir = None, thresh=10**20.3, savefile="rand_spectra_DLA.hdf5", savedir=None):
        #Load halos to push lines through them
        f = hdfsim.get_file(num, base, 0)
        self.box = f["Header"].attrs["BoxSize"]
        f.close()
        self.NumLos = numlos
        #All through y axis
        axis = np.ones(self.NumLos)
        #Sightlines at random positions
        #Re-seed for repeatability
        np.random.seed(23)
        cofm = self.get_cofm()
        spectra.Spectra.__init__(self,num, base, cofm, axis, res, cdir, savefile=savefile,savedir=savedir,reload_file=True)

        if thresh > 0:
            self.replace_not_DLA(thresh)
        print "Found DLAs"


    def get_cofm(self, num = None):
        """Find a bunch more sightlines: should be overriden by child classes"""
        if num == None:
            num = self.NumLos
        cofm = self.box*np.random.random_sample((num,3))
        return cofm

