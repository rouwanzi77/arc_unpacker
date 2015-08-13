#include "fmt/liar_soft/wcg_converter.h"
#include "test_support/converter_support.h"
#include "test_support/catch.hh"

using namespace au::fmt::liar_soft;

TEST_CASE("Decoding WCG images works")
{
    WcgConverter converter;
    au::tests::assert_decoded_image(
        converter,
        "tests/fmt/liar_soft/files/0003.wcg",
        "tests/fmt/liar_soft/files/0003-out.png");
}
