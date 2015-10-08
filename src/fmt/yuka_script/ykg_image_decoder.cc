#include "fmt/yuka_script/ykg_image_decoder.h"
#include "err.h"
#include "util/range.h"

using namespace au;
using namespace au::fmt::yuka_script;

namespace
{
    struct Header final
    {
        bool encrypted;
        size_t data_offset;
        size_t data_size;
        size_t regions_offset;
        size_t regions_size;
    };

    struct Region final
    {
        size_t width;
        size_t height;
        size_t x;
        size_t y;
    };
}

static const bstr magic = "YKG000"_b;

static std::unique_ptr<Header> read_header(io::IO &file_io)
{
    auto header = std::make_unique<Header>();
    header->encrypted = file_io.read_u16_le() > 0;

    size_t header_size = file_io.read_u32_le();
    if (header_size != 64)
        throw err::NotSupportedError("Unexpected header size");
    file_io.skip(28);

    header->data_offset = file_io.read_u32_le();
    header->data_size = file_io.read_u32_le();
    file_io.skip(8);

    header->regions_offset = file_io.read_u32_le();
    header->regions_size = file_io.read_u32_le();
    return header;
}

static std::vector<std::unique_ptr<Region>> read_regions(
    io::IO &file_io, Header &header)
{
    std::vector<std::unique_ptr<Region>> regions;

    file_io.seek(header.regions_offset);
    size_t region_count = header.regions_size / 64;
    for (auto i : util::range(region_count))
    {
        auto region = std::make_unique<Region>();
        region->x = file_io.read_u32_le();
        region->y = file_io.read_u32_le();
        region->width = file_io.read_u32_le();
        region->height = file_io.read_u32_le();
        file_io.skip(48);
        regions.push_back(std::move(region));
    }
    return regions;
}

static std::unique_ptr<File> decode_png(File &file, Header &header)
{
    file.io.seek(header.data_offset);
    bstr data = file.io.read(header.data_size);
    if (data.substr(1, 3) != "GNP"_b)
    {
        throw err::NotSupportedError(
            "Decoding non-PNG based YKG images is not supported");
    }
    data[1] = 'P';
    data[2] = 'N';
    data[3] = 'G';

    auto output_file = std::make_unique<File>();
    output_file->io.write(data);
    output_file->name = file.name;
    output_file->change_extension("png");
    return output_file;
}

bool YkgImageDecoder::is_recognized_internal(File &file) const
{
    return file.io.read(magic.size()) == magic;
}

std::unique_ptr<File> YkgImageDecoder::decode_internal(File &file) const
{
    file.io.skip(magic.size());

    auto header = read_header(file.io);
    if (header->encrypted)
    {
        throw err::NotSupportedError(
            "Decoding encrypted YKG images is not supported");
    }

    read_regions(file.io, *header);
    return decode_png(file, *header);
}

static auto dummy = fmt::Registry::add<YkgImageDecoder>("yuka/ykg");
