/*************************************************************************//**
 *****************************************************************************
 * @file   fs/open.c
 * The file contains:
 *   - do_open()
 *   - do_close()
 *   - do_lseek()
 *   - create_file()
 * @author Forrest Yu
 * @date   2007
 *****************************************************************************
 *****************************************************************************/

#include "type.h"
#include "stdio.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "fs.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "keyboard.h"
#include "proto.h"


/*****************************************************************************
 *                                do_open
 *****************************************************************************/
/**
 * Open a file and return the file descriptor.
 * 
 * @return File descriptor if successful, otherwise a negative error code.
 *****************************************************************************/
PUBLIC int do_lseek()
{
	int fd = -1;		

	char pathname[MAX_PATH];

	
	int name_len = fs_msg.NAME_LEN;	
	int src = fs_msg.source;	
	assert(name_len < MAX_PATH);
	phys_copy((void*)va2la(TASK_FS, pathname),
		  (void*)va2la(src, fs_msg.PATHNAME),
		  name_len);
	pathname[name_len] = 0;
        char filename[MAX_PATH];
        struct inode* dir_inode;
        memset(filename,0,MAX_PATH);

	/*int i = strip_path(filename,pathname,&dir_inode);
        if(i!=0){
           int dir_blk0_nr = dir_inode->i_start_sect;
	   int nr_dir_blks = (dir_inode->i_size + SECTOR_SIZE - 1)/SECTOR_SIZE;
	   int nr_dir_entries = dir_inode->i_size / DIR_ENTRY_SIZE; 
	   int m = 0;
           int j;
	   struct dir_entry * pde;
	   for (i = 0; i < nr_dir_blks; i++) {
		RD_SECT(dir_inode->i_dev, dir_blk0_nr + i);
		pde = (struct dir_entry *)fsbuf;
		for (j = 0; j < SECTOR_SIZE / DIR_ENTRY_SIZE; j++,pde++) {
			printf("%s ",pde->name);
		    }
		
	       }
	   printf("\n");
           }
*/
	return 0;
}

