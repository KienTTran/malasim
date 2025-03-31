#include "Dispatcher.h"
#include <Population/Person/Person.h>
#include "Events/Event.h"
#include "Utils/Helpers/ObjectHelpers.h"

Dispatcher::Dispatcher() : events_(nullptr) {}

void Dispatcher::initialize() {
  events_ = new std::map<uuids::uuid, Event*>();
}

Dispatcher::~Dispatcher() {
  Dispatcher::clear_dispatcher_events();
  ObjectHelpers::delete_pointer<std::map<uuids::uuid, Event*>>(events_);
}

void Dispatcher::add_dispatcher(Event* event) {
  events_->insert({event->get_id_raw(), event});
}

void Dispatcher::remove_dispatcher(Event* event) {
  events_->erase(event->get_id_raw());
}

void Dispatcher::clear_dispatcher_events() {
  if (events_== nullptr) return;
  if (events_->empty()) return;
  for (auto &event_pair : *events_) {
    event_pair.second->dispatcher = nullptr;
    event_pair.second->executable = false;
  }
  events_->clear();
}

void Dispatcher::update_dispatcher() {
  // events_->shrink_to_fit();
}
