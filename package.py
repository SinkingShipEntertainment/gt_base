
version = '0.1.0'

name = 'gt_base'

description = 'base classes and functions'

authors = [
  'Tom Sirdevan'
]

with scope("config") as c: # 'c' is from rezconfig.py

    release_as = "int"

    import os
    if release_as == "int":
        c.release_packages_path = os.environ["SSE_REZ_REPO_RELEASE_INT"]
    elif release_as == "ext":
        c.release_packages_path = os.environ["SSE_REZ_REPO_RELEASE_EXT"]

requires = [
]

private_build_requires = [
  # 'pybythec'
  'gcc'
]

variants = [['platform-linux', 'arch-x86_64', 'os-centos-7']]

uuid = "repository.gt_base"

# # calling build_command overrides using cmake
# build_command = 'python {root}/build.py'

def pre_build_commands():

    print('pre-build commands')


    

