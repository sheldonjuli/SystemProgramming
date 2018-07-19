#include "library.h"

unsigned char *disk;

int main(int argc, char **argv) {

    // argument processing
    if(argc != 4) {
        fprintf(stderr, "Usage: ext2_cp <image file name> <path on native os> <absolute path on ext2 disk>\n");
        return EINVAL;
    }

    char *diskname = argv[1];
    char *dirpath_o = argv[2];
    char *dirpath_n = argv[3];

    check_absolute_path(argv[0], dirpath_n);

    // load required objects
    disk = open_disk(diskname);
    struct ext2_super_block *sb = get_super_block(disk);
    struct ext2_group_desc *gd = get_group_desc(disk);
    struct ext2_inode *inode_tbl = get_inode_tbl(disk, gd);

    int src_file = open(dirpath_o, O_RDONLY);
    if (!src_file) {
        fprintf(stderr, "ext2_cp: cannot copy file: Source file not found\n");
        return ENOENT;
    }

    // get new file file name
    // don't tokenize the actual path
    char *path_copy = (char *)malloc(sizeof(char) * strlen(dirpath_n));
    strcpy(path_copy, dirpath_n);
    char *new_file_name = get_file_name(path_copy);

    // check if the file can fit in the disk
    int src_size  = lseek(src_file, 0, SEEK_END);
    lseek(src_file, 0, SEEK_SET);

    if (sb->s_free_blocks_count < get_blocks_needed(src_size)) {
        fprintf(stderr, "ext2_cp: cannot copy file: Not enough space in disk\n");
        return ENOSPC;
    }

    // path check
    struct ext2_inode *start_inode = get_inode_at(inode_tbl, EXT2_ROOT_INO);
    struct ext2_dir_entry *start_dir = get_dir_entry_at(disk, start_inode->i_block[0]);
    int *indices = get_last_file_and_parent_inode_idx(disk, inode_tbl, start_dir, start_inode, dirpath_n, EXT2_FT_DIR);
    
    if (indices[0] == -1) {
        fprintf(stderr, "ext2_cp: cannot copy file: Path not found\n");
        return ENOENT;
    }

    if (indices[1] > 0) {
        fprintf(stderr, "ext2_cp: cannot copy file: File exists\n");
        return EEXIST;
    }

    // create new file in this directory
    int new_inode_idx = create_file(disk, sb, gd, inode_tbl, indices[0], new_file_name, src_size);

    // copy file data
    struct ext2_inode *new_inode = get_inode_at(inode_tbl, new_inode_idx);
    char buf[EXT2_BLOCK_SIZE];
    int i_block_idx = 0;
    int block_idx;
    char *block;
    int bytes_read;
    int *indirect_block;

    if (new_inode->i_block[12]) {
        indirect_block = (int *)get_block_at(disk, new_inode->i_block[12]);
    }

    while ((bytes_read = read(src_file, buf, EXT2_BLOCK_SIZE)) > 0) {

        if (i_block_idx < 11) {
            block_idx = new_inode->i_block[i_block_idx];
            i_block_idx++;
        } else {
            block_idx = *indirect_block;
            indirect_block++;
        }

        block = get_block_at(disk, block_idx);
        memcpy(block, buf, bytes_read);
    }

    close(src_file);
    free(path_copy);
    free(indices);

    return 0;
}