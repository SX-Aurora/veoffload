/**
 * @file Command.cpp
 * @brief implementation of communication between main and pseudo thread
 */
#include "Command.hpp"

namespace veo {
typedef std::unique_ptr<Command> CmdPtr;

/**
 * @brief push a command to queue
 * @param cmd a pointer to a command to be pushed (sent).
 *
 * When pushing from multiple threads, enforce the strict
 * ordering of the requests. It is needed for a simple
 * detection of multiple "waits" upon requests, later.
 */
void BlockingQueue::push(CmdPtr cmd) {
  auto this_id = cmd.get()->getID();
  for (;;) {
    std::unique_lock<std::mutex> lock(this->mtx);
    if (this_id == this->last_pushed_id + 1) {
      this->last_pushed_id = this_id;
      this->queue.push_back(std::move(cmd));
      this->cond.notify_all();
      break;
    }
    this->cond.wait(lock);
  }
}

/**
 * @brief pop a command from queue
 * @return a pointer to a command to be poped (received).
 *
 * This function gets the first command in the queue.
 * If the queue is empty, this function blocks until a command is pushed.
 */
CmdPtr BlockingQueue::pop() {
  for (;;) {
    std::unique_lock<std::mutex> lock(this->mtx);
    if (!this->queue.empty()) {
      auto rv = std::move(this->queue.front());
      this->queue.pop_front();
      return rv;
    }
    this->cond.wait(lock);
  }
}

/**
 * @brief try to find a command with the specified message ID
 * @param msgid a message ID to find
 * @return a pointer to a command with the specified message ID;
 *         nullptr if a command with the message ID is not found in the queue.
 *
 * This function is expected to be called from a thread holding lock.
 */
CmdPtr BlockingQueue::tryFindNoLock(uint64_t msgid) {
  for (auto &&it = this->queue.begin(); it != this->queue.end(); ++it) {
    if ((*it)->getID() == msgid) {
      auto rv = std::move(*it);
      this->queue.erase(it);
      return rv;
    }
  }
  auto prev = this->getLastPushed();
  if (prev != VEO_REQUEST_ID_INVALID && msgid <= prev) {
    VEO_ERROR(nullptr, "searching for wrong reqid: %d (prev = %d)", msgid, prev);
    throw VEOException("searching for wrong reqid!");
  }
  return nullptr;
}

CmdPtr BlockingQueue::tryFind(uint64_t msgid) {
  std::lock_guard<std::mutex> lock(this->mtx);
  return this->tryFindNoLock(msgid);
}

CmdPtr BlockingQueue::wait(uint64_t msgid) {
  for (;;) {
    std::unique_lock<std::mutex> lock(this->mtx);
    auto rv = this->tryFindNoLock(msgid);
    if (rv != nullptr)
      return rv;
    this->cond.wait(lock);
  }
}

void CommQueue::pushRequest(std::unique_ptr<Command> req)
{
  this->request.push(std::move(req));
}

std::unique_ptr<Command> CommQueue::popRequest()
{
  return this->request.pop();
}

void CommQueue::pushCompletion(std::unique_ptr<Command> req)
{
  this->completion.push(std::move(req));
}

std::unique_ptr<Command> CommQueue::peekCompletion(uint64_t msgid)
{
  if (msgid >= this->getSeqNo())
    throw VEOException("peekCompletion: reqid larger than seq_no");
  return this->completion.tryFind(msgid);
}

std::unique_ptr<Command> CommQueue::waitCompletion(uint64_t msgid)
{
  if (msgid >= this->getSeqNo())
    throw VEOException("waitCompletion: reqid larger than seq_no");
  return this->completion.wait(msgid);
}
} // namespace veo
