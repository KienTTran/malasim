# Changelog v6.1

## Features

- **Memory Statistics**: Added `--memory-stats` CLI option to print sizeof for key classes (Person, ClonalParasitePopulation, Population) and exit without loading input or running simulation
- **CNV Reversion**: Added daily copy-number reversion (`n -> n - 1`, clamped at 1) for CNV genes outside the existing forward drug-mutation path
  - Supports per-gene `cnv_reversion_multiplier` and `genotype_parameters.default_cnv_reversion_multiplier` as a fallback
  - Both multipliers are validated in `[0, 10]` (the upper bound is a guardrail; reverse rates can exceed the forward rate)
  - Per-gene multipliers may only be set on genes with `max_copies > 1`
  - Applies to current CNV examples `Pfmdr1` and `Pfplasmepsin`
  - Skips reversion when an active drug with concentration > 0 has a CNV EC50 factor that favors copy 2 for the affected gene
  - Precomputes a CNV gene index in `PfGenotypeInfo` so the daily reversion step only iterates genes with `max_copies > 1`
  - Successful reversions are recorded through the existing mutation tracking path

## Bug Fixes

- **Population out-of-bounds access**: Added guards for `K_INVALID_LOCATION_ID` in `Population::add_person()` and `remove_person()` to prevent crashes when Person is created with default constructor (location_ = K_INVALID_LOCATION_ID)
- **MFTAgeBasedStrategyTest**: Reduced simulation time from 100 to 80 years to fit in `core::SimDay` (int16_t max ~89 years)
- **RouletteTest test pollution**: Added `Model::get_instance()->release()` in SetUp/TearDown to properly clean up singleton state between tests; changed from `set_location(i)` to `set_number_of_times_bitten(i)` as temporary identifier to avoid side effects
- **Roulette sampling safety**: Recompute sampling totals from weights, reject mismatched object/weight vectors and invalid weights, and clamp rounded upper-bound draws so valid samples are not returned as `nullptr`; retain zero-total sampling disable behavior
- **Population sampling totals after deaths**: Recalculate FOI, biting, and movement totals from surviving people to avoid floating-point residue or loss of small positive weights

## Refactor

- **CLI Refactor**: Removed singleton pattern from `Cli` class
  - `MaSimAppInput` and `DxGAppInput` are now standalone structs in `utils` namespace
  - Added static `Cli::parse_args()` and `Cli::parse_dxg_args()` methods
  - `Model` now owns `cli_input_` member with `set_cli_input()`/`get_cli_input()` accessors
  - `Config` and `ReporterUtils` now use `Model::get_cli_input()` instead of `Cli::get_instance()`
  - Main executable and tests updated to use new pattern

- **Age and Age Class Types**: Changed `age_` and `age_class_` member variables in `Person` class from `int` to `uint8_t` with symbolic sentinel values
  - Added `Age = uint8_t` and `AgeClass = uint8_t` typedefs in `Person.h`
  - Added `K_INVALID_AGE` (255) and `K_INVALID_AGE_CLASS` (255) constants
  - Updated `Person::age_` and `Person::age_class_` to use new types with appropriate sentinel values

- **Sentinel Comparisons**: Fixed comparisons using `-1` sentinel to use symbolic constants
  - `Person.cpp`: `age_class_ == -1` → `age_class_ == K_INVALID_AGE_CLASS`
  - `Population.h/cpp`: `age_class == -1` → `age_class == K_INVALID_AGE_CLASS`

- **Tests Updated**: `PersonBasicTest.cpp` updated to use `K_INVALID_AGE` constant

- **Test logging**: Changed default log level from `info` to `warn`; set `MALASIM_LOG_LEVEL` env var to control logging (e.g., `MALASIM_LOG_LEVEL=info` for verbose output)

## Files Changed

- `src/Configuration/GenotypeParameters.h` - Added `cnv_reversion_multiplier` and `default_cnv_reversion_multiplier` config support plus `PfGenotypeInfo::CnvGeneIndex` precomputed index
- `src/Configuration/GenotypeParameters.cpp` - Added `validate_cnv_reversion_multipliers` static helper
- `src/Configuration/Config.cpp` - Wire CNV reversion multiplier validation through the new helper
- `src/Parasites/Genotype.h` - Added CNV reversion helper declaration
- `src/Parasites/Genotype.cpp` - Added `n -> n - 1` CNV reversion logic with drug-selection checks; refactored forward mutation to use a uniform `proposed = old +/- 1` rule clamped to `[1, max_copies]`
- `src/Population/SingleHostClonalParasitePopulations.h` - Added separate daily `apply_cnv_reversion()` path
- `src/Population/SingleHostClonalParasitePopulations.cpp` - Applied CNV reversion and mutation recording
- `src/Population/Person/Person.cpp` - Invokes daily CNV reversion during host update
- `tests/Configuration/yaml_genotype_parameters_conversion_test.cpp` - Added coverage for CNV reversion config fields and out-of-bound multiplier validation
- `tests/Parasites/GenotypeTest.cpp` - Added CNV reversion behavior, fallback, and forward-mutation preservation tests
- `tests/fixtures/test_input_template.yml` - Added CNV reversion example config
- `sample_inputs/*.yml` - Added CNV reversion example config to shipped sample inputs
- `docs/cnv-reversion.md` - Added feature documentation
- `docs/cnv-reversion-plan.md` - Updated plan to match implemented behavior
- `src/Utils/Cli.h` - Added `print_memory_stats` field and `--memory-stats` flag
- `src/Utils/Random.h` - Share validated roulette sampling logic between pointer and tuple APIs
- `tests/Core/Random/RandomTest_sampling_edges.cpp` - Add regressions for stale totals, mismatched vectors, invalid weights, and subnormal weights
- `tests/Population/DailyLocationPipelineTest.cpp` - Cover sampling total recalculation after person deaths
- `src/malasim/main.cpp` - Added memory stats printing and early exit
- `tests/Utils/CliTest.cpp` - Added tests for `--memory-stats` flag
- `src/Utils/Cli.h` - Removed singleton, converted to static utility class
- `src/Simulation/Model.h` - Added cli_input_ member and accessors
- `src/Simulation/Model.cpp` - Use cli_input_ member
- `src/Configuration/Config.cpp` - Use Model::get_cli_input()
- `src/Reporters/Utility/ReporterUtils.cpp` - Use Model::get_cli_input()
- `src/malasim/main.cpp` - Use Cli::parse_args()
- `EfficacyEstimator/DxGGenerator_main.cpp` - Use Cli::parse_dxg_args()
- `tests/Utils/CliTest.cpp` - New tests for CLI parsing
- `tests/Population/Person/PersonTestBase.h` - Updated to use new CLI pattern
- `tests/Events/ProgressToClinicalEventTest.cpp` - Updated to use new CLI pattern
- `tests/Population/PopulationGenerateIndividualTest.cpp` - Updated to use new CLI pattern
- `tests/MDC/ModelDataCollectorTest.cpp` - Updated to use new CLI pattern
- `tests/Population/Person/PersonRecrudescenceTest.cpp` - Updated to use new CLI pattern
- `tests/Population/Person/PersonEventTest.cpp` - Updated to use new CLI pattern
- `tests/Population/DrugsInBloodTest.cpp` - Updated to use new CLI pattern
- `tests/Treatment/LinearTCMTest.cpp` - Updated to use new CLI pattern
- `tests/Treatment/Strategies/*.cpp` (14 files) - Updated to use new CLI pattern
- `tests/Treatment/Therapies/*.cpp` (6 files) - Updated to use new CLI pattern
- `tests/Spatial/Movement/*.cpp` (4 files) - Updated to use new CLI pattern
- `tests/Configuration/yaml_population_events_conversion_test.cpp` - Updated to use new CLI pattern
- `tests/README.md` - Documentation updates
- `CHANGELOG-6.1.md` - This changelog

## Files Changed (from v6.0)

- `src/Population/Person/Person.h` - Added type aliases and constants
- `src/Population/Person/Person.cpp` - Fixed sentinel comparison
- `src/Population/Population.h` - Updated default parameter
- `src/Population/Population.cpp` - Fixed sentinel comparison
- `tests/Population/Person/PersonBasicTest.cpp` - Updated test assertion
- `tests/Core/Random/RandomTest_random_roulette.cpp` - Fixed test pollution and identifier overflow
- `tests/Treatment/Strategies/MFTAgeBasedStrategyTest.cpp` - Fixed simulation time overflow
- `tests/SpdlogEnvironment.cpp` - Added MALASIM_LOG_LEVEL env var support
