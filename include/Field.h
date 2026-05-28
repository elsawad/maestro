#ifndef FIELD_H
#define FIELD_H

#include <string>

class Field {
  public:
    Field(const std::string & name, const std::string & value);
    const std::string & get_name() const;
    const std::string & get_value() const;
  private:
    std::string name;
    std::string value;
};

#endif
