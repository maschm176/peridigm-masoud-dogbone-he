#!/usr/bin/env bash
# Joint d1/d2/ThresholdDamage polish for the Brozzo damage model on HE_30,
# following up on the ThresholdDamage sweep that broke the stalled d1/d2-only
# polish (see CLAUDE.md "HE_30 Brozzo calibration -- ThresholdDamage sweep
# breaks the stalled d1/d2 polish"). That sweep (d1=3.5, d2=0.99 held fixed)
# found ThresholdDamage=0.5 landed fracture strain at 13.23% vs the real
# 13.27% target, obj=0.0271 -- by far the best HE_30 result, and the best in
# the whole project. This polish opens all three damage parameters jointly
# around that point to see if there's more to gain now that the real lever
# (ThresholdDamage) is in play, warm-started at (d1=3.5, d2=0.99,
# ThresholdDamage=0.5). Hardening frozen at HE_30's own Stage 1 Voce result,
# same as every other HE_30 run.
set -euo pipefail
export CONCENTRATION=30
export RUN_LABEL=HE30_brozzo_d1d2threshold_polish
export INPUT_TEMPLATE=voce_hypoelastic_brozzo_input_template.yaml
export IMAGE=peridigm-masoud:voce-hypoelastic-brozzo
exec "$(cd "$(dirname "$0")" && pwd)/driver.sh" "$@"
