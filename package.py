
version = '0.1.0'

name = 'gt_base'

description = 'base classes and functions'

authors = [
  'Tom Sirdevan'
]

with scope("config") as c: # 'c' is from rezconfig.py
  import os
  c.release_packages_path = os.environ["SSE_REZ_REPO_RELEASE_INT"]

requires = [
  "libjpeg",
  "libspng",
  "tga",
  "openexr-3",
  "ocio-2",
  "freetype",
  "x264",
  "ffmpeg",
]

variants = [['platform-linux', 'arch-x86_64', 'os-centos-7']]

uuid = "repository.gt_base"

# private_build_requires = [
#   'pybythec'
# ]

# build_command = 'python {root}/build.py "' + '|'.join(variants[0]) + '"'

def pre_build_commands():
  command("source /opt/rh/devtoolset-6/enable")

  # print('\nusing g++ version:')
  # import subprocess
  # subprocess.call(['g++', '--version'])
