#ifndef TYPEDEF_H
#define TYPEDEF_H

#include <cstdint>
#include <map>
#include <string>
#include <vector>

class Person;

class PersonIndex;

class Event;

class Reporter;

class Drug;

class IStrategy;

class Therapy;

using ul = unsigned long;
using Ul = uint64_t;

using DoubleVector = std::vector<double>;
using DoubleVector2 = std::vector<DoubleVector>;
using DoubleVector3 = std::vector<DoubleVector2>;
using IntVector = std::vector<int>;
using IntVector2 = std::vector<IntVector>;
using IntVector3 = std::vector<IntVector2>;
using IntVectorPtrVector = std::vector<IntVector*>;
using IntVector2Ptr = std::vector<IntVector>*;
using UIntVector = std::vector<unsigned int>;

using LongVector = std::vector<ul>;
using LongVector2 = std::vector<LongVector>;
using LongVector3 = std::vector<LongVector2>;

using StringVector = std::vector<std::string>;
using StringVector2 = std::vector<StringVector>;

using IntIntMap = std::map<int, int>;

using PersonPtrVector = std::vector<Person*>;
using PersonPtrVectorIterator = PersonPtrVector::iterator;

using PersonPtrVector2 = std::vector<PersonPtrVector>;
using PersonPtrVector3 = std::vector<PersonPtrVector2>;
using PersonPtrVector4 = std::vector<PersonPtrVector3>;

using EventPtrVector = std::vector<Event*>;
using EventPtrVector2 = std::vector<EventPtrVector>;

using ReporterPtrVector = std::vector<Reporter*>;

#endif /* TYPEDEF_H */
