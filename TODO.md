# Review

- p_infection_from_infectious_bite there are 2 models and need to refactor to unify them.
- transmission_parameter is not a clear name.

- Check why Population initialize takes soo long.
- RandomTest.random_multinomial_DistributionMatchesMultinomial_ChiSquaredTest sometimes failed????

- Write test for approx_norm_cdf to compare with GSL version
- Improve speed for RouletteTest.compare_with_multi_normial
- location_based settings is not working yet
- need rework on spatial data and the order of parsing method
- review void MosquitoParameters::process_config_using_locations(std::vector<Spatial::Location>& locations)

- need rework on BurkinaFasoSM or other SM to support single location / location based

- consider move age_distribution back to population demographics settings as normally we do not have very detail data up to 5x5 km.

- p_infection_from_infectious_bite biology validation
- relative_infectivity biology validation

- In config population demographic, why number_of_age_classes have to be set before the vector age_clasess?

- movement_settings: as we normally not change it much due to lack of data?
- Events in Person need to review? x Done
- Need unittest for event queue

# Recrudescence

- A seperate simulation to validate
- add number of recurrence treatments to reporter

# Unittest

- ImmuneSystem

- If there is an old test failed. Try Model::get_instance()->initialize()

# Random class

- [ ] Random unit tests

# Project wide

- [ ] add gcov
- [ ] add CI/Coverage
- [ ] choose a logging library
