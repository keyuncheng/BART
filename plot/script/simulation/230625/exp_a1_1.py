import re
import numpy as np
import matplotlib
import matplotlib.pyplot as plt
from common import *

# Define the file name
filename = "../../../data/simulation/230625/exp_a_1_1.dat"

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
        bandwidth = float(row[4])
        max_load = float(row[5])
        max_load_max = float(row[6])
        max_load_min = float(row[7])

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
xticks = np.array([0, 1, 2, 3, 4, 5, 6, 7, 8])
xticklabels = [(4,2), (6,2), (8,2), (6,3), (8,3), (12,3), (8,4), (12,4), (16,4)]
ax.set_xlabel("(k, m)", **labelfont)
yticks = [0, 150, 300, 450, 600, 750]
yticklabels = ["0", "150", "300", "450", "600", "750"]
plt.ylim((0, 750))
ax.set_ylabel("Max Load (Number of Blocks)", **labelfont)
ax.set_xticks(xticks)
ax.set_xticklabels(xticklabels, fontdict=xtickfont)
ax.set_yticks(yticks)
ax.set_yticklabels(yticklabels, fontdict=ytickfont)
plt.grid(False)

# Data (<1e-3)
width = 0.3

RDPM_datalist = []
BWPM_datalist = []
BTPM_datalist = []
RDPM_data_errlist = []
BWPM_data_errlist = []
BTPM_data_errlist = []

for key in results.keys():
    print(results[key])
    RDPM_datalist.append(results[key][0])
    BWPM_datalist.append(results[key][1])
    BTPM_datalist.append(results[key][2])

    RDPM_data_errlist.append((err_results[key][0] - err_results[key][1])/2)
    BWPM_data_errlist.append((err_results[key][2] - err_results[key][3])/2)
    BTPM_data_errlist.append((err_results[key][4] - err_results[key][5])/2)


rects0 = ax.bar(xticks - 1*width, RDPM_datalist, width=width, color=RDPM_color, label="RD")
rects1 = ax.bar(xticks - 0*width, BWPM_datalist, width=width, color=BWPM_color, label="BW")
rects2 = ax.bar(xticks + 1*width, BTPM_datalist, width=width, color=BTPM_color, label="BART")
ax.errorbar(xticks - 1*width, RDPM_datalist, RDPM_data_errlist, fmt='none', elinewidth=1, ecolor=black, capsize=6, capthick=1, barsabove=True)
ax.errorbar(xticks - 0*width, BWPM_datalist, BWPM_data_errlist, fmt='none', elinewidth=1, ecolor=black, capsize=6, capthick=1, barsabove=True)
ax.errorbar(xticks + 1*width, BTPM_datalist, BTPM_data_errlist, fmt='none', elinewidth=1, ecolor=black, capsize=4, capthick=1, barsabove=True)

# print("server errlist: {} data: {}".format(server_errlist, server_data))
# print("switch errlist: {} data: {}".format(switch_errlist, switch_data))
# # server errlist: [1.882774803307896e-05, 5.716133854272165e-05, 0.0009672278771830634] data: [1.000574, 1.002832, 1.02658]
# # switch errlist: [0.052125016384473144, 0.10116977279376682, 0.054958182066236594] data: [1.00494, 1.0939799999999997, 1.35496]

# Legend
font1 = { 'family': 'Times New Roman',
    'weight': 'normal',
    #'size': 33,
    'size': 24,
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
filepath = "../../../pdf/simulation/230625/exp_a1_1.pdf"
fig.savefig(filepath, dpi=600)

# Embed fonts
embed_fonts(filepath)

plt.show()
