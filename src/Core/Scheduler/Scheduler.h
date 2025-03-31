#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <chrono>
#include "date/date.h"
#include "Simulation/Model.h"
#include "Utils/TypeDef.hxx"

class Scheduler {
public:
  // Disable copy and assignment
  Scheduler(const Scheduler&) = delete;
  void operator=(const Scheduler&) = delete;
  Scheduler(Scheduler&&) = delete;
  Scheduler& operator=(Scheduler&&) = delete;

  int current_time_;
  int total_available_time_;
  Model* model_;
  bool is_force_stop_;

  date::sys_days calendar_date;

  // EventPtrVector2 individual_events_list_;
  EventPtrVector2 population_events_list_;

  explicit Scheduler(Model *model = nullptr);

  virtual ~Scheduler();

  // Getter and Setter for current_time
  [[nodiscard]] int current_time() const { return current_time_; }
  void set_current_time(const int time) { current_time_ = time; }

  // Getter and Setter for total_available_time
  [[nodiscard]] int total_available_time() const { return total_available_time_; }
  void set_total_available_time(const int& value);

  // Getter and Setter for model
  [[nodiscard]] Model* model() const { return model_; }
  void set_model(Model* model) { model_ = model; }

  // Getter and Setter for is_force_stop
  [[nodiscard]] bool is_force_stop() const { return is_force_stop_; }
  void set_is_force_stop(const bool force_stop) { is_force_stop_ = force_stop; }

  void extend_total_time(int new_total_time);

  void clear_all_events();

  void clear_all_events(EventPtrVector2 &events_list);
//
  // virtual void schedule_individual_event(Event *event);

  virtual void schedule_population_event(Event *event);

  virtual void schedule_event(EventPtrVector &time_events, Event *event);

  virtual void cancel(Event *event);

  void execute_events_list(EventPtrVector &events_list);

  void initialize(const date::year_month_day &starting_date,
    const date::year_month_day &ending_date);

  void run();

  void begin_time_step();

  void end_time_step();

  [[nodiscard]] bool can_stop();

  [[nodiscard]] int current_day_in_year();

  [[nodiscard]] int current_month_in_year();

  [[nodiscard]] bool is_today_last_day_of_month();

  [[nodiscard]] bool is_today_first_day_of_month();

  [[nodiscard]] bool is_today_first_day_of_year();

  [[nodiscard]] bool is_today_last_day_of_year();

  void daily_update();
};

#endif  /* SCHEDULER_H */
