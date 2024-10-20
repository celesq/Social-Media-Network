#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "friends.h"
#include "users.h"

void addfriend(int id1, int id2, list_graph_t *graph)
{
	lg_add_edge(graph, id1, id2);
	lg_add_edge(graph, id2, id1);
	printf("Added connection %s - %s\n", get_user_name(id1),
		   get_user_name(id2));
}

void removefriend(int id1, int id2, list_graph_t *graph)
{
	lg_remove_edge(graph, id1, id2);
	lg_remove_edge(graph, id2, id1);
	printf("Removed connection %s - %s\n", get_user_name(id1),
		   get_user_name(id2));
}

void print_min_path(list_graph_t *graph, int src, int dest)
{
	int *parents = calloc(graph->nodes, sizeof(int));
	int *visited = calloc(graph->nodes, sizeof(int));
	queue_t *coada = q_create(sizeof(int), graph->nodes);
	q_enqueue(coada, &src);

	while (visited[dest] == 0 && coada->size) {
		ll_node_t *head =
		graph->neighbors[*(int *)coada->buff[coada->read_idx]]->head;
		int parent = *(int *)coada->buff[coada->read_idx];
		for (int i = 0; (unsigned int)i <
			 graph->neighbors[*(int *)coada->buff[coada->read_idx]]->size;
			 i++) {
			if (visited[*(int *)head->data] == 0) {
				visited[*(int *)head->data] = 1;
				parents[*(int *)head->data] = parent;
				q_enqueue(coada, head->data);
			}
			head = head->next;
		}
		q_dequeue(coada);
	}
	q_free(coada);
	if (visited[dest] == 1) {
		int nod = dest;
		int distanta = 0;
		int pos = 1, *drum = malloc(sizeof(int) * graph->nodes);
		drum[0] = nod;
		while (nod != src) {
			nod = parents[nod];
			drum[pos] = nod;
			pos++;
		}
		for (int i = pos - 1; i >= 0; i--)
			distanta++;
		distanta--;
		free(drum);
		printf("The distance between %s - %s is %d\n",
			   get_user_name(src), get_user_name(dest), distanta);
	} else {
		printf("There is no way to get from %s to %s\n",
			   get_user_name(src), get_user_name(dest));
	}
	free(parents);
	free(visited);
}

void suggestions(list_graph_t *lg, int node)
{
	queue_t *q = q_create(sizeof(int), lg->nodes);
	q_enqueue(q, &node);
	int *visited = calloc(lg->nodes, sizeof(int));
	int *suggestion = calloc(lg->nodes, sizeof(int));
	int len = 0;
	int rounds = lg->neighbors[node]->size + 1;
	visited[node] = 1;
	while (q_is_empty(q) == 0 && rounds) {
		int *node1 = q_front(q);
		ll_node_t *current_node = lg->neighbors[*node1]->head;
		for (int i = 0; (unsigned int)i < lg->neighbors[*node1]->size; i++) {
			if (visited[*(int *)current_node->data] == 0) {
				visited[*(int *)current_node->data] = 1;
				if (node != *node1) {
					suggestion[len] = *(int *)current_node->data;
					len++;
				}
				q_enqueue(q, current_node->data);
			}
			current_node = current_node->next;
		}
		q_dequeue(q);
		rounds--;
	}
	q_free(q);
	if (len == 0) {
		printf("There are no suggestions for %s\n", get_user_name(node));
	} else {
		printf("Suggestions for %s:\n", get_user_name(node));
		for (int i = 0; i < len - 1; i++)
			for (int j = i + 1; j < len; j++)
				if (suggestion[i] > suggestion[j]) {
					int aux = suggestion[i];
					suggestion[i] = suggestion[j];
					suggestion[j] = aux;
				}
		for (int i = 0; i < len; i++)
			printf("%s\n", get_user_name(suggestion[i]));
	} free(visited);

	free(suggestion);
}

void common(int id1, int id2, list_graph_t *graph)
{
	int ok = 0, len = 0;
	int common[550];
	ll_node_t *current_node = graph->neighbors[id1]->head;
	for (int i = 0; (unsigned int)i < graph->neighbors[id1]->size; i++) {
		ll_node_t *friend_node = graph->neighbors[id2]->head;
		for (int j = 0; (unsigned int)j < graph->neighbors[id2]->size; j++) {
			if (*(int *)friend_node->data == *(int *)current_node->data) {
				if (ok == 0)
					printf("The common friends between %s and %s are:\n",
						   get_user_name(id1), get_user_name(id2));
				ok = 1;
				common[len] = *(int *)current_node->data;
				len++;
			}
			friend_node = friend_node->next;
		}
		current_node = current_node->next;
	}
	for (int i = 0; i < len - 1; i++)
		for (int j = i + 1; j < len; j++)
			if (common[i] > common[j]) {
				int aux = common[i];
				common[i] = common[j];
				common[j] = aux;
			}
	for (int i = 0; i < len; i++)
		printf("%s\n", get_user_name(common[i]));
	if (ok == 0)
		printf("No common friends for %s and %s\n",
			   get_user_name(id1), get_user_name(id2));
}

void number_friends(int id1, list_graph_t *graph)
{
	printf("%s has %d friends\n",
		   get_user_name(id1), graph->neighbors[id1]->size);
}

void most_popular(int id1, list_graph_t *graph)
{
	int max = 0, id_max = 0;
	if (graph->neighbors[id1]->size > (unsigned int)max) {
		max = graph->neighbors[id1]->size;
		id_max = id1;
	}
	ll_node_t *current_node = graph->neighbors[id1]->head;
	for (unsigned int i = 0; i < graph->neighbors[id1]->size; i++) {
		if (graph->neighbors[*(int *)current_node->data]->size >
			(unsigned int)max) {
			max = graph->neighbors[*(int *)current_node->data]->size;
			id_max = *(int *)current_node->data;
		}
		current_node = current_node->next;
	}
	if (id_max == id1)
		printf("%s is the most popular\n", get_user_name(id1));
	else
		printf("%s is the most popular friend of %s\n",
			   get_user_name(id_max), get_user_name(id1));
}

void handle_input_friends(char *input, list_graph_t *graph)
{
	char *commands = strdup(input);
	char *cmd = strtok(commands, "\n ");

	if (!cmd)
		return;

	if (!strcmp(cmd, "add")) {
		(void)cmd;
		cmd = strtok(NULL, "\n ");
		int id1, id2;
		id1 = get_user_id(cmd);
		cmd = strtok(NULL, "\n ");
		id2 = get_user_id(cmd);
		addfriend(id1, id2, graph);
	} else if (!strcmp(cmd, "remove")) {
		(void)cmd;
		cmd = strtok(NULL, "\n ");
		int id1, id2;
		id1 = get_user_id(cmd);
		cmd = strtok(NULL, "\n ");
		id2 = get_user_id(cmd);
		removefriend(id1, id2, graph);
	} else if (!strcmp(cmd, "suggestions")) {
		(void)cmd;
		cmd = strtok(NULL, "\n ");
		int id1 = get_user_id(cmd);
		suggestions(graph, id1);
	} else if (!strcmp(cmd, "distance")) {
		(void)cmd;
		cmd = strtok(NULL, "\n ");
		int id1, id2;
		id1 = get_user_id(cmd);
		cmd = strtok(NULL, "\n ");
		id2 = get_user_id(cmd);
		print_min_path(graph, id1, id2);
	} else if (!strcmp(cmd, "common")) {
		(void)cmd;
		cmd = strtok(NULL, "\n ");
		int id1, id2;
		id1 = get_user_id(cmd);
		cmd = strtok(NULL, "\n ");
		id2 = get_user_id(cmd);
		common(id1, id2, graph);
	} else if (!strcmp(cmd, "friends")) {
		(void)cmd;
		cmd = strtok(NULL, "\n ");
		int id1 = get_user_id(cmd);
		number_friends(id1, graph);
	} else if (!strcmp(cmd, "popular")) {
		(void)cmd;
		cmd = strtok(NULL, "\n ");
		int id1 = get_user_id(cmd);
		most_popular(id1, graph);
	}
	free(commands);
}
