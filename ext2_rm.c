#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "ext2.h"
#include "ext2_utils.h"
#include <string.h>

unsigned char *disk;
struct ext2_inode *inode_table;
struct ext2_super_block *super_block;
struct ext2_group_desc *group_desc;
unsigned int *data_bitmap;
unsigned int *inode_bitmap;
//struct ext2_inode root_inode;

/*

check if its asking to delete . or .. 


*/


int end_of_path(char **path) {
	
	// get rid of leading '/'s
	while(path && **path=='/')*path = *path + 1;
	
	if(path && strlen(*path) > 0)return 0;
	
	return 1;
}

void rm_file(ext2_inode inode_table, struct ext2_dir_entry prev, struct ext2_dir_entry current, unsigned int inode_bitmap, unsigned int data_bitmap){

	prev->rec_len += current->rec_len;
	//current->inode -= 1;
	// update 
	// inode bitmap - done, inode table, and data bitmap - done

	//set the bitmaps to 0:
	// update bitmaps
	// unset a bitmap 

	inode_table->i_link_count -= 1;

	inode_bitmap[0] &= (1<<(current->inode-1));

	// for whichever blocks being used by the current inode
	data_bitmap[0] &= ~(1<<(table_inode[current->inode-1].i_block[4]));
	data_bitmap[0] &= ~(1<<(table_inode[current->inode-1].i_block[6]));
	data_bitmap[0] &= ~(1<<(table_inode[current->inode-1].i_block[7]));

	inode_table -> i_size = 0;
	inode_table -> i_mode = 0;

	return;

}


int main(int argc, char **argv) {

    if(argc != 3) {
        fprintf(stderr, "Usage: ext2_rm <image file name> <absolute path>\n");
        exit(1);
    }
    
    int fd = open(argv[1], O_RDWR);

    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    
    if(disk == MAP_FAILED) {
		perror("mmap");
		exit(1);
    }
    
    
    super_block = (struct ext2_super_block *)(disk + EXT2_BLOCK_SIZE);
    group_desc = (struct ext2_group_desc *)(disk + 2*EXT2_BLOCK_SIZE);

    unsigned int BLOCK_BITMAP_BN = group_desc->bg_block_bitmap;
    unsigned int INODE_BITMAP_BN = group_desc->bg_inode_bitmap;
    unsigned int INODE_TABLE_I = group_desc->bg_inode_table;
    
    
    data_bitmap = (unsigned int *)(disk + BLOCK_BITMAP_BN*EXT2_BLOCK_SIZE);
    inode_bitmap = (unsigned int *)(disk + INODE_BITMAP_BN*EXT2_BLOCK_SIZE);
    inode_table = (struct ext2_inode *)(disk + INODE_TABLE_I*EXT2_BLOCK_SIZE);
	

	char *path = argv[2];
    char *orig_path = strdup(path);
    
    if(*path != '/') {
        fprintf(stderr, "Usage: ext2_rm <image file name> </absolute path>\n");
        exit(1);
    }
    
    if(strlen(path) > EXT2_PATH_LEN) {
        fprintf(stderr, "rm: cannot access '%s': File name too long", orig_path);
        exit(1);
    }
    
    
	/*  
    for each filename in path, we find the corresponding inode
    and search its blocks for a dir entry with the same filename
    until we find where to ls  
    */
	
	// which inode
	unsigned int inode_num = EXT2_ROOT_INO;		
	
	// which block
	struct ext2_dir_entry *dir_entry; 

	// prev block
	struct ext2_dir_entry *prev_dir_entry;
	
	// which dir entry in a block
	unsigned block_num;
	
	// ls on root '/'
	if(end_of_path(&path)){
		do_dir_ls(inode_num);
		return 0;
	}
	// which filename in the absolute path i.e. /filename1/filename2/...
	char *filename = next_filename(&path);
	
	if(strlen(filename) > EXT2_NAME_LEN) {
		fprintf(stderr, "rm: cannot access '%s': File name too long", orig_path);
		exit(1);
	}

	
	// go through the inode's blocks
	
	for(int i_block_i=0; i_block_i <= 12; i_block_i++) {
		
		if(i_block_i == 12){
		
			// filename was not found in any of the inode's blocks.
			// mkdir the filename if it's at the end of the path
			// else bad filename in the path
			
			printf("rm: cannot access '%s': No such file or directory\n", orig_path);
			//printf("bad filename: %s \nrest of path: %s\n", path_dirname, path);
			exit(1);
		}
		
		// the next block to check for the right dir entry
		block_num = inode_table[inode_num - 1].i_block[i_block_i];
		
		// skip blocks not used by current inode
		if(block_num == 0) continue;
		
		
		
		
		// enter a block of dir entries
		
		for(int byte=0; byte < EXT2_BLOCK_SIZE; byte += dir_entry->rec_len) { 
			
			prev_dir_entry = dir_entry;

			dir_entry = (struct ext2_dir_entry *)(disk + block_num*EXT2_BLOCK_SIZE + byte);

			// if not the right filename, try the next entry
			if(!same_filename(dir_entry, filename)) 
				continue;
				
			if(dir_entry->file_type & EXT2_FT_DIR) {
			
				// the entry with the specified filename is a directory
				// go 'into' it by updating inode_num and resetting i_block_i
				inode_num = dir_entry->inode;
				i_block_i = -1; // will be incremented to 0
				
				// NEED TO CHECK THIS PART!!!!!!!!!!!!!!!!!!!!!!!
				if(!path || end_of_path(&path)){
					printf("error cannot delete a directory: \n");
					exit(0);
				}

				// done with the old filename, on to the next
				filename = next_filename(&path);
				
				if(strlen(filename) > EXT2_NAME_LEN) {
					fprintf(stderr, "rm: cannot access '%s': File name too long", orig_path);
					exit(1);
				}
				break;
			}
			// if its a file (with same name)
			if(dir_entry->file_type & EXT2_FT_REG_FILE) {
				if(!path){ // s ince its a file, it should be the end
					rm_file(inode_table, prev_dir_entry, dir_entry, inode_bitmap, data_bitmap);
				}else{
					printf("error\n");
				}
			}
		
		
			// else we have a file or link
			// the rest of the path better be blank
			// (fix -- check if allowed to have file and dir same name, if so fix)
			if(path){
				printf("rm: cannot access '%s': Not a directory\n", orig_path);
				exit(1);
			}
		
			// apparently we don't need to ls on file but here it is..
			// all good, ls on the file/link
			//printf("\nls on file or link\n");
			//printf("%.*s\n", dir_entry->name_len, dir_entry->name);
			exit(0);
		}
	}
	printf("oops, don't want to be here; should ls or error.. \n");
}