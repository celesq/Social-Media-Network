#ifndef FEED_H
#define FEED_H

#include "users.h"

/**
 * Function that handles the calling of every command from task 3
 *
 * Please add any necessary parameters to the functions
*/

void handle_input_feed(char *input, list_graph_t *graph,
					   post_t **posts, int nr_posts);

void show_feed(list_graph_t *graph, post_t **posts,
			   int uid, int feed_size, int nr_posts);

int search_pid(post_t **posts, int uid);

void view_profile(post_t **posts, int uid, int nr_posts);

void friends_reposted(list_graph_t *graph, post_t **posts, int id, int pid);

void find_largest_clique(list_graph_t *graph, int node);

void bronkerbosch(list_graph_t *graph, linked_list_t *R, linked_list_t *P,
				  linked_list_t *X, linked_list_t *largest_clique);

void common_group(list_graph_t *graph, int node);

#endif // FEED_H
