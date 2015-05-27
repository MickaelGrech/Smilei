# ---------------------------------------------
# SIMULATION PARAMETERS FOR THE PIC-CODE SMILEI
# ---------------------------------------------
# Remember: never override the following names:
#           SmileiComponent, Species, Laser, Collisions, DiagProbe, DiagParticles,
#           DiagScalar, DiagPhase or ExtField

# sim_units: normalisation units for the input data
#            it is used only in the input data & log file
#            codes outputs are always in "normalised" units
#            'wavelength' : input data are in wavelength-related units
#            'normalized' : input data are put in code (relativistic) units
sim_units = "wavelength"
wavelength_SI = 1.e-6

# dim: Geometry of the simulation
#      1d3v = cartesian grid with 1d in space + 3d in velocity
#      2d3v = cartesian grid with 2d in space + 3d in velocity
#      3d3v = cartesian grid with 3d in space + 3d in velocity
#      2drz = cylindrical (r,z) grid with 3d3v particles
dim = "1d3v"

# order of interpolation
interpolation_order = 2

# SIMULATION TIME 
# set either the resolution (res_time) or the timestep
# res_time = integer, number of time-steps within one unit of time (`sim_units`)
# timestep = float, time step in units of `sim_units`
# sim_time = float, duration of the simulation  in units of `sim_units`
timestep = 0.005
sim_time  = 0.5

#  optional parameter time_fields_frozen, during which fields are not updated
time_fields_frozen = 100000000000.


# SIMULATION BOX : for all space directions (in 2D & 3D use vector of doubles)
# either use the resolution (res_space) or cell-length (cell_length)
# res_space   = list of integers, number of cells in one unit of space (`sim_units`)
# sim_length  = length of the simulation in units of `sim_units`
# cell_length = cell length  in units of `sim_units`
cell_length = [0.01]
sim_length  = [1.]

# ELECTROMAGNETIC BOUNDARY CONDITIONS
# bc_em_type_long/trans : boundary conditions used for EM fields 
#                         in the longitudinal or transverse directions
#                         periodic      = periodic BC (using MPI topology)
#                         silver-muller = injecting/absorbing
bc_em_type_long  = "periodic"


# RANDOM seed 
# this is used to randomize the random number generator
random_seed = 0


# DEFINE ALL SPECIES
#
# species_type      = string, given name to the species (e.g. ion, electron, positron, test ...)
# initPosition_type = string, "regular" or "random"
# initMomentum_type = string "cold", "maxwell-juettner" or "rectangular"
# n_part_per_cell   = integer, number of particles/cell
# c_part_max        = float, factor on the memory reserved for the total number of particles
# mass              = float, particle mass in units of the electron mass
# charge            = float, particle charge in units of the electron charge
# density           = float, species density in units of the "critical" density
# mean_velocity     = list of floats, mean velocity in units of the speed of light
# temperature       = list of floats, temperature in units of m_e c^2
# dynamics_type     = string, species type of dynamics = "norm" or "rrLL"
# time_frozen       = float, time during which particles are frozen in units of the normalization time
# radiating         = boolean, if true, incoherent radiation calculated using the Larmor formula 

Species(
	species_type = "ion1",
	initPosition_type = "random",
	initMomentum_type = "maxwell-juettner",
	vacuum_length   = [0.],
	dens_length_x   = [1000., 1000., 1000.],
	n_part_per_cell = 2000,
	c_part_max = 1.0,
	mass = 1836.0,
	charge = 1.0,
	density = 10.,
	mean_velocity = [0., 0., 0.],
	temperature = [0.00002],
	dynamics_type = "norm",
	time_frozen = 0.0,
	bc_part_type_west = "none",
	bc_part_type_east = "none"
)

Species(
	species_type = "electron1",
	initPosition_type = "random",
	initMomentum_type = "maxwell-juettner",
	vacuum_length   = [0.],
	dens_length_x   = [1000., 1000., 1000.],
	n_part_per_cell= 2000,
	c_part_max = 1.0,
	mass = 1.0,
	charge = -1.0,
	density = 10.,
	mean_velocity = [0.05, 0., 0.],
	temperature = [0.00002],
	dynamics_type = "norm",
	time_frozen = 0.0,
	bc_part_type_west = "none",
	bc_part_type_east = "none"
)

# ---------------------
# DIAGNOSTIC PARAMETERS
# ---------------------

# print_every (on screen text output) 
print_every = 10

# DIAGNOSTICS ON FIELDS
fieldDump_every    = 5
avgfieldDump_every = 5
ntime_step_avg     = 1

# DIAGNOSTICS ON SCALARS
# every = integer, number of time-steps between each output
DiagScalar(every = 1)

# PROBE DIAGNOSTICS - interpolate the fields on a N-D arbitrary grid
# ---------------------------------------------------------------------------------
# every        = an integer, number of time-steps between each output
# time_range   = two floats, optional, min and max times to output (all times if omitted)
# number       = N floats, optional, number of grid points in each dimension
# pos          = N floats, position of the reference point
# pos_first    = N floats, optional, position of the first point
# pos_second   = N floats, optional, position of the second point
DiagProbe(
	every = 1,
	pos = [1]
)

# DIAGNOSTICS ON PARTICLES - project the particles on a N-D arbitrary grid
# ------------------------------------------------------------------------
# output       = string: "density", "charge_density" or "current_density_[xyz]"
#                parameter that describes what quantity is obtained 
# every        = integer > 0: number of time-steps between each output
# time_average = integer > 0: number of time-steps to average
# species      = list of strings, one or several species whose data will be used
# axes         = list of axes
# Each axis is a list: (_type_ _min_ _max_ _nsteps_ ["logscale"] ["edge_inclusive"])
#   _type_ is a string, one of the following options:
#      x, y, z, px, py, pz, p, gamma, ekin, vx, vy, vz, v or charge
#   The data is discretized for _type_ between _min_ and _max_, in _nsteps_ bins
#   The optional "logscale" sets the scale to logarithmic
#   The optional "edge_inclusive" forces the particles that are outside (_min_,_max_)
#     to be counted in the extrema bins
#   Example : axes = ("x", 0, 1, 30)
#   Example : axes = ("px", -1, 1, 100, "edge_inclusive")

DiagParticles(
	output = "density",
	every = 4,
	time_average = 2,
	species = ["electron1"],
	axes = [
		["x", 0., 1., 100],
		["vx", -0.1, 0.1, 100]
	]
)

DiagParticles(
	output = "density",
	every = 4,
	time_average = 1,
	species = ["ion1"],
	axes = [
		("x", 0., 1., 100),
		("vx", -0.001, 0.001, 100)
	]
)

DiagParticles(
	output = "density",
	every = 4,
	time_average = 2,
	species = ["electron1"],
	axes = [
		["x", 0., 1., 100],
		["vx", -0.1, 0.1, 100]
	]
)

DiagParticles(
	output = "density",
	every = 1,
	time_average = 1,
	species = ["electron1"],
	axes = [
		["ekin", 0.0001, 0.1, 100, "logscale", "edge_inclusive"]
	]
)