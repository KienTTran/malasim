#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <chrono>
#include "date/date.h"
#include "Simulation/Model.h" // Assuming Model is defined in a separate header file
#include "Utils/TypeDef.hxx"
#include "Core/Scheduler/EventManager.h"
#include "Utils/Helpers/StringHelpers.h"

class Scheduler {
public:
  // Disable copy and assignment
  Scheduler(const Scheduler&) = delete;
  void operator=(const Scheduler&) = delete;
  Scheduler(Scheduler&&) = delete;
  Scheduler& operator=(Scheduler&&) = delete;

private:
  int current_time_;
  Model* model_;
  bool is_force_stop_;
  date::sys_days calendar_date_;
  EventManager<WorldEvent> world_events_;  // Use EventManager for world/population events

public:
  explicit Scheduler(Model *model = nullptr);
  virtual ~Scheduler();

  // Getter and Setter for current_time
  [[nodiscard]] int current_time() const { return current_time_; }
  void set_current_time(int time) { current_time_ = time; }

  // Getter and Setter for model
  [[nodiscard]] Model* model() const { return model_; }
  void set_model(Model* model) { model_ = model; }

  // Getter and Setter for is_force_stop
  [[nodiscard]] bool is_force_stop() const { return is_force_stop_; }
  void set_is_force_stop(bool force_stop) { is_force_stop_ = force_stop; }



  void extend_total_time(int new_total_time);

  // Event management methods
  void clear_all_events() {
    world_events_.get_events().clear();
  }

  virtual void schedule_population_event(WorldEvent* event) {
    if (event) {
      world_events_.schedule_event(event);
    }
  }

  virtual void cancel(WorldEvent* event) {
    if (event) {
      world_events_.cancel_all_events_except(event);
    }
  }

  // Core simulation methods
  void initialize(const date::year_month_day &starting_date,
    const date::year_month_day &ending_date);

  void run();
  void begin_time_step();
  void end_time_step();
  void daily_update();

  // Time-related query methods
  [[nodiscard]] bool can_stop();
  [[nodiscard]] int get_current_day_in_year();
  [[nodiscard]] unsigned int get_current_month_in_year();
  [[nodiscard]] bool is_today_last_day_of_month();
  [[nodiscard]] bool is_today_first_day_of_month();
  [[nodiscard]] bool is_today_first_day_of_year();
  [[nodiscard]] bool is_today_last_day_of_year();
  [[nodiscard]] int get_days_to_next_year() const;
  [[nodiscard]] int get_days_to_next_n_year(int n) const;
  [[nodiscard]] int get_days_in_current_month() const;
  [[nodiscard]] int get_current_year() const;
  [[nodiscard]] unsigned int get_current_day_of_month() const;
  [[nodiscard]] date::year_month_day get_ymd_after_months(int months) const;
  [[nodiscard]] int get_days_to_ymd(const date::year_month_day& ymd) const;
  [[nodiscard]] date::year_month_day get_ymd_after_days(int days) const;
  [[nodiscard]] int get_unix_time() const;
  [[nodiscard]] date::year_month_day get_calendar_date() const;
  [[nodiscard]] std::string get_current_date_string() const {
    return StringHelpers::date_as_string(date::year_month_day{calendar_date_});
  }
  // Access to event manager
  EventManager<WorldEvent>& get_world_events() { return world_events_; }
  const EventManager<WorldEvent>& get_world_events() const { return world_events_; }
};

#endif  /* SCHEDULER_H */
