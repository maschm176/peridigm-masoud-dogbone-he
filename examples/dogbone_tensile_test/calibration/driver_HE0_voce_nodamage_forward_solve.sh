#!/usr/bin/env bash
# Forward solve with HE_0's locked-in optimized Voce hardening parameters
# and NO damage model attached at all (voce_hypoelastic_nodamage_input_
# template.yaml omits the "Damage Models:" section and the block's "Damage
# Model:" line entirely -- a genuinely optional key per Peridigm.cpp:276 --
# so there is zero possibility of any bond ever losing strength, unlike
# pushing a damage parameter to an extreme value, which can still get
# crossed under severe enough strain localization in the neck. Retains full
# Exodus output for ParaView, to see pure Voce-hardening + Considere-necking
# behavior on its own -- a reference point for the post-fracture ringing
# investigation.
set -euo pipefail

PARAMS_FILE=$1
RESULTS_FILE=$2
NUM_PROCS=4
IMAGE=peridigm-masoud:voce-hypoelastic-brozzo

CALDIR=$(cd "$(dirname "$0")" && pwd)
DAKOTA_BIN=/Users/mschmitz/dakota/bin
WORK="$CALDIR/forward_runs/HE0_voce_nodamage"
rm -rf "$WORK"
mkdir -p "$WORK"

python3 "$DAKOTA_BIN/dprepro" --output-format '%.6e' "$PARAMS_FILE" "$CALDIR/voce_hypoelastic_nodamage_input_template.yaml" "$WORK/input.yaml"

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
