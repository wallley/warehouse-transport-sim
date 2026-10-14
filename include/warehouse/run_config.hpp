#pragma once

#include "warehouse/config.hpp"
#include <ostream>
#include <string>

namespace warehouse {

// JSON 字符串转义与配置输出共用，避免文件路径破坏输出格式。
void writeJsonString(std::ostream& output, const std::string& value);
void saveRuntimeConfig(const SimulationConfig& config, double maxSeconds);

} // namespace warehouse
