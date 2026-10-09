#!/usr/bin/env bash
# Stage 1 of the HE_30 Voce-under-Hypoelastic+Johnson-Cook calibration: Voce
# hardening only, damage disabled (Damage Parameter 1 fixed large, Threshold
# Damage fixed at 0.9, safely below the template's fixed Critical Damage =
# 0.999, so no bond ever crosses threshold). STRESS_ONLY drops the fracture-
# strain term from the objective -- same rationale as the HE_0 Voce Stage 1
# (see driver_HE0_voce_hypoelastic_stage1_hardening.sh): with damage off,
# extract_stress_strain.py's force-drop detector can still fire on genuine
# geometric necking alone (which Voce hardening reliably produces), which
# would otherwise reward warping the hardening curve to fake a fracture
# timing that isn't this stage's job.
#
# IMAGE is set to peridigm-masoud:voce-hypoelastic -- the Voce dispatch in
# Isotropic Hardening Hypoelastic Correspondence doesn't exist in
# peridigm-masoud:latest (see CLAUDE.md).
set -euo pipefail
export CONCENTRATION=30
export RUN_LABEL=HE30_voce_hypoelastic_stage1_hardening
export INPUT_TEMPLATE=voce_hypoelastic_jc_input_template.yaml
export IMAGE=peridigm-masoud:voce-hypoelastic
export STRESS_ONLY=1
exec "$(cd "$(dirname "$0")" && pwd)/driver.sh" "$@"
