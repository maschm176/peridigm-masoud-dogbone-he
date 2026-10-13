#!/usr/bin/env bash
# Run one of the Connectivity Severance / Symmetric Severance Propagation
# example cases directly, with the locked-in parameter values baked in --
# no Dakota installation required. This substitutes the same {Placeholder}
# values the corresponding dakota_*.in file uses (see that file for the
# source of these numbers) directly into the matching *_input_template.yaml
# via sed, then runs Peridigm in Docker exactly like the real driver
# scripts do. See CLAUDE.md for the full rationale behind each case, and
# review_evidence/README.md for the results these exact parameters already
# produced.
#
# Usage: ./run_standalone_example.sh <case>
#   case is one of: connectivity_diagnostic | connectivity_severance |
#                    symmetric_severance | symmetric_severance_ratelimited |
#                    symmetric_severance_ratelimited2
set -euo pipefail

CASE=${1:-}
NUM_PROCS=4
CALDIR=$(cd "$(dirname "$0")" && pwd)

usage() {
  echo "Usage: $0 <case>"
  echo "  case is one of:"
  echo "    connectivity_diagnostic           -- validates the Connectivity_Asymmetry metric itself (threshold fixed unreachable at 0.99)"
  echo "    connectivity_severance             -- Connectivity Severance enabled, threshold 0.20, no propagation fix (6 points go flying, frozen, no cascade)"
  echo "    symmetric_severance                -- Stage 1 (Symmetric Severance Propagation), instant mirror-kill (the avalanche: 2426 points)"
  echo "    symmetric_severance_ratelimited    -- same, mirror-kill throttled to 0.02/step (avalanche delayed ~15 frames, same final count)"
  echo "    symmetric_severance_ratelimited2   -- same, Bond Cleanup ALSO throttled -- this one crashes (NaN force), by design/finding, not a bug in this script"
  exit 1
}

[ -z "$CASE" ] && usage

case "$CASE" in
  connectivity_diagnostic)
    IMAGE=peridigm-masoud:voce-hypoelastic-brozzo-connectivity
    TEMPLATE=voce_hypoelastic_brozzo_rigidgrip_connectivity_diagnostic_input_template.yaml
    SUBS=(InitialYieldStress:9.77e9 SaturatedYieldStress:1.35e10 ExponentialConstant:22.0 LinearConstant:2.0e8 DamageParameter1:3.5 DamageParameter2:0.99 ThresholdDamage:0.85)
    ;;
  connectivity_severance)
    IMAGE=peridigm-masoud:voce-hypoelastic-brozzo-connectivity
    TEMPLATE=voce_hypoelastic_brozzo_rigidgrip_connectivity_severance_input_template.yaml
    SUBS=(InitialYieldStress:9.77e9 SaturatedYieldStress:1.35e10 ExponentialConstant:22.0 LinearConstant:2.0e8 DamageParameter1:3.5 DamageParameter2:0.99 ThresholdDamage:0.85 ConnectivityAsymmetryThreshold:0.20)
    ;;
  symmetric_severance)
    IMAGE=peridigm-masoud:voce-hypoelastic-brozzo-symmetric-severance
    TEMPLATE=voce_hypoelastic_brozzo_rigidgrip_symmetric_severance_input_template.yaml
    SUBS=(InitialYieldStress:9.77e9 SaturatedYieldStress:1.35e10 ExponentialConstant:22.0 LinearConstant:2.0e8 DamageParameter1:3.5 DamageParameter2:0.99 ThresholdDamage:0.85 ConnectivityAsymmetryThreshold:0.20)
    ;;
  symmetric_severance_ratelimited)
    IMAGE=peridigm-masoud:voce-hypoelastic-brozzo-symmetric-severance-ratelimited
    TEMPLATE=voce_hypoelastic_brozzo_rigidgrip_symmetric_severance_ratelimited_input_template.yaml
    SUBS=(InitialYieldStress:9.77e9 SaturatedYieldStress:1.35e10 ExponentialConstant:22.0 LinearConstant:2.0e8 DamageParameter1:3.5 DamageParameter2:0.99 ThresholdDamage:0.85 ConnectivityAsymmetryThreshold:0.20 SymmetricPropagationRate:0.02)
    ;;
  symmetric_severance_ratelimited2)
    IMAGE=peridigm-masoud:voce-hypoelastic-brozzo-symmetric-severance-ratelimited2
    TEMPLATE=voce_hypoelastic_brozzo_rigidgrip_symmetric_severance_ratelimited2_input_template.yaml
    SUBS=(InitialYieldStress:9.77e9 SaturatedYieldStress:1.35e10 ExponentialConstant:22.0 LinearConstant:2.0e8 DamageParameter1:3.5 DamageParameter2:0.99 ThresholdDamage:0.85 ConnectivityAsymmetryThreshold:0.20 SymmetricPropagationRate:0.02 BondCleanupRate:0.02)
    ;;
  *)
    usage
    ;;
esac

if ! docker image inspect "$IMAGE" >/dev/null 2>&1; then
  echo "Docker image '$IMAGE' not found locally."
  echo "Build it first from the repo root (one-time, several minutes):"
  echo "  docker build --platform linux/amd64 -t $IMAGE ."
  exit 1
fi

WORK="$CALDIR/forward_runs/standalone_${CASE}"
rm -rf "$WORK"
mkdir -p "$WORK"

SED_ARGS=()
for pair in "${SUBS[@]}"; do
  name=${pair%%:*}
  value=${pair#*:}
  SED_ARGS+=(-e "s/{${name}}/${value}/g")
done
sed "${SED_ARGS[@]}" "$CALDIR/$TEMPLATE" > "$WORK/input.yaml"

cp "$CALDIR/tensile_test.g" "$CALDIR/extract_stress_strain.py" "$WORK/"

echo "Running case '$CASE' (image $IMAGE) -- this takes roughly an hour on a 4-core Docker allocation."
docker run --rm --platform linux/amd64 -v "$WORK":/output "$IMAGE" bash -c "
set -e
cd /output
decomp -p $NUM_PROCS tensile_test.g
mpirun --allow-run-as-root -np $NUM_PROCS Peridigm input.yaml
python3 extract_stress_strain.py output.e.$NUM_PROCS.0 sim_curve.csv fracture_strain.txt
"

echo "Done. Output in: $WORK"
