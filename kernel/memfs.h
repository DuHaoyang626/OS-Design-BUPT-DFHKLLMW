#ifndef MEMFS_H
#define MEMFS_H
/*
 * 内存文件系统（内存当磁盘）。
 * 说明：实现一个 FAT 风格的最小文件系统：
 * - 以簇（cluster）为分配单位
 * - 使用 FAT 表记录簇链
 * - 目录采用固定大小目录项数组（支持层级目录）
 */
 

/* 返回值约定：>=0 成功；<0 失败 */

#define MEMFS_OK                 0
#define MEMFS_ERR_INVAL         -1
#define MEMFS_ERR_NOSPACE       -2
#define MEMFS_ERR_NOTFOUND      -3
#define MEMFS_ERR_EXISTS        -4
#define MEMFS_ERR_NOTDIR        -5
#define MEMFS_ERR_ISDIR         -6

/* 初始化/格式化 */
int memfs_format(int disk_kb);
int memfs_ready(void);

/* 目录/文件操作 */
int memfs_mkdir(const char *path);
int memfs_create(const char *path);
int memfs_delete(const char *path);

/* 读写：按偏移读写（自动扩容） */
int memfs_write(const char *path, int offset, const char *buf, int len);
int memfs_read(const char *path, int offset, char *buf, int len);
int memfs_getsize(const char *path);

/* 复制文件：src -> dst（覆盖 dst） */
int memfs_copy(const char *src, const char *dst);

/* 列目录：把当前目录下名字用 '\n' 拼到 outbuf */
int memfs_list(const char *path, char *outbuf, int outbuf_len);

#endif
