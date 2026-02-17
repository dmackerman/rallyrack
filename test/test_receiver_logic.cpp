#include <unity.h>
#include "receiver_logic.h"

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

  // 2. Reset button pressed (court taken)
  unsigned long taken_time = now + 120000; // 2 minutes later
  simulateResetButtonPress(state, 1, taken_time, 200);
  TEST_ASSERT_FALSE(state.courts[0].available);
  TEST_ASSERT_TRUE(state.courts[0].inUse);
  TEST_ASSERT_EQUAL_UINT32(taken_time, state.courts[0].inUseSinceMs);
  TEST_ASSERT_EQUAL_UINT32(1, state.courts[0].waitSamples);
  TEST_ASSERT_EQUAL_FLOAT(120000.0f, state.courts[0].avgWaitMs);

  // 3. Transmitter arrives again (court returned)
  unsigned long returned_time = taken_time + 600000; // 10 minutes later
  simulateCourtAvailable(state, 1, returned_time);
  TEST_ASSERT_TRUE(state.courts[0].available);
  TEST_ASSERT_FALSE(state.courts[0].inUse);
  TEST_ASSERT_EQUAL_UINT32(returned_time, state.courts[0].availableSinceMs);
  TEST_ASSERT_EQUAL_UINT32(0, state.courts[0].inUseSinceMs);
}

void test_debounce_prevents_duplicate_presses()
{
  SystemState state;
  initSystemState(state);
  unsigned long now = 1000000;

  // First press
  simulateCourtAvailable(state, 1, now);
  simulateResetButtonPress(state, 1, now + 100000, 200);
  TEST_ASSERT_EQUAL_UINT32(1, state.courts[0].waitSamples);

  // Second press too soon (within 200ms debounce)
  simulateResetButtonPress(state, 1, now + 100050, 200);
  TEST_ASSERT_EQUAL_UINT32(1, state.courts[0].waitSamples); // Should not increase

  // Third press after debounce
  simulateCourtAvailable(state, 1, now + 200000);
  simulateResetButtonPress(state, 1, now + 300000, 200);
  TEST_ASSERT_EQUAL_UINT32(2, state.courts[0].waitSamples); // Should increase
}

void test_reset_without_available_court()
{
  SystemState state;
  initSystemState(state);
  unsigned long now = 1000000;

  // Press reset on idle court
  simulateResetButtonPress(state, 1, now, 200);
  TEST_ASSERT_TRUE(state.courts[0].inUse);
  TEST_ASSERT_EQUAL_UINT32(0, state.courts[0].waitSamples); // No wait recorded
}

// ============================================
// AVERAGING & STATISTICS TESTS
// ============================================

void test_averaging_multiple_waits()
{
  SystemState state;
  initSystemState(state);
  unsigned long now = 1000000;
  uint32_t debounce = 200;

  // First cycle: wait 2 minutes
  simulateCourtAvailable(state, 1, now);
  simulateResetButtonPress(state, 1, now + 120000, debounce);
  TEST_ASSERT_EQUAL_FLOAT(120000.0f, state.courts[0].avgWaitMs);
  TEST_ASSERT_EQUAL_UINT32(1, state.courts[0].waitSamples);

  // Second cycle: wait 4 minutes
  simulateCourtAvailable(state, 1, now + 400000);
  simulateResetButtonPress(state, 1, now + 400000 + 240000, debounce);
  // Average should be (120000 + 240000) / 2 = 180000
  TEST_ASSERT_EQUAL_FLOAT(180000.0f, state.courts[0].avgWaitMs);
  TEST_ASSERT_EQUAL_UINT32(2, state.courts[0].waitSamples);

  // Third cycle: wait 1 minute
  simulateCourtAvailable(state, 1, now + 800000);
  simulateResetButtonPress(state, 1, now + 800000 + 60000, debounce);
  // Average should be (120000 + 240000 + 60000) / 3 = 140000
  TEST_ASSERT_EQUAL_FLOAT(140000.0f, state.courts[0].avgWaitMs);
  TEST_ASSERT_EQUAL_UINT32(3, state.courts[0].waitSamples);
}

void test_averaging_single_sample()
{
  SystemState state;
  initSystemState(state);
  unsigned long now = 1000000;

  simulateCourtAvailable(state, 1, now);
  simulateResetButtonPress(state, 1, now + 180000, 200); // 3 min wait

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
  TEST_ASSERT_EQUAL_STRING("C1 N: -- A: 2m", display.buffer);
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
  TEST_ASSERT_EQUAL_STRING("C1 N: 3m A: 2m", display.buffer);
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
  TEST_ASSERT_EQUAL_STRING("C1 N:30m A:10m", display.buffer);
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
  unsigned long now = 1000000 + 240000; // 4 minutes later
  display.generate(state.courts[0], 1, now);
  TEST_ASSERT_EQUAL_STRING("C1 U: 4m A: 2m", display.buffer);
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
  unsigned long now = 1000000 + 2400000; // 40 minutes later
  display.generate(state.courts[0], 1, now);
  TEST_ASSERT_EQUAL_STRING("C1 U:40m A: 5m", display.buffer);
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

  unsigned long now = 1000000 + 300000; // 5 minutes later

  CourtDisplayText display1, display2, display3;
  display1.generate(state.courts[0], 1, now);
  display2.generate(state.courts[1], 2, now);
  display3.generate(state.courts[2], 3, now);

  TEST_ASSERT_EQUAL_STRING("C1 N: -- A: 2m", display1.buffer);
  TEST_ASSERT_EQUAL_STRING("C2 N: 5m A: 3m", display2.buffer);
  TEST_ASSERT_EQUAL_STRING("C3 U: 5m A: 4m", display3.buffer);
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

  // Set up court 1
  simulateCourtAvailable(state, 1, now);
  simulateResetButtonPress(state, 1, now + 60000, 200);

  // Set up court 3
  simulateCourtAvailable(state, 3, now + 100000);
  simulateResetButtonPress(state, 3, now + 100000 + 120000, 200);

  // Verify they're independent
  TEST_ASSERT_TRUE(state.courts[0].inUse);
  TEST_ASSERT_FALSE(state.courts[0].available);
  TEST_ASSERT_EQUAL_UINT32(1, state.courts[0].waitSamples);

  TEST_ASSERT_TRUE(state.courts[2].inUse);
  TEST_ASSERT_FALSE(state.courts[2].available);
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
  // This test verifies a realistic day scenario with multiple courts
  SystemState state;
  initSystemState(state);
  unsigned long now = 1000000; // Start at 1 million ms
  uint32_t debounce = 200;

  // Court 1: Quick turnover (2 min waits)
  simulateCourtAvailable(state, 1, now);
  simulateResetButtonPress(state, 1, now + 120000, debounce);
  simulateCourtAvailable(state, 1, now + 500000);
  simulateResetButtonPress(state, 1, now + 620000, debounce);
  TEST_ASSERT_EQUAL_UINT32(2, state.courts[0].waitSamples);

  // Court 2: Longer waits (5-7 min)
  simulateCourtAvailable(state, 2, now + 100000);
  simulateResetButtonPress(state, 2, now + 400000, debounce);
  simulateCourtAvailable(state, 2, now + 800000);
  simulateResetButtonPress(state, 2, now + 1220000, debounce);
  TEST_ASSERT_EQUAL_UINT32(2, state.courts[1].waitSamples);

  // Verify global average considers both courts
  unsigned long globalAvg = globalAverageWaitMs(state);
  TEST_ASSERT_GREATER_THAN_UINT32(0, globalAvg);

  // Verify court independence
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

  // Boundary and edge case tests
  RUN_TEST(test_invalid_court_ids);
  RUN_TEST(test_multiple_courts_independent);

  // Integration tests
  RUN_TEST(test_realistic_scenario_full_day);

  UNITY_END();
  return 0;
}
