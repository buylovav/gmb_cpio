
#include <fcntl.h>
#include <fstream>
#include <archive.h>
#include <archive_entry.h>
#include <algorithm>

#include "cpio_api.h"

class Impl {
  static int copy_data(struct archive *ar, struct archive *aw);
public:
  static CPIO_FILES show(const std::string& file);
  static void put(std::string archive_file, CPIO_FILES& files);
  static void get(std::string archive_file,CPIO_FILES& files);
};

CPIO_FILES Impl::show(const std::string& file) {
  CPIO_FILES files;

  struct archive *a;
  struct archive_entry *entry;
  int r;

  a = archive_read_new();
  archive_read_support_filter_all(a);
  archive_read_support_format_cpio(a);
  r = archive_read_open_filename(a, file.c_str(), 10240);
  if (r != ARCHIVE_OK)
    throw std::runtime_error(std::string("Can't open an archive"));
  while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
    files.push_back(archive_entry_pathname(entry));
    archive_read_data_skip(a);
  }
  r = archive_read_free(a);
  if (r != ARCHIVE_OK)
    throw std::runtime_error(std::string("Can't free an archive"));

  return files;
}

void Impl::put(std::string archive_file, CPIO_FILES& files) {
  struct archive *a;
  struct archive_entry *entry;
  struct stat st;
  char buff[8192];
  int fd;
  int len;

  a = archive_write_new();
  archive_write_add_filter_none(a);
  archive_write_set_format_cpio(a);
  auto r = archive_write_open_filename(a, archive_file.c_str());
  if (r != ARCHIVE_OK)
    throw std::runtime_error(std::string("Can't open an archive"));
  entry = archive_entry_new();
  for (auto& filename : files) {
    fd = open(filename.c_str(), O_RDONLY);
    if (fd!=-1){
      stat(filename.c_str(), &st);
      entry = archive_entry_clear(entry);
      archive_entry_set_pathname(entry, filename.c_str());
      archive_entry_set_size(entry, st.st_size);
      archive_entry_set_filetype(entry, AE_IFREG);
      archive_entry_set_perm(entry, 0644);
      archive_write_header(a, entry);
      while ((len = read(fd, buff, sizeof(buff)))>0){
          archive_write_data(a, buff, len);
      }
      close(fd);
    }
  }
  archive_entry_free(entry);
  archive_write_close(a);
  r = archive_write_free(a);
  if (r != ARCHIVE_OK)
    throw std::runtime_error(std::string("Can't free the archive"));
}

int Impl::copy_data(struct archive *ar, struct archive *aw)
{
  int r;
  const void *buff;
  size_t size;
  la_int64_t offset;

  for (;;) {
    r = archive_read_data_block(ar, &buff, &size, &offset);
    if (r == ARCHIVE_EOF)
      return (ARCHIVE_OK);
    if (r < ARCHIVE_OK)
      return (r);
    r = archive_write_data_block(aw, buff, size, offset);
    if (r < ARCHIVE_OK) {
      throw std::runtime_error("Can't write a data block. "+std::string(archive_error_string(aw)));
    }
  }
}

void Impl::get(std::string archive_file,CPIO_FILES& files) {
  struct archive *a;
  struct archive *ext;
  struct archive_entry *entry;
  int flags = ARCHIVE_EXTRACT_TIME;
  flags |= ARCHIVE_EXTRACT_PERM;
  flags |= ARCHIVE_EXTRACT_ACL;
  flags |= ARCHIVE_EXTRACT_FFLAGS;

  a = archive_read_new();
  archive_read_support_format_cpio(a);
  archive_read_support_filter_all(a);
  ext = archive_write_disk_new();
  archive_write_disk_set_options(ext, flags);
  archive_write_disk_set_standard_lookup(ext);
  int r;
  if ((r = archive_read_open_filename(a, archive_file.c_str(), 10240)))
    throw std::runtime_error(std::string("Can't open the archive"));
  for (;;) {
    r = archive_read_next_header(a, &entry);
    if (r == ARCHIVE_EOF)
      break;
    auto name = archive_entry_pathname(entry);
    if (std::find_if(files.begin(),
                     files.end(),
                     [name](std::string& fn)
                        {return std::string(name)==fn;}
                     )==files.end()
       )
      continue;
    if (r < ARCHIVE_WARN)
      throw std::runtime_error(std::string("Can't open an entry for reading. ") + archive_error_string(a));
    r = archive_write_header(ext, entry);
    if (archive_entry_size(entry) > 0) {
      r = copy_data(a, ext);
      if (r < ARCHIVE_WARN)
        throw std::runtime_error(std::string("Can't copy archive's data. ")+archive_error_string(ext));
    }
    r = archive_write_finish_entry(ext);
    if (r < ARCHIVE_WARN)
      throw std::runtime_error(std::string("Can't finish writing of an entry's data. ")+archive_error_string(ext));
  }
  archive_read_close(a);
  archive_read_free(a);
  archive_write_close(ext);
  archive_write_free(ext);
}


