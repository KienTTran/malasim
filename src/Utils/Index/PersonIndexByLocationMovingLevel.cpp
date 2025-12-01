#include "PersonIndexByLocationMovingLevel.h"

#include <cassert>

#include "Configuration/Config.h"
#include "Simulation/Model.h"

PersonIndexByLocationMovingLevel::PersonIndexByLocationMovingLevel(const int &no_location,
                                                                   const int &no_level) {
  Initialize(no_location, no_level);
}

PersonIndexByLocationMovingLevel::~PersonIndexByLocationMovingLevel() { vPerson_.clear(); }

void PersonIndexByLocationMovingLevel::Initialize(const int &no_location, const int &no_level) {
  vPerson_.clear();

  PersonPtrVector ppv;
  PersonPtrVector2 ppv2;
  ppv2.assign(no_level, ppv);

  vPerson_.assign(no_location, ppv2);
}

void PersonIndexByLocationMovingLevel::add(Person* person) {
  assert(vPerson_.size() > person->get_location() && person->get_location() >= 0);
  assert(vPerson_[person->get_location()].size() > person->get_moving_level());
  add(person, person->get_location(), person->get_moving_level());
}

void PersonIndexByLocationMovingLevel::remove(Person* person) {
  remove_without_set_index(person);
  person->PersonIndexByLocationMovingLevelHandler::set_index(-1);
}

void PersonIndexByLocationMovingLevel::notify_change(Person* person,
                                                     const Person::Property &property,
                                                     const void* oldValue, const void* newValue) {
  switch (property) {
    case Person::LOCATION:
      change_property(person, *(int*)newValue, person->get_moving_level());
      break;
    case Person::MOVING_LEVEL:
      change_property(person, person->get_location(), *(int*)newValue);
      break;
    default:
      break;
  }
}

std::size_t PersonIndexByLocationMovingLevel::size() const {
  std::size_t total = 0;
  for (const auto &by_loc : vPerson_) {
    for (const auto &by_level : by_loc) {
      total += by_level.size();
    }
  }
  return total;
}


void PersonIndexByLocationMovingLevel::add(Person* person, const int &location,
                                           const int &moving_level) {
  vPerson_[location][moving_level].push_back(person);
  person->PersonIndexByLocationMovingLevelHandler::set_index(vPerson_[location][moving_level].size()
                                                             - 1);
}

void PersonIndexByLocationMovingLevel::remove_without_set_index(Person* person) {
  vPerson_[person->get_location()][person->get_moving_level()]
      .back()
      ->PersonIndexByLocationMovingLevelHandler::set_index(
          person->PersonIndexByLocationMovingLevelHandler::get_index());
  vPerson_[person->get_location()][person->get_moving_level()]
          [person->PersonIndexByLocationMovingLevelHandler::get_index()] =
              vPerson_[person->get_location()][person->get_moving_level()].back();
  vPerson_[person->get_location()][person->get_moving_level()].pop_back();
}

void PersonIndexByLocationMovingLevel::change_property(Person* person, const int &location,
                                                       const int &moving_level) {
  // remove from old position
  remove_without_set_index(
      person);  // to save 1 set and improve performance since the index of p will changed when add

  // add to new position
  add(person, location, moving_level);
}

void PersonIndexByLocationMovingLevel::update() {
  for (int location = 0; location < Model::get_config()->number_of_locations(); location++) {
    for (int ml = 0; ml < Model::get_config()
                              ->get_movement_settings()
                              .get_circulation_info()
                              .get_number_of_moving_levels();
         ml++) {
      std::vector<Person*>(vPerson_[location][ml]).swap(vPerson_[location][ml]);
    }
  }
}
