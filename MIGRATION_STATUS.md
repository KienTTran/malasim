# Test Migration Status

## ✅ Completed
- **28 test files migrated** (~189 tests)
- **All individual test suites pass**  
- **No tests depend on sample_inputs/input.yml**

## 🔧 Fixes Applied
1. **District raster generation** - Now creates 3 districts (IDs 1, 2, 3)
2. **Test cleanup** - Runs at start of setup to remove stale files
3. **DistrictMftStrategyTest.GetTherapyForPerson** - Now passing

## ⚠️ Known Issues
### Full Test Suite Segfault
- **Individual tests**: ✅ All pass
- **Small groups**: ✅ Pass
- **Full suite**: ❌ Segfaults after ~10 seconds

This appears to be a cumulative resource issue, not a problem with individual test logic.

### Workaround
```bash
# Copy template file manually (POST_BUILD not working)
cp tests/fixtures/test_input_template.yml build/bin/

# Run tests individually or in groups
./build/bin/malasim_test --gtest_filter="*DrugTest*"
./build/bin/malasim_test --gtest_filter="PopulationEventsTest.*:GenotypeTest.*"
```

## 📊 Migration Summary by Category
- Configuration: 4 tests  
- Parasites: 3 tests
- Population: 19 tests
- MDC: 10 tests
- Treatment/Therapies: 50 tests
- Treatment/Strategies: 85 tests  
- Treatment/LinearTCM: 6 tests
- Spatial/Movement: 12 tests

**Total: ~189 tests across 28 files**

## 🎯 Success Criteria Met
✅ No external file dependencies
✅ Self-contained test environment
✅ Programmatic file generation  
✅ Individual test reliability

## 🔍 Next Investigation
The full suite segfault needs deeper investigation:
- Memory leak?
- Resource exhaustion?
- Test interaction/ordering issue?
- Model singleton state corruption?

