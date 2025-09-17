# pyi2em — I2EM model in Python (pybind11)

pyi2em provides fast Python bindings to a C++ implementation of the Integral
Equation Model (I2EM) for rough-surface microwave scattering and emission, as
described in:

Ulaby, F.T. and Long, D.G. (2014), Microwave Radar and Radiometric Remote
Sensing, The University of Michigan Press.

The extension exposes functions to compute:
- Emissivity (V/H) for a given frequency, roughness and permittivity
- Monostatic backscatter (sigma0) co-pol (VV/HH)
- Cross-pol (VH) via the IEMX component

Bindings are implemented with pybind11 + CMake and link to GSL.

## Installation

pyi2em is a Python extension built with CMake and pybind11 and links to the
[GNU Scientific Library (GSL)](https://www.gnu.org/software/gsl/). The build
expects an external pybind11 installation (pip/conda/apt) and will write the
compiled module in place into the repository root as an ABI‑tagged `.so` (e.g.
`pyi2em.cpython-311-x86_64-linux-gnu.so`).

### Prerequisites

- CMake ≥ 3.15
- A C++ compiler (gcc/clang or MSVC)
- GSL (libgsl + gslcblas)
- pybind11 (CMake package)

Install examples:

- Debian/Ubuntu
  - `sudo apt-get update`
  - `sudo apt-get install -y build-essential cmake libgsl-dev pybind11-dev`

- Conda (no sudo)
  - `conda install -c conda-forge cmake gsl pybind11`

- Pip (pybind11 only)
  - `pip install pybind11`
  - GSL still needs to come from your OS or conda.

### Build and install (venv recommended)

```bash
python -m venv .venv
. .venv/bin/activate  # or .venv\Scripts\activate on Windows

# Optional: help the build find system GSL explicitly
export GSL_LIBRARY=/usr/lib/x86_64-linux-gnu/libgsl.so
export GSL_LIBRARY_BLAS=/usr/lib/x86_64-linux-gnu/libgslcblas.so

# Build in place (writes the .so to repo root)
python setup.py build_ext -v

# or install (editable) if you prefer importing from site‑packages
pip install -e .
```

Verify import:

```bash
python - << 'PY'
import pyi2em
print('pyi2em:', pyi2em.__file__)
PY
```

If you work from a Jupyter notebook under `notebooks/`, add the
repo root to `sys.path` so imports pick up the freshly built module:

```python
import sys, os
sys.path.insert(0, os.path.abspath('..'))
import pyi2em
print(pyi2em.__file__)
```

### Notes

- External pybind11 is required. If CMake reports it cannot find `pybind11`,
  install it in your environment (pip/conda/apt) or set `pybind11_DIR` to the
  directory containing `pybind11Config.cmake`.
- GSL must be available at link time. On most Linux systems `libgsl-dev`
  suffices; otherwise export `GSL_LIBRARY` and `GSL_LIBRARY_BLAS` as shown.
- By default the build writes the compiled extension into the repo root, e.g.
  `pyi2em.cpython-311-x86_64-linux-gnu.so`. Importing works from the repo root
  or after `pip install -e .`.

## Quick start

### Emissivity (example)

```python
import pyi2em

fr = 3.0         # GHz
sig = 0.0025     # m (RMS height)
l = 0.10         # m (correlation length)
theta_d = 30.0   # deg (incidence)
el, ei = 11.3, 1.5   # relative permittivity (real, imag)
sp = 2           # correlation function: 1=exp, 2=gauss

ev, eh = pyi2em.I2EM(fr, sig, l, theta_d, el, ei, sp)
print('Emissivity V/H:', ev, eh)
```

### Backscatter (VV, HH)

```python
vv, hh = pyi2em.I2EM_backscatter(fr, sig, l, theta_d, el, ei, sp)
print('sigma0 VV/HH (dB):', vv, hh)
```

### Cross‑pol (VH) and full triplet

```python
# Cross-pol (VH) only
vh = pyi2em.I2EM_crosspol(fr, sig, l, theta_d, el, ei, sp)

# Or compute all three in one call
vv, hh, vh = pyi2em.I2EM_Backscatter_model(fr, sig, l, theta_d, el, ei, sp)
print('sigma0 VV/HH/VH (dB):', vv, hh, vh)
```

## Test
```bash
cd pyi2em
python3 test_i2em.py
```

## Usage

```python
import pyi2em

#set frequency in ghz
fr = 3.0

# set complex dielectric of the soil
el = 11.3
ei = 1.5

# set correlation length [m]
l = 0.10  # 10 cm

# set standard deviation of the surface height variation (rms) [m]
sig = 0.0025  # .25 cm

# set incidence angle [deg]
theta_d = 30.0

# set type of surface correlation function (1) exponential (2) Gaussian
sp = 2

e_ = pyi2em.I2EM(fr, sig, l, theta_d, el, ei, sp)

print("FREQ: 3.0 [GHZ], CDC: 11.3 + i1.5, CL: 10 [cm], RMS: .25 [cm], INC: 30 [deg], CORRF: Gaussian")

print("EMISSIVITY: {:g} [V], {:g} [H]".format(*e_))
```

## Cross‑pol tuning (advanced)

The cross‑polarized backscatter (VH) integrator and series can be tuned at
runtime to improve accuracy or speed:

```python
import pyi2em

# Include more series terms (smaller => more terms)
pyi2em.set_xpol_auto_eps(1e-9)

# Increase cubature effort for VH double integral
pyi2em.set_xpol_integrator(maxeval=30000, reltol=5e-7, abstol=0.0)

# Optional: apply a power‑domain scale to VH only (to match a reference LUT)
pyi2em.set_xpol_vh_scale(10**(3.5/10))  # add ~+3.5 dB
```

## Troubleshooting

- CMake cannot find pybind11:
  - Install `pybind11` (pip/conda/apt) or set `-Dpybind11_DIR=$(python -m pybind11 --cmakedir)`.
- Linker errors for GSL:
  - Install `libgsl-dev` (or `conda install gsl`) and/or export `GSL_LIBRARY` and `GSL_LIBRARY_BLAS`.
- ImportError in notebooks under `notebooks/`:
  - Add `sys.path.insert(0, os.path.abspath('..'))` before `import pyi2em` to import from the repo root.
```
FREQ: 3.0 [GHZ], CDC: 11.3 + i1.5, CL: 10 [cm], RMS: .25 [cm], INC: 30 [deg], CORRF: Gaussian
Emissivity: 0.646743 [V], 0.75031 [H]
```
