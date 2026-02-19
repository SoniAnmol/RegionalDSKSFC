#!/usr/bin/env python3
"""
Wrapper script for running DSK-SFC macroeconomic model across multiple scenarios
with Monte Carlo simulations and structured output storage.

This script automates:
- Detection and iteration over scenario input files
- Monte Carlo replications with deterministic seeds
- Organized output storage in hierarchical directories
- Easy configuration for batch experiments

Author: Automated wrapper for DSK-SFC model
Date: February 2026
"""

import os
import sys
import subprocess
import shutil
import glob
import json
from pathlib import Path
from typing import List, Tuple
import argparse


class ScenarioRunner:
    """
    Manages execution of DSK-SFC model across multiple scenarios and MC replications.
    """

    def __init__(
        self,
        scenarios_dir: str = "./inputs/experiment/",
        output_base: str = "./output/experiment/",
        executable: str = "./dsk_SFC",
        n_replications: int = 1,
        seed_base: int = 1000,
        verbose: bool = True,
        full_output: bool = False
    ):
        """
        Initialize the scenario runner.

        Parameters:
        -----------
        scenarios_dir : str
            Directory containing scenario-specific input JSON files
        output_base : str
            Base directory for storing structured outputs
        executable : str
            Path to the DSK-SFC model executable
        n_replications : int
            Number of Monte Carlo replications per scenario
        seed_base : int
            Base value for seed calculation (seed = scenario_idx * seed_base + replication_idx)
        verbose : bool
            If True, print progress messages
        full_output : bool
            If True, pass -f 1 flag to model for full output
        """
        self.scenarios_dir = Path(scenarios_dir)
        self.output_base = Path(output_base)
        self.executable = Path(executable)
        self.n_replications = n_replications
        self.seed_base = seed_base
        self.verbose = verbose
        self.full_output = full_output

        # Validate inputs
        self._validate_setup()

    def _validate_setup(self):
        """Validate that required directories and executable exist."""
        if not self.scenarios_dir.exists():
            raise FileNotFoundError(f"Scenarios directory not found: {self.scenarios_dir}")

        if not self.executable.exists():
            raise FileNotFoundError(f"Model executable not found: {self.executable}")

        if not os.access(self.executable, os.X_OK):
            raise PermissionError(f"Executable is not executable: {self.executable}")

    def extract_scenario_name(self, input_file: Path) -> str:
        """
        Extract scenario name from input filename.
        e.g., 'scenario0_inputs.json' -> 'scenario0'
             'scenario1a_inputs.json' -> 'scenario1a'
        """
        name = input_file.stem  # Get filename without extension
        if name.endswith('_inputs'):
            return name[:-7]  # Remove '_inputs' suffix
        return name

    def get_scenario_files(self) -> List[Tuple[str, Path]]:
        """
        Detect and enumerate all input files in the scenarios directory.

        Returns:
        --------
        List of tuples: (scenario_name, file_path)
        """
        # Find all JSON input files
        pattern = str(self.scenarios_dir / "*.json")
        json_files = sorted(glob.glob(pattern))

        # Filter out non-scenario files (like verification.md converted files)
        scenario_files = [
            f for f in json_files
            if "scenario" in os.path.basename(f).lower() and "inputs" in os.path.basename(f).lower()
        ]

        if not scenario_files:
            raise ValueError(f"No scenario input files found in {self.scenarios_dir}")

        # Extract scenario names from filenames
        enumerated = [(self.extract_scenario_name(Path(f)), Path(f)) for f in scenario_files]

        if self.verbose:
            print(f"\nDetected {len(enumerated)} scenario files:")
            for idx, (scenario_name, filepath) in enumerate(enumerated, 1):
                print(f"  Scenario {idx:02d}: {filepath.name} -> '{scenario_name}'")

        return enumerated

    def calculate_seed(self, scenario_name: str, replication_idx: int) -> int:
        """
        Calculate deterministic seed based on scenario name and replication index.

        Parameters:
        -----------
        scenario_name : str
            Scenario name (e.g., 'scenario0', 'scenario1a')
        replication_idx : int
            Monte Carlo replication number (1-indexed)

        Returns:
        --------
        int : Deterministic seed value
        """
        # Create a consistent hash from scenario name
        scenario_hash = hash(scenario_name) % 1000  # Keep it reasonable
        return abs(scenario_hash) * self.seed_base + replication_idx

    def create_output_structure(self, scenario_name: str, replication_idx: int) -> Path:
        """
        Create hierarchical output directory for a specific scenario and replication.

        Parameters:
        -----------
        scenario_name : str
            Scenario name (e.g., 'scenario0', 'scenario1a')
        replication_idx : int
            Monte Carlo replication number (1-indexed)

        Returns:
        --------
        Path : Directory path for this run's outputs
        """
        scenario_dir = self.output_base / scenario_name
        run_dir = scenario_dir / f"rep_{replication_idx:03d}"
        run_dir.mkdir(parents=True, exist_ok=True)
        return run_dir

    def run_model(
        self,
        input_file: Path,
        scenario_name: str,
        replication_idx: int
    ) -> Tuple[bool, str]:
        """
        Execute the model for a specific scenario and replication.

        Parameters:
        -----------
        input_file : Path
            Path to the scenario input JSON file
        scenario_name : str
            Scenario name (e.g., 'scenario0', 'scenario1a')
        replication_idx : int
            Monte Carlo replication number (1-indexed)

        Returns:
        --------
        Tuple[bool, str] : (success, message)
        """
        # Calculate seed
        seed = self.calculate_seed(scenario_name, replication_idx)

        # Create unique run name using actual scenario name
        run_name = f"{scenario_name}_rep{replication_idx:03d}"

        # Build command - use absolute path for executable
        cmd = [
            str(self.executable.resolve()),
            str(input_file.resolve()),
            "-r", run_name,
            "-s", str(seed)
        ]

        if self.full_output:
            cmd.extend(["-f", "1"])

        # Run model
        if self.verbose:
            print(f"\n  Running: {' '.join(cmd)}")

        try:
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                check=True,
                cwd=str(Path.cwd())
            )

            if self.verbose and result.stdout:
                print(f"  Output: {result.stdout[:200]}...")  # Print first 200 chars

            return True, "Success"

        except subprocess.CalledProcessError as e:
            error_msg = f"Model execution failed: {e.stderr[:500]}"
            print(f"  ERROR: {error_msg}")
            return False, error_msg

    def organize_outputs(
        self,
        scenario_name: str,
        replication_idx: int,
        target_dir: Path
    ):
        """
        Move model output files from default output/ directory to structured directory.

        Parameters:
        -----------
        scenario_name : str
            Scenario name (e.g., 'scenario0', 'scenario1a')
        replication_idx : int
            Monte Carlo replication number (1-indexed)
        target_dir : Path
            Target directory for this run's outputs
        """
        run_name = f"{scenario_name}_rep{replication_idx:03d}"
        seed = self.calculate_seed(scenario_name, replication_idx)

        # Model writes to ./output/ directory, not the organized output directory
        model_output_dir = (Path.cwd() / "output").resolve()

        # Pattern to match output files created by this run
        # Using absolute paths to ensure correct matching regardless of working directory
        patterns = [
            str(model_output_dir / f"*_{run_name}_{seed}.txt"),
            str(model_output_dir / "errors" / f"*_{run_name}_{seed}.txt"),
        ]

        if self.verbose:
            print(f"  Searching for output files with patterns:")
            for pattern in patterns:
                print(f"    {pattern}")

        files_moved = 0
        for pattern in patterns:
            for filepath in glob.glob(pattern):
                src = Path(filepath)
                if src.exists():
                    # Check if file is in errors subdirectory
                    try:
                        rel_path = src.relative_to(model_output_dir)
                        # Preserve subdirectory structure (e.g., errors/)
                        dest = target_dir / rel_path
                    except ValueError:
                        # If not relative to model_output_dir, just use filename
                        dest = target_dir / src.name

                    dest.parent.mkdir(parents=True, exist_ok=True)
                    shutil.move(str(src), str(dest))
                    files_moved += 1

        if self.verbose:
            print(f"  Moved {files_moved} output files to {target_dir}")

    def save_run_metadata(
        self,
        target_dir: Path,
        scenario_name: str,
        replication_idx: int,
        input_file: Path,
        seed: int,
        success: bool
    ):
        """
        Save metadata about this run for reproducibility.

        Parameters:
        -----------
        target_dir : Path
            Directory where metadata should be saved
        scenario_name : str
            Scenario name
        replication_idx : int
            Replication number
        input_file : Path
            Input file used
        seed : int
            Seed used
        success : bool
            Whether the run succeeded
        """
        metadata = {
            "scenario_name": scenario_name,
            "replication_index": replication_idx,
            "input_file": str(input_file.resolve()),
            "seed": seed,
            "success": success,
            "n_replications_total": self.n_replications,
            "executable": str(self.executable.resolve())
        }

        metadata_file = target_dir / "run_metadata.json"
        with open(metadata_file, 'w') as f:
            json.dump(metadata, f, indent=2)

    def run_all_scenarios(self) -> dict:
        """
        Execute model across all scenarios and Monte Carlo replications.

        Returns:
        --------
        dict : Summary statistics of the run
        """
        scenario_files = self.get_scenario_files()

        total_runs = len(scenario_files) * self.n_replications
        completed = 0
        failed = 0

        print(f"\n{'='*70}")
        print(f"Starting batch execution:")
        print(f"  Scenarios: {len(scenario_files)}")
        print(f"  Replications per scenario: {self.n_replications}")
        print(f"  Total runs: {total_runs}")
        print(f"  Output directory: {self.output_base.resolve()}")
        print(f"{'='*70}\n")

        for idx, (scenario_name, input_file) in enumerate(scenario_files, 1):
            print(f"\n[Scenario {idx:02d}: {scenario_name}] Processing {input_file.name}")
            print(f"-" * 70)

            for rep_idx in range(1, self.n_replications + 1):
                # Create output directory structure
                target_dir = self.create_output_structure(scenario_name, rep_idx)

                # Calculate seed
                seed = self.calculate_seed(scenario_name, rep_idx)

                print(f"  [Replication {rep_idx:03d}] Seed: {seed}")

                # Run the model
                success, message = self.run_model(input_file, scenario_name, rep_idx)

                if success:
                    # Organize outputs into structured directory
                    self.organize_outputs(scenario_name, rep_idx, target_dir)
                    completed += 1
                else:
                    failed += 1

                # Save metadata
                self.save_run_metadata(
                    target_dir, scenario_name, rep_idx, input_file, seed, success
                )

        # Summary
        print(f"\n{'='*70}")
        print(f"Batch execution completed:")
        print(f"  Total runs: {total_runs}")
        print(f"  Successful: {completed}")
        print(f"  Failed: {failed}")
        print(f"  Success rate: {100 * completed / total_runs:.1f}%")
        print(f"{'='*70}\n")

        return {
            "total_runs": total_runs,
            "completed": completed,
            "failed": failed,
            "scenarios": len(scenario_files),
            "replications": self.n_replications
        }


def main():
    """Main entry point for the script."""
    parser = argparse.ArgumentParser(
        description="Run DSK-SFC model across multiple scenarios with Monte Carlo simulations",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )

    parser.add_argument(
        "-d", "--scenarios-dir",
        default="./inputs/experiment",
        help="Directory containing scenario input files"
    )

    parser.add_argument(
        "-o", "--output",
        default="./output/experiment",
        help="Base directory for structured outputs"
    )

    parser.add_argument(
        "-e", "--executable",
        default="./dsk_SFC",
        help="Path to model executable"
    )

    parser.add_argument(
        "-n", "--replications",
        type=int,
        default=1,
        help="Number of Monte Carlo replications per scenario"
    )

    parser.add_argument(
        "-s", "--seed-base",
        type=int,
        default=1000,
        help="Base value for seed calculation"
    )

    parser.add_argument(
        "-f", "--full-output",
        action="store_true",
        help="Enable full output mode in model"
    )

    parser.add_argument(
        "-q", "--quiet",
        action="store_true",
        help="Suppress verbose output"
    )

    args = parser.parse_args()

    try:
        runner = ScenarioRunner(
            scenarios_dir=args.scenarios_dir,
            output_base=args.output,
            executable=args.executable,
            n_replications=args.replications,
            seed_base=args.seed_base,
            verbose=not args.quiet,
            full_output=args.full_output
        )

        results = runner.run_all_scenarios()

        # Exit with error code if any runs failed
        if results["failed"] > 0:
            sys.exit(1)
        else:
            sys.exit(0)

    except Exception as e:
        print(f"\nERROR: {str(e)}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
