// This file is part of CAF, the C++ Actor Framework. See the file LICENSE in
// the main distribution directory for license terms and copyright or visit
// https://github.com/actor-framework/actor-framework/blob/master/LICENSE.

#pragma once

#include <chrono>
#include <cstddef>
#include <limits>
#include <string>
#include <vector>
#include <mutex>

#include "caf/detail/build_config.hpp"
#include "caf/detail/log_level.hpp"
#include "caf/string_view.hpp"
#include "caf/timestamp.hpp"

// -- hard-coded default values for various CAF options ------------------------

namespace caf::defaults::stream {

constexpr auto max_batch_delay = timespan{1'000'000};

/// Configures an algorithm for assigning credit and adjusting batch sizes.
///
/// The `size-based` controller (default) samples how many Bytes stream elements
/// occupy when serialized to CAF's binary wire format.
///
/// The `token-based` controller associates each stream element with one token.
/// Input buffer and batch sizes are then statically defined in terms of tokens.
/// This strategy makes no dynamic adjustment or sampling.
constexpr auto credit_policy = string_view{"size-based"};

[[deprecated("this parameter no longer has any effect")]] //
constexpr auto credit_round_interval
  = max_batch_delay;

} // namespace caf::defaults::stream

namespace caf::defaults::stream::size_policy {

/// Desired size of a single batch in Bytes, when serialized into CAF's binary
/// wire format.
constexpr auto bytes_per_batch = int32_t{2 * 1024}; //  2 KB

/// Number of Bytes (over all received elements) an inbound path may buffer.
/// Actors use heuristics for calculating the estimated memory use, so actors
/// may still allocate more memory in practice.
constexpr auto buffer_capacity = int32_t{64 * 1024}; // 64 KB

/// Frequency of computing the serialized size of incoming batches. Smaller
/// values may increase accuracy, but also add computational overhead.
constexpr auto sampling_rate = int32_t{100};

/// Frequency of re-calibrating batch sizes. For example, a calibration interval
/// of 10 and a sampling rate of 20 causes the actor to re-calibrate every 200
/// batches.
constexpr auto calibration_interval = int32_t{20};

/// Value between 0 and 1 representing the degree of weighting decrease for
/// adjusting batch sizes. A higher factor discounts older observations faster.
constexpr auto smoothing_factor = 0.6f;

} // namespace caf::defaults::stream::size_policy

namespace caf::defaults::stream::token_policy {

/// Number of elements in a single batch.
constexpr auto batch_size = int32_t{256}; // 2 KB for elements of size 8.

/// Maximum number of elements in the input buffer.
constexpr auto buffer_size = int32_t{4096}; // // 32 KB for elements of size 8.

} // namespace caf::defaults::stream::token_policy

namespace caf::defaults::scheduler {

constexpr auto policy = string_view{"stealing"};
constexpr auto profiling_output_file = string_view{""};
// constexpr auto max_throughput = std::numeric_limits<size_t>::max();
constexpr auto max_throughput = 1;
constexpr auto profiling_resolution = timespan(100'000'000);

} // namespace caf::defaults::scheduler

namespace caf::defaults::work_stealing {

constexpr auto aggressive_poll_attempts = size_t{100};
constexpr auto aggressive_steal_interval = size_t{10};
constexpr auto moderate_poll_attempts = size_t{500};
constexpr auto moderate_steal_interval = size_t{5};
constexpr auto moderate_sleep_duration = timespan{50'000};
constexpr auto relaxed_steal_interval = size_t{1};
constexpr auto relaxed_sleep_duration = timespan{10'000'000};

} // namespace caf::defaults::work_stealing

namespace caf::defaults::logger::file {

constexpr auto format = string_view{"%r %c %p %a %t %C %M %F:%L %m%n"};
constexpr auto path = string_view{"actor_log_[PID]_[TIMESTAMP]_[NODE].log"};

} // namespace caf::defaults::logger::file

namespace caf::defaults::logger::console {

constexpr auto colored = true;
constexpr auto format = string_view{"[%c:%p] %d %m"};

} // namespace caf::defaults::logger::console

namespace caf::defaults::middleman {

constexpr auto app_identifier = string_view{"generic-caf-app"};
constexpr auto network_backend = string_view{"default"};
constexpr auto max_consecutive_reads = size_t{50};
constexpr auto heartbeat_interval = timespan{10'000'000'000};
constexpr auto connection_timeout = timespan{600'000'000'000};
constexpr auto cached_udp_buffers = size_t{10};
constexpr auto max_pending_msgs = size_t{10};

} // namespace caf::defaults::middleman

namespace caf::defaults::default_handler {
class CAF_CORE_EXPORT buf_handler {
private:
  // buf_handler();
  // ~buf_handler();
  buf_handler& operator=(const buf_handler &);
  std::chrono::steady_clock::time_point input_time[2] = {std::chrono::steady_clock::now(), std::chrono::steady_clock::now()};
  std::chrono::steady_clock::time_point output_time[2] = {std::chrono::steady_clock::now(), std::chrono::steady_clock::now()};
  size_t input_payload{1};
  size_t output_payload{1};
  mutable std::mutex input_mtx_;
  mutable std::mutex output_mtx_;

public:
  public:
	static buf_handler& getInstance() {
		static buf_handler instance;
		return instance;
  }

  double get_input_speed() {
    input_mtx_.lock();
    auto interval = (double) (std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - input_time[0]).count())/1000000000.0;
    // printf("got input speed: %lf\n", input_payload / interval);
    input_mtx_.unlock();
    return input_payload / interval;
  }

  double get_output_speed() {
    output_mtx_.lock();
    auto interval = (double) (std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - output_time[0]).count())/1000000000.0;
    // printf("got output speed: %lf\n", output_payload / interval);
    output_mtx_.unlock();
    return output_payload / interval;
  }

  void set_new_input(std::chrono::steady_clock::time_point t, size_t payload) {
    input_mtx_.lock();
    input_time[0] = input_time[1];
    input_time[1] = t;
    input_payload = payload;
    input_mtx_.unlock();
  }

  void set_new_output(std::chrono::steady_clock::time_point t, size_t payload) {
    output_mtx_.lock();
    output_time[0] = output_time[1];
    output_time[1] = t;
    output_payload = payload;
    output_mtx_.unlock();
  }
}; 
}