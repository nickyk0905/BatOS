#include "fs.h"
#include "terminal.h"

/*
 * BatOS in-memory filesystem.
 *
 * Supports:
 *   /
 *   .
 *   ..
 *   relative paths
 *   absolute paths
 *   nested directories
 *
 * This is still an in-memory filesystem:
 * files disappear when BatOS reboots.
 */

static fs_node_t fs_nodes[FS_MAX_NODES];
static int fs_current_dir = 0;

/* --------------------------------------------------------- */
/* String helpers                                             */
/* --------------------------------------------------------- */

static unsigned int fs_strlen(const char *s)
{
    unsigned int len = 0;

    while (s[len] != '\0')
        len++;

    return len;
}

static int fs_strcmp(const char *a, const char *b)
{
    int i = 0;

    while (a[i] != '\0' && b[i] != '\0')
    {
        if (a[i] != b[i])
            return 0;

        i++;
    }

    return a[i] == '\0' && b[i] == '\0';
}

static void fs_strcpy(char *dst, const char *src, unsigned int max_len)
{
    unsigned int i = 0;

    if (max_len == 0)
        return;

    while (src[i] != '\0' && i < max_len - 1)
    {
        dst[i] = src[i];
        i++;
    }

    dst[i] = '\0';
}

/* --------------------------------------------------------- */
/* Node helpers                                               */
/* --------------------------------------------------------- */

static int fs_alloc_node(void)
{
    int i;

    for (i = 0; i < FS_MAX_NODES; i++)
    {
        if (fs_nodes[i].type == FS_TYPE_FREE)
            return i;
    }

    return -1;
}

static int fs_find_child(int parent, const char *name)
{
    int i;

    for (i = 0; i < FS_MAX_NODES; i++)
    {
        if (fs_nodes[i].type != FS_TYPE_FREE &&
            fs_nodes[i].parent == parent &&
            fs_strcmp(fs_nodes[i].name, name))
        {
            return i;
        }
    }

    return -1;
}

static int fs_has_children(int node)
{
    int i;

    for (i = 0; i < FS_MAX_NODES; i++)
    {
        if (fs_nodes[i].type != FS_TYPE_FREE &&
            fs_nodes[i].parent == node)
        {
            return 1;
        }
    }

    return 0;
}

/* --------------------------------------------------------- */
/* Path parser                                                */
/* --------------------------------------------------------- */

/*
 * Get next component from path.
 *
 * Example:
 *
 *   /home/user/file
 *
 * produces:
 *
 *   home
 *   user
 *   file
 */
static int fs_next_component(
    const char *path,
    int *position,
    char *component)
{
    int pos = *position;
    int length = 0;

    while (path[pos] == '/')
        pos++;

    if (path[pos] == '\0')
    {
        *position = pos;
        component[0] = '\0';
        return 0;
    }

    while (path[pos] != '\0' &&
           path[pos] != '/' &&
           length < FS_MAX_NAME_LEN - 1)
    {
        component[length] = path[pos];
        length++;
        pos++;
    }

    component[length] = '\0';

    while (path[pos] != '\0' && path[pos] != '/')
        pos++;

    *position = pos;

    return 1;
}

/*
 * Resolve an entire path.
 *
 * Returns node index or -1.
 *
 * Examples:
 *
 *   /
 *   /home
 *   /home/user
 *   .
 *   ..
 *   test/file.txt
 */
static int fs_resolve_path(const char *path)
{
    int node;
    int position = 0;
    char component[FS_MAX_NAME_LEN];

    if (path == 0 || path[0] == '\0')
        return -1;

    if (path[0] == '/')
        node = 0;
    else
        node = fs_current_dir;

    while (fs_next_component(path, &position, component))
    {
        int child;

        if (fs_strcmp(component, "."))
            continue;

        if (fs_strcmp(component, ".."))
        {
            if (fs_nodes[node].parent != -1)
                node = fs_nodes[node].parent;

            continue;
        }

        child = fs_find_child(node, component);

        if (child == -1)
            return -1;

        node = child;
    }

    return node;
}

/*
 * Resolve the parent directory of a path.
 *
 * Example:
 *
 *   /home/user/test.txt
 *
 * returns the node for /home/user
 *
 * and writes "test.txt" into name.
 */
static int fs_resolve_parent(
    const char *path,
    char *name)
{
    int node;
    int position = 0;
    int last_position = 0;
    char component[FS_MAX_NAME_LEN];

    if (path == 0 || path[0] == '\0')
        return -1;

    if (fs_strcmp(path, "/"))
        return -1;

    if (path[0] == '/')
        node = 0;
    else
        node = fs_current_dir;

    while (1)
    {
        last_position = position;

        if (!fs_next_component(path, &position, component))
            break;

        /*
         * Find out whether this is the last component.
         */
        {
            int test_position = position;
            char next_component[FS_MAX_NAME_LEN];

            if (!fs_next_component(
                    path,
                    &test_position,
                    next_component))
            {
                /*
                 * This is the final component.
                 */
                fs_strcpy(
                    name,
                    component,
                    FS_MAX_NAME_LEN);

                return node;
            }
        }

        if (fs_strcmp(component, "."))
            continue;

        if (fs_strcmp(component, ".."))
        {
            if (fs_nodes[node].parent != -1)
                node = fs_nodes[node].parent;

            continue;
        }

        {
            int child = fs_find_child(node, component);

            if (child == -1)
                return -1;

            if (fs_nodes[child].type != FS_TYPE_DIR)
                return -1;

            node = child;
        }

        (void)last_position;
    }

    return -1;
}

/* --------------------------------------------------------- */
/* Filesystem initialization                                  */
/* --------------------------------------------------------- */

void fs_init(void)
{
    int i;

    for (i = 0; i < FS_MAX_NODES; i++)
        fs_nodes[i].type = FS_TYPE_FREE;

    /*
     * Root directory.
     */
    fs_nodes[0].type = FS_TYPE_DIR;
    fs_strcpy(fs_nodes[0].name, "/", FS_MAX_NAME_LEN);
    fs_nodes[0].parent = -1;

    fs_current_dir = 0;

    /*
     * Basic Linux-like directory structure.
     */
    fs_cmd_mkdir("/home");
    fs_cmd_mkdir("/tmp");
    fs_cmd_mkdir("/bin");
    fs_cmd_mkdir("/etc");

    /*
     * Start at root.
     */
    fs_current_dir = 0;
}

/* --------------------------------------------------------- */
/* pwd                                                        */
/* --------------------------------------------------------- */

void fs_cmd_pwd(void)
{
    int stack[FS_MAX_DEPTH];
    int depth = 0;
    int node = fs_current_dir;
    int i;

    terminal_write("\n");

    while (node != -1 && depth < FS_MAX_DEPTH)
    {
        stack[depth] = node;
        depth++;

        node = fs_nodes[node].parent;
    }

    if (depth == 1)
    {
        terminal_write("/");
    }
    else
    {
        for (i = depth - 2; i >= 0; i--)
        {
            terminal_write("/");
            terminal_write(fs_nodes[stack[i]].name);
        }
    }

    terminal_write("\n");
}

/* --------------------------------------------------------- */
/* ls                                                         */
/* --------------------------------------------------------- */

void fs_cmd_ls(void)
{
    int i;
    int found = 0;

    terminal_write("\n");

    for (i = 0; i < FS_MAX_NODES; i++)
    {
        if (fs_nodes[i].type != FS_TYPE_FREE &&
            fs_nodes[i].parent == fs_current_dir)
        {
            terminal_write(fs_nodes[i].name);

            if (fs_nodes[i].type == FS_TYPE_DIR)
                terminal_write("/");

            terminal_write("\n");

            found = 1;
        }
    }

    if (!found)
        terminal_write("(empty)\n");
}

/* --------------------------------------------------------- */
/* cd                                                         */
/* --------------------------------------------------------- */

void fs_cmd_cd(const char *path)
{
    int target;

    if (path == 0 || fs_strlen(path) == 0)
    {
        /*
         * Linux normally goes to $HOME.
         * BatOS currently has no environment variables,
         * so /home is our home directory.
         */
        fs_current_dir = fs_resolve_path("/home");
        return;
    }

    target = fs_resolve_path(path);

    if (target == -1)
    {
        terminal_write("\ncd: no such file or directory: ");
        terminal_write(path);
        terminal_write("\n");
        return;
    }

    if (fs_nodes[target].type != FS_TYPE_DIR)
    {
        terminal_write("\ncd: not a directory: ");
        terminal_write(path);
        terminal_write("\n");
        return;
    }

    fs_current_dir = target;
}

/* --------------------------------------------------------- */
/* mkdir                                                      */
/* --------------------------------------------------------- */

void fs_cmd_mkdir(const char *path)
{
    char name[FS_MAX_NAME_LEN];
    int parent;
    int new_index;

    if (path == 0 || fs_strlen(path) == 0)
    {
        terminal_write("\nUsage: mkdir <directory>\n");
        return;
    }

    parent = fs_resolve_parent(path, name);

    if (parent == -1)
    {
        terminal_write("\nmkdir: invalid path: ");
        terminal_write(path);
        terminal_write("\n");
        return;
    }

    if (fs_find_child(parent, name) != -1)
    {
        terminal_write("\nmkdir: cannot create directory '");
        terminal_write(path);
        terminal_write("': File exists\n");
        return;
    }

    new_index = fs_alloc_node();

    if (new_index == -1)
    {
        terminal_write("\nmkdir: no space left\n");
        return;
    }

    fs_nodes[new_index].type = FS_TYPE_DIR;

    fs_strcpy(
        fs_nodes[new_index].name,
        name,
        FS_MAX_NAME_LEN);

    fs_nodes[new_index].parent = parent;
}

/* --------------------------------------------------------- */
/* rmdir                                                      */
/* --------------------------------------------------------- */

void fs_cmd_rmdir(const char *path)
{
    int target;

    if (path == 0 || fs_strlen(path) == 0)
    {
        terminal_write("\nUsage: rmdir <directory>\n");
        return;
    }

    target = fs_resolve_path(path);

    if (target == -1)
    {
        terminal_write("\nrmdir: failed to remove '");
        terminal_write(path);
        terminal_write("': No such file or directory\n");
        return;
    }

    if (target == 0)
    {
        terminal_write("\nrmdir: cannot remove '/'\n");
        return;
    }

    if (fs_nodes[target].type != FS_TYPE_DIR)
    {
        terminal_write("\nrmdir: not a directory: ");
        terminal_write(path);
        terminal_write("\n");
        return;
    }

    if (fs_has_children(target))
    {
        terminal_write("\nrmdir: failed to remove '");
        terminal_write(path);
        terminal_write("': Directory not empty\n");
        return;
    }

    fs_nodes[target].type = FS_TYPE_FREE;
}

/* --------------------------------------------------------- */
/* touch                                                       */
/* --------------------------------------------------------- */

void fs_cmd_touch(const char *path)
{
    char name[FS_MAX_NAME_LEN];
    int parent;
    int existing;
    int new_index;

    if (path == 0 || fs_strlen(path) == 0)
    {
        terminal_write("\nUsage: touch <file>\n");
        return;
    }

    existing = fs_resolve_path(path);

    if (existing != -1)
    {
        /*
         * Existing directory/file:
         * Linux touch updates timestamps.
         * BatOS has no timestamps yet, so leave it unchanged.
         */
        return;
    }

    parent = fs_resolve_parent(path, name);

    if (parent == -1)
    {
        terminal_write("\ntouch: cannot touch '");
        terminal_write(path);
        terminal_write("': No such file or directory\n");
        return;
    }

    new_index = fs_alloc_node();

    if (new_index == -1)
    {
        terminal_write("\ntouch: no space left\n");
        return;
    }

    fs_nodes[new_index].type = FS_TYPE_FILE;

    fs_strcpy(
        fs_nodes[new_index].name,
        name,
        FS_MAX_NAME_LEN);

    fs_nodes[new_index].parent = parent;

    fs_nodes[new_index].content[0] = '\0';
    fs_nodes[new_index].content_length = 0;
}

/* --------------------------------------------------------- */
/* cat                                                        */
/* --------------------------------------------------------- */

void fs_cmd_cat(const char *path)
{
    int target;

    if (path == 0 || fs_strlen(path) == 0)
    {
        terminal_write("\nUsage: cat <file>\n");
        return;
    }

    target = fs_resolve_path(path);

    if (target == -1)
    {
        terminal_write("\ncat: ");
        terminal_write(path);
        terminal_write(": No such file or directory\n");
        return;
    }

    if (fs_nodes[target].type != FS_TYPE_FILE)
    {
        terminal_write("\ncat: ");
        terminal_write(path);
        terminal_write(": Is a directory\n");
        return;
    }

    terminal_write("\n");
    terminal_write(fs_nodes[target].content);
    terminal_write("\n");
}

void fs_cmd_cat_write(const char *path, const char *text, int append)
{
    char name[FS_MAX_NAME_LEN];
    int parent;
    int target;
    unsigned int text_length;
    unsigned int old_length;

    if (path == 0 || fs_strlen(path) == 0)
    {
        terminal_write("\nUsage: cat > <file>\n");
        return;
    }

    text_length = fs_strlen(text);

    target = fs_resolve_path(path);

    /*
     * File already exists.
     */
    if (target != -1)
    {
        if (fs_nodes[target].type != FS_TYPE_FILE)
        {
            terminal_write("\ncat: ");
            terminal_write(path);
            terminal_write(": Is a directory\n");
            return;
        }

        /*
         * Append mode.
         */
        if (append)
        {
            old_length = fs_nodes[target].content_length;

            if (old_length + text_length >= FS_MAX_FILE_SIZE)
            {
                terminal_write("\ncat: ");
                terminal_write(path);
                terminal_write(": file too large\n");
                return;
            }

            fs_strcpy(
                fs_nodes[target].content + old_length,
                text,
                FS_MAX_FILE_SIZE - old_length);

            fs_nodes[target].content_length =
                old_length + text_length;
        }

        /*
         * Overwrite mode.
         */
        else
        {
            if (text_length >= FS_MAX_FILE_SIZE)
            {
                terminal_write("\ncat: ");
                terminal_write(path);
                terminal_write(": file too large\n");
                return;
            }

            fs_strcpy(
                fs_nodes[target].content,
                text,
                FS_MAX_FILE_SIZE);

            fs_nodes[target].content_length = text_length;
        }

        return;
    }

    /*
     * File doesn't exist, so create it.
     */
    parent = fs_resolve_parent(path, name);

    if (parent == -1)
    {
        terminal_write("\ncat: ");
        terminal_write(path);
        terminal_write(": No such file or directory\n");
        return;
    }

    target = fs_alloc_node();

    if (target == -1)
    {
        terminal_write("\ncat: no space left\n");
        return;
    }

    fs_nodes[target].type = FS_TYPE_FILE;

    fs_strcpy(
        fs_nodes[target].name,
        name,
        FS_MAX_NAME_LEN);

    fs_nodes[target].parent = parent;

    if (text_length >= FS_MAX_FILE_SIZE)
        text_length = FS_MAX_FILE_SIZE - 1;

    fs_strcpy(
        fs_nodes[target].content,
        text,
        FS_MAX_FILE_SIZE);

    fs_nodes[target].content_length = text_length;
}

/* --------------------------------------------------------- */
/* rm                                                         */
/* --------------------------------------------------------- */

void fs_cmd_rm(const char *path)
{
    int target;

    if (path == 0 || fs_strlen(path) == 0)
    {
        terminal_write("\nUsage: rm <file>\n");
        return;
    }

    target = fs_resolve_path(path);

    if (target == -1)
    {
        terminal_write("\nrm: cannot remove '");
        terminal_write(path);
        terminal_write("': No such file or directory\n");
        return;
    }

    if (target == 0)
    {
        terminal_write("\nrm: cannot remove '/'\n");
        return;
    }

    if (fs_nodes[target].type != FS_TYPE_FILE)
    {
        terminal_write("\nrm: cannot remove '");
        terminal_write(path);
        terminal_write("': Is a directory\n");
        return;
    }

    fs_nodes[target].type = FS_TYPE_FREE;
}

/* --------------------------------------------------------- */
/* cp                                                         */
/* --------------------------------------------------------- */

void fs_cmd_cp(const char *src, const char *dst)
{
    int source;
    int destination_parent;
    int destination;
    int new_index;
    char destination_name[FS_MAX_NAME_LEN];

    if (src == 0 || dst == 0 ||
        fs_strlen(src) == 0 ||
        fs_strlen(dst) == 0)
    {
        terminal_write("\nUsage: cp <source> <destination>\n");
        return;
    }

    source = fs_resolve_path(src);

    if (source == -1)
    {
        terminal_write("\ncp: cannot stat '");
        terminal_write(src);
        terminal_write("': No such file or directory\n");
        return;
    }

    if (fs_nodes[source].type != FS_TYPE_FILE)
    {
        terminal_write("\ncp: ");
        terminal_write(src);
        terminal_write(": Is a directory\n");
        return;
    }

    destination = fs_resolve_path(dst);

    if (destination != -1)
    {
        /*
         * cp file directory/
         */
        if (fs_nodes[destination].type == FS_TYPE_DIR)
        {
            destination_parent = destination;

            fs_strcpy(
                destination_name,
                fs_nodes[source].name,
                FS_MAX_NAME_LEN);

            if (fs_find_child(
                    destination_parent,
                    destination_name) != -1)
            {
                terminal_write("\ncp: destination exists\n");
                return;
            }
        }
        else
        {
            terminal_write("\ncp: destination already exists\n");
            return;
        }
    }
    else
    {
        destination_parent =
            fs_resolve_parent(dst, destination_name);

        if (destination_parent == -1)
        {
            terminal_write("\ncp: invalid destination: ");
            terminal_write(dst);
            terminal_write("\n");
            return;
        }
    }

    new_index = fs_alloc_node();

    if (new_index == -1)
    {
        terminal_write("\ncp: no space left\n");
        return;
    }

    fs_nodes[new_index].type = FS_TYPE_FILE;

    fs_strcpy(
        fs_nodes[new_index].name,
        destination_name,
        FS_MAX_NAME_LEN);

    fs_nodes[new_index].parent = destination_parent;

    fs_strcpy(
        fs_nodes[new_index].content,
        fs_nodes[source].content,
        FS_MAX_FILE_SIZE);

    fs_nodes[new_index].content_length =
        fs_nodes[source].content_length;
}

/* --------------------------------------------------------- */
/* mv                                                         */
/* --------------------------------------------------------- */

void fs_cmd_mv(const char *src, const char *dst)
{
    int source;
    int destination;
    int destination_parent;
    char destination_name[FS_MAX_NAME_LEN];

    if (src == 0 || dst == 0 ||
        fs_strlen(src) == 0 ||
        fs_strlen(dst) == 0)
    {
        terminal_write("\nUsage: mv <source> <destination>\n");
        return;
    }

    source = fs_resolve_path(src);

    if (source == -1)
    {
        terminal_write("\nmv: cannot stat '");
        terminal_write(src);
        terminal_write("': No such file or directory\n");
        return;
    }

    if (source == 0)
    {
        terminal_write("\nmv: cannot move '/'\n");
        return;
    }

    destination = fs_resolve_path(dst);

    if (destination != -1)
    {
        /*
         * mv file directory/
         */
        if (fs_nodes[destination].type == FS_TYPE_DIR)
        {
            destination_parent = destination;

            fs_strcpy(
                destination_name,
                fs_nodes[source].name,
                FS_MAX_NAME_LEN);

            if (fs_find_child(
                    destination_parent,
                    destination_name) != -1)
            {
                terminal_write("\nmv: destination exists\n");
                return;
            }
        }
        else
        {
            terminal_write("\nmv: destination already exists\n");
            return;
        }
    }
    else
    {
        destination_parent =
            fs_resolve_parent(dst, destination_name);

        if (destination_parent == -1)
        {
            terminal_write("\nmv: invalid destination: ");
            terminal_write(dst);
            terminal_write("\n");
            return;
        }

        if (fs_find_child(
                destination_parent,
                destination_name) != -1)
        {
            terminal_write("\nmv: destination already exists\n");
            return;
        }
    }

    /*
     * Prevent moving a directory inside itself.
     */
    if (fs_nodes[source].type == FS_TYPE_DIR)
    {
        int check = destination_parent;

        while (check != -1)
        {
            if (check == source)
            {
                terminal_write(
                    "\nmv: cannot move directory into itself\n");
                return;
            }

            check = fs_nodes[check].parent;
        }
    }

    fs_nodes[source].parent = destination_parent;

    fs_strcpy(
        fs_nodes[source].name,
        destination_name,
        FS_MAX_NAME_LEN);
}