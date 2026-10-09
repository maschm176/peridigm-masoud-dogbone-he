#!/usr/bin/env python

# only use data before a certain strain
eng_strain_lim = 31.0 # in percent

nominal_area = 1.0

to_plot = False
# to_plot = True

import netCDF4
import numpy as np
nc = netCDF4.Dataset('Output/ExodusOutputLongitudinal.h')

time_steps = nc.variables['time_whole']

Bottom_Force = nc.variables['vals_glo_var'][:,1]

# Y-coordinates of the tracking nodes in top section
coordinates_top_node = np.zeros([len(time_steps), 5])
coordinates_top_node[:,0] = nc.variables['vals_glo_var'][:,4]
coordinates_top_node[:,1] = nc.variables['vals_glo_var'][:,7]
coordinates_top_node[:,2] = nc.variables['vals_glo_var'][:,10]
coordinates_top_node[:,3] = nc.variables['vals_glo_var'][:,13]
coordinates_top_node[:,4] = nc.variables['vals_glo_var'][:,16]

# Y-coordinates of the tracking nodes in bottom section
coordinates_bottom_node = np.zeros([len(time_steps), 5])
coordinates_bottom_node[:,0] = nc.variables['vals_glo_var'][:,19]
coordinates_bottom_node[:,1] = nc.variables['vals_glo_var'][:,22]
coordinates_bottom_node[:,2] = nc.variables['vals_glo_var'][:,25]
coordinates_bottom_node[:,3] = nc.variables['vals_glo_var'][:,28]
coordinates_bottom_node[:,4] = nc.variables['vals_glo_var'][:,31]

coordinates = np.mean(coordinates_top_node, axis=1) - np.mean(coordinates_bottom_node, axis=1)

eng_strain = 100.0 * (coordinates/coordinates[0] - 1.0) #(%)
eng_stress = 1.0e-3 * Bottom_Force/nominal_area #(MPa)

# only use values before a certain strain (before necking in the experimental data)
eng_stress = eng_stress[eng_strain < eng_strain_lim]
eng_strain = eng_strain[eng_strain < eng_strain_lim]

#Load calibration data 
eps_cal = np.loadtxt('calibration_strain_data_longitudinal.txt')
sigma_cal = np.loadtxt('calibration_stress_data_longitudinal.txt')

#deliberately shifting the calibration a little down 
# sigma_cal[14:18] -= 5.0;
# sigma_cal[19:] -= 11.0;

#Compute least-square error 
obj_fun = 0.0
count = 0

import bisect
#For each strain, the two calibration data points bounding it are found and the error is computed using a linear interpolation
for i in range(1, len(eng_strain)):
    idx = bisect.bisect(eps_cal, eng_strain[i])
    if(eng_strain[i] > 4.0e-3):
      if(idx < len(eps_cal)):
          n = (eng_strain[i]-eps_cal[idx-1]) / (eps_cal[idx] - eps_cal[idx-1])
          interpolated_sigma_cal = n*sigma_cal[idx] + (1.0-n)*sigma_cal[idx-1]
          err = eng_stress[i] - interpolated_sigma_cal
          obj_fun += abs(err)/interpolated_sigma_cal
          count += 1

scale = 1.0
obj_fun /= (count * scale)

# strainPenalty = 33.0
# stressPenalty = 530.0
# scalePenalty = 10.0
# idx = bisect.bisect(eng_strain, strainPenalty)
# if(eng_stress[idx-1] < stressPenalty):
  # obj_fun *= 1 + scalePenalty * (stressPenalty - eng_stress[idx-1]) / stressPenalty

with open("Objective_function_longitudinal.txt", "w") as text_file:
    text_file.write(str(obj_fun))

if(to_plot):
    import matplotlib as mpl
    mpl.use('Agg')
    import matplotlib.pyplot as plt

    plt.plot(eng_strain, eng_stress, label='simulation') 
    plt.plot(eps_cal, sigma_cal, label='lab data') 

    plt.xlabel(r'$\epsilon$(%)', fontsize=20)
    plt.ylabel(r'$\sigma$(MPa)', fontsize=20)
    plt.legend(loc=4)

    plt.tick_params(axis='x', labelsize=16)
    plt.tick_params(axis='y', labelsize=16)

    # plt.xlim(0.0, plt.xlim()[1])
    plt.savefig('plot_calibration_longitudinal.eps', format='eps', dpi=200, bbox_inches='tight')
  
