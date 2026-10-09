#!/usr/bin/env bash
# Joint d1/d2 polish for the Brozzo damage model on HE_30, following up on
# two d1-only sweeps that hit a ceiling: d1={2.0,3.5,5.0} all fractured too
# late (target 13.27%, best was d1=3.5 at 15.40%, non-monotonic), and
# d1={5.5,6.0} both crashed (NaN returned by force evaluation) -- narrowing
# the safe/unsafe boundary to between 5.0 (safe) and 5.5 (crash), tighter
# than HE_0's own boundary (~6). Mirrors HE_0's own successful progression
# (d1 sweep -> tighter d1 sweep -> joint d1/d2 polish), warm-started at the
# best surviving point (d1=3.5, d2=0.99). Hardening frozen at HE_30's own
# Stage 1 Voce result, same as both prior sweeps.
set -euo pipefail
export CONCENTRATION=30
export RUN_LABEL=HE30_brozzo_d1d2_polish
export INPUT_TEMPLATE=voce_hypoelastic_brozzo_input_template.yaml
export IMAGE=peridigm-masoud:voce-hypoelastic-brozzo
exec "$(cd "$(dirname "$0")" && pwd)/driver.sh" "$@"
