#ifndef GRAPH_H
#define GRAPH_H

#include <algorithm>
#include <vector>
#include <list>
#include <string>
#include <unordered_map> // Keep for potential future name-to-ID mapping
#include <iostream>
#include <iomanip>
#include <limits>     // For std::numeric_limits
#include <fstream>    // Keep if you plan to load graph from file
#include <queue>
#include <sstream>    // Keep for potential string parsing
#include <stack>

// Using namespace std for convenience in this project file
using namespace std;

void clearInputBuffer() {
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

// Define INF constants
const double DOUBLE_INF = numeric_limits<double>::infinity();
const int INT_INF = numeric_limits<int>::max();

// Forward declarations (if Node/Edge were more complex or in separate files)
// class Node;
// class Edge;

// Edge class representing a route between two nodes
class Edge {
public:
    int destination_node_id; // Renamed for clarity
    double weight;           // Distance or time between nodes

    Edge(int dest_id, double w) : destination_node_id(dest_id), weight(w) {}
};

// Node class representing a bus stop
class Node {
public:
    int id;
    string name;
    list<Edge> edges;    // List of edges connected to this node

    // Default constructor
    Node() : id(-1), name("") {}

    // Parameterized constructor
    Node(int nodeId, const string& nodeName = "")
        : id(nodeId), name(nodeName){}
};



// Struct to hold pathfinding results
struct PathDetails {
    double total_weight = DOUBLE_INF;    // For Dijkstra's (e.g., distance, time)
    int num_stops = -1;                  // For BFS (number of segments/edges in path)
                                         // -1 or INT_INF can indicate no path or not applicable
    vector<int> node_ids_in_path;
    bool path_exists = false;

    // Default constructor
    PathDetails() = default;
};


// Graph class using adjacency list representation
class Graph {
public:
    vector<Node> nodes_list; // Renamed for clarity to avoid conflict with a 'nodes' variable name
    int numNodes;
    // Constructor
    Graph(int n = 0) : numNodes(n) {
        nodes_list.resize(n);
        for (int i = 0; i < n; i++) {
            // Initialize with basic Node object, ID will be set by addNode or directly
            nodes_list[i].id = i;
        }
    }

    // Add a node to the graph (or update if exists)
    void addNode(int id, const string& name = "") {
        if (id >= numNodes) {
            // Resize if ID is out of current bounds, new nodes will be default constructed
            // Ensure new nodes get their ID set if this strategy is used extensively.
            nodes_list.resize(id + 1);
            for(int i = numNodes; i <= id; ++i) {
                nodes_list[i].id = i; // Explicitly set ID for new default nodes
            }
            numNodes = id + 1;
        }
        // Now update the node at 'id'
        nodes_list[id] = Node(id, name);
    }

    // Add an edge between two nodes (undirected)
    void addEdge(int source_id, int destination_id, double weight) {
        if (source_id >= numNodes || destination_id >= numNodes || source_id < 0 || destination_id < 0) {
            cerr << "addEdge: Node index out of bounds. Ensure nodes are added before edges." << endl;
        }

        nodes_list[source_id].edges.push_back(Edge(destination_id, weight));
        nodes_list[destination_id].edges.push_back(Edge(source_id, weight)); // Assuming undirected
    }

    // Get the number of nodes
    int getNumNodes() const { return numNodes; }

    // Get a node by its ID (const reference)
    const Node& getNode(int id) const {
        if (id >= numNodes || id < 0) {
            cerr << "getNode: Node index out of bounds" << endl;
        }
        return nodes_list[id];
    }


    int getNodeIndexByname(string const& name) {
        for (int i = 0; i < numNodes; i++) {
            if (nodes_list[i].name == name) {
                return i;
            }
        }
        return -1;
    }

    // Get all edges of a node
    const list<Edge>& getEdges(int nodeId) const {
        if (nodeId >= numNodes || nodeId < 0) {
            cerr << "getEdges: Node index out of bounds" << endl;
        }
        return nodes_list[nodeId].edges;
    }

    // Create the adjacency matrix (Commented out as not essential for core Dijkstra/BFS with adjacency lists)
    typedef vector<vector<double>> AdjacencyMatrix;
    AdjacencyMatrix createAdjacencyMatrix() const {
        vector<vector<double>> matrix(numNodes, vector<double>(numNodes, DOUBLE_INF));

        for (int i = 0; i < numNodes; ++i) {
            matrix[i][i] = 0.0; // Distance to self is 0
        }

        for (int i = 0; i < numNodes; i++) {
            for (const Edge& edge : nodes_list[i].edges) {
                matrix[i][edge.destination_node_id] = edge.weight;
            }
        }
        return matrix;
    }

    // Print the adjacency matrix (Commented out)
    void printAdjacencyMatrix() const {
        vector<vector<double>> matrix = createAdjacencyMatrix();

        cout << "\nAdjacency Matrix (distances between stops):\n";
        cout << setw(6) << " "; // Adjusted for node names potentially
        for (int i = 0; i < numNodes; i++) {
            cout << setw(8) << nodes_list[i].name.substr(0,7) ; // Print part of name
        }
        cout << "\n";

        for (int i = 0; i < numNodes; i++) {
            cout << setw(5) << nodes_list[i].name.substr(0,4) << " |";
            for (int j = 0; j < numNodes; j++) {
                if (matrix[i][j] == DOUBLE_INF) {
                    cout << setw(8) << "INF";
                } else {
                    cout << setw(8) << fixed << setprecision(1) << matrix[i][j];
                }
            }
            cout << "\n";
        }
        cout << endl;
    }

    PathDetails Dijkstra(int startNodeId, int endNodeId) {
        PathDetails result;
        result.path_exists = false;

        if (startNodeId >= numNodes || endNodeId >= numNodes || startNodeId < 0 || endNodeId < 0) {
            cerr << "Error: Invalid start or end node ID in Dijkstra." << endl;
            return result;
        }

        if (startNodeId == endNodeId) {
            result.path_exists = true;
            result.node_ids_in_path.push_back(startNodeId);
            result.num_stops = 0;
            result.total_weight = 0.0;
            return result;
        }

        vector<double> distances(numNodes, DOUBLE_INF);
        vector<int> previous(numNodes, -1);
        vector<bool> visited(numNodes, false);

        distances[startNodeId] = 0;

        priority_queue<pair<double, int>, vector<pair<double, int>>, greater<pair<double, int>>> pq;
        pq.push({0.0, startNodeId});

        while (!pq.empty()) {
            double removed_distance = pq.top().first;
            int removed_node_id = pq.top().second;
            pq.pop();
            visited[removed_node_id] = true;
            for (const Edge& edge : nodes_list[removed_node_id].edges) {
                if (visited[edge.destination_node_id]) {
                    continue;
                }
                double new_distance = removed_distance + edge.weight;
                if (new_distance < distances[edge.destination_node_id]) {
                    distances[edge.destination_node_id] = new_distance;
                    previous[edge.destination_node_id] = removed_node_id;
                    pq.push({new_distance, edge.destination_node_id});
                }
            }
        }
        if (distances[endNodeId] != DOUBLE_INF) {
            result.path_exists = true;
            result.total_weight = distances[endNodeId];

            stack<int> path_stack;
            int current_node_id = endNodeId;
            while (current_node_id != -1) {
                path_stack.push(current_node_id);
                current_node_id = previous[current_node_id];
            }
            while (!path_stack.empty()) {
                result.node_ids_in_path.push_back(path_stack.top());
                path_stack.pop();
            }
            result.num_stops = static_cast<int>(result.node_ids_in_path.size()) - 1;
        }
        return result;
    }

    PathDetails BFS(int startNodeId, int endNodeId) {
        PathDetails result;
        result.path_exists = false; // Assume no path initially
        // Default: num_stops = -1, total_weight = DOUBLE_INF

        // Error Handling
        if (startNodeId >= numNodes || endNodeId >= numNodes || startNodeId < 0 || endNodeId < 0) {
            cerr << "Error: Invalid start or end node ID in BFS." << endl;
            return result;
        }

        // Error Handling
        if (startNodeId == endNodeId) {
            result.path_exists = true;
            result.node_ids_in_path.push_back(startNodeId);
            result.num_stops = 0;
            result.total_weight = 0.0;
            return result;
        }

        // Start of BFS
        queue<int> q;
        q.push(startNodeId);

        vector<bool> visited(numNodes, false);
        visited[startNodeId] = true;

        vector<int> prev(numNodes, -1);

        bool path_found_to_end_node = false; // Flag to indicate if endNodeId was reached

        while (!q.empty()) {
            int u_node_id = q.front();
            q.pop();

            if (u_node_id == endNodeId) {
                path_found_to_end_node = true;
                break;
            }

            // Explore neighbors
            for (const Edge& edge : nodes_list[u_node_id].edges) {
                int v_node_id = edge.destination_node_id;
                if (!visited[v_node_id]) {
                    visited[v_node_id] = true;
                    prev[v_node_id] = u_node_id;
                    q.push(v_node_id);
                }
            }
        }

        // Reconstruct path if the end node was reached
        if (path_found_to_end_node) {
            stack<int> path_stack;
            int current_node_id = endNodeId;
            while (current_node_id != -1) {
                path_stack.push(current_node_id);
                current_node_id = prev[current_node_id];
            }
            result.path_exists = true;
            while (!path_stack.empty()) {
                result.node_ids_in_path.push_back(path_stack.top());
                path_stack.pop();
            }
            result.num_stops = static_cast<int>(result.node_ids_in_path.size()) - 1;
        }
        return result;
    }
};

#endif
