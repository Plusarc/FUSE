

#include "u_fs.h"
#include "helper.h" 
#include <fcntl.h>
#include <string.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>   
#include <errno.h>

/* 
* 获得org_path路径下的文件的目录结构
*/
int helper_getDirectory(const char * org_path, u_fs_file_directory *file_dir)
{
   char *file_name = NULL;
   char *file_extention_name = NULL;
   char *file_path = NULL;
   long start_block;
   sb* super_block;
   
   u_fs_disk_block *file_content = malloc(sizeof(u_fs_disk_block));
			
   if(helper_getContent(0,file_content)==-1)
      goto err; 	      	
   super_block=(sb*)file_content;    
   start_block=super_block->first_blk;  
   if(strcmp(org_path,"/")==0)
   { 
      file_dir->flag=2;
      file_dir->nStartBlock=start_block;
      goto ok;
   }
   	
   file_path = strdup(org_path);
   file_name=file_path;	
   if (!file_name)
      goto err;		
   file_name++;	
   	
   file_extention_name = strchr(file_name, '/');		
   if( file_extention_name != NULL )
   {
      file_path ++;  
      *file_extention_name = '\0'; 
      file_extention_name ++;
      file_name=file_extention_name;      
   }
   file_extention_name = strchr(file_name, '.');
   if( file_extention_name != NULL )
   { 
      *file_extention_name = '\0'; 
      file_extention_name ++;     
   }	 
   if(helper_getContent(start_block,file_content) == -1)
      goto err;
   u_fs_file_directory *info=(u_fs_file_directory*)file_content->data;
   int position=0;	
   long next_block = file_content->nNextBlock;	
	
   if(*file_path != '/')
   { 
      while( 1 )
      {
         if( position == file_content->size )
         {
            if( next_block == -1 )
               break;
            else
            {
               if(helper_getContent(next_block,file_content) == -1)
                  goto err;
	       next_block = file_content->nNextBlock;
	       info=(u_fs_file_directory*)file_content->data;
	       position=0;
            }
         }
         if(strcmp(info->fname,file_path)==0 && info->flag==2 )
         {
            start_block=info->nStartBlock;
	    break;
	 }
	 info++;
	 position+=sizeof(u_fs_file_directory);
      }
      if(position == file_content->size)
         goto err;
      if(helper_getContent(start_block, file_content)==-1)
	 goto err;	
      info=(u_fs_file_directory*)file_content->data;	
      position=0;	
      next_block = file_content->nNextBlock;
   }
   while( 1 )
   {
      if( position == file_content->size )
      {
         if( next_block == -1 )
            break;
         else
         {
            if(helper_getContent(next_block,file_content) == -1)
	       goto err;
	    next_block = file_content->nNextBlock;
	    info=(u_fs_file_directory*)file_content->data;
	    position=0;
         }
      }
      if( info->flag !=0 && strcmp(info->fname,file_name)==0 && ( file_extention_name==NULL || strcmp(info->fext,file_extention_name)==0 ) )
      {
         strcpy(file_dir->fname, info->fname);
	 strcpy(file_dir->fext, info->fext);
	 file_dir->fsize=info->fsize;
	 file_dir->nStartBlock=info->nStartBlock;
	 file_dir->flag=info->flag;
	 goto ok;
      }
      info++;
      position+=sizeof(u_fs_file_directory);
   }	
   if(position == file_content->size)
      goto err;
ok:
   free(file_content);
   return 0;
err:
   free(file_content);
   return -1;
}

/*
* 使用块号获取该数据块的内容
*/
int helper_getContent(long block_Num,u_fs_disk_block * file_content)
{
   FILE* file;
   file=fopen(DISK_PATH, "r+");
   if(file==NULL)
   {
      return -1;
   }
   fseek(file, block_Num * BLOCK_BYTES, SEEK_SET);
   fread(file_content, sizeof(u_fs_disk_block), 1, file);
   fclose(file);
   return 0;	
}

/*
* 对修改后的file_dir写回数据块
*/
int helper_modifyContent(const char* path, u_fs_file_directory* file_dir)
{
   u_fs_file_directory *temp = malloc(sizeof(u_fs_file_directory));
   
   char *file_name = NULL;
   char *file_extention_name = NULL;
   char *file_path = NULL;
   file_path = strdup(path);
   file_name=file_path;
   file_name++;
   file_extention_name = strchr(file_name, '/'); 
   if( file_extention_name != NULL )
   {
      *file_extention_name = '\0';
      file_extention_name++;
      file_name=file_extention_name;
      if(helper_getDirectory(file_path,temp)==-1)
      {
         free(temp);
	     return -1; 
      }
   }
   else
   {
      if(helper_getDirectory("/",temp)==-1)
      {
         free(temp);
	 return -1;
      }
   }
   
   file_extention_name = strchr(file_name, '.');   
   if( file_extention_name != NULL )
   {
      *file_extention_name = '\0';
      file_extention_name++;
   }
	
   long block_Num=temp->nStartBlock;
   u_fs_disk_block *file_content = malloc(sizeof(u_fs_disk_block));
   if( helper_getContent(block_Num,file_content) == -1)
   {
      free( file_content);
      free( temp); 
      return -1;
   }
	
   u_fs_file_directory *info=(u_fs_file_directory*)file_content->data;
   int position = 0;
   long next_block = file_content->nNextBlock;
   while(1)
   {
      if( position == file_content->size )
      {
         if( next_block == -1 )
            break;
         else
         {
            if(helper_getContent(next_block,file_content) == -1)
            {
               free( file_content);
               free( temp); 
               return -1;
            }
	    next_block = file_content->nNextBlock;
	    info=(u_fs_file_directory*)file_content->data;
	    position=0;
         }
      }
      if( info->flag !=0 && strcmp(file_name,info->fname)==0 && ( file_extention_name==NULL || strcmp(file_extention_name,info->fext)==0 ) )
      {
	     strcpy(info->fname, file_dir->fname);
		 strcpy(info->fext, file_dir->fext);
		 info->fsize = file_dir->fsize;
		 info->nStartBlock = file_dir->nStartBlock;
		 info->flag = file_dir->flag;
		 break;
      }
      info++;
      position+=sizeof(u_fs_file_directory);
   }
   if( helper_write_content(block_Num, file_content) == -1)
   {
      free( file_content); 
      free( temp);
      return -1;
   }
   free( file_content);
   free( temp);
   return 0 ;
} 

/*
* 将file_content指向的数据块结构的内容写入块号为block_Num的数据块
*/
int helper_write_content(long block_Num,u_fs_disk_block * file_content)
{
   FILE* file;
   file=fopen(DISK_PATH, "r+");
   if(file==NULL)
   {
      return -1;
   }
   fseek(file, block_Num * BLOCK_BYTES, SEEK_SET);
   fwrite(file_content, sizeof(u_fs_disk_block), 1, file);
   fclose(file);
   return 0;
}

/*
* 在org_path路径下创建文件
*/
int helper_create(const char* org_path, int flag)
{
   u_fs_file_directory* file_dir=malloc(sizeof(u_fs_file_directory));
   
   char *file_name = NULL;
   char *file_extention_name = NULL;
   char *file_path = NULL;	
   file_path = strdup(org_path);
   file_name=file_path;	
   file_name++;	
   file_extention_name = strchr(file_name, '/');
   if( (flag == 1 && file_extention_name== NULL)||(flag==2 && file_extention_name!=NULL))
   {
      free( file_dir);
      return -EPERM;
   }
   else if( file_extention_name!=NULL )
   {
      *file_extention_name = '\0';
      file_extention_name++;
      file_name=file_extention_name;
      if(helper_getDirectory(file_path,file_dir)==-1)
      {
         free(file_dir);
         return -ENOENT;
      }
   }
   if(file_extention_name==NULL)
   {
      if(helper_getDirectory("/",file_dir)==-1)
      {
	 free(file_dir);
	 return -ENOENT;
      }
   }
   if(flag==1)
   {
      file_extention_name = strchr(file_name, '.');
      if( file_extention_name!=NULL )
      {
         *file_extention_name = '\0'; 
         file_extention_name++;     
      }	
   }
   if(flag==1)
   {		
      if( strlen(file_name) > MAX_FILENAME ||(file_extention_name != NULL && strlen(file_extention_name) > MAX_EXTENSION))
      {
         free(file_dir);	
         return -ENAMETOOLONG; 
      }
   }
   else if( flag==2 )
   {
      if(strlen(file_name)>MAX_FILENAME )
      {
	 free(file_dir);
	 return -ENAMETOOLONG;
      } 
   }
   long current_block;
   current_block=file_dir->nStartBlock;
   free(file_dir);	
   if(current_block == -1)
      return -ENOENT;
	  
   u_fs_disk_block *file_content=malloc(sizeof(u_fs_disk_block));
   if(helper_getContent(current_block, file_content)==-1)
   {
      free( file_content);
      return -ENOENT;
   }	
   u_fs_file_directory *info=(u_fs_file_directory*)file_content->data;
   int position=0;
   long next_block = file_content->nNextBlock;
   long free_block = 0;
   int offset = 0;
   while( 1 )
   {
      if( position == file_content->size )
      {
	 if( next_block == -1 )
	    break;
         else
         {
            if(helper_getContent(next_block,file_content) == -1)
	    {
               free( file_content);
               return -ENOENT;
            }
	    current_block = next_block;
	    next_block = file_content->nNextBlock;
	    info=(u_fs_file_directory*)file_content->data;
	    position=0;
         }
      }
      else
      {
         if( info->flag == 0)
         {
            free_block = current_block;
	    offset = position;
         }
         else if( flag==1 &&  info->flag==1 && strcmp(file_name,info->fname)==0 && ( (file_extention_name ==NULL && strlen(info->fext)==0 ) || (file_extention_name!=NULL && strcmp(file_extention_name,info->fext)==0) ) )
         {
            free( file_content);
	    return -EEXIST;
	 }
	 else if ( flag==2 && info->flag==2 && strcmp(file_name,info->fname)==0 ) 
	 {
	     free( file_content);
	     return -EEXIST;
	 }
	 info ++;
	 position += sizeof( u_fs_file_directory);
      }
   }
   long temp;      
   if(free_block == 0)
   { 
      if(file_content->size+sizeof(u_fs_file_directory) > MAX_DATA_IN_BLOCK)
      {
         temp=helper_getAndInitFreeBlock();
         file_content->nNextBlock=temp;
         if( helper_write_content(current_block,file_content) == -1)
         {
            free( file_content);
            return -1;
         }
		 
	 file_content->nNextBlock=-1;
	 file_content->size=sizeof(u_fs_file_directory);
	 info=(u_fs_file_directory*)file_content->data;
	 strcpy(info->fname,file_name);
	 if(flag ==1)
	    strcpy(info->fext,file_extention_name);
         info->fsize=0;
         info->flag=flag;
         long tem = helper_getAndInitFreeBlock();
	 info->nStartBlock=tem;	
	 if( helper_write_content(temp,file_content)== -1)
	 {
	    free(file_content);
	    return -1;
	 }
         goto exit;				
      }
      else
      {   
         file_content->size+=sizeof(u_fs_file_directory); 
      }	
   }
   else
   {
      position=0;
      if( helper_getContent(free_block, file_content)==-1)
      {
         free( file_content);
         return -1;
      }
      info=(u_fs_file_directory*)file_content->data;
      while(position<offset)
      {
         info++;
	 position+=sizeof(u_fs_file_directory);
      }
      current_block = free_block;
   }
   strcpy(info->fname,file_name);
   if(flag ==1 && file_extention_name!=NULL)
      strcpy(info->fext,file_extention_name);
   info->fsize=0;
   info->flag=flag;	
   temp=helper_getAndInitFreeBlock();
   info->nStartBlock=temp;
   helper_write_content(current_block,file_content);
exit:
   free(file_content);
   return 0;
}

/*
* 将path路径下的文件进行删除
*/
int helper_delete(const char *path,int flag)
{
   u_fs_file_directory *file_dir=malloc(sizeof(u_fs_file_directory));
   if(helper_getDirectory(path,file_dir)==-1)
   {
      free(file_dir);
      return -ENOENT;
   }
   if( flag == 1&& file_dir->flag == 2)
   {
      free(file_dir);
      return -EISDIR;
   }
   else if( flag ==2 && file_dir->flag == 1)
   {
      free(file_dir);
      return -ENOTDIR;
   }
   if( file_dir->flag == 2) 
   {
      if(helper_isEmptyDir( file_dir) == -1)
      {
         free( file_dir);
         return -ENOTEMPTY;
      }
   }
   else 
   {
      long next_block=file_dir->nStartBlock;
      helper_clearFile( next_block);
   }
   file_dir->flag=0;
   if( helper_modifyContent(path,file_dir)== -1)
   {
      free( file_dir);
      return -1;
   }
   free(file_dir);
   return 0;
} 

/*
* 检查file_dir指向的目录（已确定为目录文件）中是否为空
*/
int helper_isEmptyDir( u_fs_file_directory *file_dir)
{
   u_fs_disk_block *file_content = malloc(sizeof(u_fs_disk_block));
   if( helper_getContent(file_dir->nStartBlock,file_content)== -1 )
   {
      free( file_content);
      return -1;
   }
   long next_block = file_content->nNextBlock;
   int position = 0;
   u_fs_file_directory *info = (u_fs_file_directory *)file_content->data;
   while(1)
   {
      if( position == file_content->size )
      {
         if( next_block == -1)
            break;
         else
         {
            if( helper_getContent(next_block,file_content)== -1 )
            {
               free( file_content);
               return -1;
            }
            next_block = file_content->nNextBlock;
            position = 0;
            info = (u_fs_file_directory *)file_content->data;
         }
      }
      if( info->flag != 0 )
         break;
      info ++;
      position += sizeof( u_fs_file_directory);
   }
   if( position == file_content->size )
   {
      free(file_content);
      return 0;
   }
   else
   {
      free( file_content);
      return -1;
   }
}

/*
* 对文件所在块在位图块的位进行删除
*/
void helper_clearFile( long next_block)
{
   u_fs_disk_block *file_content = malloc( sizeof( u_fs_disk_block));
   long temp = next_block;
   while( temp != -1)
   {
      helper_setBitmap( temp,0);
      if( helper_getContent( next_block,file_content) == -1)
      {
         free( file_content);
         return;
      }
      temp = file_content->nNextBlock;
   }
   free(file_content);
   return ;
}

/*
* 对位图块中的位进行设置
*/
void helper_setBitmap(long block_Num,int flag)
{
   int zijiepianyi = block_Num/8;    /* 字节偏移 */
   int weipianyi = block_Num%8;    /* 位偏移 */
   FILE*file;
   file=fopen(DISK_PATH, "r+");
   int mask=0x80;
   fseek(file,BLOCK_BYTES+zijiepianyi,SEEK_SET);
   char content;
   char *temp = malloc( sizeof(char));
   fread(temp,sizeof(char),1,file);
   mask>>=weipianyi;
   content = *temp;
   if(flag==0)
   {
      content= content & ~mask;
   }
   else if(flag==1)
   {
      content=content | mask;
   }
   *temp = content;
   fseek(file,BLOCK_BYTES+zijiepianyi,SEEK_SET);
   fwrite(temp,sizeof(char),1,file);
   free( temp );
   fclose(file);
} 

/*
* 在位图块中找到空闲块，修改位图块对应的使用情况，并对该块的内容进行初始化操作
*/
int helper_getAndInitFreeBlock()
{
   FILE*file;
   file=fopen(DISK_PATH, "r+");
   int mask=0x80;
   fseek(file,BLOCK_BYTES+MAX_BITMAP_IN_BLOCK/8,SEEK_SET);
   char content;
   char *temp = malloc( sizeof( char));
   fread(temp,sizeof(char),1,file);
   int startDataBlock=MAX_BITMAP_IN_BLOCK;
   int position=0;
   content = *temp;
   mask >>=1;
   mask >>=1;
   position = 2; 
   while((content&mask) == mask)
   {
      mask>>=1;
      position++;
      if(mask==0)
      {
         fread(temp,sizeof(char),1,file);
         mask=0x80;
         content = *temp;
      }
   }
   helper_setBitmap(startDataBlock+position,1);
   fseek( file,(startDataBlock+position) * BLOCK_BYTES,SEEK_SET);
   u_fs_disk_block *file_content = malloc( sizeof(u_fs_disk_block));
   file_content->nNextBlock = -1;
   file_content->size = 0;
   strcpy(file_content->data,"\0");
   fwrite( file_content,sizeof(u_fs_disk_block),1,file);
   fclose(file);
   return startDataBlock+position;
}
