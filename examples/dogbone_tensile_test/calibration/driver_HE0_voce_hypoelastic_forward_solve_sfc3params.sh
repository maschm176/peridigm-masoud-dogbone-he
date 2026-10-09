#!/usr/bin/env bash
# One-off forward solve using the SFC3 paper's own Johnson Cook damage
# constants (Behzadinasab & Foster 2019: D1=0.04, D2=2.5, D3=3) with our own
# calibrated HE_0 Voce hardening (the paper's hardening params are for 316L
# steel, a different alloy/stress scale, and don't transfer). ThresholdDamage
# is set to 0.97, our best-validated near-hard-cutoff approximation of the
# paper's binary damage law. Purpose: an isolated diagnostic of whether the
# paper's D1/D2/D3 alone (no Contact/self-contact model, no mesh changes)
# produces smoother, less-oscillatory softening than our own calibrated
# D1/D2/D3 -- see CLAUDE.md discussion. Like the other forward-solve driver,
# this does NOT delete its work directory, so the full Exodus output is kept
# for ParaView.
set -euo pipefail

PARAMS_FILE=$1
RESULTS_FILE=$2
NUM_PROCS=4
IMAGE=peridigm-masoud:voce-hypoelastic

CALDIR=$(cd "$(dirname "$0")" && pwd)
DAKOTA_BIN=/Users/mschmitz/dakota/bin
WORK="$CALDIR/forward_runs/HE0_sfc3_damage_params"
rm -rf "$WORK"
mkdir -p "$WORK"

python3 "$DAKOTA_BIN/dprepro" --output-format '%.6e' "$PARAMS_FILE" "$CALDIR/voce_hypoelastic_jc_input_template.yaml" "$WORK/input.yaml"

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
