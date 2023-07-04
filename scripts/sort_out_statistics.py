import os

# Define the directory containing the log files
directory = '/home/hcpuyang/BalancedConversion/log/exp_b1'

merged_max_load_values_1 = []
merged_max_load_values_2 = []
merged_max_load_values_3 = []
merged_bandwidth_values_1 = []
merged_bandwidth_values_2 = []
merged_bandwidth_values_3 = []

# Loop through each file in the directory
for filename in os.listdir(directory):
    if filename.endswith('.log'):
        # Open the file for reading
        try:
            with open(os.path.join(directory, filename), 'r') as f:
                lines = f.readlines()
        except FileNotFoundError:
            print(f'File "{filename}" not found.')
            continue

        # Initialize the lists to store the max load and bandwidth values
        max_load_values = []
        bandwidth_values = []

        # Loop through each line in the file
        for line in lines:
            # Check if the line matches the pattern
            if 'max_load:' in line and 'bandwidth:' in line:
                # Extract the max load and bandwidth values
                values = line.strip().split(',')
                max_load_value = int(values[0].split(':')[1].strip())
                bandwidth_value = int(values[1].split(':')[1].strip())

                # Append the values to the corresponding lists
                max_load_values.append(max_load_value)
                bandwidth_values.append(bandwidth_value)

        # Check if the length of the lists is 75
        if len(max_load_values) != 75 or len(bandwidth_values) != 75:
            print(len(max_load_values))
            print(len(bandwidth_values))
            raise Exception(f'Invalid number of values in file "{filename}".')

        # Create three lists for the max load values
        max_load_list_1 = max_load_values[0:5] + max_load_values[15:20] + max_load_values[30:35] + max_load_values[45:50] + max_load_values[60:65]
        max_load_list_2 = max_load_values[5:10] + max_load_values[20:25] + max_load_values[35:40] + max_load_values[50:55] + max_load_values[65:70]
        max_load_list_3 = max_load_values[10:15] + max_load_values[25:30] + max_load_values[40:45] + max_load_values[55:60] + max_load_values[70:75]

        # Create three lists for the bandwidth values
        bandwidth_list_1 = bandwidth_values[0:5] + bandwidth_values[15:20] + bandwidth_values[30:35] + bandwidth_values[45:50] + bandwidth_values[60:65]
        bandwidth_list_2 = bandwidth_values[5:10] + bandwidth_values[20:25] + bandwidth_values[35:40] + bandwidth_values[50:55] + bandwidth_values[65:70]
        bandwidth_list_3 = bandwidth_values[10:15] + bandwidth_values[25:30] + bandwidth_values[40:45] + bandwidth_values[55:60] + bandwidth_values[70:75]

        # Print the filename and the merged lists
        # print(f'Filename: {filename}')
        # print(f'Merged Max Load Values: RDPM: {max_load_list_1}, BWPM: {max_load_list_2}, BTPM: {max_load_list_3}')
        # print(f'Merged Bandwidth Values: RDPM: {bandwidth_list_1}, BWPM: {bandwidth_list_2}, BTPM: {bandwidth_list_3}')

        # Append the merged lists to the corresponding lists
        # merged_max_load_values_1 += max_load_list_1
        # merged_max_load_values_2 += max_load_list_2
        # merged_max_load_values_3 += max_load_list_3

        # merged_bandwidth_values_1 += bandwidth_list_1
        # merged_bandwidth_values_2 += bandwidth_list_2
        # merged_bandwidth_values_3 += bandwidth_list_3

        # Calculate the statistics for the merged lists
        max_load_statistics_1 = {'average': sum(max_load_list_1) / len(max_load_list_1),
                                'max': max(max_load_list_1),
                                'min': min(max_load_list_1)}
        max_load_statistics_2 = {'average': sum(max_load_list_2) / len(max_load_list_2),
                                'max': max(max_load_list_2),
                                'min': min(max_load_list_2)}
        max_load_statistics_3 = {'average': sum(max_load_list_3) / len(max_load_list_3),
                                'max': max(max_load_list_3),
                                'min': min(max_load_list_3)}
        
        bandwidth_statistics_1 = sum(bandwidth_list_1) / len(bandwidth_list_1)
        bandwidth_statistics_2 = sum(bandwidth_list_2) / len(bandwidth_list_2)
        bandwidth_statistics_3 = sum(bandwidth_list_3) / len(bandwidth_list_3)

        # print(len(merged_max_load_values_3))
        # print(len(max_load_list_3))
        # print(min(max_load_list_3))

        # Print the statistics for the merged lists
        print(filename)
        print('RDPM statistics: ', bandwidth_statistics_1, max_load_statistics_1["average"], max_load_statistics_1["max"], max_load_statistics_1["min"])
        print('BWPM statistics: ', bandwidth_statistics_2, max_load_statistics_2["average"], max_load_statistics_2["max"], max_load_statistics_2["min"])
        print('BTPM statistics: ', bandwidth_statistics_3, max_load_statistics_3["average"], max_load_statistics_3["max"], max_load_statistics_3["min"])
        print("\n")
