// See LICENSE for license details.

#ifndef RTLSIM
#include "simif_f1.h"
#define SIMIF simif_f1_t
#else
#ifdef VCS
#include "simif_emul_vcs.h"
#define SIMIF simif_emul_vcs_t
#else
#include "simif_emul_verilator.h"
#define SIMIF simif_emul_verilator_t
#endif
#endif

#include "bridges/uart.h"
#include "simif_peek_poke.h"

class UARTModuleTest final : public simif_peek_poke_t, public simulation_t {
private:
  class Handler final : public uart_handler {
  public:
    Handler(UARTModuleTest &test) { test.handler = this; }

    int check() {
      // Check that the input and output buffers are equal.
      if (in_buffer != out_buffer) {
        fprintf(stderr,
                "Buffer mismatch:\n  %s\n  %s\n",
                in_buffer.c_str(),
                out_buffer.c_str());
        return EXIT_FAILURE;
      }
      return EXIT_SUCCESS;
    }

    std::optional<char> get() override {
      if (in_index >= in_buffer.size())
        return std::nullopt;
      return in_buffer[in_index++];
    }

    void put(char data) override { out_buffer += data; }

    const std::string in_buffer = "We are testing the UART bridge";
    std::string out_buffer;
    size_t in_index = 0;
  };

public:
  UARTModuleTest(simif_t &simif, const std::vector<std::string> &args)
      : simif_peek_poke_t(&simif, PEEKPOKEBRIDGEMODULE_0_substruct_create),
        simulation_t(args),
        uart(std::make_unique<uart_t>(&simif,
                                      UARTBRIDGEMODULE_0_substruct_create,
                                      std::make_unique<Handler>(*this))) {}

  void simulation_init() override { uart->init(); }

  int simulation_run() override {
    // Reset the device.
    poke(reset, 1);
    step(1);
    poke(reset, 0);
    step(1);

    // Tick until the out buffer is filled in.
    step(300000, false);
    for (unsigned i = 0; i < 100000 && !simif->done(); ++i) {
      uart->tick();
    }
    return handler->check();
  }

  void simulation_finish() override { uart->finish(); }

private:
  Handler *handler;
  std::unique_ptr<uart_t> uart;
};

int main(int argc, char **argv) {
  std::vector<std::string> args(argv + 1, argv + argc);
  SIMIF simif(args);
  UARTModuleTest test(simif, args);
  return simif.run(argc, argv, test);
}
