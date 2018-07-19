#include "library.h"

unsigned char *disk;

int main(int argc, char **argv) {

    // argument processing
    if (argc != 4 && argc != 5) {
        fprintf(stderr, "Usage: ext2_ln <image file name> [-s] <absolute path to directory or file> <absolute path to link>\n");
        return EINVAL;
    }

    int sl_flag = 0;
    char *diskname = argv[1];
    char *dirpath_file;
    char *dirpath_link;

    if (argc == 5) {
        if (strcmp(argv[2], "-s") != 0) {
            fprintf(stderr, "Usage: ext2_ln <image file name> [-s] <absolute path to directory or file> <absolute path to link>\n");
            return EINVAL;
        }
        sl_flag = 1;
        dirpath_file = argv[3];
        dirpath_link = argv[4];
    } else {
        dirpath_file = argv[2];
        dirpath_link = argv[3];
    }

    check_absolute_path(argv[0], dirpath_file);
    check_absolute_path(argv[0], dirpath_link);

    // load required objects
    disk = open_disk(diskname);
    struct ext2_super_block *sb = get_super_block(disk);
    struct ext2_group_desc *gd = get_group_desc(disk);
    struct ext2_inode *inode_tbl = get_inode_tbl(disk, gd);

    // find the inode index of both file and link
    struct ext2_inode *start_inode = get_inode_at(inode_tbl, EXT2_ROOT_INO);
    struct ext2_dir_entry *start_dir = get_dir_entry_at(disk, start_inode->i_block[0]);
    int *indices_file = get_last_file_and_parent_inode_idx(disk, inode_tbl, start_dir, start_inode, dirpath_file, EXT2_FT_UNKNOWN);
    int *indices_link = get_last_file_and_parent_inode_idx(disk, inode_tbl, start_dir, start_inode, dirpath_link, EXT2_FT_UNKNOWN);

    if (indices_file[0] == -1 || indices_file[1] == -1 || indices_link[0] == -1) {
        fprintf(stderr, "ext2_ln: cannot create link: Path not found\n");
        return ENOENT;
    }

    if (indices_link[1] > 0) {
        fprintf(stderr, "ext2_ln: cannot create link: File exists\n");
        return EEXIST;
    }

    // if directory, must use -s flag
    struct ext2_inode *file_inode = get_inode_at(inode_tbl, indices_file[1]);
    if ((I_MODE_FILE_TYPE(file_inode->i_mode) == I_MODE_FILE_TYPE(EXT2_S_IFDIR)) && !sl_flag) {
        fprintf(stderr, "ext2_ln: cannot create link: Hard link not allowed for directory\n");
        return EISDIR;
    }

    // don't tokenize the actual path
    char *path_copy = (char *)malloc(sizeof(char) * strlen(dirpath_link));
    strcpy(path_copy, dirpath_link);
    char *link_name = get_file_name(path_copy);

    if (sl_flag) {
        create_symbolic_link(disk, sb, gd, inode_tbl, indices_link[0], link_name, dirpath_file);
        // TODO
    } else {
        // hard link has the type of a regular file
        insert_into_dir(disk, sb, gd, inode_tbl, indices_link[0], indices_file[1], link_name, EXT2_FT_REG_FILE);
        file_inode->i_links_count++;
    }

    free(path_copy);
    free(indices_file);
    free(indices_link);

    return 0;
}