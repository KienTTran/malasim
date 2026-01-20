# generate parameters for immunity validation
# import numpy as np


# max_clinical_probability = [0.9,0.99]

# immune_effect_on_progression_to_clinical = [3, 6.0]

# mid_point = [0.1, 0.4]

# points = 10

# base_input_path= '/Users/neo/Projects/temple/malasim/build/bin/immunity_validation/input.yml'
# id = 0
# params = []
# for mcp in np.linspace(max_clinical_probability[0], max_clinical_probability[1], points):
#     for iepc in np.linspace(immune_effect_on_progression_to_clinical[0], immune_effect_on_progression_to_clinical[1], points):
#         for mp in np.linspace(mid_point[0], mid_point[1], points):
#             print(f"Max Clinical Probability: {mcp}, Immune Effect on Progression to Clinical: {iepc}, Mid Point: {mp}")
#             # store the parameters in a dictionary with id
            

#             params.append([
#                 id,
#                 mcp,
#                 iepc,
#                 mp
#             ])
#             id += 1

# # create df 
# import pandas as pd
# df = pd.DataFrame(params, columns=['id', 'max_clinical_probability', 'immune_effect_on_progression_to_clinical', 'mid_point'])
# # save df to csv
# df.to_csv('/Users/neo/Projects/temple/malasim/build/bin/immunity_validation/params.csv', index=False)

###
# gnerate input files from params.csv
# import pandas as pd
# import numpy as np

# # Load the parameters from the CSV file
# params_df = pd.read_csv('/Users/neo/Projects/temple/malasim/build/bin/immunity_validation/params.csv')
# # id col is integer
# params_df['id'] = params_df['id'].astype(int)

# # read the yaml file
# import yaml

# # for each row in the params_df, create a new input.yml file
# for index, row in params_df.iterrows():
#     with open('/Users/neo/Projects/temple/malasim/build/bin/immunity_validation/input.yml', 'r') as file:
#         input_data = yaml.safe_load(file)
        
#         # Convert numpy scalars to native Python floats
#         input_data["immune_system_parameters"]["max_clinical_probability"] = float(row['max_clinical_probability'])
#         input_data["immune_system_parameters"]["immune_effect_on_progression_to_clinical"] = float(row['immune_effect_on_progression_to_clinical'])
#         input_data["immune_system_parameters"]["mid_point"] = float(row['mid_point'])
        
#         with open(f'/Users/neo/Projects/temple/malasim/build/bin/immunity_validation/input_{int(row["id"])}.yml', 'w') as file:
#             yaml.dump(input_data, file, default_flow_style=False)
#             print(f"Created input_{row['id']}.yml with parameters: {input_data['immune_system_parameters']}")

## run the simulation for each input file
import os
import pandas as pd
from concurrent.futures import ProcessPoolExecutor

def run_simulation(input_file, id):
    working_dir = '/Users/neo/Projects/temple/malasim/build/bin/immunity_validation'
    os.system(f'cd {working_dir} && ./malasim -i {input_file} -j {id}')

if __name__ == '__main__':
    # read parameters from params.csv
    params_df = pd.read_csv('/Users/neo/Projects/temple/malasim/build/bin/immunity_validation/params.csv')

    # run the simulation for each input file
    # use 4 cpus
    with ProcessPoolExecutor(max_workers=4) as executor:
        for index, row in params_df.iterrows():
            id = int(row['id'])
            input_file = f'input_{id}.yml'
            executor.submit(run_simulation, input_file, id)
            print(f"Submitted simulation for {input_file} with id {id}")