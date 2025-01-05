#include "podcaster/sdl_utils.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Parse description, swallow newlines") {
  const std::string mapping =
      "19000000010000000100000000010000,Anbernic RG28XX "
      "Controller,platform:Linux,b:b3,a:b4,dpdown:h0.4,dpleft:h0.8,"
      "rightshoulder:b8,leftshoulder:b7,righttrigger:b13,dpright:h0.2,back:b9,"
      "start:b12,dpup:h0.1,y:b6,x:b5,guide:b11,";

  const std::string swapped_mapping =
      "19000000010000000100000000010000,Anbernic RG28XX "
      "Controller,platform:Linux,b:b4,a:b3,dpdown:h0.4,dpleft:h0.8,"
      "rightshoulder:b8,leftshoulder:b7,righttrigger:b13,dpright:h0.2,back:b9,"
      "start:b12,dpup:h0.1,y:b5,x:b6,guide:b11,";

  REQUIRE(sdl::SwapABXYButtons(mapping) == swapped_mapping);
}