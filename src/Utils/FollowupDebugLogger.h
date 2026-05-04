#ifndef FOLLOWUP_DEBUG_LOGGER_H
#define FOLLOWUP_DEBUG_LOGGER_H

#ifdef ENABLE_FOLLOWUP_28D_DEBUG

#include <cstdint>
#include <fstream>
#include <string>
#include "Configuration/Config.h"
#include "Simulation/Model.h"

// ----------------------------------------------------------------------------
// Lightweight CSV debug logger for 28-day follow-up counter investigation.
// Compile with -DENABLE_FOLLOWUP_28D_DEBUG to activate.
// Also requires model_settings.enable_debug_followup_28d_report: true in config.
// Output: debug_followup_28d_events.csv in the working directory.
// ----------------------------------------------------------------------------
class FollowupDebugLogger {
public:
  static FollowupDebugLogger& get() {
    static FollowupDebugLogger instance;
    return instance;
  }

  struct Row {
    int day{-1};
    int month{-1};
    uintptr_t person_id{0};
    int location{-1};
    int age{-1};
    int host_state{-1};
    std::string event_stage;
    int first_treatment_day{-1};
    int days_since_first_treatment{-1};
    uintptr_t first_treatment_parasite_ptr{0};
    uintptr_t clinical_caused_parasite_ptr{0};
    bool same_as_first_treatment_parasite{false};
    int therapy_id{-1};
    int first_treatment_therapy_id{-1};
    std::string source_hint;
    std::string classified_source_before_outcome_rule;
    std::string final_recorded_source;  // source after outcome remap (empty when pending, set at flush)
    bool in_28d_window{false};
    bool active_window{false};
    bool outcome_known{false};
    std::string outcome;
    int pending_size_before{0};
    int pending_size_after{0};
    int scheduled_recrudescence_day{-1};
    bool received_treatment{false};
    bool clinical_already_recorded{false};
    // PRD 5.1 new fields
    bool recrudescence_hint_matches_active_window{false};
    bool recrudescence_forces_failure{false};
    std::string recrudescence_mismatch_action;
    bool flush_contains_recrudescence{false};
    bool success_flush_blocked_due_to_recrudescence{false};
    int pending_recrudescence_count{0};
    std::string note;
  };

  void write(const Row& r) {
    if (Model::get_config() == nullptr ||
        !Model::get_config()->get_model_settings().get_enable_debug_followup_28d_report()) {
      return;
    }
    if (!file_.is_open()) open();
    file_ << r.day << "," << r.month << "," << r.person_id << "," << r.location << ","
          << r.age << "," << r.host_state << ","
          << "\"" << r.event_stage << "\","
          << r.first_treatment_day << "," << r.days_since_first_treatment << ","
          << r.first_treatment_parasite_ptr << "," << r.clinical_caused_parasite_ptr << ","
          << (r.same_as_first_treatment_parasite ? 1 : 0) << ","
          << r.therapy_id << "," << r.first_treatment_therapy_id << ","
          << "\"" << r.source_hint << "\","
          << "\"" << r.classified_source_before_outcome_rule << "\","
          << "\"" << r.final_recorded_source << "\","
          << (r.in_28d_window ? 1 : 0) << ","
          << (r.active_window ? 1 : 0) << ","
          << (r.outcome_known ? 1 : 0) << ","
          << "\"" << r.outcome << "\","
          << r.pending_size_before << "," << r.pending_size_after << ","
          << r.scheduled_recrudescence_day << ","
          << (r.received_treatment ? 1 : 0) << ","
          << (r.clinical_already_recorded ? 1 : 0) << ","
          << (r.recrudescence_hint_matches_active_window ? 1 : 0) << ","
          << (r.recrudescence_forces_failure ? 1 : 0) << ","
          << "\"" << r.recrudescence_mismatch_action << "\","
          << (r.flush_contains_recrudescence ? 1 : 0) << ","
          << (r.success_flush_blocked_due_to_recrudescence ? 1 : 0) << ","
          << r.pending_recrudescence_count << ","
          << "\"" << r.note << "\"\n";
    file_.flush();
  }

private:
  std::ofstream file_;

  void open() {
    file_.open("debug_followup_28d_events.csv", std::ios::out | std::ios::trunc);
    file_ << "day,month,person_id,location,age,host_state,event_stage,"
             "first_treatment_day,days_since_first_treatment,"
             "first_treatment_parasite_ptr,clinical_caused_parasite_ptr,"
             "same_as_first_treatment_parasite,therapy_id,first_treatment_therapy_id,"
             "source_hint,classified_source_before_outcome_rule,final_recorded_source,in_28d_window,active_window,"
             "outcome_known,outcome,pending_size_before,pending_size_after,"
             "scheduled_recrudescence_day,received_treatment,clinical_already_recorded,"
             "recrudescence_hint_matches_active_window,recrudescence_forces_failure,"
             "recrudescence_mismatch_action,flush_contains_recrudescence,"
             "success_flush_blocked_due_to_recrudescence,pending_recrudescence_count,"
             "note\n";
  }
};

// Convenience macro to convert enum to string
inline const char* source_hint_to_str(int h) {
  switch (h) {
    case 0: return "Unknown";
    case 1: return "NewMosquitoInfection";
    case 2: return "ExistingHostParasite";
    case 3: return "Recrudescence";
    default: return "?";
  }
}

inline const char* source_to_str(int s) {
  switch (s) {
    case 0: return "RecurrenceDisabledNewMosquitoInfection";
    case 1: return "RecurrenceDisabledExistingHostParasite";
    case 2: return "RecurrenceEnabledNewMosquitoInfection";
    case 3: return "RecurrenceEnabledExistingHostParasite";
    case 4: return "RecurrenceEnabledRecrudescence";
    case 5: return "RecurrenceEnabledRecrudescenceHintSuccessIncompatible";
    default: return "Unknown";
  }
}

inline const char* outcome_to_str(int o) {
  return (o == 0) ? "Success" : "Failure";
}

#endif  // ENABLE_FOLLOWUP_28D_DEBUG
#endif  // FOLLOWUP_DEBUG_LOGGER_H

