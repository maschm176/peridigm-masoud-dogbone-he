#!/usr/bin/env bash
# Diagnostic forward solve: "Isotropic Hardening Correspondence" material
# (Silling 2007 original correspondence kinematics, confirmed against the
# primary source -- see template header) + Critical Stretch damage,
# rigid-grip BC at 220 cm/s. Rough, non-calibrated parameters (see template)
# -- purpose is solely to check whether switching away from the
# Green-Lagrange/finite-deformation kinematics changes the post-fracture
# ringing. Retains full Exodus output for ParaView/post-processing.
set -euo pipefail

PARAMS_FILE=$1
RESULTS_FILE=$2
NUM_PROCS=4
IMAGE=peridigm-masoud:voce-hypoelastic-brozzo

CALDIR=$(cd "$(dirname "$0")" && pwd)
DAKOTA_BIN=/Users/mschmitz/dakota/bin
WORK="$CALDIR/forward_runs/HE0_isotropichardening_criticalstretch_rigidgrip"
rm -rf "$WORK"
mkdir -p "$WORK"

python3 "$DAKOTA_BIN/dprepro" --output-format '%.6e' "$PARAMS_FILE" "$CALDIR/isotropic_hardening_correspondence_criticalstretch_rigidgrip_input_template.yaml" "$WORK/input.yaml"

cp "$CALDIR/tensile_test.g" "$CALDIR/extract_stress_strain.py" "$WORK/"

docker run --rm --platform linux/amd64 -v "$WORK":/output "$IMAGE" bash -c "
set -e
cd /output
decomp -p $NUM_PROCS tensile_test.g
mpirun --allow-run-as-root -np $NUM_PROCS Peridigm input.yaml
python3 extract_stress_strain.py output.e.$NUM_PROCS.0 sim_curve.csv fracture_strain.txt
"

echo "1.0 obj_fn_1" > "$RESULTS_FILE"

echo "Forward solve complete. Exodus output retained in: $WORK"
