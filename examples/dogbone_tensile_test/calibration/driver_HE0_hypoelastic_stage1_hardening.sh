#!/usr/bin/env bash
# Stage 1 of the HE_0 Hypoelastic+Johnson-Cook calibration: power-law
# hardening only, damage disabled (Threshold Damage fixed at 0.9 (must stay
# below the template's fixed Critical Damage = 0.95) and
# Damage Parameter 1 fixed large, so no bond ever crosses threshold).
# STRESS_ONLY drops the fracture-strain term from the objective -- with
# damage off, extract_stress_strain.py's force-drop detector can still fire
# on geometric necking alone, which would otherwise reward warping the
# hardening curve to fake a fracture timing that isn't this stage's job.
set -euo pipefail
export CONCENTRATION=0
export RUN_LABEL=HE0_hypoelastic_stage1_hardening
export INPUT_TEMPLATE=hypoelastic_jc_input_template.yaml
export STRESS_ONLY=1
exec "$(cd "$(dirname "$0")" && pwd)/driver.sh" "$@"
