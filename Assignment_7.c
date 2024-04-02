#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define MAX_VERTICES 100

// Structure to represent an adjacency list node
struct AdjListNode {
    int dest;
    int weight;
    struct AdjListNode* next;
};

// Structure to represent an adjacency list
struct AdjList {
    struct AdjListNode *head;
};

// Structure to represent a graph
struct Graph {
    int V;
    struct AdjList* array;
};

// Structure to represent a node in priority queue
struct MinHeapNode {
    int v;
    int key;
};

// Structure to represent a priority queue
struct MinHeap {
    int size;
    int capacity;
    int *pos;
    struct MinHeapNode **array;
};

// Function to create a new adjacency list node
struct AdjListNode* newAdjListNode(int dest, int weight) {
    struct AdjListNode* newNode = (struct AdjListNode*)malloc(sizeof(struct AdjListNode));
    newNode->dest = dest;
    newNode->weight = weight;
    newNode->next = NULL;
    return newNode;
}

// Function to create a graph with V vertices
struct Graph* createGraph(int V) {
    struct Graph* graph = (struct Graph*)malloc(sizeof(struct Graph));
    graph->V = V;
    graph->array = (struct AdjList*)malloc(V * sizeof(struct AdjList));
    for (int i = 0; i < V; ++i)
        graph->array[i].head = NULL;
    return graph;
}

// Function to add an edge to an undirected graph
void addEdge(struct Graph* graph, int src, int dest, int weight) {
    struct AdjListNode* newNode = newAdjListNode(dest, weight);
    newNode->next = graph->array[src].head;
    graph->array[src].head = newNode;

    newNode = newAdjListNode(src, weight);
    newNode->next = graph->array[dest].head;
    graph->array[dest].head = newNode;
}

// Function to create a new min heap node
struct MinHeapNode* newMinHeapNode(int v, int key) {
    struct MinHeapNode* minHeapNode = (struct MinHeapNode*)malloc(sizeof(struct MinHeapNode));
    minHeapNode->v = v;
    minHeapNode->key = key;
    return minHeapNode;
}

// Function to create a min heap
struct MinHeap* createMinHeap(int capacity) {
    struct MinHeap* minHeap = (struct MinHeap*)malloc(sizeof(struct MinHeap));
    minHeap->pos = (int*)malloc(capacity * sizeof(int));
    minHeap->size = 0;
    minHeap->capacity = capacity;
    minHeap->array = (struct MinHeapNode**)malloc(capacity * sizeof(struct MinHeapNode*));
    return minHeap;
}

// Function to swap two min heap nodes
void swapMinHeapNode(struct MinHeapNode** a, struct MinHeapNode** b) {
    struct MinHeapNode* t = *a;
    *a = *b;
    *b = t;
}

// Function to heapify at a given index
void minHeapify(struct MinHeap* minHeap, int idx) {
    int smallest, left, right;
    smallest = idx;
    left = 2 * idx + 1;
    right = 2 * idx + 2;

    if (left < minHeap->size && minHeap->array[left]->key < minHeap->array[smallest]->key)
        smallest = left;

    if (right < minHeap->size && minHeap->array[right]->key < minHeap->array[smallest]->key)
        smallest = right;

    if (smallest != idx) {
        struct MinHeapNode* smallestNode = minHeap->array[smallest];
        struct MinHeapNode* idxNode = minHeap->array[idx];

        minHeap->pos[smallestNode->v] = idx;
        minHeap->pos[idxNode->v] = smallest;

        swapMinHeapNode(&minHeap->array[smallest], &minHeap->array[idx]);

        minHeapify(minHeap, smallest);
    }
}

// Function to check if the given min heap is empty
bool isEmpty(struct MinHeap* minHeap) {
    return minHeap->size == 0;
}

// Function to extract the minimum node from the heap
struct MinHeapNode* extractMin(struct MinHeap* minHeap) {
    if (isEmpty(minHeap))
        return NULL;

    struct MinHeapNode* root = minHeap->array[0];

    struct MinHeapNode* lastNode = minHeap->array[minHeap->size - 1];
    minHeap->array[0] = lastNode;

    minHeap->pos[root->v] = minHeap->size - 1;
    minHeap->pos[lastNode->v] = 0;

    --minHeap->size;
    minHeapify(minHeap, 0);

    return root;
}

// Function to decrease the key value of a given vertex
void decreaseKey(struct MinHeap* minHeap, int v, int key) {
    int i = minHeap->pos[v];

    minHeap->array[i]->key = key;

    while (i && minHeap->array[i]->key < minHeap->array[(i - 1) / 2]->key) {
        minHeap->pos[minHeap->array[i]->v] = (i - 1) / 2;
        minHeap->pos[minHeap->array[(i - 1) / 2]->v] = i;
        swapMinHeapNode(&minHeap->array[i], &minHeap->array[(i - 1) / 2]);

        i = (i - 1) / 2;
    }
}

// Function to check if a given vertex is in min heap
bool isInMinHeap(struct MinHeap *minHeap, int v) {
   if (minHeap->pos[v] < minHeap->size)
     return true;
   return false;
}

// Function to print the constructed MST stored in parent[]
void printMST(int parent[], int n, int **graph) {
   printf("Edge   Weight\n");
   for (int i = 1; i < n; i++)
      printf("%d - %d    %d \n", parent[i], i, graph[i][parent[i]]);
}

// Function to construct and print MST for a graph represented using adjacency
// matrix representation
void primMST(int **graph, int V) {
    int parent[V]; // Array to store constructed MST
    int key[V];    // Key values used to pick minimum weight edge in cut
    struct MinHeap* minHeap = createMinHeap(V);

    // Initialize min heap with all vertices. Key value of all vertices (except 0th vertex) is initially infinite
    for (int v = 1; v < V; ++v) {
        parent[v] = -1;
        key[v] = 10000;
        minHeap->array[v] = newMinHeapNode(v, key[v]);
        minHeap->pos[v] = v;
    }

    // Make key value of 0th vertex as 0 so that it is extracted first
    key[0] = 0;
    minHeap->array[0] = newMinHeapNode(0, key[0]);
    minHeap->pos[0] = 0;

    minHeap->size = V;

    // In the following loop, min heap contains all nodes not yet added to MST.
    while (!isEmpty(minHeap)) {
        // Extract the vertex with minimum key value
        struct MinHeapNode* minHeapNode = extractMin(minHeap);
        int u = minHeapNode->v; // Store the extracted vertex number

        // Traverse through all adjacent vertices of u (the extracted vertex) and update their key values
        struct AdjListNode* pCrawl = graph[u];
        while (pCrawl != NULL) {
            int v = pCrawl->dest;

            // If v is not yet included in MST and the weight of the edge u-v is less than the key value of v, update key value and parent index of v
            if (isInMinHeap(minHeap, v) && pCrawl->weight < key[v]) {
                key[v] = pCrawl->weight;
                parent[v] = u;
                decreaseKey(minHeap, v, key[v]);
            }
            pCrawl = pCrawl->next;
        }
    }

    // Print the constructed MST
    printMST(parent, V, graph);
}

// Function to perform DFS traversal recursively
void DFSUtil(int v, bool visited[], struct Graph* graph) {
    visited[v] = true;
    printf("%d ", v);

    struct AdjListNode* pCrawl = graph->array[v].head;
    while (pCrawl != NULL) {
        if (!visited[pCrawl->dest])
            DFSUtil(pCrawl->dest, visited, graph);
        pCrawl = pCrawl->next;
    }
}

// Function to perform DFS traversal
void DFS(struct Graph* graph, int start) {
    bool* visited = (bool*)calloc(graph->V, sizeof(bool));

    printf("DFS traversal starting from vertex %d: ", start);
    DFSUtil(start, visited, graph);
    printf("\n");

    free(visited);
}

// Function to perform BFS traversal
void BFS(struct Graph* graph, int start) {
    bool* visited = (bool*)calloc(graph->V, sizeof(bool));

    visited[start] = true;
    int queue[MAX_VERTICES];
    int front = -1, rear = -1;
    queue[++rear] = start;

    printf("BFS traversal starting from vertex %d: ", start);
    while (front != rear) {
        int v = queue[++front];
        printf("%d ", v);

        struct AdjListNode* pCrawl = graph->array[v].head;
        while (pCrawl != NULL) {
            if (!visited[pCrawl->dest]) {
                visited[pCrawl->dest] = true;
                queue[++rear] = pCrawl->dest;
            }
            pCrawl = pCrawl->next;
        }
    }
    printf("\n");

    free(visited);
}

int main() {
    int V = 5;
    struct Graph* graph = createGraph(V);
    addEdge(graph, 0, 1, 2);
    addEdge(graph, 0, 3, 6);
    addEdge(graph, 0, 2, 4);
    addEdge(graph, 1, 2, 5);
    addEdge(graph, 2, 3, 1);
    addEdge(graph, 2, 4, 2);
    addEdge(graph, 3, 7, 4);
    addEdge(graph, 3, 5, 3);
    addEdge(graph, 4, 6, 5);
    addEdge(graph, 4, 5, 1);
    addEdge(graph, 4, 8, 3);
    addEdge(graph, 5, 6, 4);


    printf("BFS Traversal:\n");
    BFS(graph, 0);

    printf("\nDFS Traversal:\n");
    DFS(graph, 0);

    printf("\nMinimum Spanning Tree (Prim's Algorithm):\n");
    primMST(graph->array, V);

    return 0;
}
