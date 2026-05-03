#ifndef FOLLOWUPEPISODETYPES_H
#define FOLLOWUPEPISODETYPES_H

#include <array>
#include <cstdint>

// -------------------------------------------------------------------
// Source categories for repeat clinical episodes within 28 days of
// a first treated clinical episode.
// -------------------------------------------------------------------
enum class FollowupEpisodeSource : uint8_t {
  RecurrenceDisabledNewMosquitoInfection = 0,
  RecurrenceDisabledExistingHostParasite = 1,
  RecurrenceEnabledNewMosquitoInfection = 2,
  RecurrenceEnabledExistingHostParasite = 3,
  RecurrenceEnabledRecrudescence = 4,
  RecurrenceEnabledRecrudescenceHintSuccessIncompatible = 5,
  Count = 6,
  Unknown = 255
};

static constexpr std::array<const char*, 6> FOLLOWUP_EPISODE_SOURCE_SUFFIXES = {
    "recurrence_disabled_new_mosquito_infection",
    "recurrence_disabled_existing_host_parasite",
    "recurrence_enabled_new_mosquito_infection",
    "recurrence_enabled_existing_host_parasite",
    "recurrence_enabled_recrudescence",
    "recurrence_enabled_recrudescence_hint_success_incompatible"};

// -------------------------------------------------------------------
// Hint set at scheduling time so transition_to_clinical_state knows
// how the ProgressToClinicalEvent was created.
// -------------------------------------------------------------------
enum class FollowupEpisodeSourceHint : uint8_t {
  Unknown = 0,
  NewMosquitoInfection = 1,
  ExistingHostParasite = 2,
  Recrudescence = 3
};

// -------------------------------------------------------------------
// Outcome of the first treatment that started the follow-up window.
// -------------------------------------------------------------------
enum class FirstTreatmentOutcome : uint8_t {
  Success = 0,
  Failure = 1
};

static constexpr std::array<const char*, 2> FIRST_TREATMENT_OUTCOME_PREFIXES = {
    "first_treatment_success",
    "first_treatment_failure"};

#endif  // FOLLOWUPEPISODETYPES_H

