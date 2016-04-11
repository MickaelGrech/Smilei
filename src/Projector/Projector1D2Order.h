#ifndef PROJECTOR1D2ORDER_H
#define PROJECTOR1D2ORDER_H

#include "Projector1D.h"

class Projector1D2Order : public Projector1D {
public:
    Projector1D2Order(Params&, Patch* patch);
    ~Projector1D2Order();

    //! Project global current densities (EMfields->Jx_/Jy_/Jz_)
    //! Not used for now
    void operator() (ElectroMagn* EMfields, Particles &particles, int ipart, double gf);

    // Projection by species, in Species::dynamics, ElectroMagn::initRhoJ
    void operator() (Field* Jx, Field* Jy, Field* Jz, Field* rho, Particles &particles, int ipart, double gf);

    //! Project global current charge (EMfields->rho_)
    //! Used in Species::dynamics if time_frozen
    void operator() (Field* rho, Particles &particles, int ipart);
    void operator() (double* rho, Particles &particles, unsigned int ipart, unsigned int bin, unsigned int b_dim0);

     //! Project local current densities if particles sorting activated in Species::dynamics
    void operator() (double* Jx, double* Jy, double* Jz, double* rho, Particles &particles, unsigned int ipart, double gf, unsigned int bin, unsigned int b_dim0, int* iold, double* delta);
    void operator() (double* Jx, double* Jy, double* Jz, Particles &particles, unsigned int ipart, double gf, unsigned int bin, unsigned int b_dim0, int* iold, double* delta);

    //! Project global current densities if Ionization in Species::dynamics,
    void operator() (Field* Jx, Field* Jy, Field* Jz, Particles &particles, int ipart, LocalFields Jion);

    //!Wrapper
    void operator() (ElectroMagn* EMfields, Particles &particles, SmileiMPI* smpi, int istart, int iend, int ithread, int ibin, int clrw, int diag_flag, int b_lastdim, int ispec);
private:
    double dx_ov_dt;
};

#endif

