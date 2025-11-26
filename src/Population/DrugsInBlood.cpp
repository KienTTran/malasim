#include "DrugsInBlood.h"

#include "Events/Event.h"
#include "Population/Person/Person.h"
#include "Treatment/Therapies/Drug.h"
#include "Treatment/Therapies/DrugType.h"
#include "Utils/TypeDef.h"

#ifndef DRUG_CUT_OFF_VALUE
#define DRUG_CUT_OFF_VALUE 0.1
#endif

DrugsInBlood::DrugsInBlood(Person* person) : person_(person) {}

void DrugsInBlood::init() { drugs_.clear(); }

DrugsInBlood::~DrugsInBlood() {
  if (!drugs_.empty()) { clear(); }
}

Drug* DrugsInBlood::add_drug(std::unique_ptr<Drug> drug) {
  int type_id = drug->drug_type()->id();
  drug->set_person_drugs(this);

  // Create unique_ptr from raw pointer
  auto [it, inserted] = drugs_.insert_or_assign(type_id, std::move(drug));
  return it->second.get();
}

// V5-like behavior
// Drug* DrugsInBlood::add_drug(std::unique_ptr<Drug> drug) {
//   int type_id = drug->drug_type()->id();
//
//   auto it = drugs_.find(type_id);
//   if (it == drugs_.end()) {
//     // No existing drug of this type: store the new one
//     drug->set_person_drugs(this);
//     auto [iter, inserted] = drugs_.emplace(type_id, std::move(drug));
//     return iter->second.get();
//   }
//
//   // Already have a drug of this type: update existing one (v5 behavior)
//   Drug* existing = it->second.get();
//
//   existing->set_starting_value(drug->starting_value());
//   existing->set_dosing_days(drug->dosing_days());
//   existing->set_last_update_value(drug->last_update_value());
//   existing->set_last_update_time(drug->last_update_time());
//   existing->set_start_time(drug->start_time());
//   existing->set_end_time(drug->end_time());
//
//   // person_drugs pointer should already be set on existing, but safe to enforce:
//   existing->set_person_drugs(this);
//
//   // `drug` goes out of scope and is destroyed here (like `delete drug;` in v5)
//   return existing;
// }

std::size_t DrugsInBlood::size() const { return drugs_.size(); }

void DrugsInBlood::clear() {
  if (drugs_.empty()) return;
  drugs_.clear();
}

void DrugsInBlood::update() {
  for (auto &drug : *this) { drug.second->update(); }
}

void DrugsInBlood::clear_cut_off_drugs() {
  if (!drugs_.empty()) {
    for (auto pos = drugs_.begin(); pos != drugs_.end();) {
      if (pos->second->last_update_value() <= DRUG_CUT_OFF_VALUE) {
        drugs_.erase(pos++);
      } else {
        ++pos;
      }
    }
  }
}
