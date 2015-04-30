#include "compat/entry_point.h"
#include "formats/rpgmaker/xyz_converter.h"
#include "test_support/catch.hpp"
#include "test_support/converter_support.h"
using namespace Formats::RpgMaker;

TEST_CASE("Decoding XYZ images works")
{
    init_fs_utf8();
    XyzConverter converter;
    assert_decoded_image(
        converter,
        "tests/formats/rpgmaker/files/浅瀬部屋a.xyz",
        "tests/formats/rpgmaker/files/浅瀬部屋a-out.png");
}
