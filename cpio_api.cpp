
#include <fcntl.h>
#include <fstream>
#include <archive.h>
#include <archive_entry.h>
#include <algorithm>
#include <memory>

#include "cpio_api.h"

class Impl {
  using unique_achive = std::unique_ptr<archive, std::function<void(archive*)>>;
  using unique_entry = std::unique_ptr<archive_entry, std::function<void(archive_entry*)>>;
  static int copy_data(struct archive *ar, struct archive *aw);
public:
  static CPIO_FILES show(const std::string& file);
  static void put(std::string archive_file, CPIO_FILES& files);
  static void get(std::string archive_file,CPIO_FILES& files);
};

CPIO_FILES Impl::show(const std::string& file) {
  CPIO_FILES files;

  unique_achive a(archive_read_new(), [](archive* a){
    auto r = archive_read_free(a);
    if (r != ARCHIVE_OK)
      throw std::runtime_error(std::string("Can't free an archive"));
  });

  archive_entry *entry;

  archive_read_support_filter_all(a.get());
  archive_read_support_format_cpio(a.get());
  auto r = archive_read_open_filename(a.get(), file.c_str(), 10240);
  if (r != ARCHIVE_OK)
    throw std::runtime_error(std::string("Can't open an archive"));
  while (archive_read_next_header(a.get(), &entry) == ARCHIVE_OK) {
    files.push_back(archive_entry_pathname(entry));
    archive_read_data_skip(a.get());
  }

  return files;
}

void Impl::put(std::string archive_file, CPIO_FILES& files) {
  struct stat st;
  char buff[8192];
  int fd;
  int len;

  unique_achive a(archive_write_new(), [](archive* a){
    archive_write_close(a);
    auto r = archive_write_free(a);
    if (r != ARCHIVE_OK)
      throw std::runtime_error(std::string("Can't free the archive"));
  });
  archive_write_add_filter_none(a.get());
  archive_write_set_format_cpio(a.get());
  auto r = archive_write_open_filename(a.get(), archive_file.c_str());
  if (r != ARCHIVE_OK)
    throw std::runtime_error(std::string("Can't open an archive"));
  unique_entry entry (archive_entry_new(), &archive_entry_free);
  for (auto& filename : files) {
    fd = open(filename.c_str(), O_RDONLY);
    if (fd!=-1){
      stat(filename.c_str(), &st);
      archive_entry_clear(entry.get());
      archive_entry_set_pathname(entry.get(), filename.c_str());
      archive_entry_set_size(entry.get(), st.st_size);
      archive_entry_set_filetype(entry.get(), AE_IFREG);
      archive_entry_set_perm(entry.get(), 0644);
      archive_write_header(a.get(), entry.get());
      while ((len = read(fd, buff, sizeof(buff)))>0){
          archive_write_data(a.get(), buff, len);
      }
      close(fd);
    }
    else
        throw std::runtime_error(std::string("File is not found: ")+filename);
  } 
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
  struct archive_entry *entry;
  int flags = ARCHIVE_EXTRACT_TIME;
  flags |= ARCHIVE_EXTRACT_PERM;
  flags |= ARCHIVE_EXTRACT_ACL;
  flags |= ARCHIVE_EXTRACT_FFLAGS;

  
  unique_achive a(archive_read_new(), [](archive* a){
    archive_read_close(a);
    archive_read_free(a);
  });
  
  archive_read_support_format_cpio(a.get());
  archive_read_support_filter_all(a.get());
  unique_achive ext (archive_write_disk_new(), [](archive* a){
    archive_write_close(a);
    archive_write_free(a);
  });
  archive_write_disk_set_options(ext.get(), flags);
  archive_write_disk_set_standard_lookup(ext.get());
  int r;
  if ((r = archive_read_open_filename(a.get(), archive_file.c_str(), 10240)))
    throw std::runtime_error(std::string("Can't open the archive"));
  for (;;) {
    r = archive_read_next_header(a.get(), &entry);
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
      throw std::runtime_error(std::string("Can't open an entry for reading. ") + archive_error_string(a.get()));
    r = archive_write_header(ext.get(), entry);
    if (archive_entry_size(entry) > 0) {
      r = copy_data(a.get(), ext.get());
      if (r < ARCHIVE_WARN)
        throw std::runtime_error(std::string("Can't copy archive's data. ")+archive_error_string(ext.get()));
    }
    r = archive_write_finish_entry(ext.get());
    if (r < ARCHIVE_WARN)
      throw std::runtime_error(std::string("Can't finish writing of an entry's data. ")+archive_error_string(ext.get()));
  }
}

//API

CPIO_FILES show(const std::string& file)
{
  return Impl::show(file);
}

void put(std::string archive_file, CPIO_FILES& files)
{
  Impl::put(archive_file, files);
}

void get(std::string archive_file,CPIO_FILES& files)
{
  Impl::get(archive_file, files);
}

