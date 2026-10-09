#!/usr/bin/env bash
# Coarse d1 sweep for the Brozzo damage model on HE_30, mirroring the
# procedure that worked for HE_0 (dakota_HE0_brozzo_d1_sweep.in /
# dakota_HE0_brozzo_d1_sweep2.in -> joint d1/d2 polish -> locked-in result
# d1=3.5, d2=0.99, ThresholdDamage=0.85, obj=0.0688). Hardening is frozen at
# HE_30's own Stage 1 Voce result (CLAUDE.md, "Stage 1 result" table:
# InitialYieldStress=1.027e10, others unchanged from HE_0). d2 is fixed at
# HE_0's converged value (0.99) rather than swept, since d1 (a damage
# accumulation RATE) is the more physically plausible embrittlement-
# sensitive parameter -- analogous to CriticalStretch in the earlier
# Johnson-Cook/Critical-Stretch work -- while d2 (an exponent shaping stress-
# state sensitivity) is expected to be closer to a fixed material property,
# the same way Voce hardening itself is held fixed here.
set -euo pipefail
export CONCENTRATION=30
export RUN_LABEL=HE30_brozzo_d1_sweep
export INPUT_TEMPLATE=voce_hypoelastic_brozzo_input_template.yaml
export IMAGE=peridigm-masoud:voce-hypoelastic-brozzo
exec "$(cd "$(dirname "$0")" && pwd)/driver.sh" "$@"
