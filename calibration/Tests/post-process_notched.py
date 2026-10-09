#!/usr/bin/env python

# only use data before a certain displacement
disp_lim = 0.48 

to_plot = False
# to_plot = True

import netCDF4
import numpy as np
nc = netCDF4.Dataset('Output/ExodusOutputNotched.h')

time_steps = nc.variables['time_whole']

Bottom_Force = nc.variables['vals_glo_var'][:,1]

# Y-displacement of the tracking nodes in top right section
displacement_top_node = np.zeros([len(time_steps), 10])
displacement_top_node[:,0] = nc.variables['vals_glo_var'][:,4]
displacement_top_node[:,1] = nc.variables['vals_glo_var'][:,7]
displacement_top_node[:,2] = nc.variables['vals_glo_var'][:,10]
displacement_top_node[:,3] = nc.variables['vals_glo_var'][:,13]
displacement_top_node[:,4] = nc.variables['vals_glo_var'][:,16]

# Y-displacement of the tracking nodes in bottom right section
displacement_bottom_node = np.zeros([len(time_steps), 10])
displacement_bottom_node[:,0] = nc.variables['vals_glo_var'][:,19]
displacement_bottom_node[:,1] = nc.variables['vals_glo_var'][:,22]
displacement_bottom_node[:,2] = nc.variables['vals_glo_var'][:,25]
displacement_bottom_node[:,3] = nc.variables['vals_glo_var'][:,28]
displacement_bottom_node[:,4] = nc.variables['vals_glo_var'][:,31]

# Y-displacement of the tracking nodes in top left section
displacement_top_node[:,5] = nc.variables['vals_glo_var'][:,34]
displacement_top_node[:,6] = nc.variables['vals_glo_var'][:,37]
displacement_top_node[:,7] = nc.variables['vals_glo_var'][:,40]
displacement_top_node[:,8] = nc.variables['vals_glo_var'][:,43]
displacement_top_node[:,9] = nc.variables['vals_glo_var'][:,46]

# Y-displacement of the tracking nodes in bottom left section
displacement_bottom_node[:,5] = nc.variables['vals_glo_var'][:,49]
displacement_bottom_node[:,6] = nc.variables['vals_glo_var'][:,52]
displacement_bottom_node[:,7] = nc.variables['vals_glo_var'][:,55]
displacement_bottom_node[:,8] = nc.variables['vals_glo_var'][:,58]
displacement_bottom_node[:,9] = nc.variables['vals_glo_var'][:,61]

displacement = np.mean(displacement_top_node, axis=1) - np.mean(displacement_bottom_node, axis=1)

disp = displacement #(mm)
force = 1.0e-3 * Bottom_Force #(N)

# only use values before a certain strain (before necking in the experimental data)
force = force[disp < disp_lim]
disp = disp[disp < disp_lim]

#Load calibration data 
disp_cal = np.loadtxt('calibration_displacement_data_notched.txt')
force_cal = np.loadtxt('calibration_force_data_notched.txt')

#Compute least-square error 
obj_fun = 0.0
count = 0

import bisect
#For each strain, the two calibration data points bounding it are found and the error is computed using a linear interpolation
for i in range(1, len(disp)):
    if(disp[i] > 1.0e-4):
      idx = bisect.bisect(disp_cal, disp[i])
      if(idx < len(disp_cal)):
          n = (disp[i]-disp_cal[idx-1]) / (disp_cal[idx] - disp_cal[idx-1])
          interpolated_force_cal = n*force_cal[idx] + (1.0-n)*force_cal[idx-1]
          err = force[i] - interpolated_force_cal
          obj_fun += abs(err) / interpolated_force_cal
          count += 1

scale = 1.0
obj_fun /= (count * scale)

with open("Objective_function_notched.txt", "w") as text_file:
    text_file.write(str(obj_fun))

if(to_plot):
    import matplotlib as mpl
    mpl.use('Agg')
    import matplotlib.pyplot as plt

    plt.plot(disp, force, label='simulation') 
    plt.plot(disp_cal, force_cal, label='lab data') 

    plt.xlabel(r'Displacement (mm)', fontsize=20)
    plt.ylabel(r'Force (N)', fontsize=20)
    plt.legend(loc=4)

    plt.tick_params(axis='x', labelsize=16)
    plt.tick_params(axis='y', labelsize=16)

    # plt.xlim(0.0, plt.xlim()[1])
    plt.savefig('plot_calibration_notched.eps', format='eps', dpi=200, bbox_inches='tight')

