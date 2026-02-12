# Scenario Runner - User Guide

## Overview

`run_scenarios.py` is a Python wrapper script that automates batch execution of the DSK-SFC macroeconomic model across multiple scenarios with Monte Carlo simulations.

## Features

- **Automatic scenario detection**: Scans a directory for scenario input files
- **Monte Carlo simulations**: Configurable number of replications per scenario
- **Deterministic seeding**: Seeds based on scenario and replication indices for reproducibility
- **Structured output**: Hierarchical directory tree organized by scenario and replication
- **Metadata tracking**: Saves run parameters for each simulation
- **Collision-free**: Prevents output files from overwriting each other

## Quick Start

### Basic Usage (N=1 replication per scenario)

```bash
python run_scenarios.py
```

This will:
- Scan `./verification/` for scenario input files
- Run the model once per scenario (N=1)
- Store outputs in `./output/scenario_XX/run_001/`

### Advanced Usage

```bash
python run_scenarios.py -n 10 -d ./verification -o ./results
```

This runs 10 Monte Carlo replications per scenario, reading from `./verification/` and saving to `./results/`.

## Command-Line Options

| Option | Default | Description |
|--------|---------|-------------|
| `-d, --scenarios-dir` | `./verification` | Directory containing scenario JSON input files |
| `-o, --output` | `./output` | Base directory for structured outputs |
| `-e, --executable` | `./dsk_SFC` | Path to DSK-SFC model executable |
| `-n, --replications` | `1` | Number of Monte Carlo replications per scenario |
| `-s, --seed-base` | `1000` | Base value for seed calculation |
| `-f, --full-output` | Off | Enable full output mode in model (`-f 1` flag) |
| `-q, --quiet` | Off | Suppress verbose progress messages |

## Output Structure

The script creates a hierarchical directory structure:

```
output/
├── scenario_01/
│   ├── run_001/
│   │   ├── run_metadata.json
│   │   ├── resultsexp_reg1_scenario01_rep001_1001.txt
│   │   ├── resultsexp_reg2_scenario01_rep001_1001.txt
│   │   └── ...
│   ├── run_002/
│   │   └── ...
│   └── run_N/
├── scenario_02/
│   ├── run_001/
│   └── ...
└── scenario_XX/
```

### Metadata Files

Each run directory contains a `run_metadata.json` file with:
- Scenario and replication indices
- Input file path
- Seed used
- Run success status
- Total number of replications
- Path to executable

This enables full reproducibility of results.

## Seed Calculation

Seeds are deterministically calculated as:

```
seed = scenario_index × seed_base + replication_index
```

**Example** (with default seed_base=1000):
- Scenario 1, Replication 1: seed = 1001
- Scenario 1, Replication 2: seed = 1002
- Scenario 2, Replication 1: seed = 2001
- Scenario 3, Replication 5: seed = 3005

This ensures:
1. Deterministic and reproducible results
2. No seed collisions across scenarios/replications
3. Easy identification of which run produced which output

## Examples

### Run 100 Monte Carlo replications per scenario

```bash
python run_scenarios.py -n 100
```

### Use a custom scenarios directory

```bash
python run_scenarios.py -d ./my_scenarios -n 50
```

### Run with full model output enabled

```bash
python run_scenarios.py -n 10 -f
```

### Quiet mode (minimal console output)

```bash
python run_scenarios.py -n 100 -q
```

### Custom seed base for different experiment batches

```bash
# First batch: seeds 1001-1100 for scenario 1
python run_scenarios.py -n 100 -s 1000 -o ./output_batch1

# Second batch: seeds 10001-10100 for scenario 1  
python run_scenarios.py -n 100 -s 10000 -o ./output_batch2
```

## Integration with Analysis Pipelines

The structured output is designed for downstream analysis:

### Monte Carlo Aggregation

Each replication is stored separately, making it easy to:
- Calculate mean and variance across replications
- Generate confidence intervals
- Perform sensitivity analysis

### Scenario Comparison

Scenarios are cleanly separated, enabling:
- Cross-scenario statistical tests
- Comparative validation tables
- Lamperti-style validation metrics

### Example Analysis Structure

```python
import pandas as pd
from pathlib import Path

# Load all replications for scenario 1
scenario_dir = Path("output/scenario_01")
runs = []
for run_dir in sorted(scenario_dir.glob("run_*")):
    # Load outputs from this run
    data = pd.read_csv(run_dir / "resultsexp_reg1_*.txt", sep="\\t")
    runs.append(data)

# Aggregate across Monte Carlo replications
mean_results = pd.concat(runs).groupby(level=0).mean()
std_results = pd.concat(runs).groupby(level=0).std()
```

## Error Handling

- **Missing executable**: Script validates executable exists and is runnable
- **Missing scenarios directory**: Script checks directory exists before running
- **Model execution errors**: Captured and reported; script continues with remaining runs
- **Exit codes**: Returns 0 if all runs succeed, 1 if any runs fail

## Extending the Script

### Modify seed calculation

Edit the `calculate_seed()` method in the `ScenarioRunner` class:

```python
def calculate_seed(self, scenario_idx: int, replication_idx: int) -> int:
    # Custom seed formula
    return scenario_idx * 10000 + replication_idx * 100 + some_offset
```

### Increase replications

Simply change the `-n` parameter:

```bash
python run_scenarios.py -n 500  # Run 500 replications per scenario
```

### Add parallel execution

The script currently runs sequentially. For parallel execution, you could:

1. Use Python's `multiprocessing` or `concurrent.futures`
2. Submit to a cluster scheduler (SLURM, PBS, etc.)
3. Use the existing `runPar.R` script but with this structured output approach

## Requirements

- Python 3.6+
- Standard library only (no external dependencies)
- Compiled DSK-SFC executable (`dsk_SFC`)
- Scenario input JSON files

## License

Same license as the DSK-SFC model (see LICENSE.txt in the main directory).

## Questions or Issues?

The script includes detailed error messages. Run with `-h` for help:

```bash
python run_scenarios.py -h
```
