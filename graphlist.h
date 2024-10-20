#ifndef GRAPHLIST_H
#define GRAPHLIST_H

#define DIE(assertion, call_description)				\
		do {								\
			if (assertion) {					\
				fprintf(stderr, "(%s, %d): ",			\
				__FILE__, __LINE__);		\
				perror(call_description);			\
				exit(errno);					\
			}							\
		} while (0)

typedef  struct ll_node_t {
	void *data;
	struct ll_node_t *next;
} ll_node_t;

typedef struct {
	ll_node_t *head;
	unsigned int data_size;
	unsigned int size;
} linked_list_t;

typedef struct {
	linked_list_t **neighbors; /* Listele de adiacenta ale grafului */
	int nodes;                 /* Numarul de noduri din graf. */
} list_graph_t;

typedef struct {
	unsigned int max_size;
	unsigned int size;
	unsigned int data_size;
	unsigned int read_idx;
	unsigned int write_idx;
	void **buff;
} queue_t;

linked_list_t *ll_create(unsigned int data_size);

ll_node_t *get_nth_node(linked_list_t *list, unsigned int n);

void ll_add_nth_node(linked_list_t *list, unsigned int n, const void *new_data);

ll_node_t *ll_remove_nth_node(linked_list_t *list, unsigned int n);

unsigned int ll_get_size(linked_list_t *list);

void ll_free(linked_list_t **pp_list);

void ll_print_int(linked_list_t *list);

void ll_print_string(linked_list_t *list);

list_graph_t *lg_create(int nodes);

void lg_add_edge(list_graph_t *graph, int src, int dest);

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

#endif
