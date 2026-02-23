// OLED display preview tool (native build)
// Renders the OLED lines to stdout for quick visual checks.

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include "receiver_logic.h"
#include "receiver_fixture.h"

namespace
{
  void printPage(const SystemState &state, int page)
  {
    unsigned long overallMs = globalAverageWaitMs(state);

    // Title row
    std::printf("RallyRack                Avg:%lum\n", minutesFromMs(overallMs));
    // Column headers
    std::printf("#  Status      Now    Avg\n");
    std::printf("------------------------------\n");

    int baseCourt = page * 4;
    for (int row = 0; row < 4; row++)
    {
      int courtIndex = baseCourt + row;
      CourtDisplayText line;
      line.generate(state.courts[courtIndex], courtIndex + 1, kPreviewNowMs);
      std::printf("%s\n", line.str());
    }
  }
}

int main(int argc, char **argv)
{
  bool printBoth = true;
  int page = 0;

  if (argc >= 2)
  {
    if (std::strcmp(argv[1], "--page") == 0 && argc >= 3)
    {
      int requested = std::atoi(argv[2]);
      if (requested == 1 || requested == 2)
      {
        page = requested - 1;
        printBoth = false;
      }
    }
  }

  SystemState state;
  seedPreviewState(state);

  if (printBoth)
  {
    printPage(state, 0);
    std::printf("\n");
    printPage(state, 1);
  }
  else
  {
    printPage(state, page);
  }

  return 0;
}
