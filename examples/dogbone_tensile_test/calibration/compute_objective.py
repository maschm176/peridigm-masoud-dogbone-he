#!/usr/bin/env python3
"""Compare a simulated stress-strain curve to the real dogbone calibration
data and write Dakota's single-objective results file.

Usage: compute_objective.py <sim_curve.csv> <fracture_strain.txt> <calibration_dir> <results_file> [concentration]

`concentration` selects which real data files to compare against
(calibration_{stress,strain}_data_HE_<concentration>.txt) and defaults to "0".

The real data's strain sample points are irregular experimental values, not
simulation output steps, so the simulated curve is interpolated onto the
real strain grid before comparing (rather than comparing by output-frame
index, which only works when both curves share the same strain sampling).

Stress is compared only up to the simulation's own predicted fracture
strain (from fracture_strain.txt, written by extract_stress_strain.py --
the strain at which damage exceeds 0.5 across an entire cross-section
slice, i.e. genuine crack traversal, not a hand-picked crop). Beyond that
point the two halves are mechanically separated and stress readings are
not physically meaningful. If the simulation never reaches that state
within its strain range, the full real curve is used as a fallback and a
large fracture-strain penalty is added, pushing the optimizer toward
parameters that actually produce a complete fracture.

The objective is the mean relative stress error (over the compared range)
plus the relative error between predicted and real fracture strain, so the
optimizer is directly rewarded for getting the failure point right, not
just the curve shape leading up to it.

If the STRESS_ONLY env var is set, the fracture-strain term is dropped
entirely and stress is compared over the full real curve regardless of
sim_fracture_strain. This is for stages that deliberately disable the
damage model (e.g. an isolated hardening-law fit): extract_stress_strain.py
detects "fracture" from any sharp force drop, which post-peak geometric
necking can trigger on its own with zero bond damage. Without STRESS_ONLY,
the optimizer would be silently rewarded for warping the hardening curve to
make necking-driven softening arrive at the right strain -- a job that
belongs to the (currently disabled) damage model, not the hardening law.

Real data points below NOISE_FLOOR_FRACTION of the curve's peak stress are
excluded from the relative-error mask (in addition to non-positive ones).
Without this, a near-zero-but-technically-positive point right at the start
of loading (sensor noise, not signal) makes relative_error's denominator
blow up -- e.g. HE_30's real data includes a 1e-6 MPa point that turned a
mean relative error of a few percent into millions.

The raw simulated curve carries dynamic/inertial ringing from explicit-
dynamics loading (most visible at low strain, before plasticity damps it
out) that isn't part of the true quasi-static material response. A
Savitzky-Golay filter removes that noise before comparing against the real
data.
"""
import csv
import os
import sys
import numpy as np
from scipy.signal import savgol_filter

NO_FRACTURE_PENALTY = 1.0
NOISE_FLOOR_FRACTION = 0.01
STRESS_ONLY = os.environ.get("STRESS_ONLY") == "1"


def main():
    sim_csv, fracture_txt, cal_dir, results_file = sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4]
    concentration = sys.argv[5] if len(sys.argv) > 5 else "0"

    with open(sim_csv) as f:
        rows = list(csv.DictReader(f))
    sim_strain = np.array([float(r["strain"]) for r in rows])
    sim_stress = np.array([float(r["stress"]) for r in rows])

    sim_fracture_strain = np.loadtxt(fracture_txt)

    real_strain = np.loadtxt(f"{cal_dir}/calibration_strain_data_HE_{concentration}.txt")
    real_stress = np.loadtxt(f"{cal_dir}/calibration_stress_data_HE_{concentration}.txt")
    real_fracture_strain = real_strain.max()

    order = np.argsort(sim_strain)
    sim_strain, sim_stress = sim_strain[order], sim_stress[order]
    sim_stress = savgol_filter(sim_stress, window_length=15, polyorder=3)
    sim_at_real_points = np.interp(real_strain, sim_strain, sim_stress)

    if STRESS_ONLY:
        compare_limit = real_fracture_strain
        fracture_error = 0.0
    elif sim_fracture_strain >= 0:
        compare_limit = min(sim_fracture_strain, real_fracture_strain)
        fracture_error = abs(sim_fracture_strain - real_fracture_strain) / real_fracture_strain
    else:
        compare_limit = real_fracture_strain
        fracture_error = NO_FRACTURE_PENALTY

    noise_floor = NOISE_FLOOR_FRACTION * real_stress.max()
    mask = (real_stress > noise_floor) & (real_strain <= compare_limit)
    relative_error = np.abs(sim_at_real_points[mask] - real_stress[mask]) / real_stress[mask]
    stress_error = relative_error.mean() if mask.any() else NO_FRACTURE_PENALTY

    objective = stress_error + fracture_error

    with open(results_file, "w") as f:
        f.write(f"{objective:.15e}\n")


if __name__ == "__main__":
    main()
