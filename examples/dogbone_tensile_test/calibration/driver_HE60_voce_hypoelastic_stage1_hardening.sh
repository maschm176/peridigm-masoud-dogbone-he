#!/usr/bin/env bash
# Stage 1 of the HE_60 Voce-under-Hypoelastic calibration: Voce hardening
# only, damage disabled (Damage Parameter 1 fixed large, Threshold Damage
# fixed at 0.9, safely below the template's fixed Critical Damage = 0.999,
# so no bond ever crosses threshold). STRESS_ONLY drops the fracture-strain
# term from the objective -- same rationale as HE_0/HE_30's own Stage 1:
# with damage off, extract_stress_strain.py's force-drop detector can still
# fire on genuine geometric necking alone, which would otherwise reward
# warping the hardening curve to fake a fracture timing that isn't this
# stage's job.
#
# Real HE_60 data overlaps HE_0/HE_30 almost exactly through the elastic and
# early-plastic region (see real_data_HE0_HE30_HE60_comparison.png) but
# fractures much earlier (7.68% vs 13.27%/15.37%) -- consistent with the
# established finding that hardening stays close to fixed across
# concentrations while ThresholdDamage is the real embrittlement lever (see
# CLAUDE.md, HE_30 final result). Still re-running Stage 1 fresh rather than
# assuming exact transfer, same practice as every prior concentration.
#
# IMAGE is set to peridigm-masoud:voce-hypoelastic -- the Voce dispatch in
# Isotropic Hardening Hypoelastic Correspondence doesn't exist in
# peridigm-masoud:latest (see CLAUDE.md).
set -euo pipefail
export CONCENTRATION=60
export RUN_LABEL=HE60_voce_hypoelastic_stage1_hardening
export INPUT_TEMPLATE=voce_hypoelastic_jc_input_template.yaml
export IMAGE=peridigm-masoud:voce-hypoelastic
export STRESS_ONLY=1
exec "$(cd "$(dirname "$0")" && pwd)/driver.sh" "$@"
