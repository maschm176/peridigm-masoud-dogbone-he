#!/usr/bin/env bash
# Joint d1/d2 polish search for the Brozzo damage model on HE_0. d1 alone
# (d2=0.95, ThresholdDamage=0.85 fixed) already found a strong result at
# d1=3.5 (frac=15.91% vs real 15.37%, obj=0.0715), but the d1 response
# surface is jagged/non-monotonic (3.0->0.133, 3.5->0.0715, 4.0->0.176,
# 4.5->0.140, 5.0->0.099) and d1/d2 are coupled in the same rate-law
# expression, so a joint search may find a better combination than
# independently tuning d1 alone. Bounds kept tight and warm-started at the
# current best to avoid wasting evals on this known-rough landscape.
set -euo pipefail
export CONCENTRATION=0
export RUN_LABEL=HE0_brozzo_d1d2_polish
export INPUT_TEMPLATE=voce_hypoelastic_brozzo_input_template.yaml
export IMAGE=peridigm-masoud:voce-hypoelastic-brozzo
exec "$(cd "$(dirname "$0")" && pwd)/driver.sh" "$@"
