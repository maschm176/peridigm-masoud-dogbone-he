#!/usr/bin/env python3
"""Extract a nominal stress-strain curve from a cylinder_tensile_test run.

Usage: extract_stress_strain.py <exodus_file.e> <output.csv>
Runs inside the peridigm-masoud container (needs netCDF4/numpy, both baked in).
"""
import sys
import netCDF4
import numpy as np

HEIGHT = 0.1
OUTER_R, INNER_R = 0.025, 0.020
AREA = np.pi * (OUTER_R**2 - INNER_R**2)


def main():
    exo_file, out_csv = sys.argv[1], sys.argv[2]
    nc = netCDF4.Dataset(exo_file)
    names = [nc.variables['name_nod_var'][i].tobytes().decode().strip('\x00')
             for i in range(nc.dimensions['num_nod_var'].size)]
    force_z_idx = names.index('ForceZ') + 1
    disp_z_idx = names.index('DisplacementZ') + 1

    force_z = nc.variables[f'vals_nod_var{force_z_idx}'][:]
    disp_z = nc.variables[f'vals_nod_var{disp_z_idx}'][:]
    bottom_ids = nc.variables['node_ns1'][:] - 1
    top_ids = nc.variables['node_ns2'][:] - 1

    reaction = force_z[:, bottom_ids].sum(axis=1)
    strain = (disp_z[:, top_ids].mean(axis=1) - disp_z[:, bottom_ids].mean(axis=1)) / HEIGHT
    stress = np.abs(reaction) / AREA

    with open(out_csv, 'w') as f:
        f.write("step,strain,stress\n")
        for i in range(len(strain)):
            f.write(f"{i},{strain[i]:.8e},{stress[i]:.8e}\n")


if __name__ == "__main__":
    main()
