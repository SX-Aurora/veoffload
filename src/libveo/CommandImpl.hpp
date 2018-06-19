/**
 * @file CommandImpl.hpp
 * @brief VEO command implementation template
 */
#include "Command.hpp"

namespace veo {
namespace internal {
/**
 * @brief template to define a command handled by pseudo thread
 */
template <typename T> class CommandImpl: public Command {
  T handler;
public:
  CommandImpl(uint64_t id, T h): handler(h), Command(id) {}
  int64_t operator()() {
    return handler();
  }
};

template <typename T> class CommandExecuteVE: public CommandImpl<T> {
  ThreadContext *context;
public:
  CommandExecuteVE(ThreadContext *tc, uint64_t id, T h):
    internal::CommandImpl<T>(id, h), context(tc) {}
  int64_t operator()() {
    auto rv = this->CommandImpl<T>::operator()();
    if (rv != 0) {
      this->setResult(rv, VEO_COMMAND_ERROR);
      return -1;
    }
    return context->_executeVE(this);
  }
};
} // namespace internal
} // namespace veo
