/**
 * @file Command.hpp
 * @brief classes for communication between main and pseudo thread
 *
 * @internal
 * @author VEO
 */
#ifndef _VEO_COMMAND_HPP_
#define _VEO_COMMAND_HPP_
#include <queue>
#include <memory>
#include <mutex>
#include <condition_variable>
#include "VEOException.hpp"
#include "log.hpp"
#include "ve_offload.h"
namespace veo {
class ThreadContext;

typedef enum veo_command_state CommandStatus;

/**
 * @brief base class of command handled by pseudo thread
 *
 * a command is to be implemented as a function object inheriting Command
 * (see CommandImpl in ThreadContext.cpp).
 */
class Command {
private:
  uint64_t msgid;/*! message ID */
  uint64_t retval;/*! returned value from the function on VE */
  int status;
public:
  explicit Command(uint64_t id): msgid(id) {}
  Command() = delete;
  Command(const Command &) = delete;
  virtual int operator()() = 0;
  void setResult(uint64_t r, int s) { this->retval = r; this->status = s; }
  uint64_t getID() { return this->msgid; }
  int getStatus() { return this->status; }
  uint64_t getRetval() { return this->retval; }
};

/**
 * @brief blocking queue used in CommQueue
 */
class BlockingQueue {
private:
  std::mutex mtx;
  std::condition_variable cond;
  std::deque<std::unique_ptr<Command> > queue;
  std::unique_ptr<Command> tryFindNoLock(uint64_t);
  uint64_t last_pushed_id = VEO_REQUEST_ID_INVALID;
public:
  void push(std::unique_ptr<Command>);
  std::unique_ptr<Command> pop();
  std::unique_ptr<Command> tryFind(uint64_t);
  std::unique_ptr<Command> wait(uint64_t);
  /* only call with lock held */
  uint64_t getLastPushed() {
    return this->last_pushed_id;
  }
};

/**
 * @brief communication queue pair between main thread and pseudo thread
 */
class CommQueue {
private:
  uint64_t seq_no = 0;
  BlockingQueue request;/*! request queue: main -> pseudo */
  BlockingQueue completion;/*! completion queue: pseudo -> main */
public:
  CommQueue() {};

  /**
   * @brief Issue a new request ID
   * @return a request ID, 64 bit integer, to identify a command
   */
  uint64_t issueRequestID() {
    uint64_t ret = VEO_REQUEST_ID_INVALID;
    while (ret == VEO_REQUEST_ID_INVALID) {
      ret = __atomic_fetch_add(&this->seq_no, 1, __ATOMIC_SEQ_CST);
    }
    return ret;
  }
  uint64_t getSeqNo() {
    uint64_t ret = VEO_REQUEST_ID_INVALID;
    __atomic_load(&this->seq_no, &ret, __ATOMIC_SEQ_CST);
    return ret;
  }
  void pushRequest(std::unique_ptr<Command>);
  std::unique_ptr<Command> popRequest();
  void pushCompletion(std::unique_ptr<Command>);
  std::unique_ptr<Command> waitCompletion(uint64_t msgid);
  std::unique_ptr<Command> peekCompletion(uint64_t msgid);
};
} // namespace veo
#endif
