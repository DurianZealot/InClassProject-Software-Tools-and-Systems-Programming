#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "ftree.h"

void build_up_tree_node(struct TreeNode *tree_node,
                        char *fname,
                        struct stat file_info,
                        struct TreeNode *content_node,
                        struct TreeNode *next_node);
struct TreeNode *populate_directory_tree(const char *fname, char *pre_path);
struct stat *get_file_info(const char *pre_path, const char *file_name);

/*
 * Returns the FTree rooted at the path fname.
 *
 * Use the following if the file fname doesn't exist and return NULL:
 * fprintf(stderr, "The path (%s) does not point to an existing entry!\n", fname);
 *
 */
struct TreeNode *generate_ftree(const char *fname)
{

    // Your implementation here.

    // Hint: consider implementing a recursive helper function that
    // takes fname and a path.  For the initial call on the
    // helper function, the path would be "", since fname is the root
    // of the FTree.  For files at other depths, the path would be the
    // file path from the root to that file.
    struct TreeNode *root = malloc(sizeof(struct TreeNode));

    if (root == NULL)
    {
        perror("malloc failed for a struct TreeNode\n");
        exit(1);
    }

    // first, we want to check whether the *fname refers to an existing file
    struct stat file_info;

    // if lstat the file name return -1, we get an error
    if (lstat(fname, &file_info) == -1)
    {
        fprintf(stderr, "The path (%s) does not point to an existing entry!\n", fname);
        return NULL;
    }

    // if the file name refers to a file that is a regular file
    // we just create a TreeNode with it and return it immediately
    if (S_ISREG(file_info.st_mode))
    {
        build_up_tree_node(root, (char *)fname, file_info, NULL, NULL);
        return root;
    }

    // if the file name refers to a directory
    if (S_ISDIR(file_info.st_mode))
    {
        build_up_tree_node(root, (char *)fname, file_info, populate_directory_tree(fname, ""), NULL);
        return root;
    }

    // if the file name refers to a link
    if (S_ISLNK(file_info.st_mode))
    {
        build_up_tree_node(root, (char *)fname, file_info, NULL, NULL);
        return root;
    }

    // if the file does not refer to any of directory/link/regular file, return NULL
    return NULL;
}

/*
* Helper function to build up the a tree node for ftree
* Precondition: file_info != NULL
*/
void build_up_tree_node(struct TreeNode *tree_node,
                        char *fname,
                        struct stat file_info,
                        struct TreeNode *content_node,
                        struct TreeNode *next_node)
{

    // set the fname for a tree node
    tree_node->fname = malloc(sizeof(char) * (strlen(fname) + 1));

    // if it is out of memory, cannot allocate memory for fname
    if (tree_node->fname == NULL)
    {
        perror("Failed to allocate memory for a fname in a TreeNode\n");
        exit(1);
    }

    strcpy(tree_node->fname, fname);

    // set the permission for a tree node
    tree_node->permissions = file_info.st_mode & 0777;

    // set the type for a tree node
    if (S_ISDIR(file_info.st_mode))
    {
        // it is a directory node
        tree_node->type = 'd';
    }
    else if (S_ISREG(file_info.st_mode))
    {
        // it is a regular file node
        tree_node->type = '-';
    }
    else if (S_ISLNK(file_info.st_mode))
    {
        // it is a (symbolic) link node
        tree_node->type = 'l';
    }

    // set up the content
    tree_node->contents = content_node;

    // set up the next node it connects to
    tree_node->next = next_node;
}

/*
* Helper function to generate a tree for any directory node
*/

struct TreeNode *populate_directory_tree(const char *folder_path, char *pre_path)
{
    struct dirent *curr_dir_reader;
    DIR *curr_dir_ptr;
    struct TreeNode *first_node = NULL;
    struct TreeNode *prev_node = NULL;
    struct TreeNode *curr_node = NULL;

    // build up path for the current directory
    // if the pre_path is "", then we need to use the folder_path to open the directory
    int len_pre_path = 0;
    int len_folder_path = strlen(folder_path);

    if (strcmp(pre_path, "") != 0)
    {
        len_pre_path = strlen(pre_path);
    }

    char path[len_pre_path + len_folder_path + 2];

    if (len_pre_path == 0)
    {
        // if the fname is the root of the tree
        strcpy(path, folder_path);
    }
    else
    {
        // if the fname is under the root of tree
        strcpy(path, pre_path);
        strcat(path, "/");
        strcat(path, folder_path);
    }

    // open the directory that path refers to
    curr_dir_ptr = opendir(path);

    if (curr_dir_ptr == NULL)
    {
        perror("Error failed to open input directory\n");
        exit(1);
    }

    curr_dir_reader = readdir(curr_dir_ptr);
    // readdir will return NULL if an error occur or no more file/directory to read

    bool first_node_found = 0;

    while (NULL != curr_dir_reader)
    {
        if (strcmp(curr_dir_reader->d_name, ".\0") != 0 && strcmp(curr_dir_reader->d_name, "..\0") != 0)
        {
            struct stat *curr_file_info = get_file_info(path, curr_dir_reader->d_name);
            if (first_node_found == 0)
            {
                // allocate space of a TreeNode pointer for first_node before we pass info into this node
                first_node = malloc(sizeof(struct TreeNode));

                if (first_node == NULL)
                {
                    perror("Failed to allocate memory for a TreeNode\n");
                    exit(1);
                }

                // if the first node in this layer is not build up yet
                // we need to set up the build up the first file based on its type
                if (S_ISREG((*curr_file_info).st_mode) || S_ISLNK((*curr_file_info).st_mode))
                {
                    // if the file is a regular file / link
                    // it has no content
                    build_up_tree_node(first_node,
                                       curr_dir_reader->d_name,
                                       (*curr_file_info),
                                       NULL,
                                       NULL);
                }
                else if (S_ISDIR((*curr_file_info).st_mode))
                {
                    // if the file is a directory
                    // it should has should have a content node refers to next layer
                    // in the tree or NULL (if there is no files in this directory)
                    build_up_tree_node(first_node,
                                       curr_dir_reader->d_name,
                                       (*curr_file_info),
                                       populate_directory_tree(curr_dir_reader->d_name, path),
                                       NULL);
                }
                // the prev_node should refers to the first node
                // so that we can update its next node if there is a next node
                prev_node = first_node;
                curr_node = first_node;
                // we finish setting up the first node for this layer
                first_node_found = 1;
            }
            // When we already have the first node set up
            else
            {
                curr_node = malloc(sizeof(struct TreeNode));

                if (curr_node == NULL)
                {
                    perror("Failed to allocate memory for a TreeNode\n");
                    exit(1);
                }

                if (S_ISREG((*curr_file_info).st_mode) || S_ISLNK((*curr_file_info).st_mode))
                {
                    build_up_tree_node(curr_node,
                                       curr_dir_reader->d_name,
                                       (*curr_file_info),
                                       NULL,
                                       NULL);
                }
                else if (S_ISDIR((*curr_file_info).st_mode))
                {
                    build_up_tree_node(curr_node,
                                       curr_dir_reader->d_name,
                                       (*curr_file_info),
                                       populate_directory_tree(curr_dir_reader->d_name, path),
                                       NULL);
                }
                prev_node->next = curr_node;
                prev_node = curr_node;
            }
            free(curr_file_info);
        }
        curr_dir_reader = readdir(curr_dir_ptr);
    }

    if (closedir(curr_dir_ptr) == 1)
    {
        perror("Fail to close directory");
        exit(1);
    }
    return first_node;
}

/*
* Helper function to get a pointer to a stat of a file/directory/link
*/

struct stat *get_file_info(const char *pre_path, const char *file_name)
{
    struct stat *file_info = malloc(sizeof(struct stat));

    if (file_info == NULL)
    {
        printf("Failed to allocate memory for a file info stat\n");
        exit(1);
    }

    char file_path[(strlen(pre_path) + strlen(file_name) + 2)];
    strcpy(file_path, pre_path);
    strcat(file_path, "/");
    strcat(file_path, file_name);

    if (lstat(file_path, file_info) == -1)
    {
        fprintf(stderr, "The path (%s) does not point to an existing entry!\n", file_name);
        return NULL;
    }

    return file_info;
}
/*
 * Prints the TreeNodes encountered on a preorder traversal of an FTree.
 *
 * The only print statements that you may use in this function are:
 * printf("===== %s (%c%o) =====\n", root->fname, root->type, root->permissions)
 * printf("%s (%c%o)\n", root->fname, root->type, root->permissions)
 *
 */
void print_ftree(struct TreeNode *root)
{

    // Here's a trick for remembering what depth (in the tree) you're at
    // and printing 2 * that many spaces at the beginning of the line.
    static int depth = 0;
    // Your implementation here.
    if (NULL != root)
    {
        if (root->type == 'l' || root->type == '-')
        {
            printf("%*s", depth * 2, "");
            printf("%s (%c%o)\n", root->fname, root->type, root->permissions);
        }
        else
        {
            printf("%*s", depth * 2, "");
            printf("===== %s (%c%o) =====\n", root->fname, root->type, root->permissions);
            struct TreeNode * next_layer = root->contents;
            depth += 1;
            while (NULL != next_layer)
            {
                print_ftree(next_layer);
                if (next_layer->type == 'd')
                {
                    depth -= 1;
                }
                next_layer = next_layer->next;
            }
        }
    }
}

/*
 * Deallocate all dynamically-allocated memory in the FTree rooted at node.
 *
 */
void deallocate_ftree(struct TreeNode *node)
{
    // Precondition: node is not NULL

    // Your implementation here.
    if (NULL != node)
    {
        deallocate_ftree(node->next);
        deallocate_ftree(node->contents);
        free(node->fname);
        free(node);
    }
}
