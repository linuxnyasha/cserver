export module cserver.components.logger;
import fmt;
import std;
import utempl;


namespace cserver {

enum class LoggingLevel { kTrace, kDebug, kInfo, kWarning, kError };

struct Logging {
  static constexpr utempl::ConstexprString kName = "logging";
  LoggingLevel level;
  template <typename T>
  explicit constexpr Logging(T&) : level(T::kConfig.template Get<T::kName>().template Get<"level">()) {
    std::ios::sync_with_stdio(false);
  };

  constexpr auto SetLoggingLevel(LoggingLevel level) {
    this->level = level;
  };

  template <std::size_t N, std::size_t NN>
  static consteval auto GetFormatStringFor(utempl::ConstexprString<N> fmt, utempl::ConstexprString<NN> level) {
    constexpr auto f = FMT_COMPILE("[{}]: {}\n");
    constexpr auto size = N + NN + 6;
    std::array<char, size> data{};
    fmt::format_to(data.begin(), f, static_cast<std::string_view>(level), static_cast<std::string_view>(fmt));
    return utempl::ConstexprString<size>(data);
  };

  constexpr auto Log(std::string_view data) {
    std::cout << data;
  };

  template <utempl::ConstexprString Fmt, typename... Ts>
  constexpr auto Debug(Ts&&... args) {
    static constexpr auto fmt = GetFormatStringFor(Fmt, utempl::ConstexprString{"DEBUG"});
    Log(fmt::format(FMT_COMPILE(fmt.data.begin()), std::forward<Ts>(args)...));
  };

  template <utempl::ConstexprString Fmt, typename... Ts>
  constexpr auto Info(Ts&&... args) {
    static constexpr auto fmt = GetFormatStringFor(Fmt, utempl::ConstexprString{"INFO"});
    Log(fmt::format(FMT_COMPILE(fmt.data.begin()), std::forward<Ts>(args)...));
  };

  template <utempl::ConstexprString Fmt, typename... Ts>
  constexpr auto Trace(Ts&&... args) {
    static constexpr auto fmt = GetFormatStringFor(Fmt, utempl::ConstexprString{"TRACE"});
    Log(fmt::format(FMT_COMPILE(fmt.data.begin()), std::forward<Ts>(args)...));
  };

  template <utempl::ConstexprString Fmt, typename... Ts>
  constexpr auto Error(Ts&&... args) {
    static constexpr auto fmt = GetFormatStringFor(Fmt, utempl::ConstexprString{"ERROR"});
    Log(fmt::format(FMT_COMPILE(fmt.data.begin()), std::forward<Ts>(args)...));
  };

  template <utempl::ConstexprString Fmt, typename... Ts>
  constexpr auto Warning(Ts&&... args) {
    static constexpr auto fmt = GetFormatStringFor(Fmt, utempl::ConstexprString{"WARNING"});
    Log(fmt::format(FMT_COMPILE(fmt.data.begin()), std::forward<Ts>(args)...));
  };
};

}  // namespace cserver
