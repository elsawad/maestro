#ifndef FIELD_COLLECTION_H
#define FIELD_COLLECTION_H

#include <optional>
#include <string>
#include <vector>

#include "Field.h"

class FieldCollection {
  private:
    std::vector<Field> fields;
  public:
    class ConstIterator {
      public:
        ConstIterator(std::vector<Field>::const_iterator it);
        const Field & operator*() const;
        ConstIterator & operator++();
        bool operator!=(const ConstIterator & other) const;
      private:
        std::vector<Field>::const_iterator it;
    };

    class Iterator {
      public:
        Iterator(std::vector<Field>::iterator it);
        Field & operator*() const;
        Iterator & operator++();
        bool operator!=(const Iterator & other) const;
      private:
        std::vector<Field>::iterator it;
    };

    ConstIterator begin() const;
    Iterator begin();

    template<typename... Args>
    Field & emplace_back(Args&&... args);

    std::vector<std::string_view> get_all(const std::string & name) const;
    bool empty() const;
    ConstIterator end() const;
    Iterator end();
    void push_back(Field & field);
};

#include "FieldCollection.tcc"

#endif
