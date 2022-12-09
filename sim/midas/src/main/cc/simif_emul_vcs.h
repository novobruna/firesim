// See LICENSE for license details.

#ifndef __SIMIF_EMUL_VCS_H
#define __SIMIF_EMUL_VCS_H

#include "emul/context.h"
#include "simif_emul.h"

/**
 * VCS-specific software emulator implementation.
 */
class simif_emul_vcs_t : public simif_emul_t {
public:
  simif_emul_vcs_t(const std::vector<std::string> &args);

  virtual ~simif_emul_vcs_t() {}

  void sim_init() override;

  void finish() override;

  void advance_target() override;

public:
  pcontext_t *host;
  pcontext_t target;
  bool vcs_rst = false;
  bool vcs_fin = false;
};

#endif // __SIMIF_EMUL_VCS_H
