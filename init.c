
#include<stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "u_fs.h"

int main(void)
{
   FILE * file=NULL;

	file=fopen(DISK_PATH, "r+");
	if(file == NULL) {
		fprintf(stderr,"Open DISK_PATH unsuccessful!\n");
		return 0;
	}
	fseek(file, 0, SEEK_END);
	sb *super_block_record=malloc(sizeof(sb));	
	super_block_record->fs_size = ftell(file)/BLOCK_BYTES;	
	super_block_record->first_blk = 1 + MAX_BITMAP_IN_BLOCK;
	super_block_record->bitmap = MAX_BITMAP_IN_BLOCK;

	if(fseek(file, 0, SEEK_SET )!=0)
		fprintf(stderr,"Something wrong happened!\n");	
	fwrite(super_block_record, sizeof(sb), 1, file);
	if(fseek(file, 512, SEEK_SET )!=0)
		fprintf(stderr,"Something wrong happened!\n");	
		
    /* 设置1282个位，其中1个为超级块，
    *1280个为位图块，
    *1个为根目录第一个块 */
	char a[160];   
	memset(a,-1,160); 
	fwrite(a, 160, 1, file);
	char temp=0xC0;    
	/* OxC0的二进制为11000000 */
	fwrite(&temp, sizeof(char), 1, file);
	
	/* 设置位图块中第2个到第1280个块 */
	char b[351];   
	memset(b,0,351);  
	fwrite(b,351,1,file);

	int total = (MAX_BITMAP_IN_BLOCK-1)*BLOCK_BYTES;
	char rest[total];
	memset(rest, 0, total);
	fwrite(rest, total, 1, file);

	/* 初始化根目录所在的第一块 */
    fseek(file, BLOCK_BYTES * (MAX_BITMAP_IN_BLOCK+1), SEEK_SET);
    u_fs_disk_block *first_root=malloc(sizeof(u_fs_disk_block));
    first_root->size= 0;
    first_root->nNextBlock=-1;
    first_root->data[0]='\0';
    fwrite(first_root, sizeof(u_fs_disk_block), 1, file);
    free( first_root );	
    free( super_block_record );	
    fclose(file);
    printf("Initializing the diskimg is done!\n");
    return 0;
}
