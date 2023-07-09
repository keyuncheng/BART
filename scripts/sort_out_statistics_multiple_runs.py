import os

# Define the directory containing the log files
directory = '/home/bart/BalancedConversion/log/exp_b3_0'

merged_max_load_values_1 = []
merged_max_load_values_2 = []
merged_max_load_values_3 = []
merged_bandwidth_values_1 = []
merged_bandwidth_values_2 = []
merged_bandwidth_values_3 = []

# Loop through each file in the directory
for filename in os.listdir(directory):
    if not filename.startswith("loads_") and filename.endswith('.log'):
        # Open the file for reading
        try:
            with open(os.path.join(directory, filename), 'r') as f:
                lines = f.readlines()
        except FileNotFoundError:
            print(f'File "{filename}" not found.')
            continue

        # print("--------" + filename + "--------")
        extract_prefix_list = filename.split('_')
        extract_prefix = extract_prefix_list[0] + "_" + extract_prefix_list[1] + "_" + extract_prefix_list[2] + "_" + extract_prefix_list[3]



        # Initialize the lists to store the max load and bandwidth values
        max_load_values = []
        bandwidth_values = []
        max_time_values = []

        # # Loop through each line in the file
        for line in lines:
            # Check if the line matches the pattern
            if extract_prefix in line:
                # Extract the max load and bandwidth values
                fields = line.split(":")
                max_time_value = fields[2].split(";")[0].strip()
                max_time_values.append(float(max_time_value))
                # print(max_time_value)

            if 'max_load:' in line and 'bandwidth:' in line:
                # Extract the max load and bandwidth values
                values = line.strip().split(',')
                max_load_value = int(values[0].split(':')[1].strip())
                bandwidth_value = int(values[1].split(':')[1].strip())

                # print(max_load_value)
                # print(bandwidth_value)

                # Append the values to the corresponding lists
                max_load_values.append(max_load_value)
                bandwidth_values.append(bandwidth_value)

        # print(max_load_values)
        # print(len(max_load_values))
        # print(sum(max_load_values))

        avg_max_time = sum(max_time_values)/len(max_time_values)
        # avg_max_load = sum(max_load_values)/len(max_load_values)s
        # avg_bandwidth = sum(bandwidth_values)/len(bandwidth_values)
        max_max_time = max(max_time_values)
        # max_max_load = max(max_load_values)
        # max_bandwidth = max(bandwidth_values)
        min_max_time = min(max_time_values)
        # min_max_load = min(max_load_values)
        # min_bandwidth = min(bandwidth_values)


        # print(avg_max_time, max_max_time, min_max_time, "\n")
        print(avg_max_time, max_max_time, min_max_time, avg_max_load, max_max_load, min_max_load, avg_bandwidth, max_bandwidth, min_bandwidth, "\n")

    elif filename.startswith("loads_"):
        try:
            with open(os.path.join(directory, filename), 'r') as f:
                lines = f.readlines()
        except FileNotFoundError:
            print(f'File "{filename}" not found.')
            continue

        print("--------" + filename + "--------")

        max_load_values = []
        bandwidth_values = []

        for line in lines:
            # Check if the line matches the pattern
            if 'max_load:' in line and 'bandwidth:' in line:
                # Extract the max load and bandwidth values
                values = line.strip().split(',')
                max_load_value = int(values[0].split(':')[1].strip())
                bandwidth_value = int(values[1].split(':')[1].strip())

                # print(max_load_value)
                # print(bandwidth_value)

                # Append the values to the corresponding lists
                max_load_values.append(max_load_value)
                bandwidth_values.append(bandwidth_value)

        avg_max_load = sum(max_load_values)/len(max_load_values)
        avg_bandwidth = sum(bandwidth_values)/len(bandwidth_values)
        max_max_load = max(max_load_values)
        max_bandwidth = max(bandwidth_values)
        min_max_load = min(max_load_values)
        min_bandwidth = min(bandwidth_values)


        print(avg_bandwidth, avg_max_load, "\n")
        # print(avg_max_load, max_max_load, min_max_load, avg_bandwidth, max_bandwidth, min_bandwidth, "\n")


        