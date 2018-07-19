#include "library.h"

unsigned char *disk;

int main(int argc, char **argv) {

    // argument processing
    if (argc != 3) {
        fprintf(stderr, "Usage: ext2_rm <image file name> <absolute path on ext2 disk>\n");
        return EINVAL;
    }

    int r_flag = 0;
    char *diskname = argv[1];
    char *dirpath = argv[2];

    check_absolute_path(argv[0], dirpath);

    // get target file file name
    // don't tokenize the actual path
    char *path_copy = (char *)malloc(sizeof(char) * strlen(dirpath));
    strcpy(path_copy, dirpath);
    char *target_file_name = get_file_name(path_copy);
    if (!target_file_name || !check_filename_operable(target_file_name)) {
        fprintf(stderr, "ext2_rm: cannot remove a directory\n");
        return EISDIR;
    }

    // load required objects
    disk = open_disk(diskname);
    struct ext2_group_desc *gd = get_group_desc(disk);
    struct ext2_inode *inode_tbl = get_inode_tbl(disk, gd);

    // find the inode index of both file and link
    struct ext2_inode *start_inode = get_inode_at(inode_tbl, EXT2_ROOT_INO);
    struct ext2_dir_entry *start_dir = get_dir_entry_at(disk, start_inode->i_block[0]);
    int *indices = get_last_file_and_parent_inode_idx(disk, inode_tbl, start_dir, start_inode, dirpath, EXT2_FT_UNKNOWN);

    if (indices[1] == -1) {
        fprintf(stderr, "ext2_rm: cannot remove: Directory or file not found\n");
        return ENOENT;
    }

    // if directory, must use -r flag
    struct ext2_inode *target_inode = get_inode_at(inode_tbl, indices[1]);
    if ((I_MODE_FILE_TYPE(target_inode->i_mode) == I_MODE_FILE_TYPE(EXT2_S_IFDIR)) && !r_flag) {
        fprintf(stderr, "ext2_rm: cannot remove: Target is a directory\n");
        return EISDIR;
    }

    recursively_remove(disk, gd, inode_tbl, indices[0], indices[1], target_file_name);

    free(indices);
    free(path_copy);

    return 0;
}