# ---------------------------------------------
# SIMULATION PARAMETERS FOR THE PIC-CODE SMILEI
# ---------------------------------------------

import math
L0 = 2.*math.pi # conversion from normalization length to wavelength


Main(
    geometry = "1d3v",

    number_of_patches = [ 4 ],

    interpolation_order = 2,

    timestep = 1 * L0,
    sim_time = 400 * L0,


    time_fields_frozen = 100000000000.,

    cell_length = [5.*L0],
    sim_length = [160.*L0],

    bc_em_type_x = ["periodic"],


    random_seed = 0,

	referenceAngularFrequency_SI = L0 * 3e8 /1.e-6,
    print_every = 100,
)



Z = 30
A = 65.
density = 10.

electrons = []
ions = []
temperature = []
Tmin = 0.001 # keV
Tmax = 2.
npoints = 10

for i in range(npoints):
	eon = "electron"+str(i)
	ion = "ion"+str(i)
	electrons.append(eon)
	ions.append(ion)
	
	T = math.exp(math.log(Tmin) + float(i)/(npoints-1)*math.log(Tmax/Tmin)) #logscale
	T /= 511.
	temperature.append(T)
	
	Zstar = (Z)*(1.-math.exp(-T/(1./511.)))
	Zstar = round(Zstar, 2)
	
	Species(
		species_type = eon,
		initPosition_type = "regular",
		initMomentum_type = "maxwell-juettner",
		n_part_per_cell= 100,
		mass = 1.0,
		charge = -1.0,
		charge_density = Zstar*density,
		mean_velocity = [0., 0., 0.],
		temperature = [T]*3,
		time_frozen = 100000000.0,
		bc_part_type_west = "none",
		bc_part_type_east = "none",
		bc_part_type_south = "none",
		bc_part_type_north = "none",
		c_part_max = 10.
	)
	
	Species(
		species_type = ion,
		initPosition_type = "regular",
		initMomentum_type = "maxwell-juettner",
		n_part_per_cell= 100,
		mass = 1836.0*A,
		charge = Zstar,
		nb_density = 10.,
		mean_velocity = [0., 0., 0.],
		temperature = [T]*3,
		time_frozen = 100000000.0,
		bc_part_type_west = "none",
		bc_part_type_east = "none",
		bc_part_type_south = "none",
		bc_part_type_north = "none",
		atomic_number = Z
	)
	
	Collisions(
		species1 = [eon],
		species2 = [ion],
		coulomb_log = 0.,
		ionizing = True
	)
	
	DiagParticles(
		output = "ekin_density",
		every = 10,
		species = [eon],
		axes = [ ["x", 0, Main.sim_length[0], 1] ]
	)
	DiagParticles(
		output = "density",
		every = 10,
		species = [eon],
		axes = [ ["x", 0, Main.sim_length[0], 1] ]
	)
	DiagParticles(
		output = "charge_density",
		every = 10,
		species = [ion],
		axes = [ ["x", 0, Main.sim_length[0], 1] ]
	)
	DiagParticles(
		output = "density",
		every = 10,
		species = [ion],
		axes = [ ["x", 0, Main.sim_length[0], 1] ]
	)
	#DiagParticles(
	#	output = "density",
	#	every = 50,
	#	species = [ion],
	#	axes = [ ["charge", -0.5, Z+0.5, Z+1] ]
	#)




DiagFields(
	every = 1000000
)


DiagScalar(
	every = 10000000
)





