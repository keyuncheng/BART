import os
import math
import numpy as np
import matplotlib.pyplot as plt

black = '#000000' 

orange = "#FF4500"
tomato = "#FF6347"

darkblue = "#00008B"
blue = "#0000FF"
dodgerblue = "#1E90FF"
royalblue = "#4169E1"

darkgreen = "#008B00"
green = "#00CD00"
springgreen = "#00EE76"
palegreen = "#9AFF9A"

RDPM_color="#1077AE"
# RDPM_color="#19376D"
BWPM_color="#008A2A"
BTPM_color="red"

def embed_fonts(filepath):
    os.system("ps2pdf -dPDFSETTINGS=/prepress {} tmp.out".format(filepath))
    os.system("rm {}".format(filepath))
    os.system("mv tmp.out {}".format(filepath))

def avgthpt_improvement(thptlist_a, thptlist_b):
    result = 0.0
    for i in range(len(thptlist_a)):
        result += (thptlist_b[i] / thptlist_a[i])
    return result / float(len(thptlist_a))

# Calculate confidence interval (CI) by student-t distribution

# t-score at confidence level (CL) of 95% (two-sided) indexed by degree of freedom (DF)
tscore_at95cl_bydf = [None, 12.706, 4.303, 3.182, 2.776, 2.571, 2.447, 2.365, 2.306, 2.262, 2.228]

# Get CI from samples
def getci_bysamples(samples):
    df = len(samples) - 1
    if df <= 0 or df > len(tscore_at95cl_bydf):
        print("[ERROR] invalid DF: {}".format(df))
        exit(-1)

    sample_mean = 0.0
    for i in range(len(samples)):
        sample_mean += samples[i]
    sample_mean /= float(len(samples))

    sample_variance = 0.0
    for i in range(len(samples)):
        sample_variance += (samples[i] - sample_mean) ** 2 # calculate square
    sample_variance /= float(df)

    standard_deviation = math.sqrt(sample_variance)

    tmp_delta = tscore_at95cl_bydf[df] * standard_deviation / math.sqrt(float(len(samples)))

    cileft = sample_mean - tmp_delta
    ciright = sample_mean + tmp_delta

    return cileft, ciright, sample_mean

# Get CI from statistics list
# @statlist: # of running times * statistics under different settings per running time
def getci_bystatlist_helper(statlist):
    samplenum = len(statlist)
    statnum = len(statlist[0])

    cilen_list = [] * statnum
    err_list = [] * statnum
    samplemean_list = [] * statnum
    cileft_list = [] * statnum
    ciright_list = [] * statnum

    for statidx in range(statnum):
        tmp_samples = []
        for i in range(samplenum):
            tmp_samples.append(statlist[i][statidx])
        tmp_cileft, tmp_ciright, tmp_samplemean = getci_bysamples(tmp_samples)

        cilen_list.append(abs(tmp_ciright - tmp_cileft))
        err_list.append(abs(tmp_ciright - tmp_cileft)/2.0)
        samplemean_list.append(tmp_samplemean)
        cileft_list.append(tmp_cileft)
        ciright_list.append(tmp_ciright)
    
    return err_list, cilen_list, samplemean_list, cileft_list, ciright_list

def getci_bystatlist(statlist):
    err_list, cilen_list, samplemean_list, cileft_list, ciright_list = getci_bystatlist_helper(statlist)
    return err_list, samplemean_list
