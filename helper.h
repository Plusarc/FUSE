#ifndef HELPER_H

#define HELPER_H
#include "u_fs.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

int helper_getAndInitFreeBlock();

void helper_setBitmap(long block_Num,int flag);

int helper_isEmptyDir( u_fs_file_directory *file_dir);

void helper_clearFile( long next_block);

int helper_getContent(long block_Num,u_fs_disk_block * blkfile_content);

int helper_write_content(long block_Num,u_fs_disk_block * blkfile_content);

int helper_getDirectory(const char * org_path, u_fs_file_directory *file_dir);

int helper_modifyContent(const char* path, u_fs_file_directory* file_dir);

int helper_delete(const char *path,int flag);

int helper_create(const char* org_path, int flag);

#endif
