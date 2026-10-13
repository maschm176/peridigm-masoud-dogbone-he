# Start here: Connectivity Severance / Symmetric Severance Propagation

This directory has ~100+ files from a multi-week calibration project (see
`CLAUDE.md` for the full history). If you're here to review the
Connectivity Severance / Symmetric Severance Propagation work specifically,
these are the files that matter — everything else in this directory is
either prior/unrelated calibration work or generated output.

## Read this first

- **`CLAUDE.md`** — full narrative: the oscillation problem, the root
  cause ("hemisphere problem"), and every prior failed intervention. The
  sections relevant here are near the end, under the Connectivity
  Severance / Symmetric Severance Propagation headers.
- **`review_evidence/README.md`** — the actual result data (stress-strain
  curves, flying-point counts per frame) for each run below, pulled out of
  the (gitignored, too-large-to-commit) raw simulation output.

## The code

- `../../../src/damage/Peridigm_BrozzoDamageModel.cpp` / `.hpp` — the
  actual mechanism implementations (Connectivity Severance, Symmetric
  Severance Propagation, and the two rate-limiting parameters).

## The five cases, in the order they were tried

| Case | YAML template | Driver script | Dakota config |
|---|---|---|---|
| Baseline metric validation | `voce_hypoelastic_brozzo_rigidgrip_connectivity_diagnostic_input_template.yaml` | `driver_HE0_brozzo_rigidgrip_connectivity_diagnostic_forward_solve.sh` | `dakota_HE0_brozzo_rigidgrip_connectivity_diagnostic_forward_solve.in` |
| Connectivity Severance (no propagation) | `voce_hypoelastic_brozzo_rigidgrip_connectivity_severance_input_template.yaml` | `driver_HE0_brozzo_rigidgrip_connectivity_severance_forward_solve.sh` | `dakota_HE0_brozzo_rigidgrip_connectivity_severance_forward_solve.in` |
| Symmetric Severance Propagation (instant kill) | `voce_hypoelastic_brozzo_rigidgrip_symmetric_severance_input_template.yaml` | `driver_HE0_brozzo_rigidgrip_symmetric_severance_forward_solve.sh` | `dakota_HE0_brozzo_rigidgrip_symmetric_severance_forward_solve.in` |
| + rate-limited mirror-kill | `voce_hypoelastic_brozzo_rigidgrip_symmetric_severance_ratelimited_input_template.yaml` | `driver_HE0_brozzo_rigidgrip_symmetric_severance_ratelimited_forward_solve.sh` | `dakota_HE0_brozzo_rigidgrip_symmetric_severance_ratelimited_forward_solve.in` |
| + rate-limited Bond Cleanup too (crashes) | `voce_hypoelastic_brozzo_rigidgrip_symmetric_severance_ratelimited2_input_template.yaml` | `driver_HE0_brozzo_rigidgrip_symmetric_severance_ratelimited2_forward_solve.sh` | `dakota_HE0_brozzo_rigidgrip_symmetric_severance_ratelimited2_forward_solve.in` |

Each YAML template has a comment block at the top explaining its specific
rationale.

## Running one yourself

The driver scripts above are Dakota analysis drivers — they expect a
Dakota-generated `params.in` file and are meant to be invoked via the
matching `dakota_*.in` config and a local Dakota install.

If you just want to reproduce one of these results directly, without
installing Dakota, use `run_standalone_example.sh` instead — it bakes in
the exact locked-in parameter values each case above actually used and
runs Peridigm in Docker directly:

```
./run_standalone_example.sh connectivity_severance
./run_standalone_example.sh symmetric_severance
./run_standalone_example.sh symmetric_severance_ratelimited
./run_standalone_example.sh symmetric_severance_ratelimited2   # this one is expected to crash -- see CLAUDE.md
```

Requires the matching Docker image to already be built (see the `IMAGE=`
line near the top of each `driver_*.sh` file for the exact tag, and the
repo root `Dockerfile` to build it: `docker build --platform linux/amd64
-t <tag> .`). Each run takes roughly an hour on a 4-core Docker
allocation.
