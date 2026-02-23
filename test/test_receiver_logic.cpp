#include <unity.h>
#include "receiver_logic.h"
#include "receiver_fixture.h"

// ============================================
// TIME CONVERSION TESTS
// ============================================

void test_minutesFromMs_zero()
{
  TEST_ASSERT_EQUAL_UINT32(0, minutesFromMs(0));
}

void test_minutesFromMs_rounds_correctly()
{
  TEST_ASSERT_EQUAL_UINT32(1, minutesFromMs(30000));   // 30 sec = 1 min
  TEST_ASSERT_EQUAL_UINT32(1, minutesFromMs(60000));   // 1 min
  TEST_ASSERT_EQUAL_UINT32(2, minutesFromMs(90000));   // 1.5 min = 2 min
  TEST_ASSERT_EQUAL_UINT32(5, minutesFromMs(300000));  // 5 min
  TEST_ASSERT_EQUAL_UINT32(10, minutesFromMs(600000)); // 10 min
}

// ============================================
// INITIALIZATION TESTS
// ============================================

void test_system_init()
{
  SystemState state;
  initSystemState(state);

  for (int i = 0; i < NUM_COURTS; i++)
  {
    TEST_ASSERT_FALSE(state.courts[i].available);
    TEST_ASSERT_FALSE(state.courts[i].inUse);
    TEST_ASSERT_EQUAL_UINT32(0, state.courts[i].availableSinceMs);
    TEST_ASSERT_EQUAL_UINT32(0, state.courts[i].waitSamples);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, state.courts[i].avgWaitMs);
  }
}

// ============================================
// AVERAGING TESTS
// ============================================

void test_global_average_empty()
{
  SystemState state;
  initSystemState(state);
  TEST_ASSERT_EQUAL_UINT32(0, globalAverageWaitMs(state));
}

void test_global_average_single_court()
{
  SystemState state;
  initSystemState(state);
  state.courts[0].avgWaitMs = 60000;
  state.courts[0].waitSamples = 1;

  TEST_ASSERT_EQUAL_UINT32(60000, globalAverageWaitMs(state));
}

void test_global_average_weighted()
{
  SystemState state;
  initSystemState(state);

  // Court 0: 60000ms average, 2 samples
  state.courts[0].avgWaitMs = 60000;
  state.courts[0].waitSamples = 2;

  // Court 1: 120000ms average, 1 sample
  state.courts[1].avgWaitMs = 120000;
  state.courts[1].waitSamples = 1;

  // Expected: (60000*2 + 120000*1) / 3 = 240000 / 3 = 80000
  TEST_ASSERT_EQUAL_UINT32(80000, globalAverageWaitMs(state));
}

void test_global_average_uniform()
{
  SystemState state;
  initSystemState(state);

  // All courts: 120000ms, 1 sample each
  for (int i = 0; i < 4; i++)
  {
    state.courts[i].avgWaitMs = 120000;
    state.courts[i].waitSamples = 1;
  }

  TEST_ASSERT_EQUAL_UINT32(120000, globalAverageWaitMs(state));
}

// ============================================
// STATE MACHINE TESTS
// ============================================

void test_court_lifecycle()
{
  SystemState state;
  initSystemState(state);
  unsigned long now = 1000000;

  // 1. Transmitter arrives (court available)
  simulateCourtAvailable(state, 1, now);
  TEST_ASSERT_TRUE(state.courts[0].available);
  TEST_ASSERT_FALSE(state.courts[0].inUse);
  TEST_ASSERT_EQUAL_UINT32(now, state.courts[0].availableSinceMs);
  TEST_ASSERT_EQUAL_UINT32(0, state.courts[0].waitSamples);

  // 2. Button pressed — game starts (court occupied)
  unsigned long taken_time = now + 120000; // 2 minutes later
  simulateCourtOccupied(state, 1, taken_time, 200);
  TEST_ASSERT_FALSE(state.courts[0].available);
  TEST_ASSERT_TRUE(state.courts[0].inUse);
  TEST_ASSERT_EQUAL_UINT32(taken_time, state.courts[0].inUseSinceMs);
  TEST_ASSERT_EQUAL_UINT32(0, state.courts[0].waitSamples); // nothing recorded until freed
  TEST_ASSERT_EQUAL_FLOAT(0.0f, state.courts[0].avgWaitMs);

  // 3. Button pressed — game ends (court freed)
  unsigned long returned_time = taken_time + 600000; // 10 minutes of play
  simulateCourtFreed(state, 1, returned_time);
  TEST_ASSERT_TRUE(state.courts[0].available);
  TEST_ASSERT_FALSE(state.courts[0].inUse);
  TEST_ASSERT_EQUAL_UINT32(returned_time, state.courts[0].availableSinceMs);
  TEST_ASSERT_EQUAL_UINT32(0, state.courts[0].inUseSinceMs);
  TEST_ASSERT_EQUAL_UINT32(1, state.courts[0].waitSamples);
  TEST_ASSERT_EQUAL_FLOAT(600000.0f, state.courts[0].avgWaitMs); // 10 min game
}

void test_debounce_prevents_duplicate_presses()
{
  SystemState state;
  initSystemState(state);
  unsigned long now = 1000000;

  // First press — court occupied
  simulateCourtAvailable(state, 1, now);
  simulateCourtOccupied(state, 1, now + 100000, 200);
  TEST_ASSERT_TRUE(state.courts[0].inUse);

  // Second press too soon (within 200ms debounce) — ignored
  simulateCourtOccupied(state, 1, now + 100050, 200);
  TEST_ASSERT_EQUAL_UINT32(now + 100000, state.courts[0].inUseSinceMs); // unchanged

  // Free the court, then press again after debounce
  simulateCourtFreed(state, 1, now + 200000);
  simulateCourtOccupied(state, 1, now + 400000, 200);
  TEST_ASSERT_TRUE(state.courts[0].inUse);
  TEST_ASSERT_EQUAL_UINT32(1, state.courts[0].waitSamples); // one game recorded
}

void test_reset_without_available_court()
{
  SystemState state;
  initSystemState(state);
  unsigned long now = 1000000;

  // Occupy an idle court (no prior available timestamp)
  simulateCourtOccupied(state, 1, now, 0);
  TEST_ASSERT_TRUE(state.courts[0].inUse);
  TEST_ASSERT_EQUAL_UINT32(0, state.courts[0].waitSamples); // no game to record yet
}

// ============================================
// AVERAGING & STATISTICS TESTS
// ============================================

void test_averaging_multiple_waits()
{
  SystemState state;
  initSystemState(state);
  unsigned long now = 1000000;

  // Game 1: 2 minute game
  simulateCourtAvailable(state, 1, now);
  simulateCourtOccupied(state, 1, now + 10000, 0);
  simulateCourtFreed(state, 1, now + 10000 + 120000); // 2 min game
  TEST_ASSERT_EQUAL_FLOAT(120000.0f, state.courts[0].avgWaitMs);
  TEST_ASSERT_EQUAL_UINT32(1, state.courts[0].waitSamples);

  // Game 2: 4 minute game
  simulateCourtOccupied(state, 1, now + 200000, 0);
  simulateCourtFreed(state, 1, now + 200000 + 240000); // 4 min game
  // Average: (120000 + 240000) / 2 = 180000
  TEST_ASSERT_EQUAL_FLOAT(180000.0f, state.courts[0].avgWaitMs);
  TEST_ASSERT_EQUAL_UINT32(2, state.courts[0].waitSamples);

  // Game 3: 1 minute game
  simulateCourtOccupied(state, 1, now + 500000, 0);
  simulateCourtFreed(state, 1, now + 500000 + 60000); // 1 min game
  // Average: (120000 + 240000 + 60000) / 3 = 140000
  TEST_ASSERT_EQUAL_FLOAT(140000.0f, state.courts[0].avgWaitMs);
  TEST_ASSERT_EQUAL_UINT32(3, state.courts[0].waitSamples);
}

void test_averaging_single_sample()
{
  SystemState state;
  initSystemState(state);
  unsigned long now = 1000000;

  simulateCourtAvailable(state, 1, now);
  simulateCourtOccupied(state, 1, now + 10000, 0);
  simulateCourtFreed(state, 1, now + 10000 + 180000); // 3 min game

  TEST_ASSERT_EQUAL_FLOAT(180000.0f, state.courts[0].avgWaitMs);
  TEST_ASSERT_EQUAL_UINT32(1, state.courts[0].waitSamples);
}

// ============================================
// DISPLAY TEXT TESTS
// ============================================

void test_display_text_idle()
{
  SystemState state;
  initSystemState(state);
  state.courts[0].avgWaitMs = 120000;
  state.courts[0].waitSamples = 1;

  CourtDisplayText display;
  display.generate(state.courts[0], 1, 1000000);
  TEST_ASSERT_EQUAL_STRING("1 --- -- 2m", display.buffer);
}

void test_display_text_available()
{
  SystemState state;
  initSystemState(state);
  state.courts[0].available = true;
  state.courts[0].availableSinceMs = 1000000;
  state.courts[0].avgWaitMs = 120000;
  state.courts[0].waitSamples = 1;

  CourtDisplayText display;
  unsigned long now = 1000000 + 180000; // 3 minutes later
  display.generate(state.courts[0], 1, now);
  TEST_ASSERT_EQUAL_STRING("1 Open -- 2m", display.buffer); // Open courts always show --
}

void test_display_text_available_long_wait()
{
  SystemState state;
  initSystemState(state);
  state.courts[0].available = true;
  state.courts[0].availableSinceMs = 1000000;
  state.courts[0].avgWaitMs = 600000;
  state.courts[0].waitSamples = 2;

  CourtDisplayText display;
  unsigned long now = 1000000 + 1800000; // 30 minutes later
  display.generate(state.courts[0], 1, now);
  TEST_ASSERT_EQUAL_STRING("1 Open -- 10m", display.buffer); // Now still --, avg=10m
}

void test_display_text_inuse()
{
  SystemState state;
  initSystemState(state);
  state.courts[0].inUse = true;
  state.courts[0].inUseSinceMs = 1000000;
  state.courts[0].avgWaitMs = 120000;
  state.courts[0].waitSamples = 1;

  CourtDisplayText display;
  unsigned long now = 1000000 + 240000;     // 4 minutes later
  state.courts[0].lastHeardMs = now - 5000; // recent heartbeat
  display.generate(state.courts[0], 1, now);
  TEST_ASSERT_EQUAL_STRING("1 Started 04:00 2m", display.buffer);
}

void test_display_text_inuse_long_time()
{
  SystemState state;
  initSystemState(state);
  state.courts[0].inUse = true;
  state.courts[0].inUseSinceMs = 1000000;
  state.courts[0].avgWaitMs = 300000;
  state.courts[0].waitSamples = 1;

  CourtDisplayText display;
  unsigned long now = 1000000 + 2400000;    // 40 minutes later
  state.courts[0].lastHeardMs = now - 5000; // recent heartbeat
  display.generate(state.courts[0], 1, now);
  TEST_ASSERT_EQUAL_STRING("1 Started 40:00 5m", display.buffer);
}

void test_display_text_multiple_courts()
{
  SystemState state;
  initSystemState(state);

  // Court 1: idle
  state.courts[0].avgWaitMs = 120000;
  state.courts[0].waitSamples = 1;

  // Court 2: available
  state.courts[1].available = true;
  state.courts[1].availableSinceMs = 1000000;
  state.courts[1].avgWaitMs = 180000;
  state.courts[1].waitSamples = 1;

  // Court 3: in use
  state.courts[2].inUse = true;
  state.courts[2].inUseSinceMs = 1000000;
  state.courts[2].avgWaitMs = 240000;
  state.courts[2].waitSamples = 1;

  unsigned long now = 1000000 + 300000;     // 5 minutes later
  state.courts[2].lastHeardMs = now - 5000; // recent heartbeat

  CourtDisplayText display1, display2, display3;
  display1.generate(state.courts[0], 1, now);
  display2.generate(state.courts[1], 2, now);
  display3.generate(state.courts[2], 3, now);

  TEST_ASSERT_EQUAL_STRING("1 --- -- 2m", display1.buffer);
  TEST_ASSERT_EQUAL_STRING("2 Open -- 3m", display2.buffer);
  TEST_ASSERT_EQUAL_STRING("3 Started 05:00 4m", display3.buffer);
}

void test_display_text_preview_fixture()
{
  SystemState state;
  seedPreviewState(state);

  CourtDisplayText display;
  display.generate(state.courts[0], 1, kPreviewNowMs);
  TEST_ASSERT_EQUAL_STRING("1 Started 04:00 2m", display.buffer);

  display.generate(state.courts[1], 2, kPreviewNowMs);
  TEST_ASSERT_EQUAL_STRING("2 Open -- 3m", display.buffer);
}

// ============================================
// FAULT DETECTION TESTS
// ============================================

void test_fault_triggers_after_timeout()
{
  SystemState state;
  initSystemState(state);
  unsigned long now = 1000000;

  simulateCourtAvailable(state, 1, now);
  simulateCourtOccupied(state, 1, now + 1000, 0);
  // lastHeardMs set to now+1000 by simulateCourtOccupied

  CourtDisplayText d;

  // Before timeout — shows Started
  d.generate(state.courts[0], 1, now + 1000 + 10000);
  TEST_ASSERT_NOT_NULL(strstr(d.buffer, "Started"));

  // After timeout — shows Fault
  d.generate(state.courts[0], 1, now + 1000 + FAULT_TIMEOUT_MS + 1000);
  TEST_ASSERT_NOT_NULL(strstr(d.buffer, "Fault"));
  TEST_ASSERT_NOT_NULL(strstr(d.buffer, "??"));
}

void test_fault_clears_on_recovery()
{
  SystemState state;
  initSystemState(state);
  unsigned long now = 1000000;

  simulateCourtAvailable(state, 1, now);
  simulateCourtOccupied(state, 1, now + 1000, 0);

  unsigned long faultTime = now + 1000 + FAULT_TIMEOUT_MS + 5000;

  CourtDisplayText d;

  // Confirm fault
  d.generate(state.courts[0], 1, faultTime);
  TEST_ASSERT_NOT_NULL(strstr(d.buffer, "Fault"));

  // Packet arrives — update lastHeardMs (simulates receiver hearing heartbeat again)
  state.courts[0].lastHeardMs = faultTime + 1000;

  // Now should show Started again
  d.generate(state.courts[0], 1, faultTime + 2000);
  TEST_ASSERT_NOT_NULL(strstr(d.buffer, "Started"));
}

// ============================================
// BOUNDARY & EDGE CASE TESTS
// ============================================

void test_invalid_court_ids()
{
  SystemState state;
  initSystemState(state);
  unsigned long now = 1000000;

  // Court 0 (invalid, too low) - should not modify
  uint32_t originalSamples = state.courts[0].waitSamples;
  simulateCourtAvailable(state, 0, now);
  TEST_ASSERT_FALSE(state.courts[0].available);
  TEST_ASSERT_EQUAL_UINT32(originalSamples, state.courts[0].waitSamples);

  // Court 9 (invalid, too high) - should not modify court 7
  originalSamples = state.courts[7].waitSamples;
  simulateCourtAvailable(state, 9, now);
  TEST_ASSERT_FALSE(state.courts[7].available);
  TEST_ASSERT_EQUAL_UINT32(originalSamples, state.courts[7].waitSamples);
}

void test_multiple_courts_independent()
{
  SystemState state;
  initSystemState(state);
  unsigned long now = 1000000;

  // Court 1: occupy then free (records 1 game)
  simulateCourtAvailable(state, 1, now);
  simulateCourtOccupied(state, 1, now + 60000, 0);
  simulateCourtFreed(state, 1, now + 60000 + 300000); // 5 min game

  // Court 3: occupy then free (records 1 game)
  simulateCourtAvailable(state, 3, now + 100000);
  simulateCourtOccupied(state, 3, now + 100000 + 120000, 0);
  simulateCourtFreed(state, 3, now + 100000 + 120000 + 600000); // 10 min game

  // Verify they're independent
  TEST_ASSERT_FALSE(state.courts[0].inUse); // freed
  TEST_ASSERT_TRUE(state.courts[0].available);
  TEST_ASSERT_EQUAL_UINT32(1, state.courts[0].waitSamples);

  TEST_ASSERT_FALSE(state.courts[2].inUse); // freed
  TEST_ASSERT_TRUE(state.courts[2].available);
  TEST_ASSERT_EQUAL_UINT32(1, state.courts[2].waitSamples);

  // Court 2 should be unaffected
  TEST_ASSERT_FALSE(state.courts[1].available);
  TEST_ASSERT_FALSE(state.courts[1].inUse);
  TEST_ASSERT_EQUAL_UINT32(0, state.courts[1].waitSamples);
}

// ============================================
// INTEGRATION TESTS
// ============================================

void test_realistic_scenario_full_day()
{
  SystemState state;
  initSystemState(state);
  unsigned long now = 1000000;

  // Court 1: two 2-minute games
  simulateCourtAvailable(state, 1, now);
  simulateCourtOccupied(state, 1, now + 10000, 0);
  simulateCourtFreed(state, 1, now + 10000 + 120000); // 2 min game
  simulateCourtOccupied(state, 1, now + 500000, 0);
  simulateCourtFreed(state, 1, now + 500000 + 120000); // 2 min game
  TEST_ASSERT_EQUAL_UINT32(2, state.courts[0].waitSamples);

  // Court 2: two longer games (5 min, 7 min)
  simulateCourtAvailable(state, 2, now + 100000);
  simulateCourtOccupied(state, 2, now + 200000, 0);
  simulateCourtFreed(state, 2, now + 200000 + 300000); // 5 min game
  simulateCourtOccupied(state, 2, now + 700000, 0);
  simulateCourtFreed(state, 2, now + 700000 + 420000); // 7 min game
  TEST_ASSERT_EQUAL_UINT32(2, state.courts[1].waitSamples);

  // Global average should reflect both courts
  unsigned long globalAvg = globalAverageWaitMs(state);
  TEST_ASSERT_GREATER_THAN_UINT32(0, globalAvg);

  // Courts should be independent
  TEST_ASSERT_NOT_EQUAL(state.courts[0].avgWaitMs, state.courts[1].avgWaitMs);
}

void setUp(void) { /* before each test */ }
void tearDown(void) { /* after each test */ }

int main(int argc, char **argv)
{
  UNITY_BEGIN();

  // Time conversion tests
  RUN_TEST(test_minutesFromMs_zero);
  RUN_TEST(test_minutesFromMs_rounds_correctly);

  // Initialization tests
  RUN_TEST(test_system_init);

  // Averaging tests
  RUN_TEST(test_global_average_empty);
  RUN_TEST(test_global_average_single_court);
  RUN_TEST(test_global_average_weighted);
  RUN_TEST(test_global_average_uniform);

  // State machine tests
  RUN_TEST(test_court_lifecycle);
  RUN_TEST(test_debounce_prevents_duplicate_presses);
  RUN_TEST(test_reset_without_available_court);

  // Averaging & statistics tests
  RUN_TEST(test_averaging_multiple_waits);
  RUN_TEST(test_averaging_single_sample);

  // Display text tests
  RUN_TEST(test_display_text_idle);
  RUN_TEST(test_display_text_available);
  RUN_TEST(test_display_text_available_long_wait);
  RUN_TEST(test_display_text_inuse);
  RUN_TEST(test_display_text_inuse_long_time);
  RUN_TEST(test_display_text_multiple_courts);
  RUN_TEST(test_display_text_preview_fixture);

  // Fault detection tests
  RUN_TEST(test_fault_triggers_after_timeout);
  RUN_TEST(test_fault_clears_on_recovery);

  // Boundary and edge case tests
  RUN_TEST(test_invalid_court_ids);
  RUN_TEST(test_multiple_courts_independent);

  // Integration tests
  RUN_TEST(test_realistic_scenario_full_day);

  UNITY_END();
  return 0;
}
