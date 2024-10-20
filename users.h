#ifndef USERS_H
#define USERS_H

#include <stdint.h>

/**
 * Initializes the user list
*/
void init_users(void);

/**
 * Find the user_id of a user by it's name
 *
 * @param name - The name of the user
 * @return the id of the user, of -1 if name is not found
*/
uint16_t get_user_id(char *name);

/**
 * Find the user_id of a user by it's name
 *
 * @param id - The id of a user
 * @return the name of a user, of NULL if not found
*/
char *get_user_name(uint16_t id);

/**
 * Frees the user list
*/
void free_users(void);

#define DIE(assertion, call_description)				\
		do {								\
			if (assertion) {					\
				fprintf(stderr, "(%s, %d): ",			\
				__FILE__, __LINE__);		\
				perror(call_description);			\
				exit(errno);					\
			}							\
		} while (0)

#define MAX_STRING_SIZE 281

typedef struct ll_node_t {
	void *data;
	struct ll_node_t *next;
} ll_node_t;

typedef struct linked_list_t {
	ll_node_t *head;
	unsigned int data_size;
	unsigned int size;
} linked_list_t;

typedef struct list_graph_t {
	linked_list_t **neighbors; /* Listele de adiacenta ale grafului */
	int nodes;                 /* Numarul de noduri din graf. */
} list_graph_t;

typedef struct queue_t {
	unsigned int max_size;
	unsigned int size;
	unsigned int data_size;
	unsigned int read_idx;
	unsigned int write_idx;
	void **buff;
} queue_t;

linked_list_t *ll_create(unsigned int data_size);

ll_node_t *get_nth_node(linked_list_t *list, unsigned int n);

ll_node_t *create_node(const void *new_data, unsigned int data_size);

void ll_add_nth_node(linked_list_t *list, unsigned int n, const void *new_data);

ll_node_t *ll_remove_nth_node(linked_list_t *list, unsigned int n);

unsigned int ll_get_size(linked_list_t *list);

void ll_free(linked_list_t **pp_list);

void ll_print_int(linked_list_t *list);

void ll_print_string(linked_list_t *list);

list_graph_t *lg_create(int nodes);

void lg_add_edge(list_graph_t *graph, int src, int dest);

int is_node_in_graph(int n, int nodes);

ll_node_t *find_node(linked_list_t *ll, int node, unsigned int *pos);

linked_list_t *lg_get_neighbours(list_graph_t *graph, int node);

int lg_has_edge(list_graph_t *graph, int src, int dest);

void lg_remove_edge(list_graph_t *graph, int src, int dest);

void lg_free(list_graph_t *graph);

void lg_print_graph(list_graph_t *graph);

queue_t *q_create(unsigned int data_size, unsigned int max_size);
unsigned int q_get_size(queue_t *q);
unsigned int q_is_empty(queue_t *q);
void *q_front(queue_t *q);
int q_dequeue(queue_t *q);
int q_enqueue(queue_t *q, void *new_data);
void q_clear(queue_t *q);
void q_free(queue_t *q);

typedef struct tree_t {
	int parinte, nr_copii;
	int *copii;
} tree_t;

typedef struct like {
	int nr_likes;
	int *likes;
} like;

typedef struct post_t {
	int id, uid, deleted;
	char title[MAX_STRING_SIZE];
	tree_t *events;
	like *aprecieri;

} post_t;

void free_post(post_t *post);

#endif
