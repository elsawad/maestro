#include <optional>

#include "FieldCollection.h"

FieldCollection::Iterator FieldCollection::begin() {
  return FieldCollection::Iterator(this->fields.begin());
}

FieldCollection::ConstIterator FieldCollection::begin() const {
  return FieldCollection::ConstIterator(this->fields.cbegin());
}

bool FieldCollection::empty() const {
  return this->fields.empty();
}

FieldCollection::ConstIterator FieldCollection::end() const {
  return FieldCollection::ConstIterator(this->fields.cend());
}

FieldCollection::ConstIterator::ConstIterator(const std::vector<Field>::const_iterator it): it(it) {}

FieldCollection::Iterator FieldCollection::end() {
  return FieldCollection::Iterator(this->fields.end());
}

std::vector<std::string_view> FieldCollection::get_all(const std::string & name) const {
  std::vector<std::string_view> values;
  for (const auto & field : *this) {
    if (field.get_name() == name) {
      values.emplace_back(field.get_value());
    }
  }
  return values;
}

FieldCollection::Iterator::Iterator(std::vector<Field>::iterator it): it(it) {}

const Field & FieldCollection::ConstIterator::operator*() const {
  return *this->it;
}

FieldCollection::ConstIterator & FieldCollection::ConstIterator::operator++() {
  ++this->it;
  return *this;
}

bool FieldCollection::ConstIterator::operator!=(const ConstIterator & other) const {
  return this->it != other.it;
}

Field & FieldCollection::Iterator::operator*() const {
  return *this->it;
}

FieldCollection::Iterator & FieldCollection::Iterator::operator++() {
  ++this->it;
  return *this;
}

bool FieldCollection::Iterator::operator!=(const Iterator & other) const {
  return this->it != other.it;
}

void FieldCollection::push_back(Field & field) {
  this->fields.push_back(field);
}
