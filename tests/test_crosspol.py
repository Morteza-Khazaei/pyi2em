import pyi2em


def main():
    print("Cross-pol (VH) backscatter test [dB]")

    fr = 3.0          # GHz
    el = 11.3         # real permittivity
    ei = 1.5          # imag permittivity
    l = 0.10          # correlation length [m]
    sig = 0.0025      # rms height [m]
    theta_d = 30.0    # incidence angle [deg]
    sp = 2            # 1=exponential, 2=Gaussian

    vh = pyi2em.I2EM_crosspol(fr, sig, l, theta_d, el, ei, sp)
    print(f"sigma0_vh: {vh} dB")


if __name__ == '__main__':
    main()

