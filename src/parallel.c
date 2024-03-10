// SPDX-License-Identifier: BSD-3-Clause

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "log/log.h"
#include "os_graph.h"
#include "os_threadpool.h"
#include "utils.h"

#define NUM_THREADS 4

static int sum;
static os_graph_t *graph;
static os_threadpool_t *tp;
/* TODO: Define graph synchronization mechanisms. */
pthread_mutex_t graph_mutex;
/* TODO: Define graph task argument. */
void *task_arg;

struct timespec ts = { .tv_sec = 0, .tv_nsec = 1000000 };

static void process_node(unsigned int idx);

void process_node_wrapper(void *arg) { process_node((*((unsigned int *)arg))); }

static void process_node(unsigned int idx)
{
	/* TODO: Implement thread-pool based processing of graph. */
	os_node_t *node;

	pthread_mutex_lock(&graph_mutex);
	node = graph->nodes[idx];
	sum += node->info;
	// nanosleep(&ts, NULL);
	graph->visited[idx] = DONE;

	for (unsigned int i = 0; i < node->num_neighbours; i++)
		if (graph->visited[node->neighbours[i]] == NOT_VISITED) {
			graph->visited[node->neighbours[i]] = PROCESSING;
			task_arg = (unsigned int *)malloc(sizeof(unsigned int));
			*(unsigned int *)(task_arg) = node->neighbours[i];
			os_task_t *t = create_task(&process_node_wrapper, task_arg, &free);

			enqueue_task(tp, t);
			pthread_cond_signal(&(tp->aux_wait_cond));
		}
	pthread_mutex_unlock(&graph_mutex);
}

int main(int argc, char *argv[])
{
	FILE *input_file;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s input_file\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	input_file = fopen(argv[1], "r");
	DIE(input_file == NULL, "fopen");

	graph = create_graph_from_file(input_file);

	/* TODO: Initialize graph synchronization mechanisms. */
	pthread_mutex_init(&graph_mutex, NULL);
	tp = create_threadpool(NUM_THREADS);

	// creating task to process node 0
	graph->visited[0] = PROCESSING;
	task_arg = (unsigned int *)malloc(sizeof(unsigned int));
	*(unsigned int *)(task_arg) = 0;
	os_task_t *t = create_task(&process_node_wrapper, task_arg, &free);

	// enqueue first task
	enqueue_task(tp, t);

	// flag for secondary threads to know that the first task has been added => they can start working
	tp->started_working = 1;
	pthread_cond_broadcast(&(tp->aux_wait_cond));

	wait_for_completion(tp);
	destroy_threadpool(tp);
	pthread_mutex_destroy(&graph_mutex);

	printf("%d", sum);

	return 0;
}
