#ifndef MIDAEXAMPLES_BRIDGEHARNESS_H
#define MIDAEXAMPLES_BRIDGEHARNESS_H

#include "simif.h"
#include "simif_peek_poke.h"

class bridge_driver_t;

/**
 * Base class for simple unit tests.
 *
 * In the constructor, macros for all bridges are invoked to create bridge
 * instances and pass them to an overload of the `add_bridge_driver` function.
 * By default, no bridge is allowed to be registered. If a test case wishes
 * to take control of bridges, it can override the appropriate method and
 * intercept the bridge for later use.
 */
class BridgeHarness : public simif_peek_poke_t, public simulation_t {
public:
  BridgeHarness(const std::vector<std::string> &args, simif_t *simif);

  virtual ~BridgeHarness();

  void add_bridge_driver(bridge_driver_t *bridge);

  void simulation_init() override;
  int simulation_run() override;
  void simulation_finish() override;

protected:
  virtual unsigned get_step_limit() const = 0;
  virtual unsigned get_tick_limit() const = 0;

private:
  std::vector<std::unique_ptr<bridge_driver_t>> bridges;
};

#define TEST_MAIN(CLASS_NAME)                                                  \
  std::unique_ptr<simulation_t> create_simulation(                             \
      const std::vector<std::string> &args, simif_t *simif) {                  \
    return std::make_unique<CLASS_NAME>(args, simif);                          \
  }
#endif // MIDAEXAMPLES_BRIDGEHARNESS_H
