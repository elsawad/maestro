template <typename T>
BasicOstreamSink<T>::BasicOstreamSink(std::basic_ostream<T> &os) : os(os) {}

template <typename T>
void BasicOstreamSink<T>::write(const void *data, size_t size) {
  os.write(static_cast<const T *>(data), size);
}

template <typename T>
void BasicOstreamSink<T>::finish() {
  os.flush();
}
