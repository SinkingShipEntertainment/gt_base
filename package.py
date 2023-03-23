
version = '0.1.0'

name = 'gt_base'

description = 'base classes and functions'

authors = [
  'Tom Sirdevan'
]

with scope("config") as c:
    # Determine location to release: internal (int) vs external (ext)

    # NOTE: Modify this variable to reflect the current package situation
    release_as = "int"

    # The `c` variable here is actually rezconfig.py
    # `release_packages_path` is a variable defined inside rezconfig.py

    import os
    if release_as == "int":
        c.release_packages_path = os.environ["SSE_REZ_REPO_RELEASE_INT"]
    elif release_as == "ext":
        c.release_packages_path = os.environ["SSE_REZ_REPO_RELEASE_EXT"]

    # print('\n***in config scope: {0}\n'.format(os.environ["SSE_REZ_REPO_RELEASE_INT"]))

requires = [

]

private_build_requires = [
  # 'pybythec',
  'gcc',
  # 'libpng',
  # 'libjpeg',
  # 'libtiff',
  # 'libtga' 
  'openexr', # Iex IlmThread Imath OpenEXRUtil OpenEXRCore OpenEXR
  # 'pystring',
  # 'libexpat',
  # 'yaml-cpp',
  # 'ocio',
  # 'x264',
  # 'ffmpeg'
]

variants = [['platform-linux', 'arch-x86_64', 'os-centos-7']]

uuid = "repository.gt_base"

# # calling this overrides using cmake
# build_command = 'python {root}/build.py'

def pre_build_commands():

    print('pre-build commands')
    # import os
    # os.environ['TESTING'] = ','.join(variants)
    # command("source /opt/rh/devtoolset-6/enable")


    

