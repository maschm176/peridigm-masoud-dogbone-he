#!/usr/bin/env bash
# Joint d1/d2/ThresholdDamage polish for HE_60, warm-started at the best
# point found by the two-stage threshold sweep: d1=3.5, d2=0.99,
# ThresholdDamage=0.19 (obj=0.0540, fracture_strain=7.80% vs real target
# 7.68%). Same procedure that worked for HE_30 (see CLAUDE.md "HE_30 Brozzo
# -- joint d1/d2/ThresholdDamage polish"). Hardening frozen at HE_60's own
# locked-in Stage 1 result.
set -euo pipefail
export CONCENTRATION=60
export RUN_LABEL=HE60_brozzo_d1d2threshold_polish
export INPUT_TEMPLATE=voce_hypoelastic_brozzo_input_template.yaml
export IMAGE=peridigm-masoud:voce-hypoelastic-brozzo
exec "$(cd "$(dirname "$0")" && pwd)/driver.sh" "$@"
