// See LICENSE for license details.

#include "PrintfTest.h"

class TestTriggerPredicatedPrintf final : public PrintTest {
public:
  using PrintTest::PrintTest;

  void run_test() override {
    for (auto &print_endpoint : print_endpoints) {
      print_endpoint->init();
    }
    run_and_collect_prints(16000);
  };
};

TEST_MAIN(TestTriggerPredicatedPrintf)
