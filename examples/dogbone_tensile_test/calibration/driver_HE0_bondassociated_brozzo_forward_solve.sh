#!/usr/bin/env bash
# One-off diagnostic forward solve: re-runs our locked-in best Brozzo result
# (d1=3.5, d2=0.99, ThresholdDamage=0.85, HE_0 Stage 1 Voce hardening) under
# the Isotropic Hardening BOND ASSOCIATED Correspondence material instead of
# the ordinary Hypoelastic Correspondence used everywhere else in this
# project (see voce_bondassociated_brozzo_input_template.yaml for full
# rationale) -- testing whether this more robust material formulation
# removes the post-fracture oscillation at its actual source (a
# shape-tensor singularity under heavy local damage), rather than attacking
# the damage law. Does NOT delete its work directory, so the full Exodus
# output is kept for ParaView comparison against forward_runs/HE0_brozzo_d1_3p5.
set -euo pipefail

PARAMS_FILE=$1
RESULTS_FILE=$2
NUM_PROCS=4
IMAGE=peridigm-masoud:voce-hypoelastic-brozzo

CALDIR=$(cd "$(dirname "$0")" && pwd)
DAKOTA_BIN=/Users/mschmitz/dakota/bin
WORK="$CALDIR/forward_runs/HE0_bondassociated_brozzo"
rm -rf "$WORK"
mkdir -p "$WORK"

python3 "$DAKOTA_BIN/dprepro" --output-format '%.6e' "$PARAMS_FILE" "$CALDIR/voce_bondassociated_brozzo_input_template.yaml" "$WORK/input.yaml"

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
