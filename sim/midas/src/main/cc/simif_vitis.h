#ifndef __SIMIF_VITIS_H
#define __SIMIF_VITIS_H

#include "simif.h"

#include "experimental/xrt_device.h"
#include "experimental/xrt_ip.h"
#include "experimental/xrt_kernel.h"

class simif_vitis_t : public simif_t {
public:
  simif_vitis_t(const std::vector<std::string> &args);
  virtual ~simif_vitis_t(){};
  // Unused by Vitis since initialization / deinitization is done in the
  // constructor
  void sim_init() override {}
  void host_init() override {}
  void host_finish() override {}

  void write(size_t addr, uint32_t data) override;
  uint32_t read(size_t addr) override;
  size_t pull(unsigned stream_idx,
              void *dest,
              size_t num_bytes,
              size_t threshold_bytes) override;
  size_t push(unsigned stream_idx,
              void *src,
              size_t num_bytes,
              size_t threshold_bytes) override;
  uint32_t is_write_ready();

private:
  int slotid;
  std::string binary_file;
  xrt::device device_handle;
  xrt::uuid uuid;
  xrt::ip kernel_handle;
  xrt::run run_handle;
};

#endif // __SIMIF_VITIS_H
