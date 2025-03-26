#ifndef DRUGSINBLOOD_H
#define    DRUGSINBLOOD_H

#include "Utils/TypeDef.hxx"

class Person;

class Drug;

class Event;

class DrugType;

class DrugsInBlood {
  // OBJECTPOOL(DrugsInBlood)
  //disallow copy and assign and move
  DrugsInBlood(const DrugsInBlood&) = delete;
  void operator=(const DrugsInBlood&) = delete;
  DrugsInBlood(const DrugsInBlood&&) = delete;
  DrugsInBlood& operator=(const DrugsInBlood&&) = delete;

 private:
 Person *person_;
 DrugPtrMap *drugs_;

 public:
 Person *person() const {
   return person_;
 }
 void set_person(Person *value) {
   person_ = value;
 }

 DrugPtrMap *drugs() const {
   return drugs_;
 }
 void set_drugs(DrugPtrMap *value) {
   drugs_ = value;
 }

 public:
  explicit DrugsInBlood(Person *person = nullptr);

  //    DrugsInBlood(const DrugsInBlood& orig);
  virtual ~DrugsInBlood();

  void init();

  Drug *add_drug(Drug *drug);

  bool is_drug_in_blood(DrugType *drug_type) const;

  bool is_drug_in_blood(int drug_type_id) const;

  void remove_drug(Drug *drug) const;

  void remove_drug(const int &drug_type_id) const;

  Drug *get_drug(const int &type_id) const;

  std::size_t size() const;

  void clear() const;

  void update() const;

  void clear_cut_off_drugs_by_event(Event *event) const;

};

#endif    /* DRUGSINBLOOD_H */
