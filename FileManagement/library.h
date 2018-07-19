#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include "ext2.h"

// A disk is 128 blocks where the block size is 1024 bytes.
#define NUM_BLOCKS 128

// only the left most 4 digits define the type
#define I_MODE_FILE_TYPE(I_MODE) ((I_MODE >> 12))

// only the lower 3 bits are useful
#define FILE_TYPE_LOWER_3(FILE_TYPE) ((FILE_TYPE & 0x7))

/*
 * Open the disk given diskname.
 */
unsigned char *open_disk(char *diskname);

/*
 * Get super block of the disk
 */
struct ext2_super_block *get_super_block(unsigned char *disk);

/*
 * Get group descriptor of the disk
 */
struct ext2_group_desc *get_group_desc(unsigned char *disk);

/*
 * Get inode bitmap of the disk
 */
unsigned char *get_inode_bitmap(unsigned char *disk, struct ext2_group_desc *gd);

/*
 * Get block bitmap of the disk
 */
unsigned char *get_block_bitmap(unsigned char *disk, struct ext2_group_desc *gd);

/*
 * Get inode table of the disk
 */
struct ext2_inode *get_inode_tbl(unsigned char *disk, struct ext2_group_desc *gd);

/*
 * Get inode given inode index
 * Index starts at 1
 */
struct ext2_inode *get_inode_at(struct ext2_inode *inode_tbl, int idx);

/*
 * Get block given block index
 */
char *get_block_at(unsigned char *disk, int idx);

/*
 * Get inode bit value of inode index
 * Index starts at 1
 */
int get_inode_bit_val_at(unsigned char *disk, struct ext2_group_desc *gd, int idx);

/*
 * Get block bit value of block index
 */
int get_block_bit_val_at(unsigned char *disk, struct ext2_group_desc *gd, int idx);

/*
 * Get the directory entry at given block
 */
struct ext2_dir_entry *get_dir_entry_at(unsigned char *disk, int block_idx);

/*
 * Check if the given path is an absolute path.
 * Exit with EINVAL if not
 */
void check_absolute_path(char *method_name, char *dirpath);

/*
 * Check filename operable
 */
int check_filename_operable(char *file_name);

/*
 * Get currect time in ms
 */
unsigned int get_time();

/*
 * Convert i_mode of type unsigned short to file_type of unsigned char
 */
unsigned char i_mode_to_file_type(unsigned short i_mode);

/*
 * Convert i_mode of type unsigned short to a printable char
 */
char i_mode_to_char(unsigned short i_mode);

/*
 * Convert file_type of unsigned char to a printable char
 */
char file_type_to_char(unsigned char file_type);

/*
 * Return the last file/dir name
 * This method will tokenize the original string, pass in a copy
 */
char *get_file_name(char *path);

/*
 * Compute the rec_len given name_len
 */
int compute_rec_len(int name_len);

/*
 * Compute the number of blocks needed for a given file size
 */
int get_blocks_needed(int file_size);

/*
 * Print out fields in super block for debugging.
 */
void print_super_block(struct ext2_super_block *sb);

/*
 * Print out fields in group descriptor for debugging.
 */
void print_group_desc(unsigned char *disk, struct ext2_super_block *sb, struct ext2_group_desc *gd);

/*
 * Print out an inode at given index for debugging
 */
void print_inodes(struct ext2_inode *inode_table, unsigned int inode_index);

/*
 * Print all the directory entries in a given directory
 */
void print_dir_entry(unsigned char *disk, struct ext2_dir_entry *dir_entry);

/*
 * Counts the number of free inodes in bitmap
 */
unsigned int count_free_inode_in_bitmap(unsigned char *disk, struct ext2_group_desc *gd);

/*
 * Counts the number of free inodes in bitmap
 */
unsigned int count_free_block_in_bitmap(unsigned char *disk, struct ext2_group_desc *gd);

/*
 * Return the index of the first free inode.
 * Index starts at 1.
 * Assume there is at least 1 free inode.
 */
int find_free_inode_idx(unsigned char *disk, struct ext2_group_desc *gd);

/*
 * Update the bit value at given index in inode_bitmap.
 * Index starts at 1.
 */
void update_inode_bitmap(unsigned char *disk, struct ext2_group_desc *gd, int inode_idx, int bit_value);

/*
 * Return the index of the first free block.
 * Index starts at 0.
 * Assume there is at least 1 free block.
 */
int find_free_block_idx(unsigned char *disk, struct ext2_group_desc *gd);

/*
 * Update the bit value at given index in block_bitmap.
 * Index starts at 1.
 */
void update_block_bitmap(unsigned char *disk, struct ext2_group_desc *gd, int block_idx, int bit_val);

/*
 * Find the inode index of the directory/file with given target_name in the current directory curr_dir
 * Returns -1 if no such directory/file exits.
 */
int get_file_inode_idx(struct ext2_dir_entry *curr_dir, struct ext2_inode *curr_inode, char *target_name, unsigned char target_type);

/*
 * Initialize a directory block
 */
void init_dir_block(struct ext2_inode *entry_inode, int new_inode_idx, struct ext2_dir_entry *new_dir);

/*
 * Create a new inode, initialize all properties
 */
int create_inode(unsigned char *disk, struct ext2_super_block *sb, struct ext2_group_desc *gd, struct ext2_inode *inode_tbl, unsigned short mode, unsigned int size);

/*
 * Insert the given dir/file name into the given parent directory
 */
void insert_into_dir(unsigned char *disk, struct ext2_super_block *sb, struct ext2_group_desc *gd, struct ext2_inode *inode_tbl, int parent_inode_idx, int new_inode_idx, char *new_name, char type);

/*
 * Remove the given dir/file name into the given parent directory
 */
void remove_from_dir(unsigned char *disk, struct ext2_group_desc *gd, struct ext2_inode *inode_tbl, int parent_inode_idx, char *target_name);

/*
 * Create a regular file in the given parent directory with given name
 * Return the new inode index
 */
int create_file(unsigned char *disk, struct ext2_super_block *sb, struct ext2_group_desc *gd, struct ext2_inode *inode_tbl, int parent_inode_idx, char *new_name, unsigned int size);

/*
 * Create a new directory in the given parent directory
 * Return the new inode index
 */
int create_dir(unsigned char *disk, struct ext2_super_block *sb, struct ext2_group_desc *gd, struct ext2_inode *inode_tbl, int parent_inode_idx, char *new_name);

/*
 * Create a symbolic(soft) link in the given parent directory with given name
 * Return the new inode index
 */
int create_symbolic_link(unsigned char *disk, struct ext2_super_block *sb, struct ext2_group_desc *gd, struct ext2_inode *inode_tbl, int parent_inode_idx, char *new_name, char *dirpath_file);

/*
 * Find the inode index of the last file/directory in the path and its parent
 * Return the [parent_idx, idx] if target found
 * Return [parent_idx, -1] if path found but target not found
 * Return [-1, -1] if path not found
 */
int *get_last_file_and_parent_inode_idx(unsigned char *disk, struct ext2_inode *inode_tbl, struct ext2_dir_entry *start_dir, struct ext2_inode *start_inode, char *path, unsigned char target_type);

/*
 * Update free inode/block count in superblock
 */
int update_sb_free_count(unsigned int *target, unsigned int val, char *target_data_name, int print);

/*
 * Update free inode/block count in group descriptor
 */
int update_gd_free_count(unsigned short *target, unsigned short val, char *target_data_name, int print);

/*
 * Recursively remove a directory/file
 */
void recursively_remove(unsigned char *disk, struct ext2_group_desc *gd, struct ext2_inode *inode_tbl, int parent_inode_idx, int target_inode_idx, char *target_name);

/*
 * Recursively remove a directory/file
 */
int recursively_restore(unsigned char *disk, struct ext2_group_desc *gd, struct ext2_inode *inode_tbl, int parent_inode_idx, char *target_name, int r_flag) ;

/*
 * Recursively fix inconsistencies
 */
int iterate_and_fix_files(unsigned char *disk, struct ext2_group_desc *gd, struct ext2_inode *inode_tbl, int start_inode_idx);