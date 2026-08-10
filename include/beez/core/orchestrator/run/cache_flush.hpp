#pragma once

namespace beez::core
{

class Orchestrator;

namespace orchestrator_detail
{

void flushBufferedCacheWrites(Orchestrator& orchestrator);
void flushBufferedCacheWritesForPhase(Orchestrator& orchestrator);
void flushBufferedCacheWritesIfEndStrategy(Orchestrator& orchestrator);
void flushBufferedCacheWritesAtRunEnd(Orchestrator& orchestrator);

}  // namespace orchestrator_detail
}  // namespace beez::core
