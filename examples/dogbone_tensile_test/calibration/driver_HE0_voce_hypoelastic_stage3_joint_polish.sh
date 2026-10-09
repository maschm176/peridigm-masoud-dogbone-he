#!/usr/bin/env bash
# Stage 3 of the HE_0 Voce-under-Hypoelastic+Johnson-Cook calibration: short
# joint 7-param polish pass (4 Voce hardening + 3 Johnson Cook damage) around
# the Stage 1 + Stage 2 result (see
# dakota_HE0_voce_hypoelastic_stage3_joint_polish.in). ThresholdDamage stays
# fixed (continuous_state, not searched) at 0.85 -- wide enough a ramp to
# produce genuine pre-fracture softening, not a free fit parameter
# (see CLAUDE.md).
#
# IMAGE is set to peridigm-masoud:voce-hypoelastic -- the Voce dispatch in
# Isotropic Hardening Hypoelastic Correspondence doesn't exist in
# peridigm-masoud:latest (see CLAUDE.md).
set -euo pipefail
export CONCENTRATION=0
export RUN_LABEL=HE0_voce_hypoelastic_stage3_joint_polish
export INPUT_TEMPLATE=voce_hypoelastic_jc_input_template.yaml
export IMAGE=peridigm-masoud:voce-hypoelastic
exec "$(cd "$(dirname "$0")" && pwd)/driver.sh" "$@"
