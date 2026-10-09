#!/usr/bin/env bash
# Stage 3 of the HE_0 Hypoelastic+Johnson-Cook calibration: short joint
# 7-param polish pass around the Stage 1 + Stage 2 result (see
# dakota_HE0_hypoelastic_stage3_joint_polish.in).
set -euo pipefail
export CONCENTRATION=0
export RUN_LABEL=HE0_hypoelastic_stage3_joint_polish
export INPUT_TEMPLATE=hypoelastic_jc_input_template.yaml
exec "$(cd "$(dirname "$0")" && pwd)/driver.sh" "$@"
