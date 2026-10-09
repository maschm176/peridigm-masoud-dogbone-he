#!/usr/bin/env bash
# Stage 2 of the HE_0 Hypoelastic+Johnson-Cook calibration: Johnson Cook
# damage only (Damage Parameter 1/2/3, Threshold Damage), hardening params
# frozen at the Stage 1 result (see dakota_HE0_hypoelastic_stage2_damage.in).
# STRESS_ONLY is NOT set here -- this stage's whole point is fitting the
# real fracture strain via the damage model, so the fracture-strain term
# must stay active.
set -euo pipefail
export CONCENTRATION=0
export RUN_LABEL=HE0_hypoelastic_stage2_damage
export INPUT_TEMPLATE=hypoelastic_jc_input_template.yaml
exec "$(cd "$(dirname "$0")" && pwd)/driver.sh" "$@"
