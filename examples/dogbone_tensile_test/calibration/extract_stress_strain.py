#!/usr/bin/env python3
"""Extract a nominal engineering stress-strain curve, plus the simulation's
own predicted fracture strain, from a dogbone_tensile_test calibration run.

Usage: extract_stress_strain.py <exodus_file.e> <output_csv> <fracture_txt>
Runs inside the peridigm-masoud container (needs netCDF4/numpy, both baked
in). Global variables (gage displacement, reaction force) are replicated
across all MPI ranks, so only rank 0's file is needed.

Fracture strain is detected from reaction force, not from spatially
locating a fully-damaged cross-section. An earlier per-element approach
(requiring damage > 0.5 across some fraction of a Y-slice) was tried and
abandoned: repeated tests showed genuine, dramatic softening (stress
crashing to ~15-20% of peak) without ever satisfying that spatial
criterion, most likely because the actual crack surface is rough/patchy
rather than a clean flat plane, so damage doesn't fill any single Y-bin
uniformly even once the specimen has essentially lost its load-bearing
capacity. Reaction force integrates the true load-bearing capacity across
the whole cross-section regardless of how the crack is shaped, so it's a
more robust proxy.

A "drops below X% of peak" rule was tried first and worked, but it's a
lagging indicator -- by the time stress decays to a low absolute level,
the specimen may have been visibly failing for a while. A real fracture
is a sudden, sharp discontinuity, not a slow fade, so fracture strain is
instead detected as the first post-peak point where the smoothed stress
drops SINGLE_STEP_DROP_FRACTION in one step, or CONSEC_DROP_FRACTION for
CONSEC_STEPS consecutive steps. Verified against real step-to-step data:
on the smoothed curve restricted to post-peak, nearly every reasonable
threshold in this range (single-step 3-10%, 2-3 consecutive steps at
3-5% each) converges on the same tight ~11.4-11.75% strain window on a
representative run, while looser thresholds start missing the real event
and catching later noise-driven spikes instead -- so the detected point
is a robust feature of the data, not sensitive to the exact percentages
chosen. Un-smoothed, single-step force data is far too noisy for this
(swings of tens to hundreds of percent even well past any real fracture),
which is why smoothing is applied first.
"""
import sys
import netCDF4
import numpy as np

GAGE_LENGTH0 = 2 * 1.238            # cm, distance between gage points at t=0
GAUGE_WIDTH = 0.635                 # cm
GAUGE_THICKNESS = 0.31496           # cm
GAUGE_AREA = GAUGE_WIDTH * GAUGE_THICKNESS   # cm^2
DYNE_CM2_PER_MPA = 1.0e7

SMOOTH_WINDOW = 9
SINGLE_STEP_DROP_FRACTION = 0.10
CONSEC_DROP_FRACTION = 0.05
CONSEC_STEPS = 2


def moving_average(x, window):
    kernel = np.ones(window) / window
    padded = np.pad(x, (window // 2, window // 2), mode='edge')
    return np.convolve(padded, kernel, mode='valid')[:len(x)]


def main():
    exo_file, out_csv, out_fracture = sys.argv[1], sys.argv[2], sys.argv[3]

    nc = netCDF4.Dataset(exo_file)
    names = [nc.variables['name_glo_var'][i].tobytes().decode().strip('\x00')
              for i in range(nc.dimensions['num_glo_var'].size)]
    idx = {n: i for i, n in enumerate(names)}
    glo = nc.variables['vals_glo_var'][:]

    gage_top_y = glo[:, idx['Gage_Top_DisplacementY']]
    gage_bottom_y = glo[:, idx['Gage_Bottom_DisplacementY']]
    strain_pct = (gage_top_y - gage_bottom_y) / GAGE_LENGTH0 * 100.0

    top_force_y = glo[:, idx['Top_Reaction_ForceY']]
    bottom_force_y = glo[:, idx['Bottom_Reaction_ForceY']]
    force_dyne = 0.5 * (np.abs(top_force_y) + np.abs(bottom_force_y))
    stress_MPa = (force_dyne / GAUGE_AREA) / DYNE_CM2_PER_MPA

    with open(out_csv, 'w') as f:
        f.write("strain,stress\n")
        for i in range(len(strain_pct)):
            f.write(f"{strain_pct[i]:.8e},{stress_MPa[i]:.8e}\n")

    smoothed = moving_average(stress_MPa, SMOOTH_WINDOW)
    peak_idx = int(np.argmax(smoothed))
    post_strain = strain_pct[peak_idx:]
    post_smoothed = smoothed[peak_idx:]
    pct_drop = -(post_smoothed[1:] - post_smoothed[:-1]) / post_smoothed[:-1]

    fracture_strain = -1.0
    single_step = pct_drop > SINGLE_STEP_DROP_FRACTION
    consec = np.zeros_like(pct_drop, dtype=bool)
    above_consec = pct_drop > CONSEC_DROP_FRACTION
    for i in range(len(above_consec) - CONSEC_STEPS + 1):
        if above_consec[i:i + CONSEC_STEPS].all():
            consec[i + CONSEC_STEPS - 1] = True
    triggered = single_step | consec
    hits = np.where(triggered)[0]
    if hits.size > 0:
        fracture_strain = float(post_strain[hits[0] + 1])

    with open(out_fracture, 'w') as f:
        f.write(f"{fracture_strain:.8e}\n")


if __name__ == "__main__":
    main()
