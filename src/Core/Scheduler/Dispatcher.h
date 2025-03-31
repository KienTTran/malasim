#ifndef DISPATCHER_H
#define    DISPATCHER_H

#include <map>
#include <uuid.h>

class Event;

class Dispatcher {
public:
 // Disallow copy and assign
 Dispatcher(const Dispatcher&) = delete;
 void operator=(const Dispatcher&) = delete;
 Dispatcher(Dispatcher&&) = delete;
 Dispatcher& operator=(Dispatcher&&) = delete;

private:
 // std::vector<Event*> *events_;
 std::map<uuids::uuid, Event*> *events_;
public:
  [[nodiscard]] std::map<uuids::uuid, Event*> *events() const { return events_; }
  void set_events(std::map<uuids::uuid, Event*> *value) { events_ = value; }

  Dispatcher();

  //    Dispatcher(const Dispatcher& orig);
  virtual ~Dispatcher();

  virtual void initialize();

  virtual void add_dispatcher(Event* event);

  virtual void remove_dispatcher(Event* event);

  virtual void clear_dispatcher_events();

  virtual void update_dispatcher();

};

#endif    /* DISPATCHER_H */

