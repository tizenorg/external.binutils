// inremental.cc -- incremental linking support for gold

// Copyright 2009, 2010 Free Software Foundation, Inc.
// Written by Mikolaj Zalewski <mikolajz@google.com>.

// This file is part of gold.

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
// MA 02110-1301, USA.

#include "gold.h"

#include <cstdarg>
#include "libiberty.h"

#include "elfcpp.h"
#include "options.h"
#include "output.h"
#include "symtab.h"
#include "incremental.h"
#include "archive.h"
#include "object.h"
#include "output.h"
#include "target-select.h"
#include "target.h"
#include "fileread.h"
#include "script.h"

namespace gold {

// Version information. Will change frequently during the development, later
// we could think about backward (and forward?) compatibility.
const unsigned int INCREMENTAL_LINK_VERSION = 1;

// This class manages the .gnu_incremental_inputs section, which holds
// the header information, a directory of input files, and separate
// entries for each input file.

template<int size, bool big_endian>
class Output_section_incremental_inputs : public Output_section_data
{
 public:
  Output_section_incremental_inputs(const Incremental_inputs* inputs,
				    const Symbol_table* symtab)
    : Output_section_data(size / 8), inputs_(inputs), symtab_(symtab)
  { }

 protected:
  // This is called to update the section size prior to assigning
  // the address and file offset.
  void
  update_data_size()
  { this->set_final_data_size(); }

  // Set the final data size.
  void
  set_final_data_size();

  // Write the data to the file.
  void
  do_write(Output_file*);

  // Write to a map file.
  void
  do_print_to_mapfile(Mapfile* mapfile) const
  { mapfile->print_output_data(this, _("** incremental_inputs")); }

 private:
  // Write the section header.
  unsigned char*
  write_header(unsigned char* pov, unsigned int input_file_count,
	       section_offset_type command_line_offset);

  // Write the input file entries.
  unsigned char*
  write_input_files(unsigned char* oview, unsigned char* pov,
		    Stringpool* strtab);

  // Write the supplemental information blocks.
  unsigned char*
  write_info_blocks(unsigned char* oview, unsigned char* pov,
		    Stringpool* strtab, unsigned int* global_syms,
		    unsigned int global_sym_count);

  // Write the contents of the .gnu_incremental_symtab section.
  void
  write_symtab(unsigned char* pov, unsigned int* global_syms,
	       unsigned int global_sym_count);

  // Write the contents of the .gnu_incremental_got_plt section.
  void
  write_got_plt(unsigned char* pov, off_t view_size);

  // Typedefs for writing the data to the output sections.
  typedef elfcpp::Swap<size, big_endian> Swap;
  typedef elfcpp::Swap<16, big_endian> Swap16;
  typedef elfcpp::Swap<32, big_endian> Swap32;
  typedef elfcpp::Swap<64, big_endian> Swap64;

  // Sizes of various structures.
  static const int sizeof_addr = size / 8;
  static const int header_size = 16;
  static const int input_entry_size = 24;

  // The Incremental_inputs object.
  const Incremental_inputs* inputs_;

  // The symbol table.
  const Symbol_table* symtab_;
};

// Inform the user why we don't do an incremental link.  Not called in
// the obvious case of missing output file.  TODO: Is this helpful?

void
vexplain_no_incremental(const char* format, va_list args)
{
  char* buf = NULL;
  if (vasprintf(&buf, format, args) < 0)
    gold_nomem();
  gold_info(_("the link might take longer: "
              "cannot perform incremental link: %s"), buf);
  free(buf);
}

void
explain_no_incremental(const char* format, ...)
{
  va_list args;
  va_start(args, format);
  vexplain_no_incremental(format, args);
  va_end(args);
}

// Report an error.

void
Incremental_binary::error(const char* format, ...) const
{
  va_list args;
  va_start(args, format);
  // Current code only checks if the file can be used for incremental linking,
  // so errors shouldn't fail the build, but only result in a fallback to a
  // full build.
  // TODO: when we implement incremental editing of the file, we may need a
  // flag that will cause errors to be treated seriously.
  vexplain_no_incremental(format, args);
  va_end(args);
}

// Find the .gnu_incremental_inputs section and related sections.

template<int size, bool big_endian>
bool
Sized_incremental_binary<size, big_endian>::find_incremental_inputs_sections(
    unsigned int* p_inputs_shndx,
    unsigned int* p_symtab_shndx,
    unsigned int* p_relocs_shndx,
    unsigned int* p_got_plt_shndx,
    unsigned int* p_strtab_shndx)
{
  unsigned int inputs_shndx =
      this->elf_file_.find_section_by_type(elfcpp::SHT_GNU_INCREMENTAL_INPUTS);
  if (inputs_shndx == elfcpp::SHN_UNDEF)  // Not found.
    return false;

  unsigned int symtab_shndx =
      this->elf_file_.find_section_by_type(elfcpp::SHT_GNU_INCREMENTAL_SYMTAB);
  if (symtab_shndx == elfcpp::SHN_UNDEF)  // Not found.
    return false;
  if (this->elf_file_.section_link(symtab_shndx) != inputs_shndx)
    return false;

  unsigned int relocs_shndx =
      this->elf_file_.find_section_by_type(elfcpp::SHT_GNU_INCREMENTAL_RELOCS);
  if (relocs_shndx == elfcpp::SHN_UNDEF)  // Not found.
    return false;
  if (this->elf_file_.section_link(relocs_shndx) != inputs_shndx)
    return false;

  unsigned int got_plt_shndx =
      this->elf_file_.find_section_by_type(elfcpp::SHT_GNU_INCREMENTAL_GOT_PLT);
  if (got_plt_shndx == elfcpp::SHN_UNDEF)  // Not found.
    return false;
  if (this->elf_file_.section_link(got_plt_shndx) != inputs_shndx)
    return false;

  unsigned int strtab_shndx = this->elf_file_.section_link(inputs_shndx);
  if (strtab_shndx == elfcpp::SHN_UNDEF
      || strtab_shndx > this->elf_file_.shnum()
      || this->elf_file_.section_type(strtab_shndx) != elfcpp::SHT_STRTAB)
    return false;

  if (p_inputs_shndx != NULL)
    *p_inputs_shndx = inputs_shndx;
  if (p_symtab_shndx != NULL)
    *p_symtab_shndx = symtab_shndx;
  if (p_relocs_shndx != NULL)
    *p_relocs_shndx = relocs_shndx;
  if (p_got_plt_shndx != NULL)
    *p_got_plt_shndx = got_plt_shndx;
  if (p_strtab_shndx != NULL)
    *p_strtab_shndx = strtab_shndx;
  return true;
}

// Set up the readers into the incremental info sections.

template<int size, bool big_endian>
void
Sized_incremental_binary<size, big_endian>::setup_readers()
{
  unsigned int inputs_shndx;
  unsigned int symtab_shndx;
  unsigned int relocs_shndx;
  unsigned int got_plt_shndx;
  unsigned int strtab_shndx;

  if (!this->find_incremental_inputs_sections(&inputs_shndx, &symtab_shndx,
					      &relocs_shndx, &got_plt_shndx,
					      &strtab_shndx))
    return;

  Location inputs_location(this->elf_file_.section_contents(inputs_shndx));
  Location symtab_location(this->elf_file_.section_contents(symtab_shndx));
  Location relocs_location(this->elf_file_.section_contents(relocs_shndx));
  Location got_plt_location(this->elf_file_.section_contents(got_plt_shndx));
  Location strtab_location(this->elf_file_.section_contents(strtab_shndx));

  View inputs_view = this->view(inputs_location);
  View symtab_view = this->view(symtab_location);
  View relocs_view = this->view(relocs_location);
  View got_plt_view = this->view(got_plt_location);
  View strtab_view = this->view(strtab_location);

  elfcpp::Elf_strtab strtab(strtab_view.data(), strtab_location.data_size);

  this->inputs_reader_ =
      Incremental_inputs_reader<size, big_endian>(inputs_view.data(), strtab);
  this->symtab_reader_ =
      Incremental_symtab_reader<big_endian>(symtab_view.data(),
					    symtab_location.data_size);
  this->relocs_reader_ =
      Incremental_relocs_reader<size, big_endian>(relocs_view.data(),
						  relocs_location.data_size);
  this->got_plt_reader_ =
      Incremental_got_plt_reader<big_endian>(got_plt_view.data());

  // Walk the list of input files (a) to setup an Input_reader for each
  // input file, and (b) to record maps of files added from archive
  // libraries and scripts.
  Incremental_inputs_reader<size, big_endian>& inputs = this->inputs_reader_;
  unsigned int count = inputs.input_file_count();
  this->input_entry_readers_.reserve(count);
  this->library_map_.resize(count);
  this->script_map_.resize(count);
  for (unsigned int i = 0; i < count; i++)
    {
      Input_entry_reader input_file = inputs.input_file(i);
      this->input_entry_readers_.push_back(Sized_input_reader(input_file));
      switch (input_file.type())
	{
	case INCREMENTAL_INPUT_OBJECT:
	case INCREMENTAL_INPUT_ARCHIVE_MEMBER:
	case INCREMENTAL_INPUT_SHARED_LIBRARY:
	  // No special treatment necessary.
	  break;
	case INCREMENTAL_INPUT_ARCHIVE:
	  {
	    Incremental_library* lib =
	        new Incremental_library(input_file.filename(), i,
					&this->input_entry_readers_[i]);
	    this->library_map_[i] = lib;
	    unsigned int member_count = input_file.get_member_count();
	    for (unsigned int j = 0; j < member_count; j++)
	      {
		int member_offset = input_file.get_member_offset(j);
		int member_index = inputs.input_file_index(member_offset);
		this->library_map_[member_index] = lib;
	      }
	  }
	  break;
	case INCREMENTAL_INPUT_SCRIPT:
	  {
	    Script_info* script = new Script_info(input_file.filename());
	    this->script_map_[i] = script;
	    unsigned int object_count = input_file.get_object_count();
	    for (unsigned int j = 0; j < object_count; j++)
	      {
		int object_offset = input_file.get_object_offset(j);
		int object_index = inputs.input_file_index(object_offset);
		this->script_map_[object_index] = script;
	      }
	  }
	  break;
	default:
	  gold_unreachable();
	}
    }

  this->has_incremental_info_ = true;
}

// Walk the list of input files given on the command line, and build
// a direct map of file index to the corresponding input argument.

void
check_input_args(std::vector<const Input_argument*>& input_args_map,
		 Input_arguments::const_iterator begin,
		 Input_arguments::const_iterator end)
{
  for (Input_arguments::const_iterator p = begin;
       p != end;
       ++p)
    {
      if (p->is_group())
	{
	  const Input_file_group* group = p->group();
	  check_input_args(input_args_map, group->begin(), group->end());
	}
      else if (p->is_lib())
	{
	  const Input_file_lib* lib = p->lib();
	  check_input_args(input_args_map, lib->begin(), lib->end());
	}
      else
        {
          gold_assert(p->is_file());
          unsigned int arg_serial = p->file().arg_serial();
          if (arg_serial > 0)
	    {
	      gold_assert(arg_serial <= input_args_map.size());
	      gold_assert(input_args_map[arg_serial - 1] == 0);
	      input_args_map[arg_serial - 1] = &*p;
	    }
        }
    }
}

// Determine whether an incremental link based on the existing output file
// can be done.

template<int size, bool big_endian>
bool
Sized_incremental_binary<size, big_endian>::do_check_inputs(
    const Command_line& cmdline,
    Incremental_inputs* incremental_inputs)
{
  Incremental_inputs_reader<size, big_endian>& inputs = this->inputs_reader_;

  if (!this->has_incremental_info_)
    {
      explain_no_incremental(_("no incremental data from previous build"));
      return false;
    }

  if (inputs.version() != INCREMENTAL_LINK_VERSION)
    {
      explain_no_incremental(_("different version of incremental build data"));
      return false;
    }

  if (incremental_inputs->command_line() != inputs.command_line())
    {
      explain_no_incremental(_("command line changed"));
      return false;
    }

  // Walk the list of input files given on the command line, and build
  // a direct map of argument serial numbers to the corresponding input
  // arguments.
  this->input_args_map_.resize(cmdline.number_of_input_files());
  check_input_args(this->input_args_map_, cmdline.begin(), cmdline.end());

  // Walk the list of input files to check for conditions that prevent
  // an incremental update link.
  unsigned int count = inputs.input_file_count();
  for (unsigned int i = 0; i < count; i++)
    {
      Input_entry_reader input_file = inputs.input_file(i);
      switch (input_file.type())
	{
	case INCREMENTAL_INPUT_OBJECT:
	case INCREMENTAL_INPUT_ARCHIVE_MEMBER:
	case INCREMENTAL_INPUT_SHARED_LIBRARY:
	case INCREMENTAL_INPUT_ARCHIVE:
	  // No special treatment necessary.
	  break;
	case INCREMENTAL_INPUT_SCRIPT:
	  if (this->do_file_has_changed(i))
	    {
	      explain_no_incremental(_("%s: script file changed"),
				     input_file.filename());
	      return false;
	    }
	  break;
	default:
	  gold_unreachable();
	}
    }

  return true;
}

// Return TRUE if input file N has changed since the last incremental link.

template<int size, bool big_endian>
bool
Sized_incremental_binary<size, big_endian>::do_file_has_changed(
    unsigned int n) const
{
  Input_entry_reader input_file = this->inputs_reader_.input_file(n);
  Incremental_disposition disp = INCREMENTAL_CHECK;
  const Input_argument* input_argument = this->get_input_argument(n);
  if (input_argument != NULL)
    disp = input_argument->file().options().incremental_disposition();

  if (disp != INCREMENTAL_CHECK)
    return disp == INCREMENTAL_CHANGED;

  const char* filename = input_file.filename();
  Timespec old_mtime = input_file.get_mtime();
  Timespec new_mtime;
  if (!get_mtime(filename, &new_mtime))
    {
      // If we can't open get the current modification time, assume it has
      // changed.  If the file doesn't exist, we'll issue an error when we
      // try to open it later.
      return true;
    }

  if (new_mtime.seconds > old_mtime.seconds)
    return true;
  if (new_mtime.seconds == old_mtime.seconds
      && new_mtime.nanoseconds > old_mtime.nanoseconds)
    return true;
  return false;
}

// Initialize the layout of the output file based on the existing
// output file.

template<int size, bool big_endian>
void
Sized_incremental_binary<size, big_endian>::do_init_layout(Layout* layout)
{
  typedef elfcpp::Shdr<size, big_endian> Shdr;
  const int shdr_size = elfcpp::Elf_sizes<size>::shdr_size;

  // Get views of the section headers and the section string table.
  const off_t shoff = this->elf_file_.shoff();
  const unsigned int shnum = this->elf_file_.shnum();
  const unsigned int shstrndx = this->elf_file_.shstrndx();
  Location shdrs_location(shoff, shnum * shdr_size);
  Location shstrndx_location(this->elf_file_.section_contents(shstrndx));
  View shdrs_view = this->view(shdrs_location);
  View shstrndx_view = this->view(shstrndx_location);
  elfcpp::Elf_strtab shstrtab(shstrndx_view.data(),
			      shstrndx_location.data_size);

  layout->set_incremental_base(this);

  // Initialize the layout.
  this->section_map_.resize(shnum);
  const unsigned char* pshdr = shdrs_view.data() + shdr_size;
  for (unsigned int i = 1; i < shnum; i++)
    {
      Shdr shdr(pshdr);
      const char* name;
      if (!shstrtab.get_c_string(shdr.get_sh_name(), &name))
        name = NULL;
      gold_debug(DEBUG_INCREMENTAL,
		 "Output section: %2d %08lx %08lx %08lx %3d %s",
	         i,
	         static_cast<long>(shdr.get_sh_addr()),
	         static_cast<long>(shdr.get_sh_offset()),
	         static_cast<long>(shdr.get_sh_size()),
	         shdr.get_sh_type(), name ? name : "<null>");
      this->section_map_[i] = layout->init_fixed_output_section(name, shdr);
      pshdr += shdr_size;
    }
}

// Mark regions of the input file that must be kept unchanged.

template<int size, bool big_endian>
void
Sized_incremental_binary<size, big_endian>::do_reserve_layout(
    unsigned int input_file_index)
{
  Input_entry_reader input_file =
      this->inputs_reader_.input_file(input_file_index);

  if (input_file.type() == INCREMENTAL_INPUT_SHARED_LIBRARY)
    return;

  unsigned int shnum = input_file.get_input_section_count();
  for (unsigned int i = 0; i < shnum; i++)
    {
      typename Input_entry_reader::Input_section_info sect =
          input_file.get_input_section(i);
      if (sect.output_shndx == 0 || sect.sh_offset == -1)
        continue;
      Output_section* os = this->section_map_[sect.output_shndx];
      gold_assert(os != NULL);
      os->reserve(sect.sh_offset, sect.sh_size);
    }
}

// Get a view of the main symbol table and the symbol string table.

template<int size, bool big_endian>
void
Sized_incremental_binary<size, big_endian>::get_symtab_view(
    View* symtab_view,
    unsigned int* nsyms,
    elfcpp::Elf_strtab* strtab)
{
  unsigned int symtab_shndx =
      this->elf_file_.find_section_by_type(elfcpp::SHT_SYMTAB);
  gold_assert(symtab_shndx != elfcpp::SHN_UNDEF);
  Location symtab_location(this->elf_file_.section_contents(symtab_shndx));
  *symtab_view = this->view(symtab_location);
  *nsyms = symtab_location.data_size / elfcpp::Elf_sizes<size>::sym_size;

  unsigned int strtab_shndx = this->elf_file_.section_link(symtab_shndx);
  gold_assert(strtab_shndx != elfcpp::SHN_UNDEF
              && strtab_shndx < this->elf_file_.shnum());

  Location strtab_location(this->elf_file_.section_contents(strtab_shndx));
  View strtab_view(this->view(strtab_location));
  *strtab = elfcpp::Elf_strtab(strtab_view.data(), strtab_location.data_size);
}

namespace
{

// Create a Sized_incremental_binary object of the specified size and
// endianness. Fails if the target architecture is not supported.

template<int size, bool big_endian>
Incremental_binary*
make_sized_incremental_binary(Output_file* file,
                              const elfcpp::Ehdr<size, big_endian>& ehdr)
{
  Target* target = select_target(ehdr.get_e_machine(), size, big_endian,
                                 ehdr.get_e_ident()[elfcpp::EI_OSABI],
                                 ehdr.get_e_ident()[elfcpp::EI_ABIVERSION]);
  if (target == NULL)
    {
      explain_no_incremental(_("unsupported ELF machine number %d"),
               ehdr.get_e_machine());
      return NULL;
    }

  if (!parameters->target_valid())
    set_parameters_target(target);
  else if (target != &parameters->target())
    gold_error(_("%s: incompatible target"), file->filename());

  return new Sized_incremental_binary<size, big_endian>(file, ehdr, target);
}

}  // End of anonymous namespace.

// Create an Incremental_binary object for FILE.  Returns NULL is this is not
// possible, e.g. FILE is not an ELF file or has an unsupported target.  FILE
// should be opened.

Incremental_binary*
open_incremental_binary(Output_file* file)
{
  off_t filesize = file->filesize();
  int want = elfcpp::Elf_recognizer::max_header_size;
  if (filesize < want)
    want = filesize;

  const unsigned char* p = file->get_input_view(0, want);
  if (!elfcpp::Elf_recognizer::is_elf_file(p, want))
    {
      explain_no_incremental(_("output is not an ELF file."));
      return NULL;
    }

  int size = 0;
  bool big_endian = false;
  std::string error;
  if (!elfcpp::Elf_recognizer::is_valid_header(p, want, &size, &big_endian,
                                               &error))
    {
      explain_no_incremental(error.c_str());
      return NULL;
    }

  Incremental_binary* result = NULL;
  if (size == 32)
    {
      if (big_endian)
        {
#ifdef HAVE_TARGET_32_BIG
          result = make_sized_incremental_binary<32, true>(
              file, elfcpp::Ehdr<32, true>(p));
#else
          explain_no_incremental(_("unsupported file: 32-bit, big-endian"));
#endif
        }
      else
        {
#ifdef HAVE_TARGET_32_LITTLE
          result = make_sized_incremental_binary<32, false>(
              file, elfcpp::Ehdr<32, false>(p));
#else
          explain_no_incremental(_("unsupported file: 32-bit, little-endian"));
#endif
        }
    }
  else if (size == 64)
    {
      if (big_endian)
        {
#ifdef HAVE_TARGET_64_BIG
          result = make_sized_incremental_binary<64, true>(
              file, elfcpp::Ehdr<64, true>(p));
#else
          explain_no_incremental(_("unsupported file: 64-bit, big-endian"));
#endif
        }
      else
        {
#ifdef HAVE_TARGET_64_LITTLE
          result = make_sized_incremental_binary<64, false>(
              file, elfcpp::Ehdr<64, false>(p));
#else
          explain_no_incremental(_("unsupported file: 64-bit, little-endian"));
#endif
        }
    }
  else
    gold_unreachable();

  return result;
}

// Class Incremental_inputs.

// Add the command line to the string table, setting
// command_line_key_.  In incremental builds, the command line is
// stored in .gnu_incremental_inputs so that the next linker run can
// check if the command line options didn't change.

void
Incremental_inputs::report_command_line(int argc, const char* const* argv)
{
  // Always store 'gold' as argv[0] to avoid a full relink if the user used a
  // different path to the linker.
  std::string args("gold");
  // Copied from collect_argv in main.cc.
  for (int i = 1; i < argc; ++i)
    {
      // Adding/removing these options should not result in a full relink.
      if (strcmp(argv[i], "--incremental") == 0
	  || strcmp(argv[i], "--incremental-full") == 0
	  || strcmp(argv[i], "--incremental-update") == 0
	  || strcmp(argv[i], "--incremental-changed") == 0
	  || strcmp(argv[i], "--incremental-unchanged") == 0
	  || strcmp(argv[i], "--incremental-unknown") == 0
	  || is_prefix_of("--debug=", argv[i]))
        continue;

      args.append(" '");
      // Now append argv[i], but with all single-quotes escaped
      const char* argpos = argv[i];
      while (1)
        {
          const int len = strcspn(argpos, "'");
          args.append(argpos, len);
          if (argpos[len] == '\0')
            break;
          args.append("'\"'\"'");
          argpos += len + 1;
        }
      args.append("'");
    }

  this->command_line_ = args;
  this->strtab_->add(this->command_line_.c_str(), false,
                     &this->command_line_key_);
}

// Record the input archive file ARCHIVE.  This is called by the
// Add_archive_symbols task before determining which archive members
// to include.  We create the Incremental_archive_entry here and
// attach it to the Archive, but we do not add it to the list of
// input objects until report_archive_end is called.

void
Incremental_inputs::report_archive_begin(Library_base* arch,
					 unsigned int arg_serial,
					 Script_info* script_info)
{
  Stringpool::Key filename_key;
  Timespec mtime = arch->get_mtime();

  // For a file loaded from a script, don't record its argument serial number.
  if (script_info != NULL)
    arg_serial = 0;

  this->strtab_->add(arch->filename().c_str(), false, &filename_key);
  Incremental_archive_entry* entry =
      new Incremental_archive_entry(filename_key, arg_serial, mtime);
  arch->set_incremental_info(entry);

  if (script_info != NULL)
    {
      Incremental_script_entry* script_entry = script_info->incremental_info();
      gold_assert(script_entry != NULL);
      script_entry->add_object(entry);
    }
}

// Visitor class for processing the unused global symbols in a library.
// An instance of this class is passed to the library's
// for_all_unused_symbols() iterator, which will call the visit()
// function for each global symbol defined in each unused library
// member.  We add those symbol names to the incremental info for the
// library.

class Unused_symbol_visitor : public Library_base::Symbol_visitor_base
{
 public:
  Unused_symbol_visitor(Incremental_archive_entry* entry, Stringpool* strtab)
    : entry_(entry), strtab_(strtab)
  { }

  void
  visit(const char* sym)
  {
    Stringpool::Key symbol_key;
    this->strtab_->add(sym, true, &symbol_key);
    this->entry_->add_unused_global_symbol(symbol_key);
  }

 private:
  Incremental_archive_entry* entry_;
  Stringpool* strtab_;
};

// Finish recording the input archive file ARCHIVE.  This is called by the
// Add_archive_symbols task after determining which archive members
// to include.

void
Incremental_inputs::report_archive_end(Library_base* arch)
{
  Incremental_archive_entry* entry = arch->incremental_info();

  gold_assert(entry != NULL);
  this->inputs_.push_back(entry);

  // Collect unused global symbols.
  Unused_symbol_visitor v(entry, this->strtab_);
  arch->for_all_unused_symbols(&v);
}

// Record the input object file OBJ.  If ARCH is not NULL, attach
// the object file to the archive.  This is called by the
// Add_symbols task after finding out the type of the file.

void
Incremental_inputs::report_object(Object* obj, unsigned int arg_serial,
				  Library_base* arch, Script_info* script_info)
{
  Stringpool::Key filename_key;
  Timespec mtime = obj->get_mtime();

  // For a file loaded from a script, don't record its argument serial number.
  if (script_info != NULL)
    arg_serial = 0;

  this->strtab_->add(obj->name().c_str(), false, &filename_key);
  Incremental_object_entry* obj_entry =
      new Incremental_object_entry(filename_key, obj, arg_serial, mtime);
  if (obj->is_in_system_directory())
    obj_entry->set_is_in_system_directory();
  this->inputs_.push_back(obj_entry);

  if (arch != NULL)
    {
      Incremental_archive_entry* arch_entry = arch->incremental_info();
      gold_assert(arch_entry != NULL);
      arch_entry->add_object(obj_entry);
    }

  if (script_info != NULL)
    {
      Incremental_script_entry* script_entry = script_info->incremental_info();
      gold_assert(script_entry != NULL);
      script_entry->add_object(obj_entry);
    }

  this->current_object_ = obj;
  this->current_object_entry_ = obj_entry;
}

// Record the input object file OBJ.  If ARCH is not NULL, attach
// the object file to the archive.  This is called by the
// Add_symbols task after finding out the type of the file.

void
Incremental_inputs::report_input_section(Object* obj, unsigned int shndx,
					 const char* name, off_t sh_size)
{
  Stringpool::Key key = 0;

  if (name != NULL)
      this->strtab_->add(name, true, &key);

  gold_assert(obj == this->current_object_);
  this->current_object_entry_->add_input_section(shndx, key, sh_size);
}

// Record that the input argument INPUT is a script SCRIPT.  This is
// called by read_script after parsing the script and reading the list
// of inputs added by this script.

void
Incremental_inputs::report_script(Script_info* script,
				  unsigned int arg_serial,
				  Timespec mtime)
{
  Stringpool::Key filename_key;

  this->strtab_->add(script->filename().c_str(), false, &filename_key);
  Incremental_script_entry* entry =
      new Incremental_script_entry(filename_key, arg_serial, script, mtime);
  this->inputs_.push_back(entry);
  script->set_incremental_info(entry);
}

// Finalize the incremental link information.  Called from
// Layout::finalize.

void
Incremental_inputs::finalize()
{
  // Finalize the string table.
  this->strtab_->set_string_offsets();
}

// Create the .gnu_incremental_inputs, _symtab, and _relocs input sections.

void
Incremental_inputs::create_data_sections(Symbol_table* symtab)
{
  switch (parameters->size_and_endianness())
    {
#ifdef HAVE_TARGET_32_LITTLE
    case Parameters::TARGET_32_LITTLE:
      this->inputs_section_ =
          new Output_section_incremental_inputs<32, false>(this, symtab);
      break;
#endif
#ifdef HAVE_TARGET_32_BIG
    case Parameters::TARGET_32_BIG:
      this->inputs_section_ =
          new Output_section_incremental_inputs<32, true>(this, symtab);
      break;
#endif
#ifdef HAVE_TARGET_64_LITTLE
    case Parameters::TARGET_64_LITTLE:
      this->inputs_section_ =
          new Output_section_incremental_inputs<64, false>(this, symtab);
      break;
#endif
#ifdef HAVE_TARGET_64_BIG
    case Parameters::TARGET_64_BIG:
      this->inputs_section_ =
          new Output_section_incremental_inputs<64, true>(this, symtab);
      break;
#endif
    default:
      gold_unreachable();
    }
  this->symtab_section_ = new Output_data_space(4, "** incremental_symtab");
  this->relocs_section_ = new Output_data_space(4, "** incremental_relocs");
  this->got_plt_section_ = new Output_data_space(4, "** incremental_got_plt");
}

// Return the sh_entsize value for the .gnu_incremental_relocs section.
unsigned int
Incremental_inputs::relocs_entsize() const
{
  return 8 + 2 * parameters->target().get_size() / 8;
}

// Class Output_section_incremental_inputs.

// Finalize the offsets for each input section and supplemental info block,
// and set the final data size of the incremental output sections.

template<int size, bool big_endian>
void
Output_section_incremental_inputs<size, big_endian>::set_final_data_size()
{
  const Incremental_inputs* inputs = this->inputs_;
  const unsigned int sizeof_addr = size / 8;
  const unsigned int rel_size = 8 + 2 * sizeof_addr;

  // Offset of each input entry.
  unsigned int input_offset = this->header_size;

  // Offset of each supplemental info block.
  unsigned int info_offset = this->header_size;
  info_offset += this->input_entry_size * inputs->input_file_count();

  // Count each input file and its supplemental information block.
  for (Incremental_inputs::Input_list::const_iterator p =
	   inputs->input_files().begin();
       p != inputs->input_files().end();
       ++p)
    {
      // Set the offset of the input file entry.
      (*p)->set_offset(input_offset);
      input_offset += this->input_entry_size;

      // Set the offset of the supplemental info block.
      switch ((*p)->type())
	{
	case INCREMENTAL_INPUT_SCRIPT:
	  {
	    Incremental_script_entry *entry = (*p)->script_entry();
	    gold_assert(entry != NULL);
	    (*p)->set_info_offset(info_offset);
	    // Object count.
	    info_offset += 4;
	    // Each member.
	    info_offset += (entry->get_object_count() * 4);
	  }
	  break;
	case INCREMENTAL_INPUT_OBJECT:
	case INCREMENTAL_INPUT_ARCHIVE_MEMBER:
	  {
	    Incremental_object_entry* entry = (*p)->object_entry();
	    gold_assert(entry != NULL);
	    (*p)->set_info_offset(info_offset);
	    // Input section count + global symbol count.
	    info_offset += 8;
	    // Each input section.
	    info_offset += (entry->get_input_section_count()
			    * (8 + 2 * sizeof_addr));
	    // Each global symbol.
	    const Object::Symbols* syms = entry->object()->get_global_symbols();
	    info_offset += syms->size() * 20;
	  }
	  break;
	case INCREMENTAL_INPUT_SHARED_LIBRARY:
	  {
	    Incremental_object_entry* entry = (*p)->object_entry();
	    gold_assert(entry != NULL);
	    (*p)->set_info_offset(info_offset);
	    // Global symbol count.
	    info_offset += 4;
	    // Each global symbol.
	    const Object::Symbols* syms = entry->object()->get_global_symbols();
	    gold_assert(syms != NULL);
	    unsigned int nsyms = syms->size();
	    unsigned int nsyms_out = 0;
	    for (unsigned int i = 0; i < nsyms; ++i)
	      {
		const Symbol* sym = (*syms)[i];
		if (sym == NULL)
		  continue;
		if (sym->is_forwarder())
		  sym = this->symtab_->resolve_forwards(sym);
	        if (sym->symtab_index() != -1U)
	          ++nsyms_out;
	      }
	    info_offset += nsyms_out * 4;
	  }
	  break;
	case INCREMENTAL_INPUT_ARCHIVE:
	  {
	    Incremental_archive_entry* entry = (*p)->archive_entry();
	    gold_assert(entry != NULL);
	    (*p)->set_info_offset(info_offset);
	    // Member count + unused global symbol count.
	    info_offset += 8;
	    // Each member.
	    info_offset += (entry->get_member_count() * 4);
	    // Each global symbol.
	    info_offset += (entry->get_unused_global_symbol_count() * 4);
	  }
	  break;
	default:
	  gold_unreachable();
	}
    }

  this->set_data_size(info_offset);

  // Set the size of the .gnu_incremental_symtab section.
  inputs->symtab_section()->set_current_data_size(this->symtab_->output_count()
						  * sizeof(unsigned int));

  // Set the size of the .gnu_incremental_relocs section.
  inputs->relocs_section()->set_current_data_size(inputs->get_reloc_count()
						  * rel_size);

  // Set the size of the .gnu_incremental_got_plt section.
  Sized_target<size, big_endian>* target =
    parameters->sized_target<size, big_endian>();
  unsigned int got_count = target->got_entry_count();
  unsigned int plt_count = target->plt_entry_count();
  unsigned int got_plt_size = 8;  // GOT entry count, PLT entry count.
  got_plt_size = (got_plt_size + got_count + 3) & ~3;  // GOT type array.
  got_plt_size += got_count * 4 + plt_count * 4;  // GOT array, PLT array.
  inputs->got_plt_section()->set_current_data_size(got_plt_size);
}

// Write the contents of the .gnu_incremental_inputs and
// .gnu_incremental_symtab sections.

template<int size, bool big_endian>
void
Output_section_incremental_inputs<size, big_endian>::do_write(Output_file* of)
{
  const Incremental_inputs* inputs = this->inputs_;
  Stringpool* strtab = inputs->get_stringpool();

  // Get a view into the .gnu_incremental_inputs section.
  const off_t off = this->offset();
  const off_t oview_size = this->data_size();
  unsigned char* const oview = of->get_output_view(off, oview_size);
  unsigned char* pov = oview;

  // Get a view into the .gnu_incremental_symtab section.
  const off_t symtab_off = inputs->symtab_section()->offset();
  const off_t symtab_size = inputs->symtab_section()->data_size();
  unsigned char* const symtab_view = of->get_output_view(symtab_off,
							 symtab_size);

  // Allocate an array of linked list heads for the .gnu_incremental_symtab
  // section.  Each element corresponds to a global symbol in the output
  // symbol table, and points to the head of the linked list that threads
  // through the object file input entries.  The value of each element
  // is the section-relative offset to a global symbol entry in a
  // supplemental information block.
  unsigned int global_sym_count = this->symtab_->output_count();
  unsigned int* global_syms = new unsigned int[global_sym_count];
  memset(global_syms, 0, global_sym_count * sizeof(unsigned int));

  // Write the section header.
  Stringpool::Key command_line_key = inputs->command_line_key();
  pov = this->write_header(pov, inputs->input_file_count(),
			   strtab->get_offset_from_key(command_line_key));

  // Write the list of input files.
  pov = this->write_input_files(oview, pov, strtab);

  // Write the supplemental information blocks for each input file.
  pov = this->write_info_blocks(oview, pov, strtab, global_syms,
				global_sym_count);

  gold_assert(pov - oview == oview_size);

  // Write the .gnu_incremental_symtab section.
  gold_assert(global_sym_count * 4 == symtab_size);
  this->write_symtab(symtab_view, global_syms, global_sym_count);

  delete[] global_syms;

  // Write the .gnu_incremental_got_plt section.
  const off_t got_plt_off = inputs->got_plt_section()->offset();
  const off_t got_plt_size = inputs->got_plt_section()->data_size();
  unsigned char* const got_plt_view = of->get_output_view(got_plt_off,
							  got_plt_size);
  this->write_got_plt(got_plt_view, got_plt_size);

  of->write_output_view(off, oview_size, oview);
  of->write_output_view(symtab_off, symtab_size, symtab_view);
  of->write_output_view(got_plt_off, got_plt_size, got_plt_view);
}

// Write the section header: version, input file count, offset of command line
// in the string table, and 4 bytes of padding.

template<int size, bool big_endian>
unsigned char*
Output_section_incremental_inputs<size, big_endian>::write_header(
    unsigned char* pov,
    unsigned int input_file_count,
    section_offset_type command_line_offset)
{
  Swap32::writeval(pov, INCREMENTAL_LINK_VERSION);
  Swap32::writeval(pov + 4, input_file_count);
  Swap32::writeval(pov + 8, command_line_offset);
  Swap32::writeval(pov + 12, 0);
  return pov + this->header_size;
}

// Write the input file entries.

template<int size, bool big_endian>
unsigned char*
Output_section_incremental_inputs<size, big_endian>::write_input_files(
    unsigned char* oview,
    unsigned char* pov,
    Stringpool* strtab)
{
  const Incremental_inputs* inputs = this->inputs_;

  for (Incremental_inputs::Input_list::const_iterator p =
	   inputs->input_files().begin();
       p != inputs->input_files().end();
       ++p)
    {
      gold_assert(static_cast<unsigned int>(pov - oview) == (*p)->get_offset());
      section_offset_type filename_offset =
          strtab->get_offset_from_key((*p)->get_filename_key());
      const Timespec& mtime = (*p)->get_mtime();
      unsigned int flags = (*p)->type();
      if ((*p)->is_in_system_directory())
        flags |= INCREMENTAL_INPUT_IN_SYSTEM_DIR;
      Swap32::writeval(pov, filename_offset);
      Swap32::writeval(pov + 4, (*p)->get_info_offset());
      Swap64::writeval(pov + 8, mtime.seconds);
      Swap32::writeval(pov + 16, mtime.nanoseconds);
      Swap16::writeval(pov + 20, flags);
      Swap16::writeval(pov + 22, (*p)->arg_serial());
      pov += this->input_entry_size;
    }
  return pov;
}

// Write the supplemental information blocks.

template<int size, bool big_endian>
unsigned char*
Output_section_incremental_inputs<size, big_endian>::write_info_blocks(
    unsigned char* oview,
    unsigned char* pov,
    Stringpool* strtab,
    unsigned int* global_syms,
    unsigned int global_sym_count)
{
  const Incremental_inputs* inputs = this->inputs_;
  unsigned int first_global_index = this->symtab_->first_global_index();

  for (Incremental_inputs::Input_list::const_iterator p =
	   inputs->input_files().begin();
       p != inputs->input_files().end();
       ++p)
    {
      switch ((*p)->type())
	{
	case INCREMENTAL_INPUT_SCRIPT:
	  {
	    gold_assert(static_cast<unsigned int>(pov - oview)
			== (*p)->get_info_offset());
	    Incremental_script_entry* entry = (*p)->script_entry();
	    gold_assert(entry != NULL);

	    // Write the object count.
	    unsigned int nobjects = entry->get_object_count();
	    Swap32::writeval(pov, nobjects);
	    pov += 4;

	    // For each object, write the offset to its input file entry.
	    for (unsigned int i = 0; i < nobjects; ++i)
	      {
		Incremental_input_entry* obj = entry->get_object(i);
		Swap32::writeval(pov, obj->get_offset());
		pov += 4;
	      }
	  }
	  break;

	case INCREMENTAL_INPUT_OBJECT:
	case INCREMENTAL_INPUT_ARCHIVE_MEMBER:
	  {
	    gold_assert(static_cast<unsigned int>(pov - oview)
			== (*p)->get_info_offset());
	    Incremental_object_entry* entry = (*p)->object_entry();
	    gold_assert(entry != NULL);
	    const Object* obj = entry->object();
	    const Object::Symbols* syms = obj->get_global_symbols();
	    // Write the input section count and global symbol count.
	    unsigned int nsections = entry->get_input_section_count();
	    unsigned int nsyms = syms->size();
	    Swap32::writeval(pov, nsections);
	    Swap32::writeval(pov + 4, nsyms);
	    pov += 8;

	    // Build a temporary array to map input section indexes
	    // from the original object file index to the index in the
	    // incremental info table.
	    unsigned int* index_map = new unsigned int[obj->shnum()];
	    memset(index_map, 0, obj->shnum() * sizeof(unsigned int));

	    // For each input section, write the name, output section index,
	    // offset within output section, and input section size.
	    for (unsigned int i = 0; i < nsections; i++)
	      {
		unsigned int shndx = entry->get_input_section_index(i);
		index_map[shndx] = i + 1;
		Stringpool::Key key = entry->get_input_section_name_key(i);
		off_t name_offset = 0;
		if (key != 0)
		  name_offset = strtab->get_offset_from_key(key);
		int out_shndx = 0;
		off_t out_offset = 0;
		off_t sh_size = 0;
		Output_section* os = obj->output_section(shndx);
		if (os != NULL)
		  {
		    out_shndx = os->out_shndx();
		    out_offset = obj->output_section_offset(shndx);
		    sh_size = entry->get_input_section_size(i);
		  }
		Swap32::writeval(pov, name_offset);
		Swap32::writeval(pov + 4, out_shndx);
		Swap::writeval(pov + 8, out_offset);
		Swap::writeval(pov + 8 + sizeof_addr, sh_size);
		pov += 8 + 2 * sizeof_addr;
	      }

	    // For each global symbol, write its associated relocations,
	    // add it to the linked list of globals, then write the
	    // supplemental information:  global symbol table index,
	    // input section index, linked list chain pointer, relocation
	    // count, and offset to the relocations.
	    for (unsigned int i = 0; i < nsyms; i++)
	      {
		const Symbol* sym = (*syms)[i];
		if (sym->is_forwarder())
		  sym = this->symtab_->resolve_forwards(sym);
		unsigned int shndx = 0;
		if (sym->source() == Symbol::FROM_OBJECT
		    && sym->object() == obj
		    && sym->is_defined())
		  {
		    bool is_ordinary;
		    unsigned int orig_shndx = sym->shndx(&is_ordinary);
		    if (is_ordinary)
		      shndx = index_map[orig_shndx];
		  }
		unsigned int symtab_index = sym->symtab_index();
		unsigned int chain = 0;
		unsigned int first_reloc = 0;
		unsigned int nrelocs = obj->get_incremental_reloc_count(i);
		if (nrelocs > 0)
		  {
		    gold_assert(symtab_index != -1U
				&& (symtab_index - first_global_index
				    < global_sym_count));
		    first_reloc = obj->get_incremental_reloc_base(i);
		    chain = global_syms[symtab_index - first_global_index];
		    global_syms[symtab_index - first_global_index] =
			pov - oview;
		  }
		Swap32::writeval(pov, symtab_index);
		Swap32::writeval(pov + 4, shndx);
		Swap32::writeval(pov + 8, chain);
		Swap32::writeval(pov + 12, nrelocs);
		Swap32::writeval(pov + 16, first_reloc * 3 * sizeof_addr);
		pov += 20;
	      }

	    delete[] index_map;
	  }
	  break;

	case INCREMENTAL_INPUT_SHARED_LIBRARY:
	  {
	    gold_assert(static_cast<unsigned int>(pov - oview)
			== (*p)->get_info_offset());
	    Incremental_object_entry* entry = (*p)->object_entry();
	    gold_assert(entry != NULL);
	    const Object* obj = entry->object();
	    const Object::Symbols* syms = obj->get_global_symbols();

	    // Skip the global symbol count for now.
	    unsigned char* orig_pov = pov;
	    pov += 4;

	    // For each global symbol, write the global symbol table index.
	    unsigned int nsyms = syms->size();
	    unsigned int nsyms_out = 0;
	    for (unsigned int i = 0; i < nsyms; i++)
	      {
		const Symbol* sym = (*syms)[i];
		if (sym == NULL)
		  continue;
		if (sym->is_forwarder())
		  sym = this->symtab_->resolve_forwards(sym);
	        if (sym->symtab_index() == -1U)
	          continue;
		unsigned int def_flag = 0;
		if (sym->source() == Symbol::FROM_OBJECT
		    && sym->object() == obj
		    && sym->is_defined())
		  def_flag = 1U << 31;
		Swap32::writeval(pov, sym->symtab_index() | def_flag);
		pov += 4;
		++nsyms_out;
	      }

	    // Now write the global symbol count.
	    Swap32::writeval(orig_pov, nsyms_out);
	  }
	  break;

	case INCREMENTAL_INPUT_ARCHIVE:
	  {
	    gold_assert(static_cast<unsigned int>(pov - oview)
			== (*p)->get_info_offset());
	    Incremental_archive_entry* entry = (*p)->archive_entry();
	    gold_assert(entry != NULL);

	    // Write the member count and unused global symbol count.
	    unsigned int nmembers = entry->get_member_count();
	    unsigned int nsyms = entry->get_unused_global_symbol_count();
	    Swap32::writeval(pov, nmembers);
	    Swap32::writeval(pov + 4, nsyms);
	    pov += 8;

	    // For each member, write the offset to its input file entry.
	    for (unsigned int i = 0; i < nmembers; ++i)
	      {
		Incremental_object_entry* member = entry->get_member(i);
		Swap32::writeval(pov, member->get_offset());
		pov += 4;
	      }

	    // For each global symbol, write the name offset.
	    for (unsigned int i = 0; i < nsyms; ++i)
	      {
		Stringpool::Key key = entry->get_unused_global_symbol(i);
		Swap32::writeval(pov, strtab->get_offset_from_key(key));
		pov += 4;
	      }
	  }
	  break;

	default:
	  gold_unreachable();
	}
    }
  return pov;
}

// Write the contents of the .gnu_incremental_symtab section.

template<int size, bool big_endian>
void
Output_section_incremental_inputs<size, big_endian>::write_symtab(
    unsigned char* pov,
    unsigned int* global_syms,
    unsigned int global_sym_count)
{
  for (unsigned int i = 0; i < global_sym_count; ++i)
    {
      Swap32::writeval(pov, global_syms[i]);
      pov += 4;
    }
}

// This struct holds the view information needed to write the
// .gnu_incremental_got_plt section.

struct Got_plt_view_info
{
  // Start of the GOT type array in the output view.
  unsigned char* got_type_p;
  // Start of the GOT descriptor array in the output view.
  unsigned char* got_desc_p;
  // Start of the PLT descriptor array in the output view.
  unsigned char* plt_desc_p;
  // Number of GOT entries.
  unsigned int got_count;
  // Number of PLT entries.
  unsigned int plt_count;
  // Offset of the first non-reserved PLT entry (this is a target-dependent value).
  unsigned int first_plt_entry_offset;
  // Size of a PLT entry (this is a target-dependent value).
  unsigned int plt_entry_size;
  // Value to write in the GOT descriptor array.  For global symbols,
  // this is the global symbol table index; for local symbols, it is
  // the offset of the input file entry in the .gnu_incremental_inputs
  // section.
  unsigned int got_descriptor;
};

// Functor class for processing a GOT offset list for local symbols.
// Writes the GOT type and symbol index into the GOT type and descriptor
// arrays in the output section.

template<int size, bool big_endian>
class Local_got_offset_visitor : public Got_offset_list::Visitor
{
 public:
  Local_got_offset_visitor(struct Got_plt_view_info& info)
    : info_(info)
  { }

  void
  visit(unsigned int got_type, unsigned int got_offset)
  {
    unsigned int got_index = got_offset / this->got_entry_size_;
    gold_assert(got_index < this->info_.got_count);
    // We can only handle GOT entry types in the range 0..0x7e
    // because we use a byte array to store them, and we use the
    // high bit to flag a local symbol.
    gold_assert(got_type < 0x7f);
    this->info_.got_type_p[got_index] = got_type | 0x80;
    unsigned char* pov = this->info_.got_desc_p + got_index * 4;
    elfcpp::Swap<32, big_endian>::writeval(pov, this->info_.got_descriptor);
  }

 private:
  static const unsigned int got_entry_size_ = size / 8;
  struct Got_plt_view_info& info_;
};

// Functor class for processing a GOT offset list.  Writes the GOT type
// and symbol index into the GOT type and descriptor arrays in the output
// section.

template<int size, bool big_endian>
class Global_got_offset_visitor : public Got_offset_list::Visitor
{
 public:
  Global_got_offset_visitor(struct Got_plt_view_info& info)
    : info_(info)
  { }

  void
  visit(unsigned int got_type, unsigned int got_offset)
  {
    unsigned int got_index = got_offset / this->got_entry_size_;
    gold_assert(got_index < this->info_.got_count);
    // We can only handle GOT entry types in the range 0..0x7e
    // because we use a byte array to store them, and we use the
    // high bit to flag a local symbol.
    gold_assert(got_type < 0x7f);
    this->info_.got_type_p[got_index] = got_type;
    unsigned char* pov = this->info_.got_desc_p + got_index * 4;
    elfcpp::Swap<32, big_endian>::writeval(pov, this->info_.got_descriptor);
  }

 private:
  static const unsigned int got_entry_size_ = size / 8;
  struct Got_plt_view_info& info_;
};

// Functor class for processing the global symbol table.  Processes the
// GOT offset list for the symbol, and writes the symbol table index
// into the PLT descriptor array in the output section.

template<int size, bool big_endian>
class Global_symbol_visitor_got_plt
{
 public:
  Global_symbol_visitor_got_plt(struct Got_plt_view_info& info)
    : info_(info)
  { }

  void
  operator()(const Sized_symbol<size>* sym)
  {
    typedef Global_got_offset_visitor<size, big_endian> Got_visitor;
    const Got_offset_list* got_offsets = sym->got_offset_list();
    if (got_offsets != NULL)
      {
        this->info_.got_descriptor = sym->symtab_index();
        Got_visitor v(this->info_);
	got_offsets->for_all_got_offsets(&v);
      }
    if (sym->has_plt_offset())
      {
	unsigned int plt_index =
	    ((sym->plt_offset() - this->info_.first_plt_entry_offset)
	     / this->info_.plt_entry_size);
	gold_assert(plt_index < this->info_.plt_count);
	unsigned char* pov = this->info_.plt_desc_p + plt_index * 4;
	elfcpp::Swap<32, big_endian>::writeval(pov, sym->symtab_index());
      }
  }

 private:
  struct Got_plt_view_info& info_;
};

// Write the contents of the .gnu_incremental_got_plt section.

template<int size, bool big_endian>
void
Output_section_incremental_inputs<size, big_endian>::write_got_plt(
    unsigned char* pov,
    off_t view_size)
{
  Sized_target<size, big_endian>* target =
    parameters->sized_target<size, big_endian>();

  // Set up the view information for the functors.
  struct Got_plt_view_info view_info;
  view_info.got_count = target->got_entry_count();
  view_info.plt_count = target->plt_entry_count();
  view_info.first_plt_entry_offset = target->first_plt_entry_offset();
  view_info.plt_entry_size = target->plt_entry_size();
  view_info.got_type_p = pov + 8;
  view_info.got_desc_p = (view_info.got_type_p
			  + ((view_info.got_count + 3) & ~3));
  view_info.plt_desc_p = view_info.got_desc_p + view_info.got_count * 4;

  gold_assert(pov + view_size ==
	      view_info.plt_desc_p + view_info.plt_count * 4);

  // Write the section header.
  Swap32::writeval(pov, view_info.got_count);
  Swap32::writeval(pov + 4, view_info.plt_count);

  // Initialize the GOT type array to 0xff (reserved).
  memset(view_info.got_type_p, 0xff, view_info.got_count);

  // Write the incremental GOT descriptors for local symbols.
  typedef Local_got_offset_visitor<size, big_endian> Got_visitor;
  for (Incremental_inputs::Input_list::const_iterator p =
	   this->inputs_->input_files().begin();
       p != this->inputs_->input_files().end();
       ++p)
    {
      if ((*p)->type() != INCREMENTAL_INPUT_OBJECT
	  && (*p)->type() != INCREMENTAL_INPUT_ARCHIVE_MEMBER)
	continue;
      Incremental_object_entry* entry = (*p)->object_entry();
      gold_assert(entry != NULL);
      const Object* obj = entry->object();
      gold_assert(obj != NULL);
      view_info.got_descriptor = (*p)->get_offset();
      Got_visitor v(view_info);
      obj->for_all_local_got_entries(&v);
    }

  // Write the incremental GOT and PLT descriptors for global symbols.
  typedef Global_symbol_visitor_got_plt<size, big_endian> Symbol_visitor;
  symtab_->for_all_symbols<size, Symbol_visitor>(Symbol_visitor(view_info));
}

// Class Sized_incr_relobj.  Most of these methods are not used for
// Incremental objects, but are required to be implemented by the
// base class Object.

template<int size, bool big_endian>
Sized_incr_relobj<size, big_endian>::Sized_incr_relobj(
    const std::string& name,
    Sized_incremental_binary<size, big_endian>* ibase,
    unsigned int input_file_index)
  : Sized_relobj_base<size, big_endian>(name, NULL), ibase_(ibase),
    input_file_index_(input_file_index),
    input_reader_(ibase->inputs_reader().input_file(input_file_index)),
    symbols_(), section_offsets_(), incr_reloc_offset_(-1U),
    incr_reloc_count_(0), incr_reloc_output_index_(0), incr_relocs_(NULL)
{
  if (this->input_reader_.is_in_system_directory())
    this->set_is_in_system_directory();
  const unsigned int shnum = this->input_reader_.get_input_section_count() + 1;
  this->set_shnum(shnum);
}

// Read the symbols.

template<int size, bool big_endian>
void
Sized_incr_relobj<size, big_endian>::do_read_symbols(Read_symbols_data*)
{
  gold_unreachable();
}

// Lay out the input sections.

template<int size, bool big_endian>
void
Sized_incr_relobj<size, big_endian>::do_layout(
    Symbol_table*,
    Layout* layout,
    Read_symbols_data*)
{
  const unsigned int shnum = this->shnum();
  Incremental_inputs* incremental_inputs = layout->incremental_inputs();
  gold_assert(incremental_inputs != NULL);
  Output_sections& out_sections(this->output_sections());
  out_sections.resize(shnum);
  this->section_offsets_.resize(shnum);
  for (unsigned int i = 1; i < shnum; i++)
    {
      typename Input_entry_reader::Input_section_info sect =
          this->input_reader_.get_input_section(i - 1);
      // Add the section to the incremental inputs layout.
      incremental_inputs->report_input_section(this, i, sect.name,
					       sect.sh_size);
      if (sect.output_shndx == 0 || sect.sh_offset == -1)
        continue;
      Output_section* os = this->ibase_->output_section(sect.output_shndx);
      gold_assert(os != NULL);
      out_sections[i] = os;
      this->section_offsets_[i] = static_cast<Address>(sect.sh_offset);
    }
}

// Layout sections whose layout was deferred while waiting for
// input files from a plugin.
template<int size, bool big_endian>
void
Sized_incr_relobj<size, big_endian>::do_layout_deferred_sections(Layout*)
{
}

// Add the symbols to the symbol table.

template<int size, bool big_endian>
void
Sized_incr_relobj<size, big_endian>::do_add_symbols(
    Symbol_table* symtab,
    Read_symbols_data*,
    Layout*)
{
  const int sym_size = elfcpp::Elf_sizes<size>::sym_size;
  unsigned char symbuf[sym_size];
  elfcpp::Sym<size, big_endian> sym(symbuf);
  elfcpp::Sym_write<size, big_endian> osym(symbuf);

  typedef typename elfcpp::Elf_types<size>::Elf_WXword Elf_size_type;

  unsigned int nsyms = this->input_reader_.get_global_symbol_count();
  this->symbols_.resize(nsyms);

  Incremental_binary::View symtab_view(NULL);
  unsigned int symtab_count;
  elfcpp::Elf_strtab strtab(NULL, 0);
  this->ibase_->get_symtab_view(&symtab_view, &symtab_count, &strtab);

  // Incremental_symtab_reader<big_endian> isymtab(this->ibase_->symtab_reader());
  // Incremental_relocs_reader<size, big_endian> irelocs(this->ibase_->relocs_reader());
  // unsigned int isym_count = isymtab.symbol_count();
  // unsigned int first_global = symtab_count - isym_count;

  unsigned const char* sym_p;
  for (unsigned int i = 0; i < nsyms; ++i)
    {
      Incremental_global_symbol_reader<big_endian> info =
	  this->input_reader_.get_global_symbol_reader(i);
      sym_p = symtab_view.data() + info.output_symndx() * sym_size;
      elfcpp::Sym<size, big_endian> gsym(sym_p);
      const char* name;
      if (!strtab.get_c_string(gsym.get_st_name(), &name))
	name = "";

      typename elfcpp::Elf_types<size>::Elf_Addr v = gsym.get_st_value();
      unsigned int shndx = gsym.get_st_shndx();
      elfcpp::STB st_bind = gsym.get_st_bind();
      elfcpp::STT st_type = gsym.get_st_type();

      // Local hidden symbols start out as globals, but get converted to
      // to local during output.
      if (st_bind == elfcpp::STB_LOCAL)
        st_bind = elfcpp::STB_GLOBAL;

      unsigned int input_shndx = info.shndx();
      if (input_shndx == 0)
	{
	  shndx = elfcpp::SHN_UNDEF;
	  v = 0;
	}
      else if (shndx != elfcpp::SHN_ABS)
	{
	  // Find the input section and calculate the section-relative value.
	  gold_assert(shndx != elfcpp::SHN_UNDEF);
	  Output_section* os = this->ibase_->output_section(shndx);
	  gold_assert(os != NULL && os->has_fixed_layout());
	  typename Input_entry_reader::Input_section_info sect =
	      this->input_reader_.get_input_section(input_shndx - 1);
	  gold_assert(sect.output_shndx == shndx);
	  if (st_type != elfcpp::STT_TLS)
	    v -= os->address();
	  v -= sect.sh_offset;
	  shndx = input_shndx;
	}

      osym.put_st_name(0);
      osym.put_st_value(v);
      osym.put_st_size(gsym.get_st_size());
      osym.put_st_info(st_bind, st_type);
      osym.put_st_other(gsym.get_st_other());
      osym.put_st_shndx(shndx);

      this->symbols_[i] =
	symtab->add_from_incrobj(this, name, NULL, &sym);
    }
}

// Return TRUE if we should include this object from an archive library.

template<int size, bool big_endian>
Archive::Should_include
Sized_incr_relobj<size, big_endian>::do_should_include_member(
    Symbol_table*,
    Layout*,
    Read_symbols_data*,
    std::string*)
{
  gold_unreachable();
}

// Iterate over global symbols, calling a visitor class V for each.

template<int size, bool big_endian>
void
Sized_incr_relobj<size, big_endian>::do_for_all_global_symbols(
    Read_symbols_data*,
    Library_base::Symbol_visitor_base*)
{
  // This routine is not used for incremental objects.
}

// Iterate over local symbols, calling a visitor class V for each GOT offset
// associated with a local symbol.

template<int size, bool big_endian>
void
Sized_incr_relobj<size, big_endian>::do_for_all_local_got_entries(
    Got_offset_list::Visitor*) const
{
  // FIXME: Implement Sized_incr_relobj::do_for_all_local_got_entries.
}

// Get the size of a section.

template<int size, bool big_endian>
uint64_t
Sized_incr_relobj<size, big_endian>::do_section_size(unsigned int)
{
  gold_unreachable();
}

// Get the name of a section.

template<int size, bool big_endian>
std::string
Sized_incr_relobj<size, big_endian>::do_section_name(unsigned int)
{
  gold_unreachable();
}

// Return a view of the contents of a section.

template<int size, bool big_endian>
Object::Location
Sized_incr_relobj<size, big_endian>::do_section_contents(unsigned int)
{
  gold_unreachable();
}

// Return section flags.

template<int size, bool big_endian>
uint64_t
Sized_incr_relobj<size, big_endian>::do_section_flags(unsigned int)
{
  gold_unreachable();
}

// Return section entsize.

template<int size, bool big_endian>
uint64_t
Sized_incr_relobj<size, big_endian>::do_section_entsize(unsigned int)
{
  gold_unreachable();
}

// Return section address.

template<int size, bool big_endian>
uint64_t
Sized_incr_relobj<size, big_endian>::do_section_address(unsigned int)
{
  gold_unreachable();
}

// Return section type.

template<int size, bool big_endian>
unsigned int
Sized_incr_relobj<size, big_endian>::do_section_type(unsigned int)
{
  gold_unreachable();
}

// Return the section link field.

template<int size, bool big_endian>
unsigned int
Sized_incr_relobj<size, big_endian>::do_section_link(unsigned int)
{
  gold_unreachable();
}

// Return the section link field.

template<int size, bool big_endian>
unsigned int
Sized_incr_relobj<size, big_endian>::do_section_info(unsigned int)
{
  gold_unreachable();
}

// Return the section alignment.

template<int size, bool big_endian>
uint64_t
Sized_incr_relobj<size, big_endian>::do_section_addralign(unsigned int)
{
  gold_unreachable();
}

// Return the Xindex structure to use.

template<int size, bool big_endian>
Xindex*
Sized_incr_relobj<size, big_endian>::do_initialize_xindex()
{
  gold_unreachable();
}

// Get symbol counts.

template<int size, bool big_endian>
void
Sized_incr_relobj<size, big_endian>::do_get_global_symbol_counts(
    const Symbol_table*, size_t*, size_t*) const
{
  gold_unreachable();
}

// Read the relocs.

template<int size, bool big_endian>
void
Sized_incr_relobj<size, big_endian>::do_read_relocs(Read_relocs_data*)
{
}

// Process the relocs to find list of referenced sections. Used only
// during garbage collection.

template<int size, bool big_endian>
void
Sized_incr_relobj<size, big_endian>::do_gc_process_relocs(Symbol_table*,
							  Layout*,
							  Read_relocs_data*)
{
  gold_unreachable();
}

// Scan the relocs and adjust the symbol table.

template<int size, bool big_endian>
void
Sized_incr_relobj<size, big_endian>::do_scan_relocs(Symbol_table*,
						    Layout* layout,
						    Read_relocs_data*)
{
  // Count the incremental relocations for this object.
  unsigned int nsyms = this->input_reader_.get_global_symbol_count();
  this->allocate_incremental_reloc_counts();
  for (unsigned int i = 0; i < nsyms; i++)
    {
      Incremental_global_symbol_reader<big_endian> sym =
	  this->input_reader_.get_global_symbol_reader(i);
      unsigned int reloc_count = sym.reloc_count();
      if (reloc_count > 0 && this->incr_reloc_offset_ == -1U)
	this->incr_reloc_offset_ = sym.reloc_offset();
      this->incr_reloc_count_ += reloc_count;
      for (unsigned int j = 0; j < reloc_count; j++)
	this->count_incremental_reloc(i);
    }
  this->incr_reloc_output_index_ =
      layout->incremental_inputs()->get_reloc_count();
  this->finalize_incremental_relocs(layout, false);

  // The incoming incremental relocations may not end up in the same
  // location after the incremental update, because the incremental info
  // is regenerated in each link.  Because the new location may overlap
  // with other data in the updated output file, we need to copy the
  // relocations into a buffer so that we can still read them safely
  // after we start writing updates to the output file.
  if (this->incr_reloc_count_ > 0)
    {
      const Incremental_relocs_reader<size, big_endian>& relocs_reader =
	  this->ibase_->relocs_reader();
      const unsigned int incr_reloc_size = relocs_reader.reloc_size;
      unsigned int len = this->incr_reloc_count_ * incr_reloc_size;
      this->incr_relocs_ = new unsigned char[len];
      memcpy(this->incr_relocs_,
	     relocs_reader.data(this->incr_reloc_offset_),
	     len);
    }
}

// Count the local symbols.

template<int size, bool big_endian>
void
Sized_incr_relobj<size, big_endian>::do_count_local_symbols(
    Stringpool_template<char>*,
    Stringpool_template<char>*)
{
  // FIXME: Count local symbols.
}

// Finalize the local symbols.

template<int size, bool big_endian>
unsigned int
Sized_incr_relobj<size, big_endian>::do_finalize_local_symbols(
    unsigned int index,
    off_t,
    Symbol_table*)
{
  // FIXME: Finalize local symbols.
  return index;
}

// Set the offset where local dynamic symbol information will be stored.

template<int size, bool big_endian>
unsigned int
Sized_incr_relobj<size, big_endian>::do_set_local_dynsym_indexes(
    unsigned int index)
{
  // FIXME: set local dynsym indexes.
  return index;
}

// Set the offset where local dynamic symbol information will be stored.

template<int size, bool big_endian>
unsigned int
Sized_incr_relobj<size, big_endian>::do_set_local_dynsym_offset(off_t)
{
  return 0;
}

// Relocate the input sections and write out the local symbols.
// We don't actually do any relocation here.  For unchanged input files,
// we reapply relocations only for symbols that have changed; that happens
// in queue_final_tasks.  We do need to rewrite the incremental relocations
// for this object.

template<int size, bool big_endian>
void
Sized_incr_relobj<size, big_endian>::do_relocate(const Symbol_table*,
						 const Layout* layout,
						 Output_file* of)
{
  if (this->incr_reloc_count_ == 0)
    return;

  const unsigned int incr_reloc_size =
      Incremental_relocs_reader<size, big_endian>::reloc_size;

  // Get a view for the .gnu_incremental_relocs section.
  Incremental_inputs* inputs = layout->incremental_inputs();
  gold_assert(inputs != NULL);
  const off_t relocs_off = inputs->relocs_section()->offset();
  const off_t relocs_size = inputs->relocs_section()->data_size();
  unsigned char* const view = of->get_output_view(relocs_off, relocs_size);

  // Copy the relocations from the buffer.
  off_t off = this->incr_reloc_output_index_ * incr_reloc_size;
  unsigned int len = this->incr_reloc_count_ * incr_reloc_size;
  memcpy(view + off, this->incr_relocs_, len);
  of->write_output_view(off, len, view);
}

// Set the offset of a section.

template<int size, bool big_endian>
void
Sized_incr_relobj<size, big_endian>::do_set_section_offset(unsigned int,
							   uint64_t)
{
}

// Class Sized_incr_dynobj.  Most of these methods are not used for
// Incremental objects, but are required to be implemented by the
// base class Object.

template<int size, bool big_endian>
Sized_incr_dynobj<size, big_endian>::Sized_incr_dynobj(
    const std::string& name,
    Sized_incremental_binary<size, big_endian>* ibase,
    unsigned int input_file_index)
  : Dynobj(name, NULL), ibase_(ibase),
    input_file_index_(input_file_index),
    input_reader_(ibase->inputs_reader().input_file(input_file_index)),
    symbols_()
{
  if (this->input_reader_.is_in_system_directory())
    this->set_is_in_system_directory();
  this->set_shnum(0);
}

// Read the symbols.

template<int size, bool big_endian>
void
Sized_incr_dynobj<size, big_endian>::do_read_symbols(Read_symbols_data*)
{
  gold_unreachable();
}

// Lay out the input sections.

template<int size, bool big_endian>
void
Sized_incr_dynobj<size, big_endian>::do_layout(
    Symbol_table*,
    Layout*,
    Read_symbols_data*)
{
}

// Add the symbols to the symbol table.

template<int size, bool big_endian>
void
Sized_incr_dynobj<size, big_endian>::do_add_symbols(
    Symbol_table* symtab,
    Read_symbols_data*,
    Layout*)
{
  const int sym_size = elfcpp::Elf_sizes<size>::sym_size;
  unsigned char symbuf[sym_size];
  elfcpp::Sym<size, big_endian> sym(symbuf);
  elfcpp::Sym_write<size, big_endian> osym(symbuf);

  typedef typename elfcpp::Elf_types<size>::Elf_WXword Elf_size_type;

  unsigned int nsyms = this->input_reader_.get_global_symbol_count();
  this->symbols_.resize(nsyms);

  Incremental_binary::View symtab_view(NULL);
  unsigned int symtab_count;
  elfcpp::Elf_strtab strtab(NULL, 0);
  this->ibase_->get_symtab_view(&symtab_view, &symtab_count, &strtab);

  // Incremental_symtab_reader<big_endian> isymtab(this->ibase_->symtab_reader());
  // Incremental_relocs_reader<size, big_endian> irelocs(this->ibase_->relocs_reader());
  // unsigned int isym_count = isymtab.symbol_count();
  // unsigned int first_global = symtab_count - isym_count;

  unsigned const char* sym_p;
  for (unsigned int i = 0; i < nsyms; ++i)
    {
      bool is_def;
      unsigned int output_symndx =
	  this->input_reader_.get_output_symbol_index(i, &is_def);
      sym_p = symtab_view.data() + output_symndx * sym_size;
      elfcpp::Sym<size, big_endian> gsym(sym_p);
      const char* name;
      if (!strtab.get_c_string(gsym.get_st_name(), &name))
	name = "";

      typename elfcpp::Elf_types<size>::Elf_Addr v;
      unsigned int shndx;
      elfcpp::STB st_bind = gsym.get_st_bind();
      elfcpp::STT st_type = gsym.get_st_type();

      // Local hidden symbols start out as globals, but get converted to
      // to local during output.
      if (st_bind == elfcpp::STB_LOCAL)
        st_bind = elfcpp::STB_GLOBAL;

      if (!is_def)
	{
	  shndx = elfcpp::SHN_UNDEF;
	  v = 0;
	}
      else
	{
	  // For a symbol defined in a shared object, the section index
	  // is meaningless, as long as it's not SHN_UNDEF.
	  shndx = 1;
	  v = gsym.get_st_value();
	}

      osym.put_st_name(0);
      osym.put_st_value(v);
      osym.put_st_size(gsym.get_st_size());
      osym.put_st_info(st_bind, st_type);
      osym.put_st_other(gsym.get_st_other());
      osym.put_st_shndx(shndx);

      this->symbols_[i] =
	symtab->add_from_incrobj<size, big_endian>(this, name, NULL, &sym);
    }
}

// Return TRUE if we should include this object from an archive library.

template<int size, bool big_endian>
Archive::Should_include
Sized_incr_dynobj<size, big_endian>::do_should_include_member(
    Symbol_table*,
    Layout*,
    Read_symbols_data*,
    std::string*)
{
  gold_unreachable();
}

// Iterate over global symbols, calling a visitor class V for each.

template<int size, bool big_endian>
void
Sized_incr_dynobj<size, big_endian>::do_for_all_global_symbols(
    Read_symbols_data*,
    Library_base::Symbol_visitor_base*)
{
  // This routine is not used for dynamic libraries.
}

// Iterate over local symbols, calling a visitor class V for each GOT offset
// associated with a local symbol.

template<int size, bool big_endian>
void
Sized_incr_dynobj<size, big_endian>::do_for_all_local_got_entries(
    Got_offset_list::Visitor*) const
{
  // FIXME: Implement Sized_incr_dynobj::do_for_all_local_got_entries.
}

// Get the size of a section.

template<int size, bool big_endian>
uint64_t
Sized_incr_dynobj<size, big_endian>::do_section_size(unsigned int)
{
  gold_unreachable();
}

// Get the name of a section.

template<int size, bool big_endian>
std::string
Sized_incr_dynobj<size, big_endian>::do_section_name(unsigned int)
{
  gold_unreachable();
}

// Return a view of the contents of a section.

template<int size, bool big_endian>
Object::Location
Sized_incr_dynobj<size, big_endian>::do_section_contents(unsigned int)
{
  gold_unreachable();
}

// Return section flags.

template<int size, bool big_endian>
uint64_t
Sized_incr_dynobj<size, big_endian>::do_section_flags(unsigned int)
{
  gold_unreachable();
}

// Return section entsize.

template<int size, bool big_endian>
uint64_t
Sized_incr_dynobj<size, big_endian>::do_section_entsize(unsigned int)
{
  gold_unreachable();
}

// Return section address.

template<int size, bool big_endian>
uint64_t
Sized_incr_dynobj<size, big_endian>::do_section_address(unsigned int)
{
  gold_unreachable();
}

// Return section type.

template<int size, bool big_endian>
unsigned int
Sized_incr_dynobj<size, big_endian>::do_section_type(unsigned int)
{
  gold_unreachable();
}

// Return the section link field.

template<int size, bool big_endian>
unsigned int
Sized_incr_dynobj<size, big_endian>::do_section_link(unsigned int)
{
  gold_unreachable();
}

// Return the section link field.

template<int size, bool big_endian>
unsigned int
Sized_incr_dynobj<size, big_endian>::do_section_info(unsigned int)
{
  gold_unreachable();
}

// Return the section alignment.

template<int size, bool big_endian>
uint64_t
Sized_incr_dynobj<size, big_endian>::do_section_addralign(unsigned int)
{
  gold_unreachable();
}

// Return the Xindex structure to use.

template<int size, bool big_endian>
Xindex*
Sized_incr_dynobj<size, big_endian>::do_initialize_xindex()
{
  gold_unreachable();
}

// Get symbol counts.

template<int size, bool big_endian>
void
Sized_incr_dynobj<size, big_endian>::do_get_global_symbol_counts(
    const Symbol_table*, size_t*, size_t*) const
{
  gold_unreachable();
}

// Allocate an incremental object of the appropriate size and endianness.

Object*
make_sized_incremental_object(
    Incremental_binary* ibase,
    unsigned int input_file_index,
    Incremental_input_type input_type,
    const Incremental_binary::Input_reader* input_reader)
{
  Object* obj = NULL;
  std::string name(input_reader->filename());

  switch (parameters->size_and_endianness())
    {
#ifdef HAVE_TARGET_32_LITTLE
    case Parameters::TARGET_32_LITTLE:
      {
	Sized_incremental_binary<32, false>* sized_ibase =
	    static_cast<Sized_incremental_binary<32, false>*>(ibase);
	if (input_type == INCREMENTAL_INPUT_SHARED_LIBRARY)
	  obj = new Sized_incr_dynobj<32, false>(name, sized_ibase,
						 input_file_index);
	else
	  obj = new Sized_incr_relobj<32, false>(name, sized_ibase,
						 input_file_index);
      }
      break;
#endif
#ifdef HAVE_TARGET_32_BIG
    case Parameters::TARGET_32_BIG:
      {
	Sized_incremental_binary<32, true>* sized_ibase =
	    static_cast<Sized_incremental_binary<32, true>*>(ibase);
	if (input_type == INCREMENTAL_INPUT_SHARED_LIBRARY)
	  obj = new Sized_incr_dynobj<32, true>(name, sized_ibase,
						input_file_index);
	else
	  obj = new Sized_incr_relobj<32, true>(name, sized_ibase,
						input_file_index);
      }
      break;
#endif
#ifdef HAVE_TARGET_64_LITTLE
    case Parameters::TARGET_64_LITTLE:
      {
	Sized_incremental_binary<64, false>* sized_ibase =
	    static_cast<Sized_incremental_binary<64, false>*>(ibase);
	if (input_type == INCREMENTAL_INPUT_SHARED_LIBRARY)
	  obj = new Sized_incr_dynobj<64, false>(name, sized_ibase,
						 input_file_index);
	else
	  obj = new Sized_incr_relobj<64, false>(name, sized_ibase,
						 input_file_index);
     }
      break;
#endif
#ifdef HAVE_TARGET_64_BIG
    case Parameters::TARGET_64_BIG:
      {
	Sized_incremental_binary<64, true>* sized_ibase =
	    static_cast<Sized_incremental_binary<64, true>*>(ibase);
	if (input_type == INCREMENTAL_INPUT_SHARED_LIBRARY)
	  obj = new Sized_incr_dynobj<64, true>(name, sized_ibase,
						input_file_index);
	else
	  obj = new Sized_incr_relobj<64, true>(name, sized_ibase,
						input_file_index);
      }
      break;
#endif
    default:
      gold_unreachable();
    }

  gold_assert(obj != NULL);
  return obj;
}

// Copy the unused symbols from the incremental input info.
// We need to do this because we may be overwriting the incremental
// input info in the base file before we write the new incremental
// info.
void
Incremental_library::copy_unused_symbols()
{
  unsigned int symcount = this->input_reader_->get_unused_symbol_count();
  this->unused_symbols_.reserve(symcount);
  for (unsigned int i = 0; i < symcount; ++i)
    {
      std::string name(this->input_reader_->get_unused_symbol(i));
      this->unused_symbols_.push_back(name);
    }
}

// Iterator for unused global symbols in the library.
void
Incremental_library::do_for_all_unused_symbols(Symbol_visitor_base* v) const
{
  for (Symbol_list::const_iterator p = this->unused_symbols_.begin();
       p != this->unused_symbols_.end();
       ++p)
  v->visit(p->c_str());
}

// Instantiate the templates we need.

#ifdef HAVE_TARGET_32_LITTLE
template
class Sized_incremental_binary<32, false>;

template
class Sized_incr_relobj<32, false>;

template
class Sized_incr_dynobj<32, false>;
#endif

#ifdef HAVE_TARGET_32_BIG
template
class Sized_incremental_binary<32, true>;

template
class Sized_incr_relobj<32, true>;

template
class Sized_incr_dynobj<32, true>;
#endif

#ifdef HAVE_TARGET_64_LITTLE
template
class Sized_incremental_binary<64, false>;

template
class Sized_incr_relobj<64, false>;

template
class Sized_incr_dynobj<64, false>;
#endif

#ifdef HAVE_TARGET_64_BIG
template
class Sized_incremental_binary<64, true>;

template
class Sized_incr_relobj<64, true>;

template
class Sized_incr_dynobj<64, true>;
#endif

} // End namespace gold.