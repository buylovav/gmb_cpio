
#ifndef CPIO_API_H
#define CPIO_API_H

#include <deque>

using CPIO_FILES = std::deque<std::string>;

CPIO_FILES show(const std::string& file);
void put(std::string archive_file, CPIO_FILES& files);
void get(std::string archive_file,CPIO_FILES& files);


#endif /* CPIO_API_H */

