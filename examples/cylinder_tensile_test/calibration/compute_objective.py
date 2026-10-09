#!/usr/bin/env python3
"""Compare a simulated stress-strain curve to the synthetic calibration data
and write Dakota's single-objective results file.

Usage: compute_objective.py <sim_curve.csv> <calibration_dir> <results_file>

The strain trajectory is prescribed (Dirichlet BC), so it's ~identical across
parameter sets -- comparing stress at the same output-frame indices used to
build the calibration data is valid without interpolation.
"""
import csv
import sys
import numpy as np


def main():
    sim_csv, cal_dir, results_file = sys.argv[1], sys.argv[2], sys.argv[3]

    with open(sim_csv) as f:
        rows = list(csv.DictReader(f))
    sim_stress = np.array([float(r["stress"]) for r in rows])

    cal_stress = np.loadtxt(f"{cal_dir}/calibration_stress_data.txt")

    idx = np.linspace(8, len(sim_stress) - 1, len(cal_stress)).astype(int)
    sim_at_cal_points = sim_stress[idx]

    relative_error = np.abs(sim_at_cal_points - cal_stress) / cal_stress
    objective = relative_error.mean()

    with open(results_file, "w") as f:
        f.write(f"{objective:.15e}\n")


if __name__ == "__main__":
    main()
