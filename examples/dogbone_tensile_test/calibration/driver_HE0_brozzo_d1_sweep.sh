#!/usr/bin/env bash
# Coarse d1 sweep for the Brozzo damage model on HE_0: the paper's own
# d1=0.76 (with d2=0.95, ThresholdDamage in {0.85,0.9,0.95}) never
# accumulated enough damage to fracture our specimen at all (identical
# obj=1.035990241 across all three thresholds -- see
# dakota_HE0_brozzo_smoketest.*). This sweep pushes d1 up log-spaced to
# find where damage actually starts biting, before attempting any
# optimization (a pattern search started in the current dead/flat region
# would have no gradient signal to work with).
set -euo pipefail
export CONCENTRATION=0
export RUN_LABEL=HE0_brozzo_d1_sweep
export INPUT_TEMPLATE=voce_hypoelastic_brozzo_input_template.yaml
export IMAGE=peridigm-masoud:voce-hypoelastic-brozzo
exec "$(cd "$(dirname "$0")" && pwd)/driver.sh" "$@"
