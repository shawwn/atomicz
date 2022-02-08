import os
import sys
import shutil
from distutils.command.build_ext import build_ext
from distutils.core import Extension, setup
from distutils.sysconfig import customize_compiler

PACKAGE_NAME='atomicz'
__version__ = '0.3.1'

if 'clean' in sys.argv:
    curdir = os.path.dirname(os.path.realpath(__file__))
    for filepath in ['build', 'dist', f'{PACKAGE_NAME}.egg-info', 'MANIFEST']:
        if os.path.exists(filepath):
            if os.path.isfile(filepath):
                os.remove(filepath)
            else:
                shutil.rmtree(filepath)

class my_build_ext(build_ext):
    def build_extensions(self):
        customize_compiler(self.compiler)
        try:
            self.compiler.compiler_so.remove("-Wstrict-prototypes")
        except (AttributeError, ValueError):
            pass
        build_ext.build_extensions(self)


module1 = Extension(PACKAGE_NAME,
                    sources=['atomicz.cc'],
                    include_dirs=[],
                    extra_compile_args=['-std=c++14'])

setup(name=PACKAGE_NAME,
      version=__version__,
      description='atomic operations on raw memory for python',
      license='Apache',
      author='Shawn Presser',
      author_email='shawnpresser@gmail.com',
      url='https://github.com/shawwn/atomicz',
      ext_modules=[module1],
      cmdclass={'build_ext': my_build_ext})
