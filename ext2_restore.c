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
struct ext2_super_block *super_block;
struct ext2_group_desc *group_desc;
unsigned int *data_bitmap;
unsigned int *inode_bitmap;
struct ext2_inode *inode_table;


/*
Few cases:
	1. the file is already there (no restore)
	2. file isn't there, restore the file

*/
void restore_function(unsigned int inode_num, char filename, char path){


	unsigned block_num;
	struct ext2_dir_entry *dir_entry;
	struct ext2_dir_entry *prev; 

	for(int i_block_i=0; i_block_i <=13; i_block_i ++){
		if(i_block_i == 13){
			////
			printf("restore: cannot access '%s': No such file or directory\n", orig_path);
			//printf("bad filename: %s \nrest of path: %s\n", path_dirname, path);
			exit(1);
		}

		block_num = inode_table[inode_num - 1].i_block[i_block_i];
		if(block_num == 0) continue;

		unsigned short actual_reclen = (sizeof (struct ext2_dir_entry) + dir_entry->name_len);

		//going through all the files to find out whether any of the files are the file we want to restore
		for(int byte=0; byte < EXT2_BLOCK_SIZE; byte += actual_reclen) { 

			prev_dir_entry = dir_entry;

			dir_entry = (struct ext2_dir_entry *)(disk + block_num*EXT2_BLOCK_SIZE + byte);

			// if we find the entry to have the same name as filename (from path), then it's the file we want to restore
			if(!same_filename(dir_entry, filename)) 
				continue;

			if(dir_entry->file_type & EXT2_FT_REG_FILE) {
				prev_dir_entry -> rec_len = byte;
				// fix current entry (bitmap flipping stuff)

				// then go onto the next directory entry
			}

		}

	}
}

int main(int argc, char **argv){
	unsigned char *disk;
	if(argc != 3) {
        fprintf(stderr, "Usage: %s <image file name> <absolute path of directory>\n", argv[0]);
        exit(-1);
    }

    int fd = open(argv[1], O_RDWR);

    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
        perror("mmap");
        exit(1);
	}

	char *path = argv[2];

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
    int len = strlen(argv[2]);

    if(argv[2][len] == '/'){
    	perror("restore: cannot restore a directory");
    }
    
    if(*path != '/') {
        fprintf(stderr, "Usage: ext2_rm <image file name> </absolute path>\n");
        exit(1);
    }
    
    if(strlen(path) > EXT2_PATH_LEN) {
        fprintf(stderr, "restore: cannot access '%s': File name too long", orig_path);
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
	//if(end_of_path(&path)){
	//	do_dir_ls(inode_num);
	//	return 0;
	//}
	// which filename in the absolute path i.e. /filename1/filename2/...
	char *filename = next_filename(&path);
	
	if(strlen(filename) > EXT2_NAME_LEN) {
		fprintf(stderr, "restore: cannot access '%s': File name too long", orig_path);
		exit(1);
	}

	
	// go through the inode's blocks
	
	for(int i_block_i=0; i_block_i <= 13; i_block_i++) {
		
		if(i_block_i == 13){
		
			// filename was not found in any of the inode's blocks.
			// mkdir the filename if it's at the end of the path
			// else bad filename in the path
			
			printf("restore: cannot access '%s': No such file or directory\n", orig_path);
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
			// only go through if the file has the same name
				
			if(dir_entry->file_type & EXT2_FT_DIR) {
			
				// the entry with the specified filename is a directory
				// go 'into' it by updating inode_num and resetting i_block_i
				inode_num = dir_entry->inode;
				i_block_i = -1; // will be incremented to 0
				
				// at the end
				// NEED TO CHECK THIS PART!!!!!!!!!!!!!!!!!!!!!!!
				if(!path || end_of_path(&path)){
					printf("no file to restore, reached end of path: \n");
					exit(0);
				}

				// done with the old filename, on to the next
				filename = next_filename(&path);
				
				if(strlen(filename) > EXT2_NAME_LEN) {
					fprintf(stderr, "restore: cannot access '%s': File name too long", orig_path);
					exit(1);
				}
				// check if path left
				// if end of path
				// 1. see if filename exists
				// 2. if not, try to restore it
				if(!path){
					// WE ARE NOW AT THE LAST DIRECTORY and the path had ended, meaning we are approaching the last file
					restore();
				}
	
				break;
			}
			// if its a file (with same name)
			if(dir_entry->file_type & EXT2_FT_REG_FILE) {
				//if (path){
				//	printf("error\n");
				//}
				printf("error\n");
			}
		
			// (fix -- check if allowed to have file and dir same name, if so fix)
			if(path){
				printf("restore: cannot access '%s': Not a directory\n", orig_path);
				exit(1);
			}
		
			exit(0);
		}
	}
	printf("oops, don't want to be here; should ls or error.. \n");
}