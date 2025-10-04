#ifndef FRIENDS_H
#define FRIENDS_H

#include "users.h"

#define MAX_COMMAND_LEN 500
#define MAX_PEOPLE 550

/**
 * Function that handles the calling of every command from task 1
 *
 * Please add any necessary parameters to the functions
*/

void addfriend(int id1, int id2, list_graph_t *graph);

void removefriend(int id1, int id2, list_graph_t *graph);

void print_min_path(list_graph_t *graph, int src, int dest);

void suggestions(list_graph_t *lg, int node);

void common(int id1, int id2, list_graph_t *graph);

void number_friends(int id1, list_graph_t *graph);

void most_popular(int id1, list_graph_t *graph);

void handle_input_friends(char *input, list_graph_t *graph);

#endif // FRIENDS_H
