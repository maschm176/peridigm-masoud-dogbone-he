#!/usr/bin/env bash
# Tighter d1 sweep for the Brozzo damage model on HE_30, following up on
# dakota_HE30_brozzo_d1_sweep.in: d1={2.0,3.5,5.0} all fractured too late
# (19.10%, 15.40%, 15.72% vs real target 13.27%, non-monotonically), and
# d1=7.0 crashed (NaN returned by force evaluation -- same failure mode as
# HE_0's own d1=6.0 crash). This probes the narrow window between the best
# surviving point (5.0) and the known crash boundary (7.0). Same fixed
# hardening/d2/ThresholdDamage as the first sweep.
set -euo pipefail
export CONCENTRATION=30
export RUN_LABEL=HE30_brozzo_d1_sweep2
export INPUT_TEMPLATE=voce_hypoelastic_brozzo_input_template.yaml
export IMAGE=peridigm-masoud:voce-hypoelastic-brozzo
exec "$(cd "$(dirname "$0")" && pwd)/driver.sh" "$@"
