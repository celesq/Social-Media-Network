#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "users.h"
#include "posts.h"

void create_post(post_t **posts, int uid, char *title, int *id)
{
	*posts = realloc(*posts, ((*id + 1) * sizeof(post_t)));
	(*posts)[*id].events = calloc(1, sizeof(tree_t));
	(*posts)[*id].aprecieri = calloc(1, sizeof(like));
	(*posts)[*id].aprecieri->likes = calloc(1, sizeof(int));
	strcpy((*posts)[*id].title, title);
	(*posts)[*id].uid = uid;
	(*posts)[*id].id = *id;
	(*posts)[*id].events->copii = calloc(1, sizeof(int));
	printf("Created \"%s\" for %s\n", (*posts)[*id].title,
		   get_user_name((*posts)[*id].uid));
}

void create_repost(post_t **posts, int uid, int pid, int rid, int *id)
{
	*posts = realloc(*posts, ((*id + 1) * sizeof(post_t)));
	(*posts)[*id].events = calloc(1, sizeof(tree_t));
	(*posts)[*id].aprecieri = calloc(1, sizeof(like));
	(*posts)[*id].aprecieri->likes = calloc(1, sizeof(int));
	strcpy((*posts)[*id].title, "NULL");
	(*posts)[*id].uid = uid;
	(*posts)[*id].id = *id;
	(*posts)[*id].events->copii = calloc(1, sizeof(int));
	if (rid == -1) {
		(*posts)[*id].events->parinte = pid;
		(*posts)[pid].events->nr_copii++;
		(*posts)[pid].events->copii = realloc((*posts)[pid].events->copii,
		(*posts)[pid].events->nr_copii * sizeof(int));
		(*posts)[pid].events->copii[(*posts)[pid].events->nr_copii - 1] = *id;
	} else {
		(*posts)[*id].events->parinte = rid;
		(*posts)[rid].events->nr_copii++;
		(*posts)[rid].events->copii = realloc((*posts)[rid].events->copii,
		(*posts)[rid].events->nr_copii * sizeof(int));
		(*posts)[rid].events->copii[(*posts)[rid].events->nr_copii - 1] = *id;
	}
	printf("Created repost #%d for %s\n",
		   (*posts)[*id].id, get_user_name((*posts)[*id].uid));
}

void common_repost(post_t **post, int pid, int rid1, int rid2)
{
	linked_list_t *visited = ll_create(sizeof(int));
	int id = rid1;
	ll_add_nth_node(visited, visited->size, &id);
	while (id != pid) {
		id = (*post)[id].events->parinte;
		ll_add_nth_node(visited, visited->size, &id);
	}
	id = rid2;
	int found = 0;
	ll_node_t *current = visited->head;
	for (unsigned int i = 0; i < visited->size && !found; i++) {
		if (id == *(int *)current->data)
			found = 1;
		current = current->next;
	}
	while (!found) {
		id = (*post)[id].events->parinte;
		ll_node_t *current = visited->head;
		for (unsigned int i = 0; i < visited->size && !found; i++) {
			if (id == *(int *)current->data)
				found = 1;
			current = current->next;
		}
	}
	printf("The first common repost of %d and %d is %d\n", rid1, rid2, id);
	ll_free(&visited);
}

void print_reposts(post_t **post, int id)
{
	printf("Repost #%d by %s\n", (*post)[id].id,
		   get_user_name((*post)[id].uid));
	if ((*post)[id].events->nr_copii) {
		for (int i = 0; i < (*post)[id].events->nr_copii; i++)
			print_reposts(post, (*post)[id].events->copii[i]);
	}
}

void like_post(post_t **post, int uid, int pid, int rid)
{
	if (rid == -1) {
		int pos = -1, ok = 1;
		for (int i = 0; i < (*post)[pid].aprecieri->nr_likes && ok; i++) {
			if (uid == (*post)[pid].aprecieri->likes[i]) {
				ok = 0;
				pos = i;
			}
		}
		if (ok) {
			(*post)[pid].aprecieri->nr_likes++;
			(*post)[pid].aprecieri->likes =
			realloc((*post)[pid].aprecieri->likes,
					(*post)[pid].aprecieri->nr_likes * sizeof(int));
			int x = (*post)[pid].aprecieri->nr_likes - 1;
			(*post)[pid].aprecieri->likes[x] = uid;
			printf("User %s liked post \"%s\"\n",
				   get_user_name(uid), (*post)[pid].title);
		} else {
			for (int i = pos; i < (*post)[pid].aprecieri->nr_likes - 1; i++)
				(*post)[pid].aprecieri->likes[i] =
				(*post)[pid].aprecieri->likes[i + 1];
			(*post)[pid].aprecieri->nr_likes--;
			(*post)[pid].aprecieri->likes = realloc
			((*post)[pid].aprecieri->likes,
			(*post)[pid].aprecieri->nr_likes * sizeof(int));
			printf("User %s unliked post \"%s\"\n",
				   get_user_name(uid), (*post)[pid].title);
		}
	} else {
		int pos = -1, ok = 1;
		for (int i = 0; i < (*post)[rid].aprecieri->nr_likes && ok; i++) {
			if (uid == (*post)[rid].aprecieri->likes[i]) {
				ok = 0;
				pos = i;
			}
		}
		if (ok) {
			(*post)[rid].aprecieri->nr_likes++;
			(*post)[rid].aprecieri->likes = realloc
			((*post)[rid].aprecieri->likes,
			(*post)[rid].aprecieri->nr_likes * sizeof(int));
			int x = (*post)[rid].aprecieri->nr_likes - 1;
			(*post)[rid].aprecieri->likes[x] = uid;
			printf("User %s liked repost \"%s\"\n",
				   get_user_name(uid), (*post)[pid].title);
		} else {
			for (int i = pos; i < (*post)[rid].aprecieri->nr_likes - 1; i++)
				(*post)[rid].aprecieri->likes[i] =
				(*post)[rid].aprecieri->likes[i + 1];
			(*post)[rid].aprecieri->nr_likes--;
			(*post)[rid].aprecieri->likes = realloc
			((*post)[rid].aprecieri->likes,
			(*post)[rid].aprecieri->nr_likes * sizeof(int));
			printf("User %s unliked repost \"%s\"\n",
				   get_user_name(uid), (*post)[pid].title);
		}
	}
}

void get_repost(post_t **post, int pid, int rid)
{
	if (rid == -1) {
		printf("\"%s\" - Post by %s\n", (*post)[pid].title,
			   get_user_name((*post)[pid].uid));
		if ((*post)[pid].events->nr_copii) {
			for (int i = 0; i < (*post)[pid].events->nr_copii; i++)
				print_reposts(post, (*post)[pid].events->copii[i]);
		}
	} else {
		print_reposts(post, rid);
	}
}

void get_likes(post_t **post, int pid, int rid)
{
	if (rid == -1)
		printf("Post \"%s\" has %d likes\n",
			   (*post)[pid].title, (*post)[pid].aprecieri->nr_likes);
	else
		printf("Repost #%d has %d likes\n",
			   (*post)[rid].id, (*post)[rid].aprecieri->nr_likes);
}

void ratio(post_t **post, int pid)
{
	int pos = pid;
	for (int i = 0; i < (*post)[pid].events->nr_copii; i++) {
		if ((*post)[pos].aprecieri->nr_likes <
			(*post)[(*post)[pid].events->copii[i]].aprecieri->nr_likes)
			pos = (*post)[pid].events->copii[i];
	}
	if (pos == pid)
		printf("The original post is the highest rated\n");
	else
		printf("Post %d got ratio'd by repost %d\n", pid, pos);
}

void delete_repost(post_t **post, int id)
{
	(*post)[id].deleted = 1;
	for (int i = 0; i < (*post)[id].events->nr_copii; i++)
		delete_repost(post, (*post)[id].events->copii[i]);
}

void delete_post(post_t **post, int pid, int rid)
{
	if (rid == -1) {
		delete_repost(post, pid);
		printf("Deleted \"%s\"\n", (*post)[pid].title);
	} else {
		printf("Deleted repost #%d of post \"%s\"\n", rid, (*post)[pid].title);
		int parent = (*post)[rid].events->parinte, pos = -1;
		for (int i = 0; i < (*post)[parent].events->nr_copii; i++) {
			if ((*post)[parent].events->copii[i] == rid)
				pos = i;
		}
		for (int i = pos; i < (*post)[parent].events->nr_copii - 1; i++)
			(*post)[parent].events->copii[i] =
			(*post)[parent].events->copii[i + 1];
		(*post)[parent].events->nr_copii--;
		delete_repost(post, rid);
	}
}

void handle_input_posts(char *input, post_t **posts, int *nr_posts)
{
	char *commands = strdup(input);
	char *cmd = strtok(commands, "\n ");
	if (!cmd)
		return;
	if (!strcmp(cmd, "create")) {
		(void)cmd;
		cmd = strtok(NULL, " \"");
		int uid = get_user_id(cmd);
		cmd = strtok(NULL, "\"");
		(*nr_posts)++;
		create_post(posts, uid, cmd, nr_posts);
	} else if (!strcmp(cmd, "repost")) {
		(void)cmd;
		cmd = strtok(NULL, " ");
		int uid = get_user_id(cmd);
		cmd = strtok(NULL, " ");
		int pid = atoi(cmd);
		cmd = strtok(NULL, " ");
		int rid = -1;
		(*nr_posts)++;
		if (cmd)
			rid = atoi(cmd);
		create_repost(posts, uid, pid, rid, nr_posts);
	} else if (!strcmp(cmd, "common-repost")) {
		(void)cmd;
		cmd = strtok(NULL, " ");
		int pid = atoi(cmd);
		cmd = strtok(NULL, " ");
		int rid1 = atoi(cmd);
		cmd = strtok(NULL, " ");
		int rid2 = atoi(cmd);
		common_repost(posts, pid, rid1, rid2);
	} else if (!strcmp(cmd, "like")) {
		(void)cmd;
		cmd = strtok(NULL, " ");
		int uid = get_user_id(cmd);
		cmd = strtok(NULL, " ");
		int pid = atoi(cmd);
		cmd = strtok(NULL, " ");
		int rid = -1;
		if (cmd)
			rid = atoi(cmd);
		like_post(posts, uid, pid, rid);
	} else if (!strcmp(cmd, "ratio")) {
		(void)cmd;
		cmd = strtok(NULL, " ");
		int pid = atoi(cmd);
		ratio(posts, pid);
	} else if (!strcmp(cmd, "delete")) {
		(void)cmd;
		cmd = strtok(NULL, " ");
		int pid = atoi(cmd), rid = -1;
		cmd = strtok(NULL, " ");
		if (cmd)
			rid = atoi(cmd);
		delete_post(posts, pid, rid);
	} else if (!strcmp(cmd, "get-likes")) {
		(void)cmd;
		cmd = strtok(NULL, " ");
		int pid = atoi(cmd);
		cmd = strtok(NULL, " ");
		int rid = -1;
		if (cmd)
			rid = atoi(cmd);
		get_likes(posts, pid, rid);
	} else if (!strcmp(cmd, "get-reposts")) {
		(void)cmd;
		cmd = strtok(NULL, " ");
		int pid = atoi(cmd);
		cmd = strtok(NULL, " ");
		int rid = -1;
		if (cmd)
			rid = atoi(cmd);
		get_repost(posts, pid, rid);
	} else if (!strcmp(cmd, "get-likes")) {
		(void)cmd;
	}
	free(commands);
}
