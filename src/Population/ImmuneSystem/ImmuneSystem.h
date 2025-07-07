#ifndef IMMUNESYSTEM_H
#define IMMUNESYSTEM_H

#include <memory>
#include <vector>

#include "Utils/TypeDef.h"

class Model;

class Person;

class ImmuneComponent;

class Config;

// typedef std::vector<ImmuneComponent*> ImmuneComponentPtrVector;

class ImmuneSystem {
  // OBJECTPOOL(ImmuneSystem)
public:
  // Disallow copy
  ImmuneSystem(const ImmuneSystem&) = delete;
  ImmuneSystem& operator=(const ImmuneSystem&) = delete;

  explicit ImmuneSystem(Person* person = nullptr);

  virtual ~ImmuneSystem();

  [[nodiscard]] Person* person() const { return person_; }
  void set_person(Person* person) { person_ = person; }

  [[nodiscard]] ImmuneComponent* immune_component() const;
  void set_immune_component(std::unique_ptr<ImmuneComponent> value);

  [[nodiscard]] bool increase() const { return increase_; }
  void set_increase(bool increase) { increase_ = increase; }

  virtual void draw_random_immune();

  virtual void update();

  [[nodiscard]] virtual double get_latest_immune_value() const;

  virtual void set_latest_immune_value(double value);

  [[nodiscard]] virtual double get_current_value() const;

  [[nodiscard]] virtual double get_parasite_size_after_t_days(const int &duration,
                                                              const double &original_size,
                                                              const double &fitness) const;

  [[nodiscard]] virtual double get_clinical_progression_probability() const;

private:
  Person* person_{nullptr};
  std::unique_ptr<ImmuneComponent> immune_component_{nullptr};
  bool increase_{false};

  //    virtual void clear();
};

#endif /* IMMUNESYSTEM_H */
