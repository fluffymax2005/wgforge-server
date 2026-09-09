#ifndef __WGFORGE_READ_WRITE_REGISTER_H__
#define __WGFORGE_READ_WRITE_REGISTER_H__

#include "config_read_register.h"
#include "config_write_register.h"

namespace WgForge {

class ConfigReadWriteRegister : public ConfigReadRegister,
                                public ConfigWriteRegister {
public:
  ConfigReadWriteRegister() = default;

  ConfigReadWriteRegister(const ConfigReadWriteRegister &) = default;
  ConfigReadWriteRegister &operator=(const ConfigReadWriteRegister &) = default;

  ConfigReadWriteRegister(ConfigReadWriteRegister &&) = default;
  ConfigReadWriteRegister &operator=(ConfigReadWriteRegister &&) = default;
};

} // namespace WgForge

#endif // __WGFORGE_READ_WRITE_REGISTER_H__
