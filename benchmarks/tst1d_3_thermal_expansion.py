# ----------------------------------------------------------------------------------------
#                     SIMULATION PARAMETERS FOR THE PIC-CODE SMILEI
# ----------------------------------------------------------------------------------------

import math

Te_keV = 1.              # electron temperature in keV
Te  = Te_keV/511.        # Te normalised in mec^2 (code units)
vth = math.sqrt(Te)      # normalised thermal velocity
Ld    = vth              # Debye length in normalised units
dx  = Ld/10.             # spatial resolution
Lsim = 100.*Ld           # simulation length
tsim = 50.               # duration of the simulation

mi = 100.0               # ion mass (use reduced one to accelerate computation)
cs = math.sqrt(Te/mi)    # ion acoustic velocity (normalised to c)
Uion = 5.*mi*cs*cs       # max. energy used to compute the ion spectrum


Main(
    geometry = "1d3v",
    
    interpolation_order = 2,
    
    timestep = 0.95*dx,
    sim_time = tsim,
    
    cell_length = [dx],
    sim_length  = [Lsim],
    
    number_of_patches = [ 8 ],
    
    bc_em_type_x = ['silver-muller','silver-muller'] ,
    
    random_seed = 0
)

Species(
    species_type = 'ion',
    initPosition_type = 'regular',
    initMomentum_type = 'mj',
    n_part_per_cell = 10,
    mass = mi, 
    charge = 1.0,
    nb_density = trapezoidal(1., xplateau=20.*Ld),
    temperature = [1.e-6],
	thermT = [1.e-6],
	thermVelocity = [0.,0.,0.],
    bc_part_type_west = 'thermalize',
    bc_part_type_east = 'refl'
)
Species(
    species_type = 'eon',
    initPosition_type = 'regular',
    initMomentum_type = 'maxwell-juettner',
    n_part_per_cell = 10,
    mass = 1.0,
    charge = -1.0,
    nb_density = trapezoidal(1., xplateau=20.*Ld),
    temperature = [Te],
    thermT = [Te],
    thermVelocity = [0.,0.,0.],
    bc_part_type_west = 'thermalize',
    bc_part_type_east = 'refl'
)



every=100

DiagScalar(every = every)#, vars=['Utot','Ubal_norm','Uelm','Ukin','Ukin_ion','Ukin_eon'])    


DiagFields(
    every = every,
    fields = ['Ex','Rho_ion','Rho_eon']
)

DiagParticles(
    output = "density",
    every = every,
    species = ["ion"],
    axes = [
        ["x", 0., Lsim, 50],
        ["px", -2.*cs, 3.*cs, 100]
    ]
)

DiagParticles(
    output = "density",
    every = every,
    time_average = 1,
    species = ["ion"],
    axes = [
        ["ekin", 0.0001, Uion, 100, "logscale", "edge_inclusive"]
    ]
)
