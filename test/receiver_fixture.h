// Shared test fixtures for receiver logic and preview tools.

#pragma once

#include "receiver_logic.h"

constexpr unsigned long kPreviewNowMs = 1000000UL;

inline void seedPreviewState(SystemState &state)
{
  initSystemState(state);

  // Court 1: in use for 4 min
  state.courts[0].inUse = true;
  state.courts[0].inUseSinceMs = kPreviewNowMs - (4UL * 60UL * 1000UL);
  state.courts[0].avgWaitMs = 2UL * 60UL * 1000UL;
  state.courts[0].waitSamples = 3;

  // Court 2: available for 1 min
  state.courts[1].available = true;
  state.courts[1].availableSinceMs = kPreviewNowMs - (1UL * 60UL * 1000UL);
  state.courts[1].avgWaitMs = 3UL * 60UL * 1000UL;
  state.courts[1].waitSamples = 2;

  // Court 3: idle, avg 2 min
  state.courts[2].avgWaitMs = 2UL * 60UL * 1000UL;
  state.courts[2].waitSamples = 1;

  // Court 4: available for 5 min
  state.courts[3].available = true;
  state.courts[3].availableSinceMs = kPreviewNowMs - (5UL * 60UL * 1000UL);
  state.courts[3].avgWaitMs = 4UL * 60UL * 1000UL;
  state.courts[3].waitSamples = 4;

  // Court 5: idle, avg 1 min
  state.courts[4].avgWaitMs = 1UL * 60UL * 1000UL;
  state.courts[4].waitSamples = 2;

  // Court 6: in use for 12 min
  state.courts[5].inUse = true;
  state.courts[5].inUseSinceMs = kPreviewNowMs - (12UL * 60UL * 1000UL);
  state.courts[5].avgWaitMs = 6UL * 60UL * 1000UL;
  state.courts[5].waitSamples = 5;

  // Court 7: available for 8 min
  state.courts[6].available = true;
  state.courts[6].availableSinceMs = kPreviewNowMs - (8UL * 60UL * 1000UL);
  state.courts[6].avgWaitMs = 5UL * 60UL * 1000UL;
  state.courts[6].waitSamples = 2;

  // Court 8: idle, avg 3 min
  state.courts[7].avgWaitMs = 3UL * 60UL * 1000UL;
  state.courts[7].waitSamples = 1;
}