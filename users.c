#include "users.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static const char *db_path = "users.db";

static char **users;
static uint16_t users_number;

void init_users(void)
{
	FILE *users_db = fopen(db_path, "r");

	if (!users_db) {
		perror("Error reading users.db");
		return;
	}

	fscanf(users_db, "%hu", &users_number);

	users = malloc(users_number * sizeof(char *));

	char temp[32];
	for (uint16_t i = 0; i < users_number; i++) {
		fscanf(users_db, "%s", temp);
		int size = strlen(temp);

		users[i] = malloc(size + 1);
		strcpy(users[i], temp);
	}

	fclose(users_db);
}

uint16_t get_user_id(char *name)
{
	if (!users)
		return -1;

	for (uint16_t i = 0; i < users_number; i++)
		if (!strcmp(users[i], name))
			return i;

	return -1;
}

char *get_user_name(uint16_t id)
{
	if (id >= users_number)
		return NULL;

	return users[id];
}

void free_users(void)
{
	for (size_t i = 0; i < users_number; i++)
		free(users[i]);

	free(users);
}

typedef struct stack_t stack;
struct stack_t {
	struct linked_list_t *list;
};

stack *st_create(unsigned int data_size)
{
	stack *st = malloc(sizeof(*st));
	st->list = ll_create(data_size);

	return st;
}

unsigned int st_get_size(stack *st)
{
	if (!st || !st->list)
		return 0;
	return st->list->size;
}

/*
 * Intoarce 1 daca stiva este goala si 0 in caz contrar.
 */
unsigned int st_is_empty(stack *st)
{
	return !st || !st->list || !st->list->size;
}

void *st_peek(stack *st)
{
	if (!st || !st->list || !st->list->size)
		return NULL;

	return st->list->head->data;
}

void st_pop(stack *st)
{
	if (!st || !st->list)
		return;

	ll_remove_nth_node(st->list, 0);
}

void st_push(stack *st, void *new_data)
{
	if (!st || !st->list)
		return;

	ll_add_nth_node(st->list, 0, new_data);
}

void st_clear(stack *st)
{
	if (!st || !st->list)
		return;

	ll_free(&st->list);
}

void st_free(stack *st)
{
	if (!st || !st->list)
		return;

	ll_free(&st->list);
	free(st);
}

#define MAX_USERS 520

/* --- LINKED LIST SUPPORT START --- */

linked_list_t *ll_create(unsigned int data_size)
{
	linked_list_t *ll;

	ll = malloc(sizeof(*ll));

	ll->head = NULL;
	ll->data_size = data_size;
	ll->size = 0;

	return ll;
}

/*
 * Elimina nodul de pe pozitia n din lista al carei pointer este trimis ca
 * parametru. Pozitiile din lista se indexeaza de la 0 (i.e. primul nod din
 * lista se afla pe pozitia n=0). Daca n >= nr_noduri - 1, se returneaza
 * ultimul nod.
 */
ll_node_t *get_nth_node(linked_list_t *list, unsigned int n)
{
	if (!list)
		return NULL;

	unsigned int len = list->size - 1;
	unsigned int i;
	ll_node_t *node = list->head;

	n = (n < len ? n : len);

	for (i = 0; i < n; ++i)
		node = node->next;

	return node;
}

ll_node_t *create_node(const void *new_data, unsigned int data_size)
{
	ll_node_t *node = calloc(1, sizeof(*node));
	DIE(!node, "calloc node");

	node->data = malloc(data_size);
	DIE(!node->data, "malloc data");

	memcpy(node->data, new_data, data_size);

	return node;
}

/*
 * Pe baza datelor trimise prin pointerul new_data, se creeaza un nou nod care e
 * adaugat pe pozitia n a listei reprezentata de pointerul list. Pozitiile din
 * lista sunt indexate incepand cu 0 (i.e. primul nod din lista se afla pe
 * pozitia n=0). Daca n >= nr_noduri, noul nod se adauga la finalul listei. Daca
 * n < 0, eroare.
 */
void ll_add_nth_node(linked_list_t *list, unsigned int n, const void *new_data)
{
	ll_node_t *new_node, *prev_node;

	if (!list)
		return;

	new_node = create_node(new_data, list->data_size);

	if (!n || !list->size) {
		new_node->next = list->head;
		list->head = new_node;
	} else {
		prev_node = get_nth_node(list, n - 1);
		new_node->next = prev_node->next;
		prev_node->next = new_node;
	}

	++list->size;
}

/*
 * Elimina nodul de pe pozitia n din lista al carei pointer este trimis ca
 * parametru. Pozitiile din lista se indexeaza de la 0 (i.e. primul nod din
 * lista se afla pe pozitia n=0). Daca n >= nr_noduri - 1, se elimina nodul de
 * la finalul listei. Daca n < 0, eroare. Functia intoarce un pointer spre acest
 * nod proaspat eliminat din lista. Este responsabilitatea apelantului sa
 * elibereze memoria acestui nod.
 */
ll_node_t *ll_remove_nth_node(linked_list_t *list, unsigned int n)
{
	ll_node_t *prev, *curr;

	if (!list || !list->head)
		return NULL;

	/* n >= list->size - 1 inseamna eliminarea nodului de la finalul listei. */
	if (n > list->size - 1)
		n = list->size - 1;

	curr = list->head;
	prev = NULL;
	while (n > 0) {
		prev = curr;
		curr = (ll_node_t *)curr->next;
		--n;
	}

	if (!prev) {
		/* Adica n == 0. */
		list->head = (ll_node_t *)curr->next;
	} else {
		prev->next = curr->next;
	}

	list->size--;

	return curr;
}

/*
 * Functia intoarce numarul de noduri din lista al carei pointer este trimis ca
 * parametru.
 */
unsigned int ll_get_size(linked_list_t *list)
{
	if (!list)
		return -1;

	return list->size;
}

/*
 * Procedura elibereaza memoria folosita de toate nodurile din lista, iar la
 * sfarsit, elibereaza memoria folosita de structura lista si actualizeaza la
 * NULL valoarea pointerului la care pointeaza argumentul (argumentul este un
 * pointer la un pointer).
 */
void ll_free(linked_list_t **pp_list)
{
	ll_node_t *currnode;

	if (!pp_list || !*pp_list)
		return;

	while (ll_get_size(*pp_list) > 0) {
		currnode = ll_remove_nth_node(*pp_list, 0);
		free(currnode->data);
		currnode->data = NULL;
		free(currnode);
		currnode = NULL;
	}

	free(*pp_list);
	*pp_list = NULL;
}

/*
 * Atentie! Aceasta functie poate fi apelata doar pe liste ale caror noduri STIM
 * ca stocheaza int-uri. Functia afiseaza toate valorile int stocate in nodurile
 * din lista inlantuita separate printr-un spatiu.
 */
void ll_print_int(linked_list_t *list)
{
	ll_node_t *curr;

	if (!list)
		return;

	curr = list->head;
	while (curr) {
		printf("%d ", *((int *)curr->data));
		curr = (ll_node_t *)curr->next;
	}

	printf("\n");
}

/*
 * Atentie! Aceasta functie poate fi apelata doar pe liste ale caror noduri STIM
 * ca stocheaza string-uri. Functia afiseaza toate string-urile stocate in
 * nodurile din lista inlantuita, separate printr-un spatiu.
 */
void ll_print_string(linked_list_t *list)
{
	ll_node_t *curr;

	if (!list)
		return;

	curr = list->head;
	while (curr) {
		printf("%s ", (char *)curr->data);
		curr = (ll_node_t *)curr->next;
	}

	printf("\n");
}

/**
 * Initializeaza graful cu numarul de noduri primit ca parametru si aloca
 * memorie pentru lista de adiacenta a grafului.
 */
list_graph_t *lg_create(int nodes)
{
	int i;

	list_graph_t *g = malloc(sizeof(*g));
	DIE(!g, "malloc graph failed");

	g->neighbors = malloc(nodes * sizeof(*g->neighbors));
	DIE(!g->neighbors, "malloc neighbours failed");

	for (i = 0; i != nodes; ++i)
		g->neighbors[i] = ll_create(sizeof(int));

	g->nodes = nodes;

	return g;
}

/* Adauga o muchie intre nodurile primite ca parametri */
void lg_add_edge(list_graph_t *graph, int src, int dest)
{
	if (!graph || !graph->neighbors || !is_node_in_graph(src, graph->nodes) ||
		!is_node_in_graph(dest, graph->nodes))
		return;

	ll_add_nth_node(graph->neighbors[src], graph->neighbors[src]->size, &dest);
}

int is_node_in_graph(int n, int nodes)
{
	return n >= 0 && n < nodes;
}

ll_node_t *find_node(linked_list_t *ll, int node, unsigned int *pos)
{
	ll_node_t *crt = ll->head;
	unsigned int i;

	for (i = 0; i != ll->size; ++i) {
		if (node == *(int *)crt->data) {
			*pos = i;
			return crt;
		}

		crt = crt->next;
	}

	return NULL;
}

/* Returneaza 1 daca exista muchie intre cele doua noduri, 0 in caz contrar */
int lg_has_edge(list_graph_t *graph, int src, int dest)
{
	unsigned int pos;

	if (!graph || !graph->neighbors || !is_node_in_graph(src, graph->nodes) ||
		!is_node_in_graph(dest, graph->nodes))
		return 0;

	return *(int *)find_node(graph->neighbors[src], dest, &pos);
}

/* Elimina muchia dintre nodurile primite ca parametri */
linked_list_t *lg_get_neighbours(list_graph_t *graph, int node)
{
	if (!graph || !graph->neighbors || !is_node_in_graph(node, graph->nodes))
		return NULL;

	return graph->neighbors[node];
}

void lg_remove_edge(list_graph_t *graph, int src, int dest)
{
	unsigned int pos;

	if (!graph || !graph->neighbors || !is_node_in_graph(src, graph->nodes) ||
		!is_node_in_graph(dest, graph->nodes))
		return;

	if (!find_node(graph->neighbors[src], dest, &pos))
		return;

	ll_node_t *removednode = ll_remove_nth_node(graph->neighbors[src], pos);
	free(removednode->data);
	free(removednode);
}

/* Elibereaza memoria folosita de lista de adiacenta a grafului */
void lg_free(list_graph_t *graph)
{
	int i;

	for (i = 0; i != graph->nodes; ++i)
		ll_free(graph->neighbors + i);

	free(graph->neighbors);
	free(graph);
}

/* Printeaza lista de adiacenta a grafului
 */
void lg_print_graph(list_graph_t *graph)
{
	for (int i = 0; i < graph->nodes; i++) {
		printf("%d: ", i);
		ll_node_t *current_node = graph->neighbors[i]->head;
		for (unsigned int j = 0; j < graph->neighbors[i]->size; j++) {
			printf("%d ", *(int *)current_node->data);
			current_node = (ll_node_t *)current_node->next;
		}
		printf("\n");
	}
}

queue_t *
q_create(unsigned int data_size, unsigned int max_size)
{
	queue_t *q = calloc(1, sizeof(*q));
	DIE(!q, "calloc queue failed");

	q->data_size = data_size;
	q->max_size = max_size;

	q->buff = malloc(max_size * sizeof(*q->buff));
	DIE(!q->buff, "malloc buffer failed");

	return q;
}

unsigned int
q_get_size(queue_t *q)
{
	return !q ? 0 : q->size;
}

unsigned int
q_is_empty(queue_t *q)
{
	return !q ? 1 : !q->size;
}

void *
q_front(queue_t *q)
{
	if (!q || !q->size)
		return NULL;

	return q->buff[q->read_idx];
}

int q_dequeue(queue_t *q)
{
	if (!q || !q->size)
		return 0;

	free(q->buff[q->read_idx]);

	q->read_idx = (q->read_idx + 1) % q->max_size;
	--q->size;
	return 1;
}

int q_enqueue(queue_t *q, void *new_data)
{
	void *data;
	if (!q || q->size == q->max_size)
		return 0;

	data = malloc(q->data_size);
	DIE(!data, "malloc data failed");
	memcpy(data, new_data, q->data_size);

	q->buff[q->write_idx] = data;
	q->write_idx = (q->write_idx + 1) % q->max_size;
	++q->size;

	return 1;
}

void q_clear(queue_t *q)
{
	unsigned int i;
	if (!q || !q->size)
		return;

	for (i = q->read_idx; i != q->write_idx; i = (i + 1) % q->max_size)
		free(q->buff[i]);

	q->read_idx = 0;
	q->write_idx = 0;
	q->size = 0;
}

void q_free(queue_t *q)
{
	if (!q)
		return;

	q_clear(q);
	free(q->buff);
	free(q);
}

void free_post(post_t *post)
{
	free(post->events->copii);
	free(post->events);
	free(post->aprecieri->likes);
	free(post->aprecieri);
}
