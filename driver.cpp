/* This is main driver of the lp-recognizer
   tool. The tool is used to automatically
   recognize license plates and it uses
   the OpenALPR library.

   Copyright (c) 2022 Djordje Todorovic <djolertrk@gmail.com>.
*/

#include <iostream>
#include <string>

#include "alpr.h"

int main(int argc, char const *argv[])
{
  std::cout << "lp-recognizer\n";

  // FIXME: This should be dynamic set up.
  alpr::Alpr openalpr("us", "/etc/openalpr/openalpr.conf");

  if (!openalpr.isLoaded()) {
    std::cerr << "Error with loading OpenALPR\n";
    return 1;
  }

  alpr::AlprResults results = openalpr.recognize("./plate-example.jpg");

  for (int i = 0; i < results.plates.size(); i++) {
    alpr::AlprPlateResult plate = results.plates[i];
    std::cout << "plate" << i << ": " << plate.topNPlates.size() << " results" << std::endl;

    for (int k = 0; k < plate.topNPlates.size(); k++) {
      alpr::AlprPlate candidate = plate.topNPlates[k];
      std::cout << "    - " << candidate.characters << "\t confidence: " << candidate.overall_confidence;
      std::cout << "\t pattern_match: " << candidate.matches_template << std::endl;
    }
  }

  return 0;
}
