
import pybythec

import os
os.chdir('../') # if called with 'rez-build -i' will be _rez_build dir so go up one 
print(f'cwd: {os.getcwd()}')

# os.environ['PYBYTHEC_LOCAL'] = '../pybythec.json' 

print(f'PYBYTHEC_GLOBALS: {os.environ["PYBYTHEC_GLOBALS"]}')

pybythec.build()
