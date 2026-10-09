#!/usr/bin/env bash
# HE_30 D2 sweep, replicating the HE_0 SFC3-style Stage 2 approach: D1=0.02
# (floor) and D3=3.0 (localization sharpness) fixed at HE_0's own final
# values, treated as material-class properties; D2 (bulk-toughness ceiling
# term) swept to find where fracture strain crosses HE_30's real 13.27%
# (earlier than HE_0's 15.37%, so a smaller D2 than HE_0's final 0.95 is
# expected). ThresholdDamage=0.97, matching HE_0. Hardening frozen at
# HE_30's own Stage 1 result.
set -euo pipefail
export CONCENTRATION=30
export RUN_LABEL=HE30_sfc3_d2_sweep
export INPUT_TEMPLATE=voce_hypoelastic_jc_input_template.yaml
export IMAGE=peridigm-masoud:voce-hypoelastic
exec "$(cd "$(dirname "$0")" && pwd)/driver.sh" "$@"
