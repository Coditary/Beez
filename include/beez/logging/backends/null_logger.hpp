#pragma once

#include "beez/logging/contract/logger.hpp"
#include "beez/logging/contract/run_types.hpp"

#include <string>
#include <string_view>

namespace beez::logging
{

class NullLogger : public ILogger
{
  public:
    void beginRun(const std::string& /*runKind*/, const std::string& /*name*/) override {}
    void logProgress(const ExecutionProgress& /*progress*/) override {}
    void logCommandOutput(LogChannelId /*channel*/, std::string_view /*output*/) override {}
    void logFailureOutput(std::string_view /*output*/, LogChannelId /*channel*/ = {}) override {}
    void endRun(bool /*success*/,
                double /*durationSeconds*/,
                const RunSummary& /*summary*/ = {}) override
    {
    }
    LogChannelId openChannel(const std::string& /*label*/) override
    {
        return {};
    }
    void closeChannel(LogChannelId /*channel*/) override {}
};

}  // namespace beez::logging
