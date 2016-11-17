// Copyright (C) 2016 by rr-
//
// This file is part of arc_unpacker.
//
// arc_unpacker is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or (at
// your option) any later version.
//
// arc_unpacker is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with arc_unpacker. If not, see <http://www.gnu.org/licenses/>.

#include "dec/whale/dat_archive_decoder.h"
#include <cmath>
#include <map>
#include "algo/format.h"
#include "algo/locale.h"
#include "algo/pack/zlib.h"
#include "algo/range.h"
#include "io/memory_byte_stream.h"

using namespace au;
using namespace au::dec::whale;

namespace
{
    enum class TableEntryType : u8
    {
        Plain      = 0,
        Obfuscated = 1,
        Compressed = 2,
    };

    struct CustomArchiveMeta final : dec::ArchiveMeta
    {
        bstr game_title;
        bstr game_key;
    };

    struct CustomArchiveEntry final : dec::CompressedArchiveEntry
    {
        u64 hash;
        TableEntryType type;
        bstr path_orig;

        bool name_valid;
        bool offset_valid;
        bool size_comp_valid;
        bool size_orig_valid;
    };
}

static const u32 file_count_hash = 0x26ACA46E;

static u64 CRC_TABLE[0x100] =
{
    0x0000000000000000,0x42F0E1EBA9EA3693,0x85E1C3D753D46D26,0xC711223CFA3E5BB5,
    0x493366450E42ECDF,0x0BC387AEA7A8DA4C,0xCCD2A5925D9681F9,0x8E224479F47CB76A,
    0x9266CC8A1C85D9BE,0xD0962D61B56FEF2D,0x17870F5D4F51B498,0x5577EEB6E6BB820B,
    0xDB55AACF12C73561,0x99A54B24BB2D03F2,0x5EB4691841135847,0x1C4488F3E8F96ED4,
    0x663D78FF90E185EF,0x24CD9914390BB37C,0xE3DCBB28C335E8C9,0xA12C5AC36ADFDE5A,
    0x2F0E1EBA9EA36930,0x6DFEFF5137495FA3,0xAAEFDD6DCD770416,0xE81F3C86649D3285,
    0xF45BB4758C645C51,0xB6AB559E258E6AC2,0x71BA77A2DFB03177,0x334A9649765A07E4,
    0xBD68D2308226B08E,0xFF9833DB2BCC861D,0x388911E7D1F2DDA8,0x7A79F00C7818EB3B,
    0xCC7AF1FF21C30BDE,0x8E8A101488293D4D,0x499B3228721766F8,0x0B6BD3C3DBFD506B,
    0x854997BA2F81E701,0xC7B97651866BD192,0x00A8546D7C558A27,0x4258B586D5BFBCB4,
    0x5E1C3D753D46D260,0x1CECDC9E94ACE4F3,0xDBFDFEA26E92BF46,0x990D1F49C77889D5,
    0x172F5B3033043EBF,0x55DFBADB9AEE082C,0x92CE98E760D05399,0xD03E790CC93A650A,
    0xAA478900B1228E31,0xE8B768EB18C8B8A2,0x2FA64AD7E2F6E317,0x6D56AB3C4B1CD584,
    0xE374EF45BF6062EE,0xA1840EAE168A547D,0x66952C92ECB40FC8,0x2465CD79455E395B,
    0x3821458AADA7578F,0x7AD1A461044D611C,0xBDC0865DFE733AA9,0xFF3067B657990C3A,
    0x711223CFA3E5BB50,0x33E2C2240A0F8DC3,0xF4F3E018F031D676,0xB60301F359DBE0E5,
    0xDA050215EA6C212F,0x98F5E3FE438617BC,0x5FE4C1C2B9B84C09,0x1D14202910527A9A,
    0x93366450E42ECDF0,0xD1C685BB4DC4FB63,0x16D7A787B7FAA0D6,0x5427466C1E109645,
    0x4863CE9FF6E9F891,0x0A932F745F03CE02,0xCD820D48A53D95B7,0x8F72ECA30CD7A324,
    0x0150A8DAF8AB144E,0x43A04931514122DD,0x84B16B0DAB7F7968,0xC6418AE602954FFB,
    0xBC387AEA7A8DA4C0,0xFEC89B01D3679253,0x39D9B93D2959C9E6,0x7B2958D680B3FF75,
    0xF50B1CAF74CF481F,0xB7FBFD44DD257E8C,0x70EADF78271B2539,0x321A3E938EF113AA,
    0x2E5EB66066087D7E,0x6CAE578BCFE24BED,0xABBF75B735DC1058,0xE94F945C9C3626CB,
    0x676DD025684A91A1,0x259D31CEC1A0A732,0xE28C13F23B9EFC87,0xA07CF2199274CA14,
    0x167FF3EACBAF2AF1,0x548F120162451C62,0x939E303D987B47D7,0xD16ED1D631917144,
    0x5F4C95AFC5EDC62E,0x1DBC74446C07F0BD,0xDAAD56789639AB08,0x985DB7933FD39D9B,
    0x84193F60D72AF34F,0xC6E9DE8B7EC0C5DC,0x01F8FCB784FE9E69,0x43081D5C2D14A8FA,
    0xCD2A5925D9681F90,0x8FDAB8CE70822903,0x48CB9AF28ABC72B6,0x0A3B7B1923564425,
    0x70428B155B4EAF1E,0x32B26AFEF2A4998D,0xF5A348C2089AC238,0xB753A929A170F4AB,
    0x3971ED50550C43C1,0x7B810CBBFCE67552,0xBC902E8706D82EE7,0xFE60CF6CAF321874,
    0xE224479F47CB76A0,0xA0D4A674EE214033,0x67C58448141F1B86,0x253565A3BDF52D15,
    0xAB1721DA49899A7F,0xE9E7C031E063ACEC,0x2EF6E20D1A5DF759,0x6C0603E6B3B7C1CA,
    0xF6FAE5C07D3274CD,0xB40A042BD4D8425E,0x731B26172EE619EB,0x31EBC7FC870C2F78,
    0xBFC9838573709812,0xFD39626EDA9AAE81,0x3A28405220A4F534,0x78D8A1B9894EC3A7,
    0x649C294A61B7AD73,0x266CC8A1C85D9BE0,0xE17DEA9D3263C055,0xA38D0B769B89F6C6,
    0x2DAF4F0F6FF541AC,0x6F5FAEE4C61F773F,0xA84E8CD83C212C8A,0xEABE6D3395CB1A19,
    0x90C79D3FEDD3F122,0xD2377CD44439C7B1,0x15265EE8BE079C04,0x57D6BF0317EDAA97,
    0xD9F4FB7AE3911DFD,0x9B041A914A7B2B6E,0x5C1538ADB04570DB,0x1EE5D94619AF4648,
    0x02A151B5F156289C,0x4051B05E58BC1E0F,0x87409262A28245BA,0xC5B073890B687329,
    0x4B9237F0FF14C443,0x0962D61B56FEF2D0,0xCE73F427ACC0A965,0x8C8315CC052A9FF6,
    0x3A80143F5CF17F13,0x7870F5D4F51B4980,0xBF61D7E80F251235,0xFD913603A6CF24A6,
    0x73B3727A52B393CC,0x31439391FB59A55F,0xF652B1AD0167FEEA,0xB4A25046A88DC879,
    0xA8E6D8B54074A6AD,0xEA16395EE99E903E,0x2D071B6213A0CB8B,0x6FF7FA89BA4AFD18,
    0xE1D5BEF04E364A72,0xA3255F1BE7DC7CE1,0x64347D271DE22754,0x26C49CCCB40811C7,
    0x5CBD6CC0CC10FAFC,0x1E4D8D2B65FACC6F,0xD95CAF179FC497DA,0x9BAC4EFC362EA149,
    0x158E0A85C2521623,0x577EEB6E6BB820B0,0x906FC95291867B05,0xD29F28B9386C4D96,
    0xCEDBA04AD0952342,0x8C2B41A1797F15D1,0x4B3A639D83414E64,0x09CA82762AAB78F7,
    0x87E8C60FDED7CF9D,0xC51827E4773DF90E,0x020905D88D03A2BB,0x40F9E43324E99428,
    0x2CFFE7D5975E55E2,0x6E0F063E3EB46371,0xA91E2402C48A38C4,0xEBEEC5E96D600E57,
    0x65CC8190991CB93D,0x273C607B30F68FAE,0xE02D4247CAC8D41B,0xA2DDA3AC6322E288,
    0xBE992B5F8BDB8C5C,0xFC69CAB42231BACF,0x3B78E888D80FE17A,0x7988096371E5D7E9,
    0xF7AA4D1A85996083,0xB55AACF12C735610,0x724B8ECDD64D0DA5,0x30BB6F267FA73B36,
    0x4AC29F2A07BFD00D,0x08327EC1AE55E69E,0xCF235CFD546BBD2B,0x8DD3BD16FD818BB8,
    0x03F1F96F09FD3CD2,0x41011884A0170A41,0x86103AB85A2951F4,0xC4E0DB53F3C36767,
    0xD8A453A01B3A09B3,0x9A54B24BB2D03F20,0x5D45907748EE6495,0x1FB5719CE1045206,
    0x919735E51578E56C,0xD367D40EBC92D3FF,0x1476F63246AC884A,0x568617D9EF46BED9,
    0xE085162AB69D5E3C,0xA275F7C11F7768AF,0x6564D5FDE549331A,0x279434164CA30589,
    0xA9B6706FB8DFB2E3,0xEB46918411358470,0x2C57B3B8EB0BDFC5,0x6EA7525342E1E956,
    0x72E3DAA0AA188782,0x30133B4B03F2B111,0xF7021977F9CCEAA4,0xB5F2F89C5026DC37,
    0x3BD0BCE5A45A6B5D,0x79205D0E0DB05DCE,0xBE317F32F78E067B,0xFCC19ED95E6430E8,
    0x86B86ED5267CDBD3,0xC4488F3E8F96ED40,0x0359AD0275A8B6F5,0x41A94CE9DC428066,
    0xCF8B0890283E370C,0x8D7BE97B81D4019F,0x4A6ACB477BEA5A2A,0x089A2AACD2006CB9,
    0x14DEA25F3AF9026D,0x562E43B4931334FE,0x913F6188692D6F4B,0xD3CF8063C0C759D8,
    0x5DEDC41A34BBEEB2,0x1F1D25F19D51D821,0xD80C07CD676F8394,0x9AFCE626CE85B507
};

static u64 crc64(const bstr &buffer)
{
    u64 crc = 0xFFFFFFFFFFFFFFFF;
    for (const auto i : algo::range(buffer.size()))
    {
        const auto c = static_cast<u8>(buffer[i]);
        const auto tab_index = ((crc >> 56) ^ c) & 0xFF;
        crc = CRC_TABLE[tab_index] ^ ((crc << 8) & 0xFFFFFFFFFFFFFFFF);
    }
    return crc ^ 0xFFFFFFFFFFFFFFFF;
}

static void transform_regular_content(
    bstr &buffer, const bstr &file_name)
{
    auto block_size = static_cast<size_t>(
        std::floor(buffer.size() / static_cast<float>(file_name.size())));
    u8 *buffer_ptr = buffer.get<u8>();
    u8 *buffer_end = buffer_ptr + buffer.size();
    for (const auto j : algo::range(file_name.size() - 1))
    for (const auto k : algo::range(block_size))
    {
        if (buffer_ptr >= buffer_end)
            return;
        *buffer_ptr++ ^= file_name[j];
    }
}

static void transform_script_content(
    bstr &buffer, const u64 hash, const bstr &game_key)
{
    const u32 xor_value = (hash ^ crc64(game_key)) & 0xFFFFFFFF;
    for (const auto i : algo::range(buffer.size() / 4))
        buffer.get<u32>()[i] ^= xor_value;
}

static u32 read_file_count(io::BaseByteStream &input_stream)
{
    return input_stream.read_le<u32>() ^ file_count_hash;
}

static void dump(const CustomArchiveMeta &meta, const std::string &dump_path)
{
    // make it static, so that ./au *.dat --dump=x doesn't write info only
    // about the last archive
    static io::FileByteStream stream(dump_path, io::FileMode::Write);

    stream.write(meta.game_title.str() + "\n");
    for (const auto &e : meta.entries)
    {
        const auto entry = static_cast<CustomArchiveEntry*>(e.get());
        if (entry->name_valid)
            stream.write(entry->path.str() + "\n");
        else
            stream.write(algo::format("unk:%016llx\n", entry->hash));
    }
}

DatArchiveDecoder::DatArchiveDecoder()
{
    add_arg_parser_decorator(
        [](ArgParser &arg_parser)
        {
            arg_parser.register_switch({"--file-names"})
                ->set_value_name("PATH")
                ->set_description(
                    "Specifies path to file containing list of game's file "
                    "names");

            arg_parser.register_switch({"--dump"})
                ->set_value_name("PATH")
                ->set_description(
                    "Rather than unpacking, create dump of the file names.\n"
                    "This is useful when adding support for new games.");
        },
        [&](const ArgParser &arg_parser)
        {
            dump_path = arg_parser.get_switch("dump");

            if (arg_parser.has_switch("file-names"))
            {
                io::FileByteStream stream(
                    arg_parser.get_switch("file-names"), io::FileMode::Read);
                set_game_title(stream.read_line().str());
                bstr line;
                while ((line = stream.read_line()) != ""_b)
                    add_file_name(line.str());
            }
        });
}

void DatArchiveDecoder::set_game_title(const std::string &new_game_title)
{
    game_title = new_game_title;
    game_key = algo::utf8_to_sjis(new_game_title);
}

void DatArchiveDecoder::add_file_name(const std::string &file_name)
{
    const auto file_name_sjis = algo::utf8_to_sjis(file_name);
    file_names_map[crc64(file_name_sjis)] = file_name_sjis;
}

bool DatArchiveDecoder::is_recognized_impl(io::File &input_file) const
{
    if (!input_file.path.has_extension("dat"))
        return false;
    const auto file_count = read_file_count(input_file.stream);
    return file_count * (8 + 1 + 4 + 4 + 4) < input_file.stream.size();
}

std::unique_ptr<dec::ArchiveMeta> DatArchiveDecoder::read_meta_impl(
    const Logger &logger, io::File &input_file) const
{
    auto meta = std::make_unique<CustomArchiveMeta>();
    meta->game_key = game_key;
    meta->game_title = game_title;

    const auto file_count = read_file_count(input_file.stream);
    for (const auto i : algo::range(file_count))
    {
        const auto hash64 = input_file.stream.read_le<u64>();
        const auto hash32 = hash64 & 0xFFFFFFFF;
        const auto hash8 = hash64 & 0xFF;

        auto entry = std::make_unique<CustomArchiveEntry>();
        entry->hash = hash64;
        entry->type = static_cast<TableEntryType>(
            input_file.stream.read<u8>() ^ hash8);

        entry->offset    = input_file.stream.read_le<u32>() ^ hash32;
        entry->size_comp = input_file.stream.read_le<u32>() ^ hash32;
        entry->size_orig = input_file.stream.read_le<u32>() ^ hash32;

        entry->name_valid
            = file_names_map.find(entry->hash) != file_names_map.end();
        const auto name = entry->name_valid
            ? file_names_map.at(entry->hash)
            : std::string("");

        if (entry->type == TableEntryType::Compressed)
        {
            entry->path_orig = entry->name_valid
                ? name
                : algo::format("%04d.txt", i);
            entry->name_valid = true;
            entry->offset_valid = true;
            entry->size_comp_valid = true;
            entry->size_orig_valid = true;
        }
        else if (!name.empty())
        {
            entry->path_orig = name;
            entry->offset ^= name[name.size() >> 1] & 0xFF;
            entry->size_comp ^= name[name.size() >> 2] & 0xFF;
            entry->size_orig ^= name[name.size() >> 3] & 0xFF;

            entry->offset_valid = true;
            entry->size_comp_valid = true;
            entry->size_orig_valid = true;
        }

        entry->path = algo::sjis_to_utf8(entry->path_orig).str();
        meta->entries.push_back(std::move(entry));
    }

    for (const auto i : algo::range(1, meta->entries.size() - 1))
    {
        const auto prev_entry
            = static_cast<CustomArchiveEntry*>(meta->entries[i-1].get());
        const auto entry
            = static_cast<CustomArchiveEntry*>(meta->entries[i].get());
        const auto next_entry
            = static_cast<CustomArchiveEntry*>(meta->entries[i+1].get());

        if (!entry->offset_valid
        && prev_entry->offset_valid
        && prev_entry->size_comp_valid)
        {
            entry->offset = prev_entry->offset + prev_entry->size_comp;
            entry->offset_valid = true;
        }

        if (!entry->size_comp_valid
        && entry->offset_valid
        && next_entry->offset_valid)
        {
            entry->size_comp = next_entry->offset - entry->offset;
            entry->size_comp_valid = true;
        }

        if (!entry->size_orig_valid
        && entry->size_comp_valid
        && entry->type != TableEntryType::Compressed)
        {
            entry->size_orig = entry->size_comp;
            entry->size_orig_valid = true;
        }
    }

    if (!dump_path.empty())
    {
        dump(*meta, dump_path);
        meta->entries.clear();
    }

    return std::move(meta);
}

std::unique_ptr<io::File> DatArchiveDecoder::read_file_impl(
    const Logger &logger,
    io::File &input_file,
    const dec::ArchiveMeta &m,
    const dec::ArchiveEntry &e) const
{
    const auto meta = static_cast<const CustomArchiveMeta*>(&m);
    const auto entry = static_cast<const CustomArchiveEntry*>(&e);

    if (!entry->name_valid && entry->type == TableEntryType::Obfuscated)
    {
        logger.err(
            "Unknown name for file %016llx. File cannot be unpacked.\n",
            entry->hash);
        return nullptr;
    }

    if (!entry->offset_valid)
    {
        logger.err(
            "Unknown offset for file %016llx. File cannot be unpacked.\n",
            entry->hash);
        return nullptr;
    }

    if (!entry->size_comp_valid)
    {
        logger.err(
            "Unknown size for file %016llx. File cannot be unpacked.\n",
            entry->hash);
        return nullptr;
    }

    auto data = input_file.stream.seek(entry->offset).read(entry->size_comp);
    if (entry->type == TableEntryType::Compressed)
    {
        transform_script_content(data, entry->hash, game_key);
        data = algo::pack::zlib_inflate(data);
    }
    else
    {
        if (entry->type == TableEntryType::Obfuscated)
            transform_regular_content(data, entry->path_orig);
    }

    return std::make_unique<io::File>(entry->path, data);
}

std::vector<std::string> DatArchiveDecoder::get_linked_formats() const
{
    return {"kirikiri/tlg"};
}

static auto _ = dec::register_decoder<DatArchiveDecoder>("whale/dat");
