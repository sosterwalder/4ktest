import os
import platform

Import('main_path')

operating_system = platform.system()

if operating_system == 'Darwin':
    libs = [
        'GLEW',
        'assimp',
        'Magick++-6.Q16',
        'glfw3'
    ]
else:
    libs = [
        'GL',
        'GLEW',
        'assimp',
        'glfw',
        'Magick++-6.Q16HDRI'
    ]

file_list = [
	Glob('*.cpp')
]

include_dirs = [
    '/usr/local/include/ImageMagick-6/',
    '/usr/include/ImageMagick-6/',
    '/usr/local/include/assimp/'
    '/usr/include/assimp/'
]

flags = [
	'-std=c++11',
	'-Wall'
]

link_flags = [
    '-framework',
    'OpenGL',
    '-framework',
    'IOKit',
    '-framework',
    'Cocoa'
]

defines = [
    '-DMAGICKCORE_QUANTUM_DEPTH=16',
    '-DMAGICKCORE_HDRI_ENABLE=1'
]

if ARGUMENTS.get('debug', 0):
	flags.extend(['-g'])
else:
	flags.extend(['-O3'])


env = Environment(
    CPPPATH = include_dirs,
	CCFLAGS = flags,
    CPPDEFINES = defines,
    ENV = {
        'PATH' : os.environ['PATH'],
        'TERM' : os.environ['TERM'],
        'HOME' : os.environ['HOME']
    }
)

if operating_system == 'Darwin':
    env.Append(
        RPATH = '/usr/local/lib'
    )
    env.Append(
        LINKFLAGS = link_flags
    )

env.Program(
    os.path.join(
        main_path,
        'solarsystem'
    ),
    file_list,
    LIBS=libs
)
