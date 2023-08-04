#include <iostream>
#include <fstream>
#include <vector>
#include <pthread.h>
#include <limits>


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
    vector<int>* distance;
    pthread_barrier_t* barrier;
};


void* bellmanFordPartial(void* args) {
    ThreadData* data = (ThreadData*)args;
    int start = data->start;
    int end = data->end;
    Graph* graph = data->graph;
    vector<int>* distance = data->distance;
    pthread_barrier_t* barrier = data->barrier;


    for (int i = 0; i < graph->V - 1; ++i) {
        for (int j = start; j < end; ++j) {
            int u = graph->edges[j].src;
            int v = graph->edges[j].dest;
            int weight = graph->edges[j].weight;


            if ((*distance)[u] != numeric_limits<int>::max() && (*distance)[u] + weight < (*distance)[v]) {
                (*distance)[v] = (*distance)[u] + weight;
            }
        }


        // Wait for all threads to finish their updates
        pthread_barrier_wait(barrier);
    }


    return nullptr;
}


int main() {
    ifstream infile("/home/virgil/CLionProjects/untitled/USA-road-d.NY.gr");
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


    vector<int> distance(num_nodes, numeric_limits<int>::max());
    distance[1] = 0; // Assuming the source node is 0


    int num_threads = 4;
    pthread_t threads[num_threads];
    ThreadData thread_data[num_threads];
    pthread_barrier_t barrier;


    pthread_barrier_init(&barrier, nullptr, num_threads);


    int edges_per_thread = (num_edges + num_threads - 1) / num_threads;


    for (int i = 0; i < num_threads; ++i) {
        thread_data[i].start = i * edges_per_thread;
        thread_data[i].end = min((i + 1) * edges_per_thread, num_edges);
        thread_data[i].graph = &graph;
        thread_data[i].distance = &distance;
        thread_data[i].barrier = &barrier;


        pthread_create(&threads[i], nullptr, bellmanFordPartial, &thread_data[i]);
    }


    for (int i = 0; i < num_threads; ++i) {
        pthread_join(threads[i], nullptr);
    }


    stop = clock();
    duration = ((double)(stop - start)) / CLOCKS_PER_SEC;

    cout << "Vertex\tDistance from Source" << endl;
    for (int i = 1; i < num_nodes; ++i) {
        cout << i << "\t\t" << distance[i] << endl;
    }
    cout << duration << endl;
    cout << "finished" << endl;

    return 0;
}
