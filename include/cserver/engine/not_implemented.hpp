#include <stdexcept>


namespace cserver::engine {

struct NotImplemented : std::runtime_error {
  using std::runtime_error::runtime_error;
};

} // namespace cserver::engine
