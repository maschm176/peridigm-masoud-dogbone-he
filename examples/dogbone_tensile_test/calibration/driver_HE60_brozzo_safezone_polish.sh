#!/usr/bin/env bash
# Bounded joint d1/d2/ThresholdDamage polish for HE_60, restricted to the
# now-confirmed-stable neighborhood found by the cautious narrowing test
# (d1=4.6, ThresholdDamage=0.25 ran stably, obj=0.1167, softening dropped
# from 31.4% to 25.2%) while staying clear of the known-unstable boundary
# (d1=6.4, ThresholdDamage=0.35 produced violent dynamic oscillation; Hard
# Cutoff at the unchanged 3.5/0.19 also oscillated -- see CLAUDE.md "HE_60
# -- pre-fracture softening shape investigation"). Warm-started at the
# original best point (d1=3.5, ThresholdDamage=0.19, obj=0.0540), since
# that remains the best-scoring point found so far by the existing
# objective -- which already implicitly penalizes excess pre-fracture
# softening (real HE_60 data stays near-flat close to its own fracture
# point, so a simulated curve that declines early loses points on the
# ordinary stress-shape comparison, not just the fracture-strain term).
# Hardening frozen at HE_60's own locked-in Stage 1 result.
set -euo pipefail
export CONCENTRATION=60
export RUN_LABEL=HE60_brozzo_safezone_polish
export INPUT_TEMPLATE=voce_hypoelastic_brozzo_input_template.yaml
export IMAGE=peridigm-masoud:voce-hypoelastic-brozzo
exec "$(cd "$(dirname "$0")" && pwd)/driver.sh" "$@"
