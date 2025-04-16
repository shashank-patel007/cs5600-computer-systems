/*
 * unittest-2.c
 * Description: Unit tests (libcheck) for write operations of the file system.
 */

 #define _FILE_OFFSET_BITS 64
 #define FUSE_USE_VERSION 26
 
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <check.h>
 #include <errno.h>
 #include <sys/stat.h>
 #include <utime.h>
 #include <fuse.h>
 #include "fs5600.h"
 
 /* Mock fuse context to set uid and gid. */
 struct fuse_context ctx = { .uid = 500, .gid = 500 };
 struct fuse_context *fuse_get_context(void) {
     return &ctx;
 }
 
 extern struct fuse_operations fs_ops;
 extern void block_init(char *file);
 
 /* Test: fs_create - create new file "/newfile" */
 START_TEST(test_create_file) {
     int ret = fs_ops.create("/newfile", 0100666, NULL);
     ck_assert_int_eq(ret, 0);
     struct stat st;
     ret = fs_ops.getattr("/newfile", &st);
     ck_assert_int_eq(ret, 0);
     ck_assert(S_ISREG(st.st_mode));
     ck_assert_int_eq(st.st_size, 0);
 }
 END_TEST
 
 /* Test: fs_mkdir - create new directory "/newdir" */
 START_TEST(test_mkdir) {
     int ret = fs_ops.mkdir("/newdir", 0777);
     ck_assert_int_eq(ret, 0);
     struct stat st;
     ret = fs_ops.getattr("/newdir", &st);
     ck_assert_int_eq(ret, 0);
     ck_assert(S_ISDIR(st.st_mode));
 }
 END_TEST
 
 /* Test: fs_write and fs_read - write data to file and read it back */
 START_TEST(test_write_read) {
     int ret = fs_ops.create("/writefile", 0100666, NULL);
     ck_assert_int_eq(ret, 0);
     const char *data = "Hello, FS!";
     ret = fs_ops.write("/writefile", data, strlen(data), 0, NULL);
     ck_assert_int_eq(ret, (int)strlen(data));
     char buf[50] = {0};
     ret = fs_ops.read("/writefile", buf, 50, 0, NULL);
     ck_assert_int_eq(ret, (int)strlen(data));
     ck_assert_str_eq(buf, data);
 }
 END_TEST
 
 /* Test: fs_truncate - truncate a file to zero length */
 START_TEST(test_truncate) {
     int ret = fs_ops.create("/truncfile", 0100666, NULL);
     ck_assert_int_eq(ret, 0);
     const char *data = "Data to be removed.";
     ret = fs_ops.write("/truncfile", data, strlen(data), 0, NULL);
     ck_assert_int_eq(ret, (int)strlen(data));
     ret = fs_ops.truncate("/truncfile", 0);
     ck_assert_int_eq(ret, 0);
     struct stat st;
     ret = fs_ops.getattr("/truncfile", &st);
     ck_assert_int_eq(ret, 0);
     ck_assert_int_eq(st.st_size, 0);
 }
 END_TEST
 
 /* Test: fs_unlink - remove a file */
 START_TEST(test_unlink) {
     int ret = fs_ops.create("/unlinkfile", 0100666, NULL);
     ck_assert_int_eq(ret, 0);
     struct stat st;
     ret = fs_ops.getattr("/unlinkfile", &st);
     ck_assert_int_eq(ret, 0);
     ret = fs_ops.unlink("/unlinkfile");
     ck_assert_int_eq(ret, 0);
     ret = fs_ops.getattr("/unlinkfile", &st);
     ck_assert_int_eq(ret, -ENOENT);
 }
 END_TEST
 
 /* Test: fs_rmdir - remove a directory */
 START_TEST(test_rmdir) {
     int ret = fs_ops.mkdir("/rmdir_dir", 0777);
     ck_assert_int_eq(ret, 0);
     ret = fs_ops.rmdir("/rmdir_dir");
     ck_assert_int_eq(ret, 0);
     struct stat st;
     ret = fs_ops.getattr("/rmdir_dir", &st);
     ck_assert_int_eq(ret, -ENOENT);
 }
 END_TEST
 
 /* Test: fs_utime - update access and modification times */
 START_TEST(test_utime) {
     int ret = fs_ops.create("/utimefile", 0100666, NULL);
     ck_assert_int_eq(ret, 0);
     struct stat st;
     ret = fs_ops.getattr("/utimefile", &st);
     ck_assert_int_eq(ret, 0);
     struct utimbuf new_time;
     new_time.actime = st.st_mtime - 100;  // arbitrary new value
     new_time.modtime = st.st_mtime - 100;
     ret = fs_ops.utime("/utimefile", &new_time);
     ck_assert_int_eq(ret, 0);
     ret = fs_ops.getattr("/utimefile", &st);
     ck_assert_int_eq(ret, 0);
     ck_assert_int_eq(st.st_mtime, new_time.modtime);
 }
 END_TEST
 
 /* Main: add tests to the suite */
 int main(int argc, char **argv) {
     /* Initialize disk image "test2.img" for write tests */
     block_init("test2.img");
     fs_ops.init(NULL);
 
     Suite *s = suite_create("fs5600-Write");
     TCase *tc = tcase_create("write_mostly");
     tcase_add_test(tc, test_create_file);
     tcase_add_test(tc, test_mkdir);
     tcase_add_test(tc, test_write_read);
     tcase_add_test(tc, test_truncate);
     tcase_add_test(tc, test_unlink);
     tcase_add_test(tc, test_rmdir);
     tcase_add_test(tc, test_utime);
     suite_add_tcase(s, tc);
 
     SRunner *sr = srunner_create(s);
     srunner_set_fork_status(sr, CK_NOFORK);
     srunner_run_all(sr, CK_VERBOSE);
     int n_failed = srunner_ntests_failed(sr);
     srunner_free(sr);
     return (n_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
 }
 