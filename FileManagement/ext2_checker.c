#include "library.h"

unsigned char *disk;

int main(int argc, char **argv) {

    // argument processing
    if(argc != 2) {
        fprintf(stderr, "Usage: ext2_checker <image file name>\n");
        return EINVAL;
    }

    char *diskname = argv[1];

    // load required objects
    disk = open_disk(diskname);
    struct ext2_super_block *sb = get_super_block(disk);
    struct ext2_group_desc *gd = get_group_desc(disk);
    struct ext2_inode *inode_tbl = get_inode_tbl(disk, gd);

    int total_fixes = 0;

    // a should be the last one to update

    // b, c, d, e
    total_fixes += iterate_and_fix_files(disk, gd, inode_tbl, EXT2_ROOT_INO);

    // a, check free count matches
    int num_free_inode_in_bitmap = count_free_inode_in_bitmap(disk, gd);
    int num_free_block_in_bitmap = count_free_block_in_bitmap(disk, gd);
    total_fixes += update_sb_free_count(&(sb->s_free_inodes_count), num_free_inode_in_bitmap, "free inodes", 1);
    total_fixes += update_sb_free_count(&(sb->s_free_blocks_count), num_free_block_in_bitmap, "free blocks", 1);
    total_fixes += update_gd_free_count(&(gd->bg_free_inodes_count), num_free_inode_in_bitmap, "free inodes", 1);
    total_fixes += update_gd_free_count(&(gd->bg_free_blocks_count), num_free_block_in_bitmap, "free blocks", 1);

    if (total_fixes > 0) {
        printf("%d file system inconsistencies repaired!\n", total_fixes);
    } else {
        printf("No file system inconsistencies detected!\n");
    }

    return 0;
}