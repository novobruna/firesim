
#include <signal.h>

#include "simif_emul_vcs.h"

/**
 * Arguments saved by fake main before handing control to VCS.
 */
static std::vector<std::string> saved_args;

static int saved_argc;
static char **saved_argv;

extern "C" {
extern int vcs_main(int argc, char **argv);
}

extern std::unique_ptr<simulation_t> create_simulation(
    const std::vector<std::string> &args,
    simif_t *simif);

simif_emul_vcs_t::simif_emul_vcs_t()
  : simif_emul_t(saved_args) {
  sim = create_simulation(saved_args, this);
}

static void *thread_starter(void *ptr) {
  ((simif_emul_vcs_t*)ptr)->thread_main();
  return nullptr;
}

void simif_emul_vcs_t::begin() {
  host_init();

  pthread_mutex_lock(&target_mutex);
  target_flag = false;
  if (pthread_create(&thread, nullptr, &thread_starter, this))
    abort();

  while (!target_flag)
    pthread_cond_wait(&target_cond, &target_mutex);
  pthread_mutex_unlock(&target_mutex);
}

int simif_emul_vcs_t::end() {
  assert(finished && "simulation not yet finished");

  if (pthread_join(thread, nullptr))
    abort();

  return exit_code;
}

void simif_emul_vcs_t::do_tick() {
  sim_flag = false;
  target_flag = true;
  pthread_mutex_lock(&target_mutex);
  pthread_cond_signal(&target_cond);
  pthread_mutex_unlock(&target_mutex);
  pthread_mutex_lock(&sim_mutex);
  while (!sim_flag)
    pthread_cond_wait(&sim_cond, &sim_mutex);
  pthread_mutex_unlock(&sim_mutex);
}

bool simif_emul_vcs_t::to_sim() {
  target_flag = false;
  sim_flag = true;
  pthread_mutex_lock(&sim_mutex);
  pthread_cond_signal(&sim_cond);
  pthread_mutex_unlock(&sim_mutex);
  pthread_mutex_lock(&target_mutex);
  while (!target_flag && !finished)
    pthread_cond_wait(&target_cond, &target_mutex);
  pthread_mutex_unlock(&target_mutex);
  return finished;
}

void simif_emul_vcs_t::thread_main() {
  // Switch over to the target thread and wait for the first tick.
  do_tick();

  // Run the simulation flow.
  exit_code = target_run();

  // Wake the target thread before returning from the simulation thread.
  finished = true;
  pthread_mutex_lock(&target_mutex);
  pthread_cond_signal(&target_cond);
  pthread_mutex_unlock(&target_mutex);
}

extern "C" {
int vcs_main(int argc, char **argv);
}

/**
 * Entry point to the VCS meta-simulation hijacked from VCS itself.
 */
int main(int argc, char **argv) {
  saved_args = std::vector<std::string>{argv + 1, argv + argc};
  return vcs_main(argc, argv);
}

int main(int argc, char **argv) {
  saved_argc = argc;
  saved_argv = argv;
  std::vector<std::string> args(argv + 1, argv + argc);
  return simif_emul_vcs_t(args).run();
}
