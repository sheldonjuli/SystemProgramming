#include "library.h"

unsigned char *disk;

int main(int argc, char **argv) {

    // argument processing
    if(argc != 3) {
        fprintf(stderr, "Usage: ext2_mkdir <image file name> <absolute path on ext2 disk>\n");
        return EINVAL;
    }

    char *diskname = argv[1];
    char *dirpath = argv[2];

    check_absolute_path(argv[0], dirpath);

    // load required objects
    disk = open_disk(diskname);
    struct ext2_super_block *sb = get_super_block(disk);
    struct ext2_group_desc *gd = get_group_desc(disk);
    struct ext2_inode *inode_tbl = get_inode_tbl(disk, gd);

    // search the target and its parent
    struct ext2_inode *start_inode = get_inode_at(inode_tbl, EXT2_ROOT_INO);
    struct ext2_dir_entry *start_dir = get_dir_entry_at(disk, start_inode->i_block[0]);
    int *indices = get_last_file_and_parent_inode_idx(disk, inode_tbl, start_dir, start_inode, dirpath, EXT2_FT_UNKNOWN);

    // error checking
    if (indices[0] == -1) {
        fprintf(stderr, "ext2_mkdir: cannot create directory: Path not found\n");
        return ENOENT;
    }
    if (indices[1] > 0) {
        fprintf(stderr, "ext2_mkdir: cannot create directory: Directory or file exists\n");
        return EEXIST;
    }

    // don't tokenize the actual path
    char *path_copy = (char *)malloc(sizeof(char) * strlen(dirpath));
    strcpy(path_copy, dirpath);
    char *new_name = get_file_name(path_copy);
    if (strlen(new_name) > EXT2_NAME_LEN) {
        fprintf(stderr, "ext2_mkdir: cannot create directory: Name too long\n");
        return ENAMETOOLONG;
    }

    // mkdir
    create_dir(disk, sb, gd, inode_tbl, indices[0], new_name);

    free(indices);
    free(path_copy);

    return 0;
}
