#include "ReceiveTherapyEvent.h"

#include "Population/Person/Person.h"
#include "Population/SingleHostClonalParasitePopulations.h"

void ReceiveTherapyEvent::do_execute() {
  auto* person = get_person();
  if (person == nullptr) { throw std::runtime_error("Person is nullptr"); }

  // The clone that caused the clinical episode may have been cleared (and its
  // memory reused) since this event was scheduled; only pass it on if it is
  // still the same clone in this host.
  auto* live_parasite =
      person->get_all_clonal_parasite_populations()->contain(clinical_caused_parasite_,
                                                             clinical_caused_parasite_uid_)
          ? clinical_caused_parasite_
          : nullptr;

  person->receive_therapy(received_therapy_, live_parasite, is_part_of_mac_therapy_);

  person->schedule_update_by_drug_event(live_parasite);
}
