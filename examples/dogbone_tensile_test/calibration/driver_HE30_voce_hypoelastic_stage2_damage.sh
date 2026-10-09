#!/usr/bin/env bash
# Stage 2 of the HE_30 Voce-under-Hypoelastic+Johnson-Cook calibration:
# Johnson Cook damage only (Damage Parameter 1/2/3), hardening frozen at
# the HE_30 Stage 1 result (see dakota_HE30_voce_hypoelastic_stage2_damage.in).
# ThresholdDamage is fixed (continuous_state, not searched) at 0.85 -- a
# wide enough softening ramp to produce visible pre-fracture softening,
# following the same lesson learned on HE_0 (see CLAUDE.md): ThresholdDamage
# sets the ramp's width/gradualness, D1/D2/D3 set when it begins.
# STRESS_ONLY is NOT set here -- this stage's whole point is fitting the
# real fracture strain via the damage model, so the fracture-strain term
# must stay active.
#
# IMAGE is set to peridigm-masoud:voce-hypoelastic -- the Voce dispatch in
# Isotropic Hardening Hypoelastic Correspondence doesn't exist in
# peridigm-masoud:latest (see CLAUDE.md).
set -euo pipefail
export CONCENTRATION=30
export RUN_LABEL=HE30_voce_hypoelastic_stage2_damage
export INPUT_TEMPLATE=voce_hypoelastic_jc_input_template.yaml
export IMAGE=peridigm-masoud:voce-hypoelastic
exec "$(cd "$(dirname "$0")" && pwd)/driver.sh" "$@"
