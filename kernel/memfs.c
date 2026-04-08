#include "bootpack.h"
#include "memfs.h"

/*
 * 设计（内存当磁盘的布局）：
 *   memdisk = [header][FAT(u16)][目录项表][数据区(簇)]
 *   - header：保存布局参数
 *   - FAT：0=空闲，0xffff=簇链结束，其它值=下一簇号
 *   - 目录：固定大小目录项数组；层级目录用 parent 索引表示
 *   - 路径：仅支持绝对路径，用 '/' 分隔，根目录为 "/"
 */

#define MEMFS_MAGIC 0x53464d48 /* 'HMFS' */

#define MEMFS_NAME_MAX  24
#define MEMFS_MAX_ENT   256

#define MEMFS_TYPE_FREE 0
#define MEMFS_TYPE_FILE 1
#define MEMFS_TYPE_DIR  2

#define MEMFS_FAT_FREE  0x0000
#define MEMFS_FAT_EOC   0xffff

struct MEMFS_HDR {
    unsigned int magic;
    unsigned int disk_bytes;
    unsigned int cluster_bytes;
    unsigned int cluster_count;
    unsigned int fat_off;
    unsigned int fat_bytes;
    unsigned int dir_off;
    unsigned int dir_bytes;
    unsigned int data_off;
};

struct MEMFS_DIR {
    unsigned char type; /* FREE/FILE/DIR */
    unsigned char used;
    unsigned short parent; /* parent dir entry index, root=0xffff */
    unsigned short first_cluster;
    unsigned short reserved;
    unsigned int size;
    char name[MEMFS_NAME_MAX];
};

static unsigned char *g_memdisk = 0;
static int g_memdisk_bytes = 0;

static struct MEMFS_HDR *hdr(void)
{
    return (struct MEMFS_HDR *) g_memdisk;
}

static unsigned short *fat(void)
{
    return (unsigned short *) (g_memdisk + hdr()->fat_off);
}

static struct MEMFS_DIR *dirent0(void)
{
    return (struct MEMFS_DIR *) (g_memdisk + hdr()->dir_off);
}

static unsigned char *cluster_ptr(unsigned short clus)
{
    return g_memdisk + hdr()->data_off + (int) clus * (int) hdr()->cluster_bytes;
}

static void memfs_memset(void *p, int v, int n)
{
    int i;
    unsigned char *q = (unsigned char *) p;
    for (i = 0; i < n; i++) {
        q[i] = (unsigned char) v;
    }
}

static int memfs_streq(const char *a, const char *b)
{
    while (*a && *b) {
        if (*a != *b) return 0;
        a++; b++;
    }
    return (*a == 0 && *b == 0);
}

static int memfs_strncpy0(char *dst, const char *src, int max)
{
    int i;
    for (i = 0; i < max - 1 && src[i]; i++) {
        dst[i] = src[i];
    }
    dst[i] = 0;
    return i;
}

static int memfs_name_valid(const char *name)
{
    int i;
    if (name == 0 || name[0] == 0) return 0;
    for (i = 0; name[i]; i++) {
        if (name[i] == '/' || name[i] == '\\') return 0;
        if (i >= MEMFS_NAME_MAX - 1) return 0;
    }
    return 1;
}

static int memfs_alloc_dirent(void)
{
    int i;
    struct MEMFS_DIR *d = dirent0();
    for (i = 0; i < MEMFS_MAX_ENT; i++) {
        if (d[i].type == MEMFS_TYPE_FREE) {
            memfs_memset(&d[i], 0, sizeof(struct MEMFS_DIR));
            d[i].used = 1;
            return i;
        }
    }
    return MEMFS_ERR_NOSPACE;
}

static int memfs_find_child(int parent, const char *name)
{
    int i;
    struct MEMFS_DIR *d = dirent0();
    for (i = 0; i < MEMFS_MAX_ENT; i++) {
        if (d[i].type != MEMFS_TYPE_FREE && d[i].used) {
            if ((int) d[i].parent == parent && memfs_streq(d[i].name, name)) {
                return i;
            }
        }
    }
    return MEMFS_ERR_NOTFOUND;
}

static int memfs_path_walk(const char *path, int *out_parent, char *out_name)
{
    /* 解析到最后一段：返回父目录索引 + 最后一段名字 */
    int cur = 0xffff; /* root */
    int i = 0;
    int seglen = 0;
    char seg[MEMFS_NAME_MAX];

    if (path == 0 || path[0] == 0) return MEMFS_ERR_INVAL;
    if (path[0] != '/') return MEMFS_ERR_INVAL;
    if (memfs_streq(path, "/")) {
        if (out_parent) *out_parent = 0xffff;
        if (out_name) out_name[0] = 0;
        return MEMFS_OK;
    }

    /* 跳过开头 '/' */
    i = 1;
    for (;;) {
        char c = path[i];
        if (c == '/' || c == 0) {
            seg[seglen] = 0;
            if (seglen == 0) return MEMFS_ERR_INVAL;
            if (c == 0) {
                /* 最后一段 */
                if (out_parent) *out_parent = cur;
                if (out_name) memfs_strncpy0(out_name, seg, MEMFS_NAME_MAX);
                return MEMFS_OK;
            }

            /* 中间段：必须是目录 */
            {
                int idx = memfs_find_child(cur, seg);
                if (idx < 0) return idx;
                if (dirent0()[idx].type != MEMFS_TYPE_DIR) return MEMFS_ERR_NOTDIR;
                cur = idx;
            }
            seglen = 0;
            i++;
            continue;
        }
        if (seglen >= MEMFS_NAME_MAX - 1) return MEMFS_ERR_INVAL;
        seg[seglen++] = c;
        i++;
    }
}

static int memfs_lookup(const char *path)
{
    int parent;
    char name[MEMFS_NAME_MAX];
    int r;
    if (memfs_streq(path, "/")) return -2; /* root special */
    r = memfs_path_walk(path, &parent, name);
    if (r < 0) return r;
    return memfs_find_child(parent, name);
}

static int memfs_alloc_cluster(void)
{
    unsigned int i;
    unsigned short *f = fat();
    for (i = 0; i < hdr()->cluster_count; i++) {
        if (f[i] == MEMFS_FAT_FREE) {
            f[i] = MEMFS_FAT_EOC;
            memfs_memset(cluster_ptr((unsigned short) i), 0, hdr()->cluster_bytes);
            return (int) i;
        }
    }
    return MEMFS_ERR_NOSPACE;
}

static void memfs_free_chain(unsigned short first)
{
    unsigned short *f = fat();
    unsigned short cur = first;
    while (cur != MEMFS_FAT_EOC && cur != MEMFS_FAT_FREE) {
        unsigned short next = f[cur];
        f[cur] = MEMFS_FAT_FREE;
        cur = next;
    }
}

static int memfs_ensure_chain(struct MEMFS_DIR *e, int need_bytes)
{
    int need_clusters;
    int have_clusters = 0;
    unsigned short *f = fat();
    unsigned short cur;

    if (need_bytes < 0) return MEMFS_ERR_INVAL;
    need_clusters = (need_bytes + (int) hdr()->cluster_bytes - 1) / (int) hdr()->cluster_bytes;
    if (need_clusters == 0) need_clusters = 1;

    if (e->first_cluster == MEMFS_FAT_EOC || e->first_cluster == MEMFS_FAT_FREE) {
        int c = memfs_alloc_cluster();
        if (c < 0) return c;
        e->first_cluster = (unsigned short) c;
    }

    cur = e->first_cluster;
    while (1) {
        have_clusters++;
        if (f[cur] == MEMFS_FAT_EOC) break;
        cur = f[cur];
    }

    while (have_clusters < need_clusters) {
        int nc = memfs_alloc_cluster();
        if (nc < 0) return nc;
        f[cur] = (unsigned short) nc;
        cur = (unsigned short) nc;
        f[cur] = MEMFS_FAT_EOC;
        have_clusters++;
    }
    return MEMFS_OK;
}

int memfs_ready(void)
{
    if (g_memdisk == 0) return 0;
    if (hdr()->magic != MEMFS_MAGIC) return 0;
    return 1;
}

int memfs_format(int disk_kb)
{
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    int bytes;
    int cluster_bytes = 512;
    int dir_bytes;
    int fat_bytes;
    int data_off;
    int cluster_count;
    int need;

    if (disk_kb <= 0) return MEMFS_ERR_INVAL;
    bytes = disk_kb * 1024;

    /* 简单上限保护：最多使用当前可用内存的一半 */
    if (bytes > (int) (memman_total(memman) / 2)) {
        return MEMFS_ERR_NOSPACE;
    }

    /* 释放旧盘 */
    if (g_memdisk) {
        memman_free_4k(memman, (int) g_memdisk, g_memdisk_bytes);
        g_memdisk = 0;
        g_memdisk_bytes = 0;
    }

    /* 目录项表固定大小 */
    dir_bytes = (int) (sizeof(struct MEMFS_DIR) * MEMFS_MAX_ENT);
    /* 估算簇数：FAT 大小依赖簇数，先近似再校验 */
    cluster_count = (bytes - (int) sizeof(struct MEMFS_HDR) - dir_bytes) / (cluster_bytes + 2);
    if (cluster_count <= 0) return MEMFS_ERR_NOSPACE;
    fat_bytes = cluster_count * 2;

    data_off = (int) sizeof(struct MEMFS_HDR) + fat_bytes + dir_bytes;
    need = data_off + cluster_count * cluster_bytes;
    if (need > bytes) {
        /* 保守回退一次 */
        cluster_count = (bytes - (int) sizeof(struct MEMFS_HDR) - dir_bytes) / (cluster_bytes + 2);
        if (cluster_count <= 0) return MEMFS_ERR_NOSPACE;
        fat_bytes = cluster_count * 2;
        data_off = (int) sizeof(struct MEMFS_HDR) + fat_bytes + dir_bytes;
        need = data_off + cluster_count * cluster_bytes;
        if (need > bytes) return MEMFS_ERR_NOSPACE;
    }

    /* 按 4K 对齐申请 */
    bytes = (bytes + 4095) & ~4095;
    g_memdisk = (unsigned char *) memman_alloc_4k(memman, bytes);
    if (g_memdisk == 0) return MEMFS_ERR_NOSPACE;
    g_memdisk_bytes = bytes;

    memfs_memset(g_memdisk, 0, bytes);

    hdr()->magic = MEMFS_MAGIC;
    hdr()->disk_bytes = bytes;
    hdr()->cluster_bytes = cluster_bytes;
    hdr()->cluster_count = cluster_count;
    hdr()->fat_off = (unsigned int) sizeof(struct MEMFS_HDR);
    hdr()->fat_bytes = fat_bytes;
    hdr()->dir_off = (unsigned int) (sizeof(struct MEMFS_HDR) + fat_bytes);
    hdr()->dir_bytes = dir_bytes;
    hdr()->data_off = (unsigned int) (sizeof(struct MEMFS_HDR) + fat_bytes + dir_bytes);

    /* FAT 全清零 = 空闲 */
    memfs_memset(fat(), 0, fat_bytes);
    /* 目录项全清零 = 空闲 */
    memfs_memset(dirent0(), 0, dir_bytes);

    return MEMFS_OK;
}

int memfs_mkdir(const char *path)
{
    int parent;
    char name[MEMFS_NAME_MAX];
    int r;
    int idx;
    struct MEMFS_DIR *d;

    if (!memfs_ready()) return MEMFS_ERR_INVAL;
    r = memfs_path_walk(path, &parent, name);
    if (r < 0) return r;
    if (!memfs_name_valid(name)) return MEMFS_ERR_INVAL;
    if (memfs_find_child(parent, name) >= 0) return MEMFS_ERR_EXISTS;

    idx = memfs_alloc_dirent();
    if (idx < 0) return idx;

    d = &dirent0()[idx];
    d->type = MEMFS_TYPE_DIR;
    d->parent = (unsigned short) parent;
    memfs_strncpy0(d->name, name, MEMFS_NAME_MAX);
    d->first_cluster = MEMFS_FAT_EOC;
    d->size = 0;
    return MEMFS_OK;
}

int memfs_create(const char *path)
{
    int parent;
    char name[MEMFS_NAME_MAX];
    int r;
    int idx;
    struct MEMFS_DIR *d;

    if (!memfs_ready()) return MEMFS_ERR_INVAL;
    r = memfs_path_walk(path, &parent, name);
    if (r < 0) return r;
    if (!memfs_name_valid(name)) return MEMFS_ERR_INVAL;
    if (memfs_find_child(parent, name) >= 0) return MEMFS_ERR_EXISTS;

    idx = memfs_alloc_dirent();
    if (idx < 0) return idx;

    d = &dirent0()[idx];
    d->type = MEMFS_TYPE_FILE;
    d->parent = (unsigned short) parent;
    memfs_strncpy0(d->name, name, MEMFS_NAME_MAX);
    d->first_cluster = MEMFS_FAT_EOC;
    d->size = 0;
    return MEMFS_OK;
}

int memfs_delete(const char *path)
{
    int idx;
    struct MEMFS_DIR *d;
    int i;

    if (!memfs_ready()) return MEMFS_ERR_INVAL;
    if (memfs_streq(path, "/")) return MEMFS_ERR_INVAL;
    idx = memfs_lookup(path);
    if (idx < 0) return idx;
    d = &dirent0()[idx];
    if (d->type == MEMFS_TYPE_DIR) {
        /* 目录必须为空 */
        for (i = 0; i < MEMFS_MAX_ENT; i++) {
            if (dirent0()[i].type != MEMFS_TYPE_FREE && dirent0()[i].used) {
                if ((int) dirent0()[i].parent == idx) {
                    return MEMFS_ERR_INVAL;
                }
            }
        }
        d->type = MEMFS_TYPE_FREE;
        d->used = 0;
        return MEMFS_OK;
    }
    if (d->type == MEMFS_TYPE_FILE) {
        if (d->first_cluster != MEMFS_FAT_EOC && d->first_cluster != MEMFS_FAT_FREE) {
            memfs_free_chain(d->first_cluster);
        }
        d->type = MEMFS_TYPE_FREE;
        d->used = 0;
        return MEMFS_OK;
    }
    return MEMFS_ERR_INVAL;
}

int memfs_getsize(const char *path)
{
    int idx;
    if (!memfs_ready()) return MEMFS_ERR_INVAL;
    idx = memfs_lookup(path);
    if (idx < 0) return idx;
    if (dirent0()[idx].type != MEMFS_TYPE_FILE) return MEMFS_ERR_ISDIR;
    return (int) dirent0()[idx].size;
}

static int memfs_rw_common(const char *path, int offset, char *buf, int len, int is_write)
{
    int idx;
    struct MEMFS_DIR *e;
    unsigned short cur;
    unsigned short *f;
    int clsz;
    int pos;
    int done = 0;
    int need;

    if (!memfs_ready()) return MEMFS_ERR_INVAL;
    if (offset < 0 || len < 0) return MEMFS_ERR_INVAL;
    idx = memfs_lookup(path);
    if (idx < 0) return idx;
    e = &dirent0()[idx];
    if (e->type != MEMFS_TYPE_FILE) return MEMFS_ERR_ISDIR;

    if (is_write) {
        need = offset + len;
        if (need < 0) return MEMFS_ERR_INVAL;
        if (need > (int) e->size) {
            int r = memfs_ensure_chain(e, need);
            if (r < 0) return r;
            e->size = (unsigned int) need;
        } else if (e->first_cluster == MEMFS_FAT_EOC) {
            int r2 = memfs_ensure_chain(e, 1);
            if (r2 < 0) return r2;
        }
    } else {
        if (offset >= (int) e->size) return 0;
        if (offset + len > (int) e->size) len = (int) e->size - offset;
        if (len <= 0) return 0;
        if (e->first_cluster == MEMFS_FAT_EOC) return 0;
    }

    f = fat();
    clsz = (int) hdr()->cluster_bytes;
    pos = 0;
    cur = e->first_cluster;
    while (pos + clsz <= offset) {
        pos += clsz;
        if (f[cur] == MEMFS_FAT_EOC) {
            return done;
        }
        cur = f[cur];
    }

    while (done < len) {
        int in_clus = offset - pos;
        int can = clsz - in_clus;
        unsigned char *cp = cluster_ptr(cur);
        int n = (len - done < can) ? (len - done) : can;
        int k;
        if (is_write) {
            for (k = 0; k < n; k++) {
                cp[in_clus + k] = (unsigned char) buf[done + k];
            }
        } else {
            for (k = 0; k < n; k++) {
                buf[done + k] = (char) cp[in_clus + k];
            }
        }
        done += n;
        offset += n;
        pos += clsz;
        if (done >= len) break;
        if (f[cur] == MEMFS_FAT_EOC) break;
        cur = f[cur];
    }
    return done;
}

int memfs_write(const char *path, int offset, const char *buf, int len)
{
    return memfs_rw_common(path, offset, (char *) buf, len, 1);
}

int memfs_read(const char *path, int offset, char *buf, int len)
{
    return memfs_rw_common(path, offset, buf, len, 0);
}

int memfs_list(const char *path, char *outbuf, int outbuf_len)
{
    int dir_idx;
    int i;
    int w = 0;
    if (!memfs_ready()) return MEMFS_ERR_INVAL;
    if (outbuf == 0 || outbuf_len <= 0) return MEMFS_ERR_INVAL;

    if (memfs_streq(path, "/")) {
        dir_idx = 0xffff;
    } else {
        dir_idx = memfs_lookup(path);
        if (dir_idx < 0) return dir_idx;
        if (dirent0()[dir_idx].type != MEMFS_TYPE_DIR) return MEMFS_ERR_NOTDIR;
    }

    for (i = 0; i < MEMFS_MAX_ENT; i++) {
        struct MEMFS_DIR *e = &dirent0()[i];
        int k;
        if (e->type == MEMFS_TYPE_FREE || !e->used) continue;
        if ((int) e->parent != dir_idx) continue;
        for (k = 0; e->name[k] && w < outbuf_len - 2; k++) {
            outbuf[w++] = e->name[k];
        }
        if (w < outbuf_len - 2) {
            outbuf[w++] = (e->type == MEMFS_TYPE_DIR) ? '/' : ' ';
        }
        if (w < outbuf_len - 1) outbuf[w++] = '\n';
        if (w >= outbuf_len - 1) break;
    }
    outbuf[w] = 0;
    return w;
}

int memfs_copy(const char *src, const char *dst)
{
    int sz;
    int r;
    int off = 0;
    char tmp[512];

    if (!memfs_ready()) return MEMFS_ERR_INVAL;
    sz = memfs_getsize(src);
    if (sz < 0) return sz;
    /* dst 不存在则创建 */
    if (memfs_lookup(dst) < 0) {
        r = memfs_create(dst);
        if (r < 0) return r;
    }
    /* 清空 dst：直接删除再创建（简单） */
    memfs_delete(dst);
    r = memfs_create(dst);
    if (r < 0) return r;

    while (off < sz) {
        int n = sz - off;
        if (n > 512) n = 512;
        r = memfs_read(src, off, tmp, n);
        if (r < 0) return r;
        if (r == 0) break;
        r = memfs_write(dst, off, tmp, r);
        if (r < 0) return r;
        off += r;
    }
    return MEMFS_OK;
}
