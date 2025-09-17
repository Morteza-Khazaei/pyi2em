import os
import re
import sys
import platform
import subprocess

from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext
from distutils.version import LooseVersion


class CMakeExtension(Extension):
    def __init__(self, name, sourcedir=''):
        Extension.__init__(self, name, sources=[])
        self.sourcedir = os.path.abspath(sourcedir)


class CMakeBuild(build_ext):
    def run(self):
        try:
            out = subprocess.check_output(['cmake', '--version'])
        except OSError:
            raise RuntimeError("CMake must be installed to build the following extensions: " +
                               ", ".join(e.name for e in self.extensions))

        if platform.system() == "Windows":
            cmake_version = LooseVersion(re.search(r'version\s*([\d.]+)', out.decode()).group(1))
            if cmake_version < '3.1.0':
                raise RuntimeError("CMake >= 3.1.0 is required on Windows")

        for ext in self.extensions:
            self.build_extension(ext)

    def build_extension(self, ext):
        # Where CMake should place the built extension
        # Always build the extension in-place (repo root) so imports work
        # without relying on site-packages or post-build copying.
        project_root = os.path.abspath(os.path.dirname(__file__))
        cmake_args = ['-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=' + project_root,
                      '-DPYTHON_EXECUTABLE=' + sys.executable]
        
        # If user provides GSL paths, pass them explicitly to CMake to aid discovery
        gsl_lib = os.environ.get('GSL_LIBRARY')
        gsl_blas = os.environ.get('GSL_LIBRARY_BLAS')
        if gsl_lib:
            cmake_args.append('-DGSL_LIBRARY=' + gsl_lib)
        if gsl_blas:
            cmake_args.append('-DGSL_LIBRARY_BLAS=' + gsl_blas)

        cfg = 'Debug' if self.debug else 'Release'
        build_args = ['--config', cfg]

        if platform.system() == "Windows":
            # Ensure config-specific output also lands in project root
            cmake_args += ['-DCMAKE_LIBRARY_OUTPUT_DIRECTORY_{}={}'.format(cfg.upper(), project_root)]
            if sys.maxsize > 2**32:
                cmake_args += ['-A', 'x64']
            build_args += ['--', '/m']
        else:
            cmake_args += ['-DCMAKE_BUILD_TYPE=' + cfg]
            build_args += ['--', '-j2']

        env = os.environ.copy()
        env['CXXFLAGS'] = '{} -DVERSION_INFO=\\"{}\\"'.format(env.get('CXXFLAGS', ''),
                                                              self.distribution.get_version())
        # Help CMake find external pybind11 if installed via pip/conda
        try:
            import pybind11
            try:
                cmake_dir = pybind11.get_cmake_dir()
            except AttributeError:
                cmake_dir = subprocess.check_output([sys.executable, '-m', 'pybind11', '--cmakedir']).decode().strip()
            if cmake_dir:
                cmake_args.append('-Dpybind11_DIR=' + cmake_dir)
        except Exception:
            # If pybind11 is not importable, rely on system CMake/package paths
            pass
        if not os.path.exists(self.build_temp):
            os.makedirs(self.build_temp)
        subprocess.check_call(['cmake', ext.sourcedir] + cmake_args, cwd=self.build_temp, env=env)
        subprocess.check_call(['cmake', '--build', '.'] + build_args, cwd=self.build_temp)

        # Normalize artifact name in-place: some old pybind11/CMake combos
        # emit a non-standard filename like 'pyi2emNone'. Rename it to the
        # expected ABI-tagged filename so Python can import it.
        try:
            import sysconfig
            # Derive expected filename using Python's EXT_SUFFIX
            suffix = sysconfig.get_config_var('EXT_SUFFIX') or '.so'
            expected_name = ext.name + suffix  # e.g., pyi2em.cpython-311-...so
            expected_path = os.path.join(project_root, expected_name)
            if not os.path.exists(expected_path):
                # find any candidate produced in project_root
                for fname in os.listdir(project_root):
                    if fname.startswith(ext.name):
                        src = os.path.join(project_root, fname)
                        # rename first candidate to the expected path
                        try:
                            os.replace(src, expected_path)
                            break
                        except Exception:
                            pass
                # Fallback: some generators emit exactly 'pyi2emNone'
                if not os.path.exists(expected_path):
                    cand = os.path.join(project_root, ext.name + 'None')
                    if os.path.isfile(cand):
                        import shutil
                        shutil.copy2(cand, expected_path)
                        try:
                            os.remove(cand)
                        except Exception:
                            pass
        except Exception:
            pass

def _read_requirements():
    req_path = os.path.join(os.path.abspath(os.path.dirname(__file__)), 'requirements.txt')
    if os.path.exists(req_path):
        with open(req_path, 'r', encoding='utf-8') as f:
            reqs = [ln.strip() for ln in f if ln.strip() and not ln.strip().startswith('#')]
        return reqs
    return []

setup(
    name='pyi2em',
    version='0.0.1',
    author='Morteza Khazaei',
    author_email='mortezakhazaei1370@gmail.com',
    description='A I2EM library using pybind11 and CMake',
    long_description='',
    ext_modules=[CMakeExtension('pyi2em')],
    cmdclass=dict(build_ext=CMakeBuild),
    install_requires=_read_requirements(),
    zip_safe=False,
)
