import os

main_path = os.getcwd()
Export('main_path')

# Tell SCons to create our build files in the 'build' directory
SConscript('src/SConscript', variant_dir='build', src='src', main_path='main_path', duplicate=0)
