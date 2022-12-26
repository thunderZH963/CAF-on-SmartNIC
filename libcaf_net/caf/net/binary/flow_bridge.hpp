// This file is part of CAF, the C++ Actor Framework. See the file LICENSE in
// the main distribution directory for license terms and copyright or visit
// https://github.com/actor-framework/actor-framework/blob/master/LICENSE.

#pragma once

#include "caf/async/consumer_adapter.hpp"
#include "caf/async/producer_adapter.hpp"
#include "caf/async/spsc_buffer.hpp"
#include "caf/detail/flow_bridge_base.hpp"
#include "caf/fwd.hpp"
#include "caf/net/binary/lower_layer.hpp"
#include "caf/net/binary/upper_layer.hpp"
#include "caf/net/flow_connector.hpp"
#include "caf/sec.hpp"

#include <utility>

namespace caf::net::binary {

/// Convenience alias for referring to the base type of @ref flow_bridge.
template <class Trait>
using flow_bridge_base_t
  = detail::flow_bridge_base<upper_layer, lower_layer, Trait>;

/// Translates between a message-oriented transport and data flows.
template <class Trait>
class flow_bridge : public flow_bridge_base_t<Trait> {
public:
  using super = flow_bridge_base_t<Trait>;

  using input_type = typename Trait::input_type;

  using output_type = typename Trait::output_type;

  using connector_pointer = flow_connector_ptr<Trait>;

  using super::super;

  static std::unique_ptr<flow_bridge> make(async::execution_context_ptr loop,
                                           connector_pointer conn) {
    return std::make_unique<flow_bridge>(std::move(loop), std::move(conn));
  }

  bool write(const output_type& item) override {
    super::down_->begin_message();
    auto& bytes = super::down_->message_buffer();
    return super::trait_.convert(item, bytes) && super::down_->end_message();
  }

  // -- implementation of binary::lower_layer ----------------------------------

  ptrdiff_t consume(byte_span buf) override {
    if (!super::out_)
      return -1;
    input_type val;
    if (!super::trait_.convert(buf, val))
      return -1;
    if (super::out_.push(std::move(val)) == 0)
      super::down_->suspend_reading();
    return static_cast<ptrdiff_t>(buf.size());
  }
};

} // namespace caf::net::binary
