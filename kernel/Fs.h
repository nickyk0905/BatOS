#ifndef FS_H
#define FS_H

#define FS_MAX_NODES      64
#define FS_MAX_NAME_LEN   32
#define FS_MAX_FILE_SIZE  256
#define FS_MAX_PATH_LEN   256
#define FS_MAX_DEPTH      16

typedef enum {
    FS_TYPE_FREE = 0,
    FS_TYPE_FILE,
    FS_TYPE_DIR
} fs_node_type;

typedef struct {
    fs_node_type  type;
    char          name[FS_MAX_NAME_LEN];
    int           parent;                     /* index of parent node, -1 for root */
    char          content[FS_MAX_FILE_SIZE];  /* only used when type == FS_TYPE_FILE */
    unsigned int  content_length;
} fs_node_t;

/* Call once at boot, before any fs_cmd_* function is used. */
void fs_init(void);

/* Shell command handlers. `name`/`src`/`dst` are plain, already-trimmed
   arguments (no leading command word, no trailing newline). */
void fs_cmd_pwd(void);
void fs_cmd_ls(void);
void fs_cmd_cd(const char *name);
void fs_cmd_mkdir(const char *name);
void fs_cmd_rmdir(const char *name);
void fs_cmd_touch(const char *name);
void fs_cmd_cat(const char *name);
void fs_cmd_cat_write(const char *path, const char *text, int append);
void fs_cmd_rm(const char *name);
void fs_cmd_cp(const char *src, const char *dst);
void fs_cmd_mv(const char *src, const char *dst);

#endif