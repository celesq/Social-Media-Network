#ifndef POSTS_H
#define POSTS_H

#include "users.h"

/**
 * Function that handles the calling of every command from task 2
 *
 * Please add any necessary parameters to the functions
*/
void create_post(post_t **posts, int uid, char *title, int *id);

void create_repost(post_t **post, int uid, int pid, int rid, int *nr_posts);

void common_repost(post_t **post, int pid, int rid1, int rid2);

void print_reposts(post_t **post, int id);

void like_post(post_t **post, int uid, int pid, int rid);

void get_repost(post_t **post, int pid, int rid);

void get_likes(post_t **post, int pid, int rid);

void ratio(post_t **post, int pid);

void delete_repost(post_t **post, int id);

void delete_post(post_t **post, int pid, int rid);

void handle_input_posts(char *input, post_t **posts, int *nr_posts);

#endif // POSTS_H
