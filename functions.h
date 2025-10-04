#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "graphlist.h"

void addfriend(int id1, int id2, list_graph_t *graph);

void removefriend(int id1, int id2, list_graph_t *graph);

void print_min_path(list_graph_t *graph, int src, int dest);

void suggestions(list_graph_t *lg, int node);

void common(int id1, int id2, list_graph_t *graph);

void number_friends(int id1, list_graph_t *graph);

void most_popular(int id1, list_graph_t *graph);

#endif
