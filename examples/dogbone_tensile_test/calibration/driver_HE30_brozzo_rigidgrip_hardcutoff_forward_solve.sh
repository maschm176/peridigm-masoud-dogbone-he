#!/usr/bin/env bash
# Forward solve with HE_30's locked-in optimized Brozzo parameters
# (d1=3.5, d2=0.99, ThresholdDamage=0.51, HE_30 Stage 1 Voce hardening),
# rigid-grip BC at the original/faster rate (-220.0*t/220.0*t), with
# "Hard Cutoff: true" added -- see
# voce_hypoelastic_brozzo_rigidgrip_hardcutoff_input_template.yaml.
# Retains full Exodus output for ParaView.
set -euo pipefail

PARAMS_FILE=$1
RESULTS_FILE=$2
NUM_PROCS=4
IMAGE=peridigm-masoud:voce-hypoelastic-brozzo

CALDIR=$(cd "$(dirname "$0")" && pwd)
DAKOTA_BIN=/Users/mschmitz/dakota/bin
WORK="$CALDIR/forward_runs/HE30_brozzo_rigidgrip_hardcutoff"
rm -rf "$WORK"
mkdir -p "$WORK"

python3 "$DAKOTA_BIN/dprepro" --output-format '%.6e' "$PARAMS_FILE" "$CALDIR/voce_hypoelastic_brozzo_rigidgrip_hardcutoff_input_template.yaml" "$WORK/input.yaml"

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
