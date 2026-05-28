template<typename... Args>
Field & FieldCollection::emplace_back(Args&&... args) {
  return this->fields.emplace_back(std::forward<Args>(args)...);
}
