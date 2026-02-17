// ============================================
// RECEIVER LOGIC (Testable Functions)
// ============================================
// Core state machine and calculation logic
// extracted for unit testing

#pragma once

#include <cstdint>
#include <cstring>
#include <cstdio>

#define NUM_COURTS 8

// ============================================
// TIME CALCULATIONS
// ============================================

inline unsigned long minutesFromMs(unsigned long valueMs)
{
  return (valueMs + 30000UL) / 60000UL;
}

// ============================================
// COURT STATE & STATISTICS
// ============================================

struct CourtState
{
  bool available;
  bool inUse;
  unsigned long availableSinceMs;
  unsigned long inUseSinceMs;
  float avgWaitMs;
  uint32_t waitSamples;
  unsigned long lastResetPressMs;
};

struct SystemState
{
  CourtState courts[NUM_COURTS];
};

// Initialize system state
inline void initSystemState(SystemState &state)
{
  for (int i = 0; i < NUM_COURTS; i++)
  {
    state.courts[i].available = false;
    state.courts[i].inUse = false;
    state.courts[i].availableSinceMs = 0;
    state.courts[i].inUseSinceMs = 0;
    state.courts[i].avgWaitMs = 0.0f;
    state.courts[i].waitSamples = 0;
    state.courts[i].lastResetPressMs = 0;
  }
}

// Get global average wait time
inline unsigned long globalAverageWaitMs(const SystemState &state)
{
  uint64_t weightedSum = 0;
  uint32_t sampleCount = 0;

  for (int i = 0; i < NUM_COURTS; i++)
  {
    weightedSum += (uint64_t)state.courts[i].avgWaitMs * state.courts[i].waitSamples;
    sampleCount += state.courts[i].waitSamples;
  }

  if (sampleCount == 0)
    return 0;

  return weightedSum / sampleCount;
}

// ============================================
// STATE TRANSITIONS
// ============================================

// Simulate receiving a transmitter signal (court available)
inline void simulateCourtAvailable(SystemState &state, int courtId, unsigned long now)
{
  if (courtId < 1 || courtId > NUM_COURTS)
    return;

  int idx = courtId - 1;

  // If court was in use, stop the in-use timer
  if (state.courts[idx].inUse)
  {
    state.courts[idx].inUse = false;
    state.courts[idx].inUseSinceMs = 0;
  }

  // Mark as available if not already
  if (!state.courts[idx].available)
  {
    state.courts[idx].available = true;
    state.courts[idx].availableSinceMs = now;
  }
}

// Simulate pressing reset button (court taken off rack)
inline void simulateResetButtonPress(SystemState &state, int courtId, unsigned long now, uint32_t debounceMs)
{
  if (courtId < 1 || courtId > NUM_COURTS)
    return;

  int idx = courtId - 1;

  // Check debounce
  if (now - state.courts[idx].lastResetPressMs <= debounceMs)
    return;

  // If court was available, record wait time
  if (state.courts[idx].available && state.courts[idx].availableSinceMs > 0)
  {
    unsigned long waitMs = now - state.courts[idx].availableSinceMs;
    state.courts[idx].waitSamples++;
    state.courts[idx].avgWaitMs += (waitMs - state.courts[idx].avgWaitMs) / state.courts[idx].waitSamples;
  }

  // Start in-use timer
  state.courts[idx].available = false;
  state.courts[idx].availableSinceMs = 0;
  state.courts[idx].inUse = true;
  state.courts[idx].inUseSinceMs = now;

  state.courts[idx].lastResetPressMs = now;
}

// ============================================
// DISPLAY HELPERS
// ============================================

// Get display text for a court (for testing)
class CourtDisplayText
{
public:
  char buffer[50];

  void generate(const CourtState &court, int courtNum, unsigned long now)
  {
    unsigned long avgMin = minutesFromMs((unsigned long)(court.avgWaitMs + 0.5f));

    if (court.inUse && court.inUseSinceMs > 0)
    {
      // Court is in use
      unsigned long inUseMs = now - court.inUseSinceMs;
      unsigned long inUseMin = minutesFromMs(inUseMs);
      snprintf(buffer, sizeof(buffer), "C%d U:%2lum A:%2lum", courtNum, inUseMin, avgMin);
    }
    else if (court.available && court.availableSinceMs > 0)
    {
      // Court is available
      unsigned long nowWaitMs = now - court.availableSinceMs;
      unsigned long nowWaitMin = minutesFromMs(nowWaitMs);
      snprintf(buffer, sizeof(buffer), "C%d N:%2lum A:%2lum", courtNum, nowWaitMin, avgMin);
    }
    else
    {
      // Court is idle
      snprintf(buffer, sizeof(buffer), "C%d N: -- A:%2lum", courtNum, avgMin);
    }
  }

  const char *str() const { return buffer; }
};
