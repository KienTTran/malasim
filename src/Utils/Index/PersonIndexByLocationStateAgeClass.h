#ifndef PERSONINDEXBYLOCATIONSTATEAGECLASS_H
#define    PERSONINDEXBYLOCATIONSTATEAGECLASS_H

#include "Utils/TypeDef.h"
#include "Population/Person/Person.h"
#include "PersonIndex.h"

class PersonIndexByLocationStateAgeClass : public PersonIndex {
public:
  //disable copy and assign
  PersonIndexByLocationStateAgeClass(const PersonIndexByLocationStateAgeClass &) = delete;
  void operator=(const PersonIndexByLocationStateAgeClass &) = delete;

  PersonPtrVector4 &vPerson() {
    return vPerson_;
  }
  void vPerson(const PersonPtrVector4 &v) {
    vPerson_ = v;
  }

private:
  PersonPtrVector4 vPerson_;
  std::vector<std::size_t> living_by_location_;

 public:
  //    PersonIndexByLocationStateAgeClass();
  PersonIndexByLocationStateAgeClass(const int &no_location = 1, const int &no_host_state = 1,
                                     const int &no_age_class = 1);

  //    PersonIndexByLocationStateAgeClass(const PersonIndexByLocationStateAgeClass& orig);
  virtual ~PersonIndexByLocationStateAgeClass();

  void Initialize(const int &no_location = 1, const int &no_host_state = 1, const int &no_age_class = 1);

  virtual void add(Person *p);

  virtual void remove(Person *p);

  virtual std::size_t size() const;

  // Running count of NON-DEAD people per location, maintained in lockstep with
  // the vPerson_ buckets in add()/remove_without_set_index(). Equals
  //   sum over states 0..DEAD-1, all age classes, of vPerson_[loc][s][ac].size()
  // by construction, so it is a bit-identical O(1) replacement for that sum.
  // Verified equal to size(location) at every birth/circulation call across a
  // full 4740-day run on the ago grid.
  [[nodiscard]] std::size_t living_at(int location) const {
    return living_by_location_[location];
  }

  virtual void update();

  virtual void notify_change(Person *p, const Person::Property &property, const void *oldValue, const void *newValue);

 private:
  void remove_without_set_index(Person *p);

  void add(Person *p, core::LocationId location, const Person::HostStates &host_state, core::AgeClass age_class);

  void change_property(Person *p, core::LocationId location, const Person::HostStates &host_state, core::AgeClass age_class);
};

#endif    /* PERSONINDEXBYLOCATIONSTATEAGECLASS_H */

