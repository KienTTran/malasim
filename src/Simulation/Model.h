#ifndef MODEL_H
#define MODEL_H

#include <Utils/Random.h>

#include <cstddef>
#include <memory>

#include "Configuration/Config.h"
#include "Core/Scheduler/Scheduler.h"
#include "MDC/ModelDataCollector.h"
#include "Mosquito/Mosquito.h"
#include "Population/ClinicalUpdateFunction.h"
#include "Population/ImmuneSystem/ImmunityClearanceUpdateFunction.h"
#include "Population/Population.h"
#include "Reporters/Reporter.h"
#include "Treatment/ITreatmentCoverageModel.h"
#include "Treatment/Strategies/IStrategy.h"
#include "Treatment/Therapies/DrugDatabase.h"

#include "Agents/AdaptiveCyclingAgent.h"

namespace Spatial {
class Location;
}

class Cli;
class Model {
public:
  // Provides global access to the singleton instance
  static Model* get_instance() {
    static Model instance;
    return &instance;
  }

  // Initialize the model
  bool initialize();

  // Prevent copying and moving
  Model(const Model &) = delete;
  Model& operator=(const Model &) = delete;
  Model(Model &&) = delete;
  Model &operator=(Model &&) = delete;

private:
  // Private constructor and destructor
  // Model(const int &object_pool_size = 100000);
  Model() = default;
  ~Model() = default;

  bool is_initialized_{false};

  std::unique_ptr<Config> config_{nullptr};
  std::unique_ptr<Scheduler> scheduler_{nullptr};
  std::unique_ptr<Population> population_{nullptr};
  std::unique_ptr<utils::Random> random_{nullptr};
  std::unique_ptr<ModelDataCollector> mdc_{nullptr};
  std::unique_ptr<Mosquito> mosquito_{nullptr};
  std::unique_ptr<ClinicalUpdateFunction> progress_to_clinical_update_function_{nullptr};
  std::unique_ptr<ImmunityClearanceUpdateFunction> immunity_clearance_update_function_{nullptr};
  std::unique_ptr<ImmunityClearanceUpdateFunction> having_drug_update_function_{nullptr};
  std::unique_ptr<ImmunityClearanceUpdateFunction> clinical_update_function_{nullptr};
  std::unique_ptr<ITreatmentCoverageModel> treatment_coverage_{nullptr};

  std::unique_ptr<GenotypeDatabase> genotype_db_{nullptr};
  std::unique_ptr<DrugDatabase> drug_db_{nullptr};

  std::vector<std::unique_ptr<Reporter>> reporters_;
  std::vector<std::unique_ptr<IStrategy>> strategy_db_;
  std::vector<std::unique_ptr<Therapy>> therapy_db_;

  std::unique_ptr<AdaptiveCyclingAgent> adaptive_cycling_agent_{nullptr};

  IStrategy* treatment_strategy_{nullptr};

public:
  void before_run();
  void run();
  void after_run();
  void begin_time_step();
  void end_time_step();
  void daily_update();
  void monthly_update();
  void yearly_update();
  void release();

  static Config* get_config() { return get_instance()->config_.get(); }
  static void set_config(std::unique_ptr<Config> config) {
    get_instance()->config_ = std::move(config);
  }

  static Scheduler* get_scheduler() { return get_instance()->scheduler_.get(); }

  static void set_scheduler(std::unique_ptr<Scheduler> scheduler) {
    get_instance()->scheduler_ = std::move(scheduler);
  }

  static utils::Random* get_random() { return get_instance()->random_.get(); }

  static void set_random(std::unique_ptr<utils::Random> random) {
    get_instance()->random_ = std::move(random);
  }

  static Population* get_population() { return get_instance()->population_.get(); }
  static void set_population(std::unique_ptr<Population> population) {
    get_instance()->population_ = std::move(population);
  }

  static GenotypeDatabase* get_genotype_db() { return get_instance()->genotype_db_.get(); }
  static void set_genotype_db(std::unique_ptr<GenotypeDatabase> genotype_db) {
    get_instance()->genotype_db_ = std::move(genotype_db);
  }

  static DrugDatabase* get_drug_db() { return get_instance()->drug_db_.get(); }
  static void set_drug_db(std::unique_ptr<DrugDatabase> value) {
    get_instance()->drug_db_ = std::move(value);
  }

  static SpatialData* get_spatial_data() {
    return Model::get_config()->get_spatial_settings().spatial_data();
  }

  static std::vector<std::unique_ptr<Therapy>> &get_therapy_db() {
    return get_instance()->therapy_db_;
  }

  static std::vector<std::unique_ptr<IStrategy>> &get_strategy_db() {
    return get_instance()->strategy_db_;
  }

  static ModelDataCollector* get_mdc() { return get_instance()->mdc_.get(); }
  static void set_mdc(ModelDataCollector* mdc) { get_instance()->mdc_.reset(mdc); }

  static Mosquito* get_mosquito() { return get_instance()->mosquito_.get(); }
  static void set_mosquito(Mosquito* mosquito) { get_instance()->mosquito_.reset(mosquito); }

  static ClinicalUpdateFunction* progress_to_clinical_update_function() {
    return get_instance()->progress_to_clinical_update_function_.get();
  }
  static void set_progress_to_clinical_update_function(ClinicalUpdateFunction* function) {
    get_instance()->progress_to_clinical_update_function_.reset(function);
  }

  static ImmunityClearanceUpdateFunction* having_drug_update_function() {
    return get_instance()->having_drug_update_function_.get();
  }
  static void set_having_drug_update_function(ImmunityClearanceUpdateFunction* function) {
    get_instance()->having_drug_update_function_.reset(function);
  }

  static ImmunityClearanceUpdateFunction* immunity_clearance_update_function() {
    return get_instance()->immunity_clearance_update_function_.get();
  }
  static void set_immunity_clearance_update_function(ImmunityClearanceUpdateFunction* function) {
    get_instance()->immunity_clearance_update_function_.reset(function);
  }

  static ImmunityClearanceUpdateFunction* clinical_update_function() {
    return get_instance()->clinical_update_function_.get();
  }
  static void set_clinical_update_function(ImmunityClearanceUpdateFunction* function) {
    get_instance()->clinical_update_function_.reset(function);
  }

  static ITreatmentCoverageModel* get_treatment_coverage();
  void set_treatment_coverage(std::unique_ptr<ITreatmentCoverageModel> tcm);

  static IStrategy* get_treatment_strategy();
  void set_treatment_strategy(const int &strategy_id);

  static AdaptiveCyclingAgent* get_adc_agent() {
    return get_instance()->adaptive_cycling_agent_.get();
  }

  void build_initial_treatment_coverage();
  void monthly_report();
  void report_begin_of_time_step();
  void add_reporter(std::unique_ptr<Reporter> reporter);

  std::vector<std::unique_ptr<Reporter>>& get_reporters();
};

#endif  // MODEL_H

