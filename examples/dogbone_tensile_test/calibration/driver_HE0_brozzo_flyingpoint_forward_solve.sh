#!/usr/bin/env bash
# One-off diagnostic forward solve: re-runs our locked-in best Brozzo result
# (d1=3.5, d2=0.99, ThresholdDamage=0.85, HE_0 Stage 1 Voce hardening) with
# "Critical Damage To Neglect Material Point" lowered from Peridigm's default
# (0.95) to 0.5, to test whether more aggressively freezing nearly-failed
# material points removes the post-fracture oscillation (see
# voce_hypoelastic_brozzo_flyingpoint_input_template.yaml for the full
# rationale). Does NOT delete its work directory, so the full Exodus output
# is kept for ParaView comparison against forward_runs/HE0_brozzo_d1_3p5 and
# forward_runs/HE0_brozzo_contact.
set -euo pipefail

PARAMS_FILE=$1
RESULTS_FILE=$2
NUM_PROCS=4
IMAGE=peridigm-masoud:voce-hypoelastic-brozzo

CALDIR=$(cd "$(dirname "$0")" && pwd)
DAKOTA_BIN=/Users/mschmitz/dakota/bin
WORK="$CALDIR/forward_runs/HE0_brozzo_flyingpoint_0p5"
rm -rf "$WORK"
mkdir -p "$WORK"

python3 "$DAKOTA_BIN/dprepro" --output-format '%.6e' "$PARAMS_FILE" "$CALDIR/voce_hypoelastic_brozzo_flyingpoint_input_template.yaml" "$WORK/input.yaml"

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
