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

#ifndef FAULT_TIMEOUT_MS
#define FAULT_TIMEOUT_MS 45000
#endif

inline void fmtMMSS(char *buf, size_t sz, unsigned long ms)
{
  unsigned long totalSec = ms / 1000;
  unsigned long m = totalSec / 60;
  unsigned long s = totalSec % 60;
  if (m > 99)
    m = 99;
  snprintf(buf, sz, "%02lu:%02lu", m, s);
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
  unsigned long lastHeardMs;
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
    state.courts[i].lastHeardMs = 0;
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

// Simulate court becoming occupied (button press, game starts)
inline void simulateCourtOccupied(SystemState &state, int courtId, unsigned long now, uint32_t debounceMs = 0)
{
  if (courtId < 1 || courtId > NUM_COURTS)
    return;

  int idx = courtId - 1;

  if (debounceMs > 0 && now - state.courts[idx].lastResetPressMs <= debounceMs)
    return;

  state.courts[idx].available = false;
  state.courts[idx].availableSinceMs = 0;
  state.courts[idx].inUse = true;
  state.courts[idx].inUseSinceMs = now;
  state.courts[idx].lastHeardMs = now;
  state.courts[idx].lastResetPressMs = now;
}

// Simulate court becoming available (game ends, button pressed when occupied)
inline void simulateCourtFreed(SystemState &state, int courtId, unsigned long now)
{
  if (courtId < 1 || courtId > NUM_COURTS)
    return;

  int idx = courtId - 1;

  // Record game duration into rolling average
  if (state.courts[idx].inUse && state.courts[idx].inUseSinceMs > 0)
  {
    unsigned long gameMs = now - state.courts[idx].inUseSinceMs;
    state.courts[idx].waitSamples++;
    state.courts[idx].avgWaitMs += (gameMs - state.courts[idx].avgWaitMs) / state.courts[idx].waitSamples;
  }

  state.courts[idx].inUse = false;
  state.courts[idx].inUseSinceMs = 0;
  state.courts[idx].available = true;
  state.courts[idx].availableSinceMs = now;
  state.courts[idx].lastHeardMs = now;
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

    char numStr[4];
    char statusStr[8];
    char nowStr[7];
    char avgStr[6];

    snprintf(numStr, sizeof(numStr), "%d", courtNum);
    snprintf(avgStr, sizeof(avgStr), "%lum", avgMin);

    if (court.inUse && court.inUseSinceMs > 0)
    {
      bool fault = (court.lastHeardMs > 0) &&
                   (now - court.lastHeardMs > FAULT_TIMEOUT_MS);
      if (fault)
      {
        snprintf(statusStr, sizeof(statusStr), "Fault");
        snprintf(nowStr, sizeof(nowStr), "??");
      }
      else
      {
        snprintf(statusStr, sizeof(statusStr), "Started");
        fmtMMSS(nowStr, sizeof(nowStr), now - court.inUseSinceMs);
      }
    }
    else if (court.available)
    {
      snprintf(statusStr, sizeof(statusStr), "Open");
      snprintf(nowStr, sizeof(nowStr), "--");
    }
    else
    {
      snprintf(statusStr, sizeof(statusStr), "---");
      snprintf(nowStr, sizeof(nowStr), "--");
    }

    snprintf(buffer, sizeof(buffer), "%s %s %s %s", numStr, statusStr, nowStr, avgStr);
  }

  const char *str() const { return buffer; }
};
