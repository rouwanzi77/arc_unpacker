// ARC archive
//
// Company:   -
// Engine:    BGI/Ethornell
// Extension: .arc
//
// Known games:
// - Higurashi No Naku Koro Ni

#include "formats/bgi/arc_archive.h"
#include "formats/bgi/cbg_converter.h"
#include "formats/bgi/sound_converter.h"
using namespace Formats::Bgi;

namespace
{
    const std::string magic("PackFile    ", 12);

    std::unique_ptr<File> read_file(IO &arc_io, size_t file_count)
    {
        std::unique_ptr<File> file(new File);

        size_t old_pos = arc_io.tell();
        file->name = arc_io.read_until_zero();
        arc_io.seek(old_pos + 16);

        size_t offset = arc_io.read_u32_le();
        size_t size = arc_io.read_u32_le();
        offset += magic.size() + 4 + file_count * 32;
        arc_io.skip(8);
        if (offset + size > arc_io.size())
            throw std::runtime_error("Bad offset to file");

        old_pos = arc_io.tell();
        arc_io.seek(offset);
        file->io.write_from_io(arc_io, size);
        arc_io.seek(old_pos);

        return file;
    }
}

struct ArcArchive::Internals
{
    SoundConverter sound_converter;
    CbgConverter cbg_converter;
};

ArcArchive::ArcArchive() : internals(new Internals)
{
}

ArcArchive::~ArcArchive()
{
}

void ArcArchive::add_cli_help(ArgParser &arg_parser) const
{
    internals->sound_converter.add_cli_help(arg_parser);
    internals->cbg_converter.add_cli_help(arg_parser);
}

void ArcArchive::parse_cli_options(ArgParser &arg_parser)
{
    internals->sound_converter.parse_cli_options(arg_parser);
    internals->cbg_converter.parse_cli_options(arg_parser);
}

void ArcArchive::unpack_internal(File &arc_file, FileSaver &file_saver) const
{
    if (arc_file.io.read(magic.size()) != magic)
        throw std::runtime_error("Not an ARC archive");

    size_t file_count = arc_file.io.read_u32_le();
    if (file_count * 32 > arc_file.io.size())
        throw std::runtime_error("Bad file count");

    for (size_t i = 0; i < file_count; i ++)
    {
        auto file = read_file(arc_file.io, file_count);
        internals->sound_converter.try_decode(*file);
        internals->cbg_converter.try_decode(*file);
        file_saver.save(std::move(file));
    }
}