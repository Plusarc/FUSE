
#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <malloc.h>

#include "u_fs.h"
#include "helper.h"

/* 
* 检查输入的路径是一个目录还是一个普通文件。
*/
static int u_fs_getattr(const char *path, struct stat *stbuf)
{  
   memset(stbuf,0,sizeof(struct stat));
   u_fs_file_directory *file_dir = malloc( sizeof( u_fs_file_directory ));   
   /* 为file_dir分配内存空间 */
	
   if( helper_getDirectory( path, file_dir ) == -1 )   
   	 /* 使用路径path获取该路径下的文件信息 */
   {
      free(file_dir);    
      /*diposit the room of u_fs_file_directry*/
      return -ENOENT;
   }
   if( file_dir->flag == 2 )
   {
      stbuf->st_mode = S_IFDIR | 0666;    
      /* 设置stbuf的文件模式为目录 */
   }
   else if( file_dir->flag == 1 )
   {
      stbuf->st_mode = S_IFREG | 0666;    
      /* 设置stbuf的文件模式为文件 */
      stbuf->st_size = file_dir->fsize; 
   }

   free(file_dir);
   return 0;    /* 将函数执行的结果返回 */
}

/* 
* 这个函数首先检查输入的路径，如果是一个目录，
*/
static int u_fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,off_t offset, struct fuse_file_info *fi)
{  
   u_fs_disk_block *file_content=malloc(sizeof(u_fs_disk_block));
   u_fs_file_directory *file_dir=malloc(sizeof(u_fs_file_directory));
	
   if(helper_getDirectory(path,file_dir)==-1)
   {		
      free(file_dir);
      free(file_content);
      return -ENOENT;
   }
   long block_Num=file_dir->nStartBlock;
   if(file_dir->flag==1)
   {
      free(file_dir);
      free(file_content);
      return -ENOENT;
   }    	
   if( helper_getContent(block_Num, file_content)== -1 )
   {
      free( file_dir); 
      free( file_content);
      return -ENOENT;
   }	
   filler(buf, ".", NULL, 0);
   filler(buf, "..", NULL, 0);

   int position=0;
   char file_name[MAX_FILENAME + MAX_EXTENSION + 2];
   u_fs_file_directory *info=(u_fs_file_directory*)file_content->data;	
   long next_Block = file_content->nNextBlock;
   while(1)
   {
      if( position >= file_content->size )
      {
         if( next_Block == -1 )
         {
            break;
         }
         else
         {
            if( helper_getContent(block_Num, file_content)== -1 )
            {
               free( file_dir);
	           free( file_content);
	           return -ENOENT;
	        }
		    position=0;
            info=(u_fs_file_directory*)file_content->data;
            next_Block = file_content->nNextBlock;
         }
      }
      strcpy(file_name,info->fname);
      if(strlen(info->fext)!=0)
      {
         strcat(file_name,".");
         strcat(file_name,info->fext);
      }		
      if (info->flag !=0 && filler(buf, file_name, NULL, 0))
	 break;
      info++;
      position+=sizeof(u_fs_file_directory);
   }

   free(file_dir);
   free(file_content);
   return 0;
}

/*
*添加新的目录文件到根目录下，而且恰当地更新目录文件。
*/
static int u_fs_mkdir(const char *path, mode_t mode)
{
   int res=helper_create(path, 2);
   return res;
}

/*
*删除一个空的目录。
*/
static int u_fs_rmdir(const char *path)
{
   int res=helper_delete(path,2);
   return res;
}

/*
*添加一个新的文件到一个子目录中，并适当地修改目录条目结构和更新目录文件。
*/
static int u_fs_mknod(const char *path, mode_t mode, dev_t rdev)
{
   int res=helper_create(path, 1);
   return res;
}

/*
* 这个函数将buf中的数据写入路径指示的文件，在offset指定的位置开始写起。
*/
static int u_fs_write(const char *path, const char *buf, size_t size,off_t offset, struct fuse_file_info *fi)
{
   u_fs_file_directory* temp = malloc(sizeof(u_fs_file_directory));
   if( helper_getDirectory(path,temp) == -1 )
   {
      free( temp );
      return -ENOENT;
   }
   if(temp->flag==2)
   {
      free( temp);
      return -1;
   }
   if(offset>temp->fsize)
   {
      free( temp);
      return -1;
   }
	
   u_fs_disk_block* file_content = malloc(sizeof(u_fs_disk_block));	
   long write_offset = offset;
   long current_block = temp->nStartBlock;
	
   while(1)
   {
      if( helper_getContent(current_block, file_content)== -1 )
      {
         free( temp);
         free( file_content);
         return -1;
      }
      if(write_offset <= file_content->size)
         break;		
      write_offset -= file_content->size;
      current_block = file_content->nNextBlock;
   }
   char* dat = file_content->data;
   dat += write_offset;
   int length=0;
   int total=0;
   length = (MAX_BITMAP_IN_BLOCK - write_offset < size ? MAX_BITMAP_IN_BLOCK -write_offset : size);
   strncpy(dat,buf,length);
   buf += length;
   file_content->size = write_offset + length;
   size -= length;
   if( size > 0)
   {
      long clear_block = file_content->nNextBlock;
      helper_clearFile( clear_block);
      long block_Num = helper_getAndInitFreeBlock();
      file_content->nNextBlock = block_Num;
      if( helper_write_content( current_block, file_content)== -1)
      {
         free( temp);
         free( file_content);
         return -1;
      }
      total += length;
      while( size > 0 )
      {
         length = (MAX_BITMAP_IN_BLOCK < size ? MAX_BITMAP_IN_BLOCK : size);
	 current_block = block_Num;
	 file_content->size = length;
	 strncpy(file_content->data,buf,length);
	 buf += length;
	 size -= length;
	 if( size == 0)
	 {
	    file_content->nNextBlock = -1;
	 }
	 else
	 { 
	    block_Num = helper_getAndInitFreeBlock();
	    file_content->nNextBlock = block_Num;
	 }
	 if( helper_write_content( current_block, file_content)== -1)
	 {
	    free( temp);
	    free( file_content);
	    return -1;
	 }
	 total += length;
      }
   }
   else if( size== 0)
   {
      long clear_block = file_content->nNextBlock;
      helper_clearFile( clear_block);
      file_content->nNextBlock = -1;
      if( helper_write_content( current_block, file_content)== -1)
      {
         free( temp);
         free( file_content);
         return -1;
      }
      total += length;
   }
	
   size = total;
   temp->fsize = offset + total;
   if( helper_modifyContent(path,temp)== -1 )
   {
      free(file_content);
      free(temp);
      return size;
   }
   free(file_content);
   free(temp);
   return size;
}

/*
* 这个函数从offset指定的位置开始读取路径指示的文件。
*/
static int u_fs_read(const char *path, char *buf, size_t size,off_t offset, struct fuse_file_info *fi)
{ 
   u_fs_file_directory *file_dir = malloc(sizeof(u_fs_file_directory));
   long read_offset = offset;
   if( helper_getDirectory(path,file_dir) == -1 )
   {
      free( file_dir);
      return -ENOENT;
   }
   if(file_dir->flag==2)
   {
      free(file_dir);
      return -1;
   }
   
   u_fs_disk_block *file_content = malloc(sizeof(u_fs_disk_block));
   if( helper_getContent(file_dir->nStartBlock,file_content) == -1)
   {
      free( file_dir);
      free( file_content);
      return -1;
   }
   if(offset < file_dir->fsize)
   {
      if( offset + size > file_dir->fsize )
      {
         size = file_dir->fsize-offset;
      }
   }
   else
   {   
      size = 0;
   }
   while( read_offset > file_content->size )
   {
      read_offset -= file_content->size;
      long temp = file_content->nNextBlock;
      if( helper_getContent(temp,file_content) == -1)
      {
          free( file_dir);
	  free( file_content);
	  return -1;
      }
   }
   int read_size = size;
   char* dat=file_content->data;
   dat += read_offset;
   strcpy(buf,dat);
   read_size-=file_content->size-offset;
   while(read_size>0)
   {
      if( helper_getContent(file_content->nNextBlock,file_content) == -1 )
      {
         free( file_dir);
         free( file_content);
         return -1;
      }
      strcat(buf,file_content->data);
      read_size-=file_content->size;
   }
   free( file_content);
   free( file_dir);
   return size;
}

/*
* 删除一个文件
*/
static int u_fs_unlink(const char *path)
{
   int res=helper_delete(path,1);
   return res;
}

/*
* 不需要修改
*/
static int u_fs_open(const char *path, struct fuse_file_info *fi)
{
   u_fs_file_directory *file_dir = malloc( sizeof( u_fs_file_directory ));
   if( helper_getDirectory( path, file_dir ) != 0 )
   {
      free( file_dir );
      return -errno;
   }
   free( file_dir );
   return 0;
}

/*
* 此函数不需要被修改
*/
static int u_fs_flush(const char *path, struct fuse_file_info *fi)
{
   return 0;
}

/*
* 此函数不需要被修改
*/
static int u_fs_truncate(const char *path, off_t size)
{
   return 0;
}

/* 
* 定义u_fs文件系统中需要的文件操作函数 
*/
static struct fuse_operations u_fs_oper = {
   .getattr    = u_fs_getattr,
   .readdir    = u_fs_readdir,
   .open       = u_fs_open,
   .read       = u_fs_read,
   .mkdir      = u_fs_mkdir,
   .rmdir      = u_fs_rmdir,
   .mknod      = u_fs_mknod,
   .write      = u_fs_write,
   .unlink     = u_fs_unlink,
   .flush      = u_fs_flush,
   .truncate   = u_fs_truncate,
};

/* 
* u_fs文件系统的主函数 
*/
int main(int argc, char *argv[])
{
   return fuse_main(argc, argv, &u_fs_oper, NULL);
}
