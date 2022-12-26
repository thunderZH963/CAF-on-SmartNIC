// This file is part of CAF, the C++ Actor Framework. See the file LICENSE in
// the main distribution directory for license terms and copyright or visit
// https://github.com/actor-framework/actor-framework/blob/master/LICENSE.

#pragma once

#include "caf/flow/coordinator.hpp"
#include "caf/flow/observer.hpp"
#include "caf/flow/op/empty.hpp"
#include "caf/flow/op/hot.hpp"
#include "caf/flow/op/ucast.hpp"
#include "caf/flow/subscription.hpp"
#include "caf/intrusive_ptr.hpp"
#include "caf/make_counted.hpp"

#include <algorithm>
#include <deque>
#include <memory>

namespace caf::flow::op {

/// State shared between one multicast operator and one subscribed observer.
template <class T>
using mcast_sub_state = ucast_sub_state<T>;

template <class T>
using mcast_sub_state_ptr = intrusive_ptr<mcast_sub_state<T>>;

template <class T>
class mcast_sub : public subscription::impl_base {
public:
  // -- constructors, destructors, and assignment operators --------------------

  mcast_sub(coordinator* ctx, mcast_sub_state_ptr<T> state)
    : ctx_(ctx), state_(std::move(state)) {
    // nop
  }

  // -- implementation of subscription -----------------------------------------

  bool disposed() const noexcept override {
    return !state_;
  }

  void dispose() override {
    if (state_) {
      ctx_->delay_fn([state = std::move(state_)]() { state->do_dispose(); });
    }
  }

  void request(size_t n) override {
    state_->demand += n;
    if (!state_->running) {
      state_->running = true;
      ctx_->delay_fn([state = state_] { state->do_run(); });
    }
  }

private:
  /// Stores the context (coordinator) that runs this flow.
  coordinator* ctx_;

  /// Stores a handle to the state.
  mcast_sub_state_ptr<T> state_;
};

// Base type for *hot* operators that multicast data to subscribed observers.
template <class T>
class mcast : public hot<T> {
public:
  // -- member types -----------------------------------------------------------

  using super = hot<T>;

  using state_type = mcast_sub_state<T>;

  using state_ptr_type = mcast_sub_state_ptr<T>;

  using observer_type = observer<T>;

  // -- constructors, destructors, and assignment operators --------------------

  explicit mcast(coordinator* ctx) : super(ctx) {
    // nop
  }

  /// Pushes @p item to all subscribers.
  void push_all(const T& item) {
    for (auto& state : states_)
      state->push(item);
  }

  /// Closes the operator, eventually emitting on_complete on all observers.
  void close() {
    if (!closed_) {
      closed_ = true;
      for (auto& state : states_)
        state->close();
      states_.clear();
    }
  }

  /// Closes the operator, eventually emitting on_error on all observers.
  void abort(const error& reason) {
    if (!closed_) {
      closed_ = true;
      for (auto& state : states_)
        state->abort(reason);
      states_.clear();
    }
  }

  size_t max_demand() const noexcept {
    if (states_.empty()) {
      return 0;
    } else {
      auto pred = [](const state_ptr_type& x, const state_ptr_type& y) {
        return x->demand < y->demand;
      };
      auto& ptr = *std::max_element(states_.begin(), states_.end(), pred);
      return ptr->demand;
    }
  }

  size_t min_demand() const noexcept {
    if (states_.empty()) {
      return 0;
    } else {
      auto pred = [](const state_ptr_type& x, const state_ptr_type& y) {
        return x->demand < y->demand;
      };
      auto& ptr = *std::min_element(states_.begin(), states_.end(), pred);
      ptr->demand;
    }
  }

  size_t max_buffered() const noexcept {
    if (states_.empty()) {
      return 0;
    } else {
      auto pred = [](const state_ptr_type& x, const state_ptr_type& y) {
        return x->buf.size() < y->buf.size();
      };
      auto& ptr = *std::max_element(states_.begin(), states_.end(), pred);
      return ptr->buf.size();
    }
  }

  size_t min_buffered() const noexcept {
    if (states_.empty()) {
      return 0;
    } else {
      auto pred = [](const state_ptr_type& x, const state_ptr_type& y) {
        return x->buf.size() < y->buf.size();
      };
      auto& ptr = *std::min_element(states_.begin(), states_.end(), pred);
      ptr->buf.size();
    }
  }

  /// Queries whether there is at least one observer subscribed to the operator.
  bool has_observers() const noexcept {
    return !states_.empty();
  }

  /// Queries the current number of subscribed observers.
  size_t observer_count() const noexcept {
    return states_.size();
  }

  state_ptr_type add_state(observer_type out) {
    auto state = make_counted<state_type>(super::ctx_, std::move(out));
    auto mc = strong_this();
    state->when_disposed = make_action([mc, state]() mutable { //
      mc->do_dispose(state);
    });
    state->when_consumed_some = make_action([mc, state]() mutable { //
      mc->on_consumed_some(*state);
    });
    states_.push_back(state);
    return state;
  }

  disposable subscribe(observer<T> out) override {
    if (!closed_) {
      auto ptr = make_counted<mcast_sub<T>>(super::ctx_, add_state(out));
      out.on_subscribe(subscription{ptr});
      return disposable{std::move(ptr)};
    } else if (!err_) {
      return make_counted<op::empty<T>>(super::ctx_)->subscribe(out);
    } else {
      out.on_error(err_);
      return disposable{};
    }
  }

protected:
  bool closed_ = false;
  error err_;
  std::vector<state_ptr_type> states_;

private:
  intrusive_ptr<mcast> strong_this() {
    return {this};
  }

  void do_dispose(state_ptr_type& state) {
    auto e = states_.end();
    if (auto i = std::find(states_.begin(), e, state); i != e) {
      states_.erase(i);
      on_dispose(*state);
    }
  }

  virtual void on_dispose(state_type&) {
    // nop
  }

  virtual void on_consumed_some(state_type&) {
    // nop
  }
};

} // namespace caf::flow::op
