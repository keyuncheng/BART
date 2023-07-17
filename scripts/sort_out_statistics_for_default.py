import os

# Define the directory containing the log files
directory = '/home/bart/BalancedConversion/scripts/placements/1022/'

# method_list = ["BWPM", "BTPM", "RDPM"]
method_list = ["BWPM", "BTPM"]

for method in method_list:
    max_load_values = []
    bandwidth_values = []
    solution_generation_times = []
    for round_index in range(10):
        filename = directory + "round" + str(round_index) + "/" + method + "/post_placement_results.log"
        # print(filename)

        if filename.endswith('.log'):
            # Open the file for reading
            try:
                with open(os.path.join(directory, filename), 'r') as f:
                    lines = f.readlines()
            except FileNotFoundError:
                print(f'File "{filename}" not found.')
                continue

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
                if 'finished generating solution,' in line:
                    values = line.strip().split(':')
                    # print(values[1].split(" "))
                    generation_time = float(values[1].split(" ")[1].strip())
                    solution_generation_times.append(generation_time)
    avg_max_load = sum(max_load_values)/len(max_load_values)
    avg_bandwidth_load = sum(bandwidth_values)/len(bandwidth_values)
    print(method, avg_bandwidth_load, avg_max_load)
    print(sum(solution_generation_times)/len(solution_generation_times))
    print(max_load_values)
