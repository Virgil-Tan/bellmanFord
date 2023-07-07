#include <iostream>
#include <fstream>
#include <vector>
#include <limits>
#include <ctime>
#include <atomic>
#include <pthread.h>
#include <algorithm>
#include <iomanip>

using namespace std;

struct Edge {
    int src, dest, weight;
};

struct Graph {
    int V, E;
    vector<Edge> edges;
};

struct ThreadData {
    int start, end;
    Graph* graph;
    vector<atomic<int>>* distance;
};

void* bellmanFordPartial(void* args) {
    ThreadData* data = (ThreadData*)args;
    int start = data->start;
    int end = data->end;
    Graph* graph = data->graph;
    vector<atomic<int>>* distance = data->distance;

    bool changed = true;
    while (changed) {
        changed = false;
        for (int j = start; j < end; ++j) {
            int u = graph->edges[j].src;
            int v = graph->edges[j].dest;
            int weight = graph->edges[j].weight;

            int current_dist = (*distance)[v].load();
            int new_dist = (*distance)[u].load() + weight;
            while (new_dist < current_dist && !(*distance)[v].compare_exchange_weak(current_dist, new_dist)) {
                new_dist = (*distance)[u].load() + weight;
                changed = true;
            }
        }
    }

    return nullptr;
}

int main() {
    ifstream infile("/home/virgil/Documents/data/USA-road-d.FLA.gr");
//    ifstream infile("/home/virgil/CLionProjects/bellmanFord/mapDataTest.txt");
    if (!infile) {
        cerr << "Error opening the file." << endl;
        return 1;
    }

    int num_nodes, num_edges;
    infile >> num_nodes >> num_edges;
    num_nodes = num_nodes + 1;
    Graph graph;
    graph.V = num_nodes;
    graph.E = num_edges;

    cout << "Node num: " << num_nodes << ", edges: " << num_edges << endl;
    cout << "Running Bellman Ford's algorithm with starting node 1" << endl;

    for (int i = 0; i < num_edges; ++i) {
        int src, dest, weight;
        char skip_word;
        infile >> skip_word >> src >> dest >> weight;
        graph.edges.push_back({src, dest, weight});
    }

    infile.close();

    clock_t start, stop;
    double duration;
    start = clock();

    vector<atomic<int>> distance(num_nodes);
    for (int i = 0; i < num_nodes; ++i) {
        distance[i] = (i == 1) ? 0 : numeric_limits<int>::max();
    }

    int num_threads = 4;
    pthread_t threads[num_threads];
    ThreadData thread_data[num_threads];

    int edges_per_thread = (num_edges + num_threads - 1) / num_threads;  // ceil division
    for (int i = 0; i < num_threads; ++i) {
        thread_data[i].start = i * edges_per_thread;
        thread_data[i].end = min((i + 1) * edges_per_thread, num_edges);
        thread_data[i].graph = &graph;
        thread_data[i].distance = &distance;

        pthread_create(&threads[i], nullptr, bellmanFordPartial, &thread_data[i]);
    }

    for (int i = 0; i < num_threads; ++i) {
        pthread_join(threads[i], nullptr);
    }

    stop = clock();
    duration = ((double)(stop - start)) / CLOCKS_PER_SEC;

    cout << "Execution time: " << duration << endl;
    cout << "Finished" << endl;

    cout << "Shortest distances from node 1:" << endl;
    for (int i = 1; i < num_nodes; ++i) {
        if (distance[i] != numeric_limits<int>::max()) {
            cout << "Distance to node " << setw(2) << i << ": " << distance[i] << endl;
        } else {
            cout << "Distance to node " << setw(2) << i << ": INF" << endl;
        }
    }

    return 0;
}
