#include "podcaster/podcaster_impl.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Parse description, swallow newlines") {
  auto parsed = podcaster::ParseDescription("foo<br><br><br><br>bar");
  REQUIRE(parsed.short_description == "foo\n\nbar\n");
  REQUIRE(parsed.long_description == "");
}

TEST_CASE("Parse description, limit preview length") {
  auto parsed = podcaster::ParseDescription("foo bar baz qux");
  REQUIRE(parsed.short_description == "foo bar baz qux\n");
  REQUIRE(parsed.long_description == "");
}

std::string ToBytes(const std::string& str) {
  std::stringstream result;
  for (char c : str) {
    result << std::hex << std::setw(2) << std::setfill('0')
           << static_cast<int>(c);
  }
  return result.str();
}

TEST_CASE("Parse description, complex example") {
  auto parsed = podcaster::ParseDescription(
      R"(<p>Hoje Lucas e Marcelo (no modo lero-lero) caem de boca no peru e passam mais um Natal junto com você!</p> <p><strong>Coleção </strong>⁠⁠⁠⁠⁠⁠<a href="https://www.lolja.com.br/bocadinhas" target="_blank" rel="ugc noopener noreferrer">⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠<strong>BOCADINHAS na LOLJA</strong>⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠</a>⁠⁠⁠⁠⁠⁠⁠</p> <p><strong>Edição: </strong>⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠<a href="https://instagram.com/danebayer" target="_blank" rel="ugc noopener noreferrer">⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠<strong>Daniel Bayer</strong>⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠</a></p> <p><strong>Arte da Capa:</strong> ⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠<a href="https://www.instagram.com/daltrinador" target="_blank" rel="ugc noopener noreferrer">⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠⁠<strong>Daltrinador</strong></a></p>)");

  REQUIRE(
      parsed.short_description ==
      R"(Hoje Lucas e Marcelo (no modo lero-lero) caem de boca no peru e passam mais um Natal junto com você!
Coleção BOCADINHAS na LOLJA
Edição: Daniel Bayer
Arte da Capa: Daltrinador)");

  REQUIRE(parsed.long_description == "");
}
