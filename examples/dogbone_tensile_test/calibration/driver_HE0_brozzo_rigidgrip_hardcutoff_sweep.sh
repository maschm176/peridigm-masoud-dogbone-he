#!/usr/bin/env bash
# ThresholdDamage sweep for HE_0 under Hard Cutoff=true, rigid-grip BC
# (-220.0*t/220.0*t, matching HE_30's Hard Cutoff test and the paper's
# described methodology). d1/d2 fixed at HE_0's locked-in values (3.5/0.99),
# hardening fixed at HE_0's Stage 1 Voce result. Points span both below and
# above the locked-in ramp-mode threshold (0.85) to see whether oscillation
# keeps improving monotonically with a narrower deleted region, or whether
# there's a sweet spot / crossover like the one seen comparing HE_0/HE_30/
# HE_60's single Hard Cutoff data points.
set -euo pipefail
export CONCENTRATION=0
export RUN_LABEL=HE0_brozzo_rigidgrip_hardcutoff_sweep
export INPUT_TEMPLATE=voce_hypoelastic_brozzo_rigidgrip_hardcutoff_input_template.yaml
export IMAGE=peridigm-masoud:voce-hypoelastic-brozzo
exec "$(cd "$(dirname "$0")" && pwd)/driver.sh" "$@"
