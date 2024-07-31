export module cserver.components.loggable_component_base;
import cserver.components.logger;

namespace cserver {

export struct ComponentBase {
  Logging& logging;
  template <typename T>
  explicit constexpr ComponentBase(T& context) : logging(context.template FindComponent<"logging">()){};
};

}  // namespace cserver
