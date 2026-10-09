#!/usr/bin/env bash
# Smoke test for the new Brozzo damage model (new C++ code + new Docker
# image peridigm-masoud:voce-hypoelastic-brozzo). Uses HE_0's own converged
# Stage 1 Voce hardening and the paper's own calibrated d1/d2
# (Behzadinasab & Foster 2020, Table 1: d1=0.76, d2=0.95) as a first check
# that the new damage model runs without crashing and produces a sane
# fracture, before doing any real calibration.
set -euo pipefail
export CONCENTRATION=0
export RUN_LABEL=HE0_brozzo_smoketest
export INPUT_TEMPLATE=voce_hypoelastic_brozzo_input_template.yaml
export IMAGE=peridigm-masoud:voce-hypoelastic-brozzo
exec "$(cd "$(dirname "$0")" && pwd)/driver.sh" "$@"
