#!/usr/bin/env bash
# Narrow-ramp hypothesis test for HE_60. The wide-ramp result locked in so
# far (d1=3.5, ThresholdDamage=0.19) matches the real fracture STRAIN
# (7.80% vs 7.68%) but produces a 31.4% peak-to-fracture stress drop in the
# simulated curve, versus only 0.9% in the real data -- real HE_60 fails
# almost brittlely, with essentially no softening warning, while our sim
# shows a long gradual decline. Mechanism: ThresholdDamage=0.19 means the
# bond-strength ramp spans a huge range of accumulated damage (0.19 to
# 1.0), smearing the stress drop out over a real strain range. Testing the
# opposite regime instead: a NARROW ramp (ThresholdDamage much closer to 1,
# towards HE_0's original 0.85) combined with a correspondingly LARGER d1,
# so damage stays negligible until very late and then races to full
# failure almost instantly -- aiming for fracture still landing near 7.68%
# strain, but via a sharp drop instead of a long slide.
#
# d1 values are rough linear-scaling guesses (D_dot ~ d1 * stress-state-term
# * eqps_dot; since the stress-state term doesn't change, roughly
# d1_new ~ d1_old * threshold_new/threshold_old to reach the new, higher
# threshold in about the same strain/time as before) -- not a precise
# derivation, since the stress-state/triaxiality history isn't actually
# constant-rate. Treated as an empirical starting point to iterate from,
# same practice as every other sweep in this project. Real crash risk:
# HE_0/HE_30's own d1 crash boundary was around 5.0-6.0 at their much
# higher thresholds (0.85/0.51) -- these scaled-up d1 guesses may exceed
# that boundary for HE_60's regime, which hasn't been characterized here.
# A crash at any of these points is itself informative (narrows the
# boundary), not just a failure to route around.
set -euo pipefail
export CONCENTRATION=60
export RUN_LABEL=HE60_brozzo_narrowramp_test
export INPUT_TEMPLATE=voce_hypoelastic_brozzo_input_template.yaml
export IMAGE=peridigm-masoud:voce-hypoelastic-brozzo
exec "$(cd "$(dirname "$0")" && pwd)/driver.sh" "$@"
