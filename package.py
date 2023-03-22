
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

requires = [

]

private_build_requires = [
  'pybythec'

  'libpng',
  'libjpeg',
  'libtiff',
  'libtga' 
  'openexr', # Iex IlmThread Imath OpenEXRUtil OpenEXRCore OpenEXR
  'pystring',
  'libexpat',
  'yaml-cpp',
  'ocio',
  'x264',
  'ffmpeg'

  # 'ffmpeg'
]

variants = [['platform-linux', 'arch-x86_64', 'os-centos-7']]

def pre_build_commands():
  print('pre-build commands')


def commands():
    
    env.FFMPEG_INC.append('{root}/python')

