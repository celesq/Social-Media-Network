/**
 * The entrypoint of the homework. Every initialization must be done here
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "users.h"
#include "friends.h"
#include "posts.h"
#include "feed.h"

/**
 * Initializez every task based on which task we are running
*/
void init_tasks(void)
{
#ifdef TASK_1

#endif

#ifdef TASK_2

#endif

#ifdef TASK_3

#endif
}

/**
 * Entrypoint of the program, compiled with different defines for each task
*/
int main(void)
{
	init_users();

	init_tasks();

	int nr_posts = -1;
	nr_posts++;

	list_graph_t *graph = lg_create(MAX_PEOPLE);

	post_t *posts = calloc(1, sizeof(post_t));

	char *input = (char *)malloc(MAX_COMMAND_LEN);
	while (1) {
		char *command = fgets(input, MAX_COMMAND_LEN, stdin);

		// If fgets returns null, we reached EOF
		if (!command)
			break;

#ifdef TASK_1
		handle_input_friends(input, graph);
#endif

#ifdef TASK_2
		handle_input_posts(input, &posts, &nr_posts);
#endif

#ifdef TASK_3
		handle_input_feed(input, graph, &posts, nr_posts);
#endif
	}

	free_users();
	free(input);
	lg_free(graph);
	for (int i = 1; i <= nr_posts; i++)
		free_post(&posts[i]);
	free(posts);
	return 0;
}
