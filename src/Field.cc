#include "Field.h"

Field::Field(const std::string & name, const std::string & value): name(name), value(value) {}

const std::string & Field::get_name() const {
  return this->name;
}

const std::string & Field::get_value() const {
  return this->value;
}
