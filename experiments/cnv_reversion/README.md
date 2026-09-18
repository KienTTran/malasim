# CNV Reversion Validation

This experiment validates CNV reversion by comparing 20-year simulations with
and without withdrawal of the selecting therapy after 10 years.

The generated scenarios are derived from:

```text
experiments/cnv_reversion/base_input.yml
```

## Arms

- `al`: AL therapy `6`, selecting Pfmdr1 CNV through Lumefantrine drug `1`
- `as_mq`: AS-MQ therapy `9`, selecting Pfmdr1 CNV through Mefloquine drug `4`
- `dha_ppq`: DHA-PPQ therapy `8`, selecting Pfplasmepsin CNV through Piperaquine drug `3`

Each arm has:

- `control`: keeps the selecting therapy through year 20
- `withdrawal`: switches to ASAQ strategy `1` on `2010/1/1`

Replicates are handled by the CLI/job runner. The generated YAML files do not
encode seeds or replicate numbers.

## Generate Scenarios

```sh
python3 experiments/cnv_reversion/generate_scenarios.py
```

This creates:

- `configs/<arm>/<policy>/*.yml`
- `outputs/<arm>/<policy>/<scenario>/`
- `manifests/scenarios.csv`
- `manifests/runs.csv`

## Run A Scenario

```sh
./build/bin/malasim \
  -i experiments/cnv_reversion/configs/al/withdrawal/mp_0p001983__rev_0p5__cost_0p0005.yml \
  -o experiments/cnv_reversion/outputs/al/withdrawal/mp_0p001983__rev_0p5__cost_0p0005/ \
  -j 0
```

The trailing `/` on `-o` is intentional: it tells MalaSim to treat the output
as a directory and create it if necessary.

For the first pass, run jobs `0-9` for each scenario. For the expanded pass,
run jobs `0-99`.

## Extract CNV Frequencies

After simulations have produced `monthly_data_<job>.db` files:

```sh
python3 experiments/cnv_reversion/analysis/extract_cnv_frequencies.py
python3 experiments/cnv_reversion/analysis/summarize_cnv_reversion.py
```

The extractor writes:

```text
experiments/cnv_reversion/analysis/tables/cnv_frequencies.csv
```

The summarizer writes:

```text
experiments/cnv_reversion/analysis/tables/cnv_reversion_summary.csv
```
