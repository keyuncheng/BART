import re
import numpy as np
import matplotlib
import matplotlib.pyplot as plt
from common import *

# Define the file name
filename = "../../../data/simulation/230625/exp_a_2.dat"

# Define the column names
column_names = ["code_param_id", "code_param", "method_id", "method", "bandwidth", "max_load", "max_load_max", "max_load_min"]

results = {}
err_results = {}

# Open the file for reading
with open(filename, "r") as file:
    # Skip the first row (column names)
    next(file)

    # Loop through each row in the file
    for line in file:
        # Split the line into columns using regular expressions to match any whitespace characters
        row = re.split(r"\s+", line.strip())

        # Extract the values from the row
        code_param_id = row[0]
        code_param = row[1]
        method_id = row[2]
        method = row[3]
        num_of_stripes = int(row[4])
        bandwidth = float(row[5])
        max_load = float(row[6])
        max_load_max = float(row[7])
        max_load_min = float(row[8])

        if code_param in results.keys():
            results[code_param].append(max_load)
        else:
            results[code_param] = [max_load]

        if code_param in err_results.keys():
            err_results[code_param].append(max_load_max)
            err_results[code_param].append(max_load_min)
        else:
            err_results[code_param] = [max_load_max, max_load_min]
        # Process the row
        print(f"code_param_id: {code_param_id}, code_param: {code_param}, method_id: {method_id}, method: {method}, bandwidth: {bandwidth}, max_load: {max_load}, max_load_max: {max_load_max}, max_load_min: {max_load_min}")

print(results.keys())

matplotlib.rcParams['pdf.fonttype'] = 42
matplotlib.rcParams['ps.fonttype'] = 42

# Create figure
fig, ax = plt.subplots(figsize=(8, 5.5))
ax.margins(0.01, 0)

labelfont = { 'fontfamily': 'Times New Roman',
    'weight': 'normal',
    #'size': 33,
    'size': 19,
}
xtickfont = { 'family': 'Times New Roman',
    'weight': 'normal',
    #'size': 33,
    'size': 17,
}
ytickfont = { 'family': 'Times New Roman',
    'weight': 'normal',
    #'size': 33,
    'size': 19,
}

# Ticks
xlabel_string = "Number of Stripes under (6,3,3)"
xticks = np.array([0, 1, 2, 3, 4])
xticklabels = [0, 5000, 10000, 15000, 20000]
ax.set_xlabel(xlabel_string, **labelfont)
yticks = [0, 200, 400, 600, 800, 1000, 1200]
yticklabels = ["0", "200", "400", "600", "800", "1000", "1200"]
plt.ylim((0, 1200))
ax.set_ylabel("Max Load (blocks)", **labelfont)
ax.set_xticks(xticks)
ax.set_xticklabels(xticklabels, fontdict=xtickfont)
ax.set_yticks(yticks)
ax.set_yticklabels(yticklabels, fontdict=ytickfont)
plt.grid(False)

# Data (<1e-3)
width = 0.2

RDPM_datalist = []
BWPM_datalist = []
BTPM_datalist = []
RDPM_data_errlist = []
BWPM_data_errlist = []
BTPM_data_errlist = []

print(results["(6,3,3)"])
RDPM_datalist.append(results["(6,3,3)"][0])
RDPM_datalist.append(results["(6,3,3)"][3])
RDPM_datalist.append(results["(6,3,3)"][6])
RDPM_datalist.append(results["(6,3,3)"][9])
BWPM_datalist.append(results["(6,3,3)"][1])
BWPM_datalist.append(results["(6,3,3)"][4])
BWPM_datalist.append(results["(6,3,3)"][7])
BWPM_datalist.append(results["(6,3,3)"][10])
BTPM_datalist.append(results["(6,3,3)"][2])
BTPM_datalist.append(results["(6,3,3)"][5])
BTPM_datalist.append(results["(6,3,3)"][8])
BTPM_datalist.append(results["(6,3,3)"][11])

RDPM_data_errlist.append(err_results["(6,3,3)"][0]-err_results["(6,3,3)"][1])
RDPM_data_errlist.append(err_results["(6,3,3)"][6]-err_results["(6,3,3)"][7])
RDPM_data_errlist.append(err_results["(6,3,3)"][12]-err_results["(6,3,3)"][13])
RDPM_data_errlist.append(err_results["(6,3,3)"][18]-err_results["(6,3,3)"][19])
BWPM_data_errlist.append(err_results["(6,3,3)"][2]-err_results["(6,3,3)"][3])
BWPM_data_errlist.append(err_results["(6,3,3)"][8]-err_results["(6,3,3)"][9])
BWPM_data_errlist.append(err_results["(6,3,3)"][14]-err_results["(6,3,3)"][15])
BWPM_data_errlist.append(err_results["(6,3,3)"][20]-err_results["(6,3,3)"][21])
BTPM_data_errlist.append(err_results["(6,3,3)"][4]-err_results["(6,3,3)"][5])
BTPM_data_errlist.append(err_results["(6,3,3)"][10]-err_results["(6,3,3)"][11])
BTPM_data_errlist.append(err_results["(6,3,3)"][16]-err_results["(6,3,3)"][17])
BTPM_data_errlist.append(err_results["(6,3,3)"][22]-err_results["(6,3,3)"][23])


xdata = [1, 2, 3, 4]

print(len(RDPM_data_errlist))
print(len(RDPM_datalist))
# plt.plot(xdata, RDPM_datalist, color=RDPM_color, lw=2, ms=5, ls='-', marker='o', label="RD")
# plt.plot(xdata, BWPM_datalist, color=BWPM_color, lw=2, ms=5, ls='-.', marker='v', label="BW")
# plt.plot(xdata, BTPM_datalist, color=BTPM_color, lw=2, ms=5, ls='-', marker='^', label="BART")
plt.errorbar(x=xdata, y=RDPM_datalist, yerr=RDPM_data_errlist, marker='s', capsize=2,
             color=RDPM_color, markersize=4, linewidth=1, linestyle='-', label="RD")
plt.errorbar(x=xdata, y=BWPM_datalist, yerr=RDPM_data_errlist, marker='v', capsize=2,
             color=BWPM_color, markersize=4, linewidth=1, linestyle='-', label="BW")
plt.errorbar(x=xdata, y=BTPM_datalist, yerr=RDPM_data_errlist, marker='^', capsize=2,
             color=BTPM_color, markersize=4, linewidth=1, linestyle='-', label="BART")


# print("server errlist: {} data: {}".format(server_errlist, server_data))
# print("switch errlist: {} data: {}".format(switch_errlist, switch_data))
# # server errlist: [1.882774803307896e-05, 5.716133854272165e-05, 0.0009672278771830634] data: [1.000574, 1.002832, 1.02658]
# # switch errlist: [0.052125016384473144, 0.10116977279376682, 0.054958182066236594] data: [1.00494, 1.0939799999999997, 1.35496]

# Legend
font1 = { 'family': 'Times New Roman',
    'weight': 'normal',
    #'size': 33,
    'size': 26,
}
legend = plt.legend(prop=font1, frameon=False,
                    labelspacing=0.1, handlelength=0.7, 
                    loc='upper center',
                    #bbox_to_anchor=(-0.15, 1), loc='lower left',
                    handletextpad=0.2, borderaxespad=0.001,
                    ncol=3, columnspacing=0.3)

plt.subplots_adjust(right=1.0)

# Save figure
fig.tight_layout()
filepath = "../../../pdf/simulation/230625/exp_a2_1.pdf"
fig.savefig(filepath, dpi=600)

# Embed fonts
embed_fonts(filepath)

plt.show()
