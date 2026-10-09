#!/usr/bin/env bash
# ThresholdDamage sweep for HE_30, d1=3.5 and d2=0.99 fixed at the best
# surviving point from the stalled joint d1/d2 polish (eval 1, obj=0.1840 --
# the polish only found a ~1.5% improvement over 5 evals before both d1
# directions came out worse, meaning d1=3.5 is already a local optimum and
# further polishing won't close the fracture-strain gap: d1=3.5 fractures at
# 15.40% vs the real HE_30 target of 13.27%, and pushing d1 higher to close
# that gap crashes somewhere between 5.0 and 5.5, see CLAUDE.md).
#
# ThresholdDamage has been fixed at 0.85 (HE_0's value) throughout every
# HE_30 run so far, never itself tuned for this curve. Testing whether
# lowering it -- widening the damage ramp so bonds lose stiffness more
# gradually over a larger accumulated-damage range instead of a sharp final
# 15% -- can (a) shift the force-drop-detected fracture point earlier at
# this same, numerically-safe d1=3.5, reducing the need to push d1 into the
# crash zone, and/or (b) raise the crash ceiling itself, if the instability
# is tied to how ABRUPTLY stiffness is lost in a single step rather than to
# d1 in isolation. Hardening frozen at HE_30's own Stage 1 Voce result, same
# as every other HE_30 run.
set -euo pipefail
export CONCENTRATION=30
export RUN_LABEL=HE30_brozzo_threshold_sweep
export INPUT_TEMPLATE=voce_hypoelastic_brozzo_input_template.yaml
export IMAGE=peridigm-masoud:voce-hypoelastic-brozzo
exec "$(cd "$(dirname "$0")" && pwd)/driver.sh" "$@"
