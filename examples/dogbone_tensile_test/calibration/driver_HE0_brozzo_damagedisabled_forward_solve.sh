#!/usr/bin/env bash
# Forward solve with HE_0's locked-in optimized parameters (InitialYieldStress=
# 9.77e9, SaturatedYieldStress=1.35e10, ExponentialConstant=22.0,
# LinearConstant=2.0e8, DamageParameter1=3.5, DamageParameter2=0.99), but with
# Threshold Damage pushed to 0.999 so no bond ever ramps/loses strength --
# same "disable damage" trick used in every Stage 1 hardening-only run, just
# applied here with full Exodus output retained for ParaView, to see what
# the material/geometry (pure Voce hardening + Considere necking, zero
# damage) looks like on its own -- a reference point for the post-fracture
# ringing investigation.
set -euo pipefail

PARAMS_FILE=$1
RESULTS_FILE=$2
NUM_PROCS=4
IMAGE=peridigm-masoud:voce-hypoelastic-brozzo

CALDIR=$(cd "$(dirname "$0")" && pwd)
DAKOTA_BIN=/Users/mschmitz/dakota/bin
WORK="$CALDIR/forward_runs/HE0_brozzo_damagedisabled"
rm -rf "$WORK"
mkdir -p "$WORK"

python3 "$DAKOTA_BIN/dprepro" --output-format '%.6e' "$PARAMS_FILE" "$CALDIR/voce_hypoelastic_brozzo_input_template.yaml" "$WORK/input.yaml"

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
