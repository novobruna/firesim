// See LICENSE for license details.

#ifndef __PEEK_POKE
#define __PEEK_POKE

#include <gmp.h>
#include <string_view>

#include "simif.h"

typedef struct PEEKPOKEBRIDGEMODULE_struct {
  uint64_t PRECISE_PEEKABLE;
  uint64_t READY;
} PEEKPOKEBRIDGEMODULE_struct;

// This derivation of simif_t is used to implement small integration
// tests where the test writer wants fine grained control over individual
// token channels. In these tests, the peek poke bridge drives all IO of the
// DUT, and there are generally* no other bridges. The driver can then be
// written using a ChiselTesters-like interface consisting of peek, poke and
// expect (defined in this class).
//
// * If other bridges are used their construction, initialization, and tick()
// invocations
//   must be managed manually. Care must be taken to avoid deadlock, since calls
//   to step. must be blocking for peeks and pokes to capture and drive values
//   at the correct cycles.
//
// Note: This presents the same set of methods legacy simif_t used prior to
// FireSim 1.13.

class peek_poke_t final {
public:
  struct port {
    uint64_t address;
    uint32_t chunks;
  };

  using port_map = std::map<std::string, port, std::less<>>;

  peek_poke_t(simif_t *simif,
              const PEEKPOKEBRIDGEMODULE_struct &mmio_addrs,
              unsigned poke_size,
              const uint32_t *input_addrs,
              const char *const *input_names,
              const uint32_t *input_chunks,
              unsigned peek_size,
              const uint32_t *output_addrs,
              const char *const *output_names,
              const uint32_t *output_chunks);
  ~peek_poke_t() {}

  void poke(std::string_view id, uint32_t value, bool blocking);
  void poke(std::string_view id, mpz_t &value);

  uint32_t peek(std::string_view id, bool blocking);
  void peek(std::string_view id, mpz_t &value);
  uint32_t sample_value(std::string_view id) { return peek(id, false); }

  /// Returns true if the last request times out.
  bool timeout() const { return req_timeout; }
  /// Returns true if the last peek was unstable.
  bool unstable() const { return req_unstable; }

protected:
  /// Simulation interface.
  simif_t *simif;

private:
  /// Base MMIO port mapping.
  const PEEKPOKEBRIDGEMODULE_struct mmio_addrs;
  /// Flag to indicate whether the last request was on an unstable value.
  bool req_unstable;
  /// Flag to indicate whether the last request timed out.
  bool req_timeout;
  /// Addresses of input ports.
  port_map inputs;
  /// Addresses of output ports.
  port_map outputs;

  bool wait_on(size_t flag_addr, double timeout) {
    midas_time_t start = timestamp();
    while (!simif->read(flag_addr))
      if (diff_secs(timestamp(), start) > timeout)
        return false;
    return true;
  }

  bool wait_on_ready(double timeout) {
    return wait_on(mmio_addrs.READY, timeout);
  }

  bool wait_on_stable_peeks(double timeout) {
    return wait_on(mmio_addrs.PRECISE_PEEKABLE, timeout);
  }
};

#endif // __PEEK_POKE
