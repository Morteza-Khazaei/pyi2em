import pyi2em


def main():
    print("Backscatter test: VV/HH (dB)")

    # Example parameters
    fr = 3.0          # GHz
    el = 11.3         # real permittivity
    ei = 1.5          # imag permittivity
    l = 0.10          # correlation length [m]
    sig = 0.0025      # rms height [m]
    theta_d = 30.0    # incidence angle [deg]
    sp = 2            # 1=exponential, 2=Gaussian

    vv, hh = pyi2em.I2EM_backscatter(fr, sig, l, theta_d, el, ei, sp)
    print(f"sigma0: {vv} [VV], {hh} [HH]")


if __name__ == '__main__':
    main()

