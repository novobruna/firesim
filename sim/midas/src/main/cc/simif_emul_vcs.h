// See LICENSE for license details.

#ifndef __SIMIF_EMUL_VCS_H
#define __SIMIF_EMUL_VCS_H

#include <atomic>
#include <memory>
#include <optional>

#include <pthread.h>

#include "simif_emul.h"

/// Helper to handle signals.
void emul_signal_handler(int sig);

/**
 * VCS-specific software emulator implementation.
 */
class simif_emul_vcs_t : public simif_emul_t {
public:
  /**
   * Sets up the simulation.
   *
   * Invokes the externally registered `create_simulation` method to
   * create a simulation object to connect to the RTL.
   */
  simif_emul_vcs_t();

  /**
   * Simulation cleanup.
   *
   * Should by called by the RTL when the simulation is no longer needed.
   */
  virtual ~simif_emul_vcs_t() {}

  /**
   * Start the simulation.
   *
   * This function spawns the simulation thread.
   */
  void begin() override;

  /**
   * End the simulation.
   *
   * This function joins the simulation thread. Before calling it, the
   * simulation should finish either by returning from its loop or by having
   * called finish with some other error condition.
   */
  int end() override;

  /**
   * Simulation thread implementation.
   *
   * This thread is synchronized by the main thread driven by Verilator/VCS
   * which simulates the RTL and invokes the tick function through DPI. At
   * any point in time, only one of the two is allowed to run and the
   * pthread synchronization primitives switch between the two contexts.
   *
   * When the thread starts, it is put to sleep.
   *
   * The simulation thread is woken by the target thread in the tick function,
   * after it posts data received from the RTL. The simulation thread then
   * runs for one cycle, handling the AXI transactions, before switching control
   * back to the target thread that reads outputs into the RTL.
   */
  void thread_main();

  /**
   * Transfers control to the simulator on a tick.
   *
   * Helper to be called solely from the target thread.
   */
  bool to_sim();

  /**
   * Transfers control to the target and lets the simulator wait for a tick.
   *
   * To be called solely from the target.
   */
  void do_tick() override;

private:
  /// Reference to the simulation object created by `create_simulation`.
  std::unique_ptr<simulation_t> sim;

  /**
   * The flag is set when the simulation thread returns.
   *
   * If finished is set, the simulation thread can be joined into the target
   * thread to gracefully finalize the simulation.
   */
  std::atomic<bool> finished = false;

  /**
   * Exit code returned by the simulation, if it finished.
   */
  std::atomic<int> exit_code = EXIT_FAILURE;

  /**
   * Identifier of the simulation thread.
   *
   * The main thread is the target thread.
   */
  pthread_t thread;

  // Synchronisation primitives blocking the simulator.
  pthread_mutex_t sim_mutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_cond_t sim_cond = PTHREAD_COND_INITIALIZER;
  std::atomic<bool> sim_flag = false;

  // Synchronisation primitives blocking the target.
  pthread_mutex_t target_mutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_cond_t target_cond = PTHREAD_COND_INITIALIZER;
  std::atomic<bool> target_flag = true;

};

#endif // __SIMIF_EMUL_VCS_H
