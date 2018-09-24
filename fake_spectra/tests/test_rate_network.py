"""Tests for the rate network."""

import time
import numpy as np
import rate_network
import ratenetworkspectra

def _exact_alphaHp():
    """For hydrogen recombination we have an exact answer from Ferland et al 1992 (http://adsabs.harvard.edu/abs/1992ApJ...387...95F).
    This function returns as an array these rates, for testing purposes."""
    #case B recombination rates for hydrogen from Ferland 92, final column of Table 1. For n >= 2.
    f92g2 = np.array([5.758e-11, 2.909e-11, 1.440e-11, 6.971e-12,3.282e-12, 1.489e-12, 6.43e-13, 2.588e-13, 9.456e-14, 3.069e-14, 8.793e-15, 2.245e-15, 5.190e-16, 1.107e-16, 2.221e-17, 4.267e-18, 7.960e-19, 1.457e-19,2.636e-20, 4.737e-21])
    #case B recombination rates for hydrogen from Ferland 92, second column of Table 1. For n == 1.
    f92n1 = np.array([9.258e-12, 5.206e-12, 2.927e-12, 1.646e-12, 9.246e-13, 5.184e-13, 2.890e-13, 1.582e-13, 8.255e-14, 3.882e-14, 1.545e-14, 5.058e-15, 1.383e-15, 3.276e-16, 7.006e-17, 1.398e-17, 2.665e-18, 4.940e-19, 9.001e-20, 1.623e-20])
    tt = 10**np.linspace(0.5, 10, 20)
    return (tt, f92g2+f92n1)

def testRecombRates():
    """Test the recombination rates"""
    (tt, recomb_exact) = _exact_alphaHp()
    recomb = rate_network.RecombRatesVerner96()
    assert np.all(np.abs(recomb.alphaHp(tt)/ recomb_exact-1.) < 1e-2)
    #Cen rates are not very accurate.
    recomb = rate_network.RecombRatesCen92()
    assert np.all(np.abs(recomb.alphaHp(tt[4:12])/ recomb_exact[4:12]-1.) < 0.5)

def testRateNetwork():
    """Simple tests for the rate network"""
    rates = rate_network.RateNetwork(redshift=2.)
    #Complete ionisation at low density
    assert np.abs(rates.get_equilib_ne(1e-6, 200.,helium=0.24) / (1e-6*0.76) - (1 + 2* 0.24/(1-0.24)/4)) < 3e-5
    assert np.abs(rates.get_equilib_ne(1e-6, 200.,helium=0.12) / (1e-6*0.88) - (1 + 2* 0.12/(1-0.12)/4)) < 3e-5
    assert np.abs(rates.get_equilib_ne(1e-5, 200.,helium=0.24) / (1e-5*0.76) - (1 + 2* 0.24/(1-0.24)/4)) < 3e-4
    assert np.abs(rates.get_equilib_ne(1e-4, 200.,helium=0.24) / (1e-4*0.76) - (1 + 2* 0.24/(1-0.24)/4)) < 2e-3

    assert 9500 < rates.get_temp(1e-4, 200.,helium=0.24) < 9510
    #Roughly prop to internal energy when ionised
    assert np.abs(rates.get_temp(1e-4, 400.,helium=0.24) / rates.get_temp(1e-4, 200.,helium=0.24) - 2.) < 1e-3
    #Neutral fraction prop to density.
    for dens in (1e-4, 1e-5, 1e-6):
        assert np.abs(rates.get_neutral_fraction(dens, 200.,helium=0.24) / dens - 0.31817) < 1e-3
    #Neutral (self-shielded) at high density:
    assert rates.get_neutral_fraction(1, 100.,helium=0.24) > 0.95
    assert 0.75 > rates.get_neutral_fraction(0.1, 100.,helium=0.24) > 0.74

    #Check self-shielding is working.
    rates2 = rate_network.RateNetwork(redshift=2., selfshield=False)
    assert rates2.get_neutral_fraction(1, 100.,helium=0.24) < 0.25
    assert rates2.get_neutral_fraction(0.1, 100.,helium=0.24) <0.05

    #Higher temp for same int. energy.
    assert 14500 < rates.get_temp(1, 200.,helium=0.24) < 14900

    #Check self-shielding is working.
    rates2 = rate_network.RateNetwork(redshift=2., selfshield=False)
    dens = np.logspace(-6,1,50)
    neuts = rates.get_neutral_fraction(dens, 100.)/(0.76*dens)
    neuts2 = rates2.get_neutral_fraction(dens, 100.)/(0.76*dens)
    ii = np.where(dens < 1e-4)
    assert np.all(np.abs(neuts[ii]/neuts2[ii] -1.) < 2e-2)
    before = time.clock()
    ntests = 100
    for i in range(ntests):
        nH_bench = ((1.*i)/ntests-1e-6)
        for j in range(ntests):
            uu_bench = ((2.e5*j)/ntests + 200.)
            rates2.get_temp(nH_bench, uu_bench)
    after = time.clock()
    print(after-before)

def testRateNetworkGas():
    """Test that the spline is working."""
    gasprop = ratentworkspectra.RateNetworkGas(3, None)
    dlim = (np.log(1e-7), np.log(3))
    elim = (np.log(20), np.log(3e6))
    randd = (dlim[1] - dlim[0]) * np.random.random(size=2000) + dlim[0]
    randi = (elim[1] - elim[0]) * np.random.random(size=2000) + elim[0]
    spline = gasprop.build_interp(dlim, elim)
    for dd, ii in zip(randd, randi):
        spl = spline(dd, ii)[0]
        rate = np.log(gasprop.rates.get_neutral_fraction(np.exp(dd), np.exp(ii)))
        assert np.abs(spl - rate) < 1e-5