#include "dec/amuse_craft/pgd_c00_image_decoder.h"
#include "algo/cyclic_buffer.h"
#include "algo/range.h"
#include "dec/truevision/tga_image_decoder.h"
#include "io/memory_stream.h"

using namespace au;
using namespace au::dec::amuse_craft;

static const bstr magic = "00_C"_b;

static bstr decompress(const bstr &input, const size_t size_orig)
{
    bstr output;
    output.reserve(size_orig);
    io::MemoryStream input_stream(input);
    algo::CyclicBuffer<0xBB8> dict(0);
    u16 control = 0;
    while (output.size() < size_orig)
    {
        control >>= 1;
        if (!(control & 0x100))
            control = input_stream.read_u8() | 0xFF00;
        if (control & 1)
        {
            const auto look_behind_pos = input_stream.read_u16_le();
            const auto repetitions = input_stream.read_u8();
            const auto dict_start = dict.start();
            for (const auto i : algo::range(repetitions))
            {
                const u8 c = dict[dict_start + look_behind_pos + i];
                output += c;
                dict << c;
            }
        }
        else
        {
            const auto repetitions = input_stream.read_u8();
            const auto chunk = input_stream.read(repetitions);
            output += chunk;
            dict << chunk;
        }
    }
    return output;
}

bool PgdC00ImageDecoder::is_recognized_impl(io::File &input_file) const
{
    input_file.stream.seek(24);
    return input_file.stream.read(magic.size()) == magic;
}

res::Image PgdC00ImageDecoder::decode_impl(
    const Logger &logger, io::File &input_file) const
{
    input_file.stream.seek(24 + magic.size());
    const auto size_orig = input_file.stream.read_u32_le();
    const auto size_comp = input_file.stream.read_u32_le();
    auto data = input_file.stream.read(size_comp - 12);
    data = decompress(data, size_orig);
    io::File tmp_file("test.tga", data);
    const auto tga_image_decoder = dec::truevision::TgaImageDecoder();
    return tga_image_decoder.decode(logger, tmp_file);
}

static auto _
    = dec::register_decoder<PgdC00ImageDecoder>("amuse-craft/pgd-c00");