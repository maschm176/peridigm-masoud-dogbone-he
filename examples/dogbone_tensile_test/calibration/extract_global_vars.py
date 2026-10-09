#!/usr/bin/env python3
"""Extract Exodus global variable time histories via ncdump text output.

Runs inside the peridigm-masoud container (needs ncdump, no netCDF4/exodus
python module required). Usage:
    python3 extract_global_vars.py <exodus_file.e> <output.csv> [var1 var2 ...]
If no variable names are given, all global variables are dumped.
"""
import subprocess
import sys
import re

NCDUMP = "/usr/local/netcdf/bin/ncdump"


def get_names(exo_file):
    out = subprocess.run([NCDUMP, "-v", "name_glo_var", exo_file],
                          capture_output=True, text=True, check=True).stdout
    block = re.search(r"name_glo_var\s*=\s*(.*?);", out, re.S).group(1)
    return [s.strip().strip('"') for s in block.split(",") if s.strip()]


def get_values(exo_file):
    out = subprocess.run([NCDUMP, "-v", "vals_glo_var", exo_file],
                          capture_output=True, text=True, check=True).stdout
    block = re.search(r"vals_glo_var\s*=\s*(.*?);", out, re.S).group(1)
    nums = [float(s) for s in block.replace("\n", " ").split(",") if s.strip()]
    return nums


def main():
    exo_file, out_csv = sys.argv[1], sys.argv[2]
    wanted = sys.argv[3:]

    names = get_names(exo_file)
    values = get_values(exo_file)
    n = len(names)
    assert len(values) % n == 0, f"{len(values)} values not divisible by {n} vars"
    steps = len(values) // n

    cols = wanted if wanted else names
    indices = [names.index(c) for c in cols]

    with open(out_csv, "w") as f:
        f.write("step," + ",".join(cols) + "\n")
        for step in range(steps):
            row = values[step * n:(step + 1) * n]
            f.write(str(step) + "," + ",".join(str(row[i]) for i in indices) + "\n")

    print(f"Wrote {steps} steps x {len(cols)} vars to {out_csv}")


if __name__ == "__main__":
    main()
