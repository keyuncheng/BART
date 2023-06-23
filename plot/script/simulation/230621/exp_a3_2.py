import re
import numpy as np
import matplotlib
import matplotlib.pyplot as plt
from common import *

# Define the file name
filename = "../../../data/simulation/230622/exp_a_3.dat"

# Define the column names
column_names = ["code_param_id", "code_param", "method_id", "method", "num_stripes", "bandwidth", "max_load"]

results = {}

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
        num_stripes = row[4]
        max_load = int(row[5])
        bandwidth = int(row[6])

        if code_param in results.keys():
            results[code_param].append(max_load)
        else:
            results[code_param] = [max_load]
        # Process the row
        print(f"code_param_id: {code_param_id}, code_param: {code_param}, method_id: {method_id}, method: {method}, num_stripes: {num_stripes}, bandwidth: {bandwidth}, max_load: {max_load}")

print(results.keys())

matplotlib.rcParams['pdf.fonttype'] = 42
matplotlib.rcParams['ps.fonttype'] = 42

# Create figure
fig, ax = plt.subplots(figsize=(8, 5.5))
ax.margins(0.01, 0)

labelfont = { 'fontfamily': 'Times New Roman',
    'weight': 'normal',
    #'size': 33,
    'size': 20,
}
xtickfont = { 'family': 'Times New Roman',
    'weight': 'normal',
    #'size': 33,
    'size': 20,
}
ytickfont = { 'family': 'Times New Roman',
    'weight': 'normal',
    #'size': 33,
    'size': 20,
}

# Ticks
xlabel_string = "Number of Nodes under (8,4,2)"
xticks = np.array([0, 1, 2, 3, 4])
xticklabels = [0, 100, 200, 300, 400]
ax.set_xlabel(xlabel_string, **labelfont)
yticks = [0, 100, 200, 300, 400, 500, 600]
yticklabels = ["0", "100", "200", "300", "400", "500", "600"]
plt.ylim((0, 600))
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

print(results["(8,4,2)"])
RDPM_datalist.append(results["(8,4,2)"][0])
RDPM_datalist.append(results["(8,4,2)"][3])
RDPM_datalist.append(results["(8,4,2)"][6])
RDPM_datalist.append(results["(8,4,2)"][9])
BWPM_datalist.append(results["(8,4,2)"][1])
BWPM_datalist.append(results["(8,4,2)"][4])
BWPM_datalist.append(results["(8,4,2)"][7])
BWPM_datalist.append(results["(8,4,2)"][10])
BTPM_datalist.append(results["(8,4,2)"][2])
BTPM_datalist.append(results["(8,4,2)"][5])
BTPM_datalist.append(results["(8,4,2)"][8])
BTPM_datalist.append(results["(8,4,2)"][11])

xdata = [1, 2, 3, 4]

plt.plot(xdata, RDPM_datalist, color=RDPM_color, lw=2, ms=5, ls='-', marker='o', label="RD")
plt.plot(xdata, BWPM_datalist, color=BWPM_color, lw=2, ms=5, ls='-.', marker='v', label="BW")
plt.plot(xdata, BTPM_datalist, color=BTPM_color, lw=2, ms=5, ls='-', marker='^', label="BART")

# print("server errlist: {} data: {}".format(server_errlist, server_data))
# print("switch errlist: {} data: {}".format(switch_errlist, switch_data))
# # server errlist: [1.882774803307896e-05, 5.716133854272165e-05, 0.0009672278771830634] data: [1.000574, 1.002832, 1.02658]
# # switch errlist: [0.052125016384473144, 0.10116977279376682, 0.054958182066236594] data: [1.00494, 1.0939799999999997, 1.35496]

# Legend
font1 = { 'family': 'Times New Roman',
    'weight': 'normal',
    #'size': 33,
    'size': 28,
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
filepath = "../../../pdf/simulation/230621/exp_a3_2.pdf"
fig.savefig(filepath, dpi=600)

# Embed fonts
embed_fonts(filepath)

plt.show()
