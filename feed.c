#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "feed.h"
#include "users.h"

void show_feed(list_graph_t *graph, post_t **posts,
			   int uid, int feed_size, int nr_posts)
{
	int cnt = feed_size;
	for (int i = 0; i < nr_posts; i++) {
		if (cnt == 0)
			break;
		if (strcmp((*posts)[nr_posts - i].title, "NULL")) {
			int ok = 0;
			if (uid == (*posts)[nr_posts - i].uid) {
				ok = 1;
				printf("%s: \"%s\"\n",
					   get_user_name(uid), (*posts)[nr_posts - i].title);
			}
			ll_node_t *current_node = graph->neighbors[uid]->head;
			for (int j = 0; (unsigned int)j
				 < graph->neighbors[uid]->size && !ok; j++) {
				if (*(int *)current_node->data == (*posts)[nr_posts - i].uid) {
					ok = 1;
					int id = *(int *)current_node->data;
					printf("%s: \"%s\"\n",
						   get_user_name(id), (*posts)[nr_posts - i].title);
				}
				current_node = current_node->next;
			}
		}
	}
}

int search_pid(post_t **posts, int uid)
{
	int id = uid;
	if (strcmp((*posts)[uid].title, "NULL") == 0)
		id = search_pid(posts, (*posts)[uid].events->parinte);
	return id;
}

void view_profile(post_t **posts, int uid, int nr_posts)
{
	for (int i = 1; i <= nr_posts; i++)
		if ((*posts)[i].uid == uid) {
			if (strcmp((*posts)[i].title, "NULL")) {
				printf("Posted: \"%s\"\n", (*posts)[i].title);
			} else {
				int pid = search_pid(posts, i);
				printf("Reposted: \"%s\"\n", (*posts)[pid].title);
			}
		}
}

void friends_reposted(list_graph_t *graph, post_t **posts, int id, int pid)
{
	for (int i = 0; i < (*posts)[pid].events->nr_copii; i++) {
		ll_node_t *current_node = graph->neighbors[id]->head;
		for (int j = 0; (unsigned int)j < graph->neighbors[id]->size; j++) {
			if ((*posts)[(*posts)[pid].events->copii[i]].uid
				== *(int *)current_node->data)
				printf("%s\n", get_user_name(*(int *)current_node->data));
			current_node = current_node->next;
		}
	}
}

void bronkerbosch(list_graph_t *graph, linked_list_t *R, linked_list_t *P,
				  linked_list_t *X, linked_list_t *largest_clique)
{
	if (ll_get_size(P) == 0 && ll_get_size(X) == 0) {
		if (ll_get_size(R) > ll_get_size(largest_clique)) {
			while (ll_get_size(largest_clique) > 0) {
				ll_node_t *node = ll_remove_nth_node(largest_clique, 0);
				free(node->data);
				free(node);
			}
			ll_node_t *node = R->head;
			while (node) {
				ll_add_nth_node(largest_clique,
								ll_get_size(largest_clique), node->data);
				node = node->next;
			}
		}
		return;
	}

	ll_node_t *node = P->head;
	while (node) {
		int v = *(int *)(node->data);
		linked_list_t *new_R = ll_create(sizeof(int));
		linked_list_t *new_P = ll_create(sizeof(int));
		linked_list_t *new_X = ll_create(sizeof(int));

		ll_node_t *r_node = R->head;
		while (r_node) {
			ll_add_nth_node(new_R, ll_get_size(new_R), r_node->data);
			r_node = r_node->next;
		}
		ll_add_nth_node(new_R, ll_get_size(new_R), &v);

		ll_node_t *p_node = P->head;
		while (p_node) {
			int neighbor = *(int *)(p_node->data);
			ll_node_t *neighbor_node = graph->neighbors[v]->head;
			while (neighbor_node) {
				if (*(int *)neighbor_node->data == neighbor) {
					ll_add_nth_node(new_P, ll_get_size(new_P), &neighbor);
					break;
				}
				neighbor_node = neighbor_node->next;
			}
			p_node = p_node->next;
		}

		ll_node_t *x_node = X->head;
		while (x_node) {
			int neighbor = *(int *)(x_node->data);
			ll_node_t *neighbor_node = graph->neighbors[v]->head;
			while (neighbor_node) {
				if (*(int *)neighbor_node->data == neighbor) {
					ll_add_nth_node(new_X, ll_get_size(new_X), &neighbor);
					break;
				}
				neighbor_node = neighbor_node->next;
			}
			x_node = x_node->next;
		}

		bronkerbosch(graph, new_R, new_P, new_X, largest_clique);

		ll_free(&new_R);
		ll_free(&new_P);
		ll_free(&new_X);

		ll_node_t *tmp = ll_remove_nth_node(P, 0);
		free(tmp->data);
		free(tmp);
		ll_add_nth_node(X, ll_get_size(X), &v);

		node = P->head;
	}
}

void common_group(list_graph_t *graph, int node)
{
	linked_list_t *R = ll_create(sizeof(int));
	linked_list_t *P = ll_create(sizeof(int));
	linked_list_t *X = ll_create(sizeof(int));
	linked_list_t *largest_clique = ll_create(sizeof(int));

	ll_node_t *neighbor = graph->neighbors[node]->head;
	while (neighbor) {
		ll_add_nth_node(P, ll_get_size(P), neighbor->data);
		neighbor = neighbor->next;
	}

	ll_add_nth_node(R, ll_get_size(R), &node);

	bronkerbosch(graph, R, P, X, largest_clique);

	printf("The closest friend group of %s is:\n", get_user_name(node));
	ll_node_t *node_it = largest_clique->head;
	int nodes[largest_clique->size];
	int cnt = 0;
	while (node_it) {
		nodes[cnt] = *(int *)node_it->data;
		cnt++;
		node_it = node_it->next;
	}
	for (int i = 0; (unsigned int)i < largest_clique->size - 1; i++)
		for (int j = i + 1; (unsigned int)j < largest_clique->size; j++)
			if (nodes[i] > nodes[j]) {
				int aux = nodes[i];
				nodes[i] = nodes[j];
				nodes[j] = aux;
			}
	for (int i = 0; (unsigned int)i < largest_clique->size; i++)
		printf("%s\n", get_user_name(nodes[i]));
	ll_free(&R);
	ll_free(&P);
	ll_free(&X);
	ll_free(&largest_clique);
}

void handle_input_feed(char *input, list_graph_t *graph,
					   post_t **posts, int nr_posts)
{
	char *commands = strdup(input);
	char *cmd = strtok(commands, "\n ");

	if (!cmd)
		return;

	if (!strcmp(cmd, "feed")) {
		(void)cmd;
		cmd = strtok(NULL, "\n ");
		int id = get_user_id(cmd);
		cmd = strtok(NULL, "\n ");
		int feed_size = atoi(cmd);
		show_feed(graph, posts, id, feed_size, nr_posts);
		// TODO: Add function
	} else if (!strcmp(cmd, "view-profile")) {
		(void)cmd;
		cmd = strtok(NULL, "\n ");
		int id = get_user_id(cmd);
		view_profile(posts, id, nr_posts);
		// TODO: Add function
	} else if (!strcmp(cmd, "friends-repost")) {
		(void)cmd;
		cmd = strtok(NULL, "\n ");
		int id = get_user_id(cmd);
		cmd = strtok(NULL, "\n ");
		int pid = atoi(cmd);
		friends_reposted(graph, posts, id, pid);
		// TODO: Add function
	} else if (!strcmp(cmd, "common-group")) {
		(void)cmd;
		cmd = strtok(NULL, "\n ");
		int id = get_user_id(cmd);
		common_group(graph, id);
		// TODO: Add function
	}
	free(commands);
}
