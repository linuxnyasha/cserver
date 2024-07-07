#include <tuple>
#include <utempl/loopholes/core.hpp>
#include <utempl/type_list.hpp>

struct SomeStruct {
  explicit constexpr SomeStruct(auto&& arg) {
    arg.Method(42);  // NOLINT
  };
};

struct Injector {
  template <typename T, auto _ = utempl::loopholes::Injector<0, utempl::TypeList<T>{}>{}>
  auto Method(T&&) const -> void;
};

template <typename T, auto... Args>
inline constexpr auto Use() {
  std::ignore = __builtin_constant_p(T{Args...});
};

template <typename...>
consteval auto Ignore() {};

auto main() -> int {
  Ignore<decltype(Use<SomeStruct, Injector{}>())>();
  static_assert(std::is_same_v<decltype(Magic(utempl::loopholes::Getter<0>{})), utempl::TypeList<int>>);
};
