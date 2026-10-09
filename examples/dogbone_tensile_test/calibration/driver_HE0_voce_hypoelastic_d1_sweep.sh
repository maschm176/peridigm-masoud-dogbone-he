#!/usr/bin/env bash
# D1 sweep diagnostic: holding DamageParameter2=2.5, DamageParameter3=3.0
# (the SFC3 paper's own values, preserving their ~60x floor/ceiling epsF
# ratio and the resulting spatially-localized, smooth softening behavior),
# stepping DamageParameter1 down from the paper's 0.04 to find where the
# predicted fracture strain crosses HE_0's real 15.37% -- see CLAUDE.md /
# session discussion. ThresholdDamage=0.97 and hardening are our own
# calibrated HE_0 Voce values (paper's hardening is for a different alloy,
# doesn't transfer).
set -euo pipefail
export CONCENTRATION=0
export RUN_LABEL=HE0_sfc3_d1_sweep
export INPUT_TEMPLATE=voce_hypoelastic_jc_input_template.yaml
export IMAGE=peridigm-masoud:voce-hypoelastic
exec "$(cd "$(dirname "$0")" && pwd)/driver.sh" "$@"
