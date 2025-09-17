#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "em_i2em.h"
#include <vector>


std::vector<double> I2EM(double fr, double sig, double l, double theta_d, double el, double ei, double sp){
    /**
        Emissivity from I2EM model
        
        Ulaby, F.T. and Long D.G.(2014), Microwave Radar and Radiometric Remote Sensing, The University of Michigan Press
        
        fr - frequency [ghz], double

        sig - standard deviation of the surface height variation (rms) [m], double

        l - correlation length [m], double

        theta_d - incidence angle [deg], double

        el - real relative permittivity (complex dielectric constant) of the surface, double
        
        ei - imaginary relative permittivity (complex dielectric constant) of the surface, double
    
        sp - type of surface correlation function (1) exponential (2) Gaussian, integer
     */

    cdouble er;
    er = cdouble(el, ei);

    std::vector<double> e(2);

    Calc_emissivity_I2EMmodel(fr, sig, l, theta_d, er, sp, &e[0], &e[1]);

    return e;
};

std::vector<double> I2EM_backscatter(double fr, double sig, double l, double theta_d, double el, double ei, int sp, float xcoeff = 1.0f){
    // Backscattering (monostatic): th_inc = th_scat = theta_d, ph_scat = 180 deg
    cdouble er(el, ei);
    double sigma0_hh = 0.0, sigma0_vv = 0.0;
    double thi = theta_d * DEG2RAD;
    double ths = thi;
    double phs = 180.0 * DEG2RAD;
    I2EM_Bistat_model((float)fr, (float)sig, (float)l, thi, ths, phs, er, sp, xcoeff, &sigma0_hh, &sigma0_vv);
    std::vector<double> s(2);
    // Return order: VV, HH (in dB)
    s[0] = sigma0_vv;
    s[1] = sigma0_hh;
    return s;
}

double I2EM_crosspol(double fr, double sig, double l, double theta_d,
                     double el, double ei, int sp, double xcoeff = 1.0,
                     int auto_select = 0) {
    // Cross-pol (VH/HV) sigma0 (dB) using IEMX_model, monostatic geometry
    cdouble er(el, ei);
    double sigma0_vh = 0.0;
    double thi = theta_d * DEG2RAD;
    IEMX_model((float)fr, (float)sig, (float)l, thi, er, sp, (float)xcoeff, auto_select, &sigma0_vh);
    return sigma0_vh; // dB
}

std::vector<double> test_I2EM(){
    // set frequency in ghz
    double fr = 3.0;

    double el = 11.3;
    double ei = 1.5;

    // set correlation length [m]
    double l = 0.10; // 10 cm

    // set standard deviation of the surface height variation (rms) [m]
    double sig = 0.0025; // .25 cm

    // incidence angle [deg]
    double theta_d = 30.0;

    // type of surface correlation function (1) exponential (2) Gaussian
    int sp = 2;

    vector<double> e;

    e = I2EM(fr, sig, l, theta_d, el, ei, sp);
    
    return e;
}

std::vector<double> I2EM_Backscatter_model(double fr, double sig, double l, double theta_d,
                                           double el, double ei, int sp,
                                           double xcoeff = 1.0, int auto_select = 1) {
    // Combined co-pol and cross-pol, following MATLAB I2EM_Backscatter_model
    auto co = I2EM_backscatter(fr, sig, l, theta_d, el, ei, sp, (float)xcoeff);
    double vh = I2EM_crosspol(fr, sig, l, theta_d, el, ei, sp, xcoeff, auto_select);
    return {co[0], co[1], vh};
}

namespace py = pybind11;

PYBIND11_MODULE(pyi2em, m) {
    m.doc() = "I2EM library";
    m.def("I2EM", &I2EM, R"pbdoc(
    
        Emissivity from I2EM model

        Ulaby, F.T. and Long D.G.(2014), Microwave Radar and Radiometric Remote Sensing, The University of Michigan Press
        
        usage: I2EM(fr, sig, l, theta_d, el, ei, sp)

        fr - frequency [ghz], double

        sig - standard deviation of the surface height variation (rms) [m], double

        l - correlation length [m], double

        theta_d - incidence angle [deg], double

        el - real relative permittivity (complex dielectric constant) of the surface, double
        
        ei - imaginary relative permittivity (complex dielectric constant) of the surface, double
    
        sp - type of surface correlation function (1) exponential (2) Gaussian, integer

    )pbdoc");

    m.def("test_I2EM", &test_I2EM, "test I2EM - pybind11");

    m.def("I2EM_backscatter", &I2EM_backscatter,
          py::arg("fr"), py::arg("sig"), py::arg("l"), py::arg("theta_d"),
          py::arg("el"), py::arg("ei"), py::arg("sp"), py::arg("xcoeff") = 1.0f,
          R"pbdoc(

        Backscattering sigma0 (VV, HH) in dB using I2EM, monostatic case.

        usage: I2EM_backscatter(fr, sig, l, theta_d, el, ei, sp, xcoeff=1.0)

        fr - frequency [GHz], double
        sig - surface rms height [m], double
        l - correlation length [m], double
        theta_d - incidence angle [deg], double
        el - real part of permittivity, double
        ei - imaginary part of permittivity, double
        sp - correlation function: 1=exponential, 2=Gaussian, 3=power-law, 4=x-exponential
        xcoeff - exponent parameter for 3/4 (ignored for 1/2)

        returns: [sigma0_vv_dB, sigma0_hh_dB]

    )pbdoc");

    m.def("I2EM_crosspol", &I2EM_crosspol,
          py::arg("fr"), py::arg("sig"), py::arg("l"), py::arg("theta_d"),
          py::arg("el"), py::arg("ei"), py::arg("sp"),
          py::arg("xcoeff") = 1.0, py::arg("auto_select") = 1,
          R"pbdoc(

        Cross-polarized backscattering sigma0 (VH) in dB using I2EM cross-pol model (IEMX).

        usage: I2EM_crosspol(fr, sig, l, theta_d, el, ei, sp, xcoeff=1.0, auto_select=0)

        fr - frequency [GHz]
        sig - surface rms height [m]
        l - correlation length [m]
        theta_d - incidence angle [deg]
        el, ei - real/imag parts of relative permittivity
        sp - correlation: 1=exponential, 2=Gaussian, 3=power-law, 4=x-exponential
        xcoeff - exponent parameter for power-law/x-exponential spectra (ignored for 1/2)
        auto_select - 0: fixed spectral terms, 1: auto-select terms

        returns: sigma0_vh_dB

    )pbdoc");

    m.def("I2EM_Backscatter_model", &I2EM_Backscatter_model,
          py::arg("fr"), py::arg("sig"), py::arg("l"), py::arg("theta_d"),
          py::arg("el"), py::arg("ei"), py::arg("sp"),
          py::arg("xcoeff") = 1.0, py::arg("auto_select") = 1,
          R"pbdoc(

        Full I2EM backscatter: returns [VV_dB, HH_dB, VH_dB].

        Mirrors the MATLAB I2EM_Backscatter_model: uses I2EM_Bistat_model for co-pol and
        IEMX_model for cross-pol. Monostatic geometry (backscatter).

        usage: I2EM_Backscatter_model(fr, sig, l, theta_d, el, ei, sp, xcoeff=1.0, auto_select=1)

    )pbdoc");

    // Cross-pol tuning knobs
    m.def("set_xpol_integrator", &set_xpol_integrator,
          py::arg("maxeval") = -1, py::arg("reltol") = -1.0, py::arg("abstol") = -1.0,
          R"pbdoc(
        Set cross-pol integrator settings (pcubature):
        - maxeval: total evaluations (leave negative to keep current)
        - reltol: relative tolerance (<=0 to keep current)
        - abstol: absolute tolerance (negative to keep current)
    )pbdoc");
    m.def("set_xpol_auto_eps", &set_xpol_auto_eps, py::arg("eps") = 1.0e-8,
          R"pbdoc(
        Set spectral auto-selection threshold for cross-pol series (default 1e-8).
        Larger eps selects fewer terms; smaller eps increases terms.
    )pbdoc");
    m.def("set_xpol_vh_scale", &set_xpol_vh_scale, py::arg("scale_power") = 1.0,
          R"pbdoc(
        Optional power-domain scale factor for cross-pol (VH) to account for
        small systematic normalization differences with external references.
        Applied as: sigma0_vh_dB = 10*log10(scale_power * sigma0_vh_linear).
    )pbdoc");
}
