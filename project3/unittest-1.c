/*
 * unittest-1.c
 * Description: Unit tests (libcheck) for read-only file system functions.
 */

 #define _FILE_OFFSET_BITS 64
 #define FUSE_USE_VERSION 26
 
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <check.h>
 #include <errno.h>
 #include <sys/stat.h>
 #include <sys/statvfs.h>
 #include <fuse.h>
 #include "fs5600.h"
 
 /* Declare the fs_ops operations vector from your implementation. */
 extern struct fuse_operations fs_ops;
 extern void block_init(char *file);
 
 /* Test: fs_getattr for root directory "/" */
 START_TEST(test_getattr_root) {
     struct stat st;
     int ret = fs_ops.getattr("/", &st);
     ck_assert_int_eq(ret, 0);
     // The root directory in test.img is specified to have mode 040777 and size 4096.
     ck_assert(S_ISDIR(st.st_mode));
     ck_assert_int_eq(st.st_size, 4096);
 }
 END_TEST
 
 /* Test: fs_getattr for a known file "/file.1k" */
 START_TEST(test_getattr_file1k) {
     struct stat st;
     int ret = fs_ops.getattr("/file.1k", &st);
     ck_assert_int_eq(ret, 0);
     ck_assert(S_ISREG(st.st_mode));
     ck_assert_int_eq(st.st_size, 1000);  // Expected size from test.img.
 }
 END_TEST
 
 /* Test: fs_getattr for a non-existent file */
 START_TEST(test_getattr_nonexistent) {
     struct stat st;
     int ret = fs_ops.getattr("/nonexistent", &st);
     ck_assert_int_lt(ret, 0);
     ck_assert_int_eq(ret, -ENOENT);
 }
 END_TEST
 
 /* Test: fs_readdir for the root directory */
 START_TEST(test_readdir_root) {
     /* Expected entries in root from test.img:
        "dir2", "dir3", "dir-with-long-name", "file.10", "file.1k", "file.8k+"
     */
     struct {
         char *name;
         int seen;
     } root_entries[] = {
        { "dir2", 0 },
        { "dir3", 0 },
        { "dir-with-long-name", 0 },
        { "file.10", 0 },
        { "file.1k", 0 },
        { "file.8k+", 0 },
        { NULL, 0 }
     };
 
     /* Filler callback: mark an entry as seen if it matches one in root_entries */
     int filler(void *ptr, const char *name, const struct stat *stbuf, off_t off) {
          int i;
          for (i = 0; root_entries[i].name != NULL; i++) {
               if (strcmp(root_entries[i].name, name) == 0) {
                   root_entries[i].seen = 1;
                   break;
               }
          }
          return 0;
     }
 
     int ret = fs_ops.readdir("/", root_entries, filler, 0, NULL);
     ck_assert_int_eq(ret, 0);
     for (int i = 0; root_entries[i].name != NULL; i++) {
          ck_assert_int_eq(root_entries[i].seen, 1);
     }
 }
 END_TEST
 
 /* Test: fs_read for "/file.1k" */
 START_TEST(test_read_file_1k) {
     char buf[2000];
     int ret = fs_ops.read("/file.1k", buf, 2000, 0, NULL);
     ck_assert_int_eq(ret, 1000);  // The entire file should be 1000 bytes.
 }
 END_TEST
 
 /* Test: fs_statfs */
 START_TEST(test_statfs) {
     struct statvfs sv;
     int ret = fs_ops.statfs("/", &sv);
     ck_assert_int_eq(ret, 0);
     ck_assert_int_eq(sv.f_blocks, 400);
     // For test.img, total blocks = disk size - 2. The exact value depends on the generated image.
     // You could add more checks based on your image contents.
 }
 END_TEST
 
 /* Test: fs_rename - rename "/file.10" to "/file.new" and verify */
 START_TEST(test_rename) {
     int ret = fs_ops.rename("/file.10", "/file.new");
     ck_assert_int_eq(ret, 0);
     struct stat st;
     ret = fs_ops.getattr("/file.new", &st);
     ck_assert_int_eq(ret, 0);
     // Optionally, rename back to restore test image state.
     ret = fs_ops.rename("/file.new", "/file.10");
     ck_assert_int_eq(ret, 0);
 }
 END_TEST
 
 /* Test: fs_chmod - change permissions of "/file.1k" */
 START_TEST(test_chmod) {
     int ret = fs_ops.chmod("/file.1k", 0600);
     ck_assert_int_eq(ret, 0);
     struct stat st;
     ret = fs_ops.getattr("/file.1k", &st);
     ck_assert_int_eq(ret, 0);
     // Check that the permission bits are now 0600.
     ck_assert_int_eq(st.st_mode & 0777, 0600);
 }
 END_TEST
 
 /* Main: Add all tests to the suite */
 int main(int argc, char **argv) {
     /* Initialize the disk image "test.img" as per assignment instructions */
     block_init("test.img");
     fs_ops.init(NULL);
 
     Suite *s = suite_create("fs5600-ReadOnly");
     TCase *tc = tcase_create("read_mostly");
     tcase_add_test(tc, test_getattr_root);
     tcase_add_test(tc, test_getattr_file1k);
     tcase_add_test(tc, test_getattr_nonexistent);
     tcase_add_test(tc, test_readdir_root);
     tcase_add_test(tc, test_read_file_1k);
     tcase_add_test(tc, test_statfs);
     tcase_add_test(tc, test_rename);
     tcase_add_test(tc, test_chmod);
     suite_add_tcase(s, tc);
 
     SRunner *sr = srunner_create(s);
     srunner_set_fork_status(sr, CK_NOFORK);
     srunner_run_all(sr, CK_VERBOSE);
     int n_failed = srunner_ntests_failed(sr);
     srunner_free(sr);
     return (n_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
 }
 