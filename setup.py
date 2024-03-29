"""
Bacon 2 Python module setup using setuptools
Adapted from https://github.com/pybind/python_example
"""
import sys
from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext
from setuptools.command.develop import develop
import setuptools

__version__ = '2.0.0'


class get_pybind_include(object):
    """Helper class to determine the pybind11 include path
    The purpose of this class is to postpone importing pybind11
    until it is actually installed, so that the ``get_include()``
    method can be invoked. """

    def __init__(self, user=False):
        self.user = user

    def __str__(self):
        import pybind11
        return pybind11.get_include(self.user)


EXT_MODULES = [
    Extension(
        '_bacon',
        [
            'main.cpp',
            'strategy.cpp',
            'session.cpp',
            'util.cpp',
            'core.cpp',
            'config.cpp'
        ],
        include_dirs=[
            # Path to pybind11 headers
            get_pybind_include(),
            get_pybind_include(user=True),
            'include/'
        ],
        language='c++'
    ),
]


# As of Python 3.6, CCompiler has a `has_flag` method.
# cf http://bugs.python.org/issue26689
def has_flag(compiler, flagname):
    """Return a boolean indicating whether a flag name is supported on
    the specified compiler.
    """
    import tempfile
    with tempfile.NamedTemporaryFile('w', suffix='.cpp') as f:
        f.write('int main (int argc, char **argv) { return 0; }')
        try:
            compiler.compile([f.name], extra_postargs=[flagname])
        except setuptools.distutils.errors.CompileError:
            return False
    return True


def cpp_flag(compiler):
    """Return the -std=c++[11/14/17] compiler flag.
    The newer version is prefered over c++11 (when it is available).
    """
    flags = ['-std=c++17', '-std=c++14', '-std=c++11']

    for flag in flags:
        if has_flag(compiler, flag):
            return flag

    raise RuntimeError('Unsupported compiler -- at least C++11 support '
                       'is needed!')


class BuildExt(build_ext):
    """A custom build extension for adding compiler-specific options."""
    c_opts = {
        'msvc': ['/EHsc'],
        'unix': [],
    }
    l_opts = {
        'msvc': [],
        'unix': [],
    }

    if sys.platform == 'darwin':
        darwin_opts = ['-stdlib=libc++', '-mmacosx-version-min=10.7']
        c_opts['unix'] += darwin_opts
        l_opts['unix'] += darwin_opts

    def build_extensions(self):
        ct = self.compiler.compiler_type
        opts = self.c_opts.get(ct, [])
        link_opts = self.l_opts.get(ct, [])
        ver = self.distribution.get_version()
        if ct == 'unix':
            opts.append('-DVERSION_INFO="%s"' % ver)
            opts.append(cpp_flag(self.compiler))
            if has_flag(self.compiler, '-fvisibility=hidden'):
                opts.append('-fvisibility=hidden')
        elif ct == 'msvc':
            opts.append('/DVERSION_INFO=\\"%s\\"' % ver)
        for ext in self.extensions:
            ext.extra_compile_args = opts
            ext.extra_link_args = link_opts
        build_ext.build_extensions(self)


class UpdateSubmodules(develop):
    def run(self):
        from os import path
        from subprocess import check_call
        if path.exists('.git'):
            check_call(['git', 'submodule', 'update', '--init', '--recursive'])
        develop.run(self)


setup(
    name='bacon',
    version=__version__,
    author='Alex Yu',
    author_email='sxyu@berkeley.edu',
    description='Bacon 2: Hog Contest Engine',
    long_description='',
    packages=['bacon'],
    ext_modules=EXT_MODULES,
    install_requires=['pybind11>=2.3'],
    setup_requires=['pybind11>=2.3'],
    cmdclass={
      'build_ext': BuildExt,
      'develop': UpdateSubmodules
    },
    zip_safe=False,
)

