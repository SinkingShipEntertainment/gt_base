
import pybythec
import sys
import os

def build_direct():
    '''
    '''

    os.environ['EXTERNAL'] = '/mnt/rez/release/ext'
    os.environ['VARIANT'] = 'platform-linux/arch-x86_64/os-centos-7'
    pybythec.build()


def main():
    '''
    '''

    # print(sys.argv)
    variants = sys.argv[1].split('|')

    rel_path = '../'
    for i in range(len(variants)):
        rel_path += '../'

    os.chdir(rel_path) # if called with 'rez-build -i' will be _rez_build dir so go up one 
    print(f'cwd: {os.getcwd()}')

    os.environ['PYBYTHEC_LOCAL'] = './pybythec.json' 

    # print(f'PYBYTHEC_GLOBALS: {os.environ["PYBYTHEC_GLOBALS"]}')   

    # import subprocess
    # subprocess.call(['g++', '--version']) 

    os.environ['EXTERNAL'] = '/mnt/rez/release/ext'
    os.environ['VARIANT'] = '/'.join(variants)

    pybythec.build()
    

if __name__ == '__main__':

    build_direct()
    # main()
