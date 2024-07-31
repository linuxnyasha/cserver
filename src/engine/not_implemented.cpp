export module cserver.engine.not_implemented;
import std;

namespace cserver::engine {

export struct NotImplemented : std::runtime_error {
  using std::runtime_error::runtime_error;
};

}  // namespace cserver::engine
