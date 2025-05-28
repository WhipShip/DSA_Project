#include <iostream>
#include <fstream>   // For file input
#include <string>
#include <vector>
#include <unordered_map>
#include <limits>    // For numeric_limits
#include <sstream>   // For robust input for numbers
#include "graphV1.h"   // Your graph header
#include "map.h"

using namespace std;


// Function to append a new node to the nodes file
void appendNodeToFile(const Node& node, const string& filename) {
    ofstream outFile(filename, ios::app); // Open in append mode
    if (!outFile.is_open()) {
        cerr << "Error: Could not open nodes file for appending: '" << filename << "'" << endl;
        return;
    }
    outFile << endl << node.id << " " << node.name;
    outFile.close();
    cout << "Node '" << node.name << "' appended to " << filename << endl;
}

// Function to append a new edge to the edges file
void appendEdgeToFile(int source_id, int dest_id, double weight, const string& filename) {
    ofstream outFile(filename, ios::app); // Open in append mode
    if (!outFile.is_open()) {
        cerr << "Error: Could not open edges file for appending: '" << filename << "'" << endl;
        return;
    }
    outFile << endl << source_id << " " << dest_id << " " << weight;
    outFile.close();
    cout << "Edge " << source_id << " " << dest_id << " " << weight << " appended to " << filename << endl;
}


void handleAddData(Graph& graph, const string& nodes_filename, const string& edges_filename) {
    cout << "\n--- Add New Location/Route ---" << endl;
    cout << "Do you want to add a (1) New Location or (2) New Route? Enter 1 or 2: ";
    int choice;
    while (!(cin >> choice) || (choice != 1 && choice != 2)) {
        cout << "Invalid input. Please enter 1 or 2: ";
        clearInputBuffer();
    }
    clearInputBuffer(); // Clear buffer after reading choice

    if (choice == 1) { // Add New Location (Node)
        cout << "Enter the name for the new location: ";
        string new_location_name;
        getline(cin, new_location_name);

        if (new_location_name.empty()) {
            cout << "Location name cannot be empty. Aborting." << endl;
            return;
        }

        if (graph.getNodeIndexByname(new_location_name) != -1) {
            cout << "Location '" << new_location_name << "' already exists. Aborting." << endl;
            return;
        }

        int new_node_id = graph.getNumNodes(); // Get the next available ID
        graph.addNode(new_node_id, new_location_name);
        cout << "Successfully added new location: " << new_location_name << " with ID: " << new_node_id << endl;

        // Persist the new node to file
        appendNodeToFile(graph.getNode(new_node_id), nodes_filename);

    } else { // Add New Route (Edge)
        string source_name, dest_name;
        double weight;

        cout << "Enter the name of the source location: ";
        getline(cin, source_name);
        int source_id = graph.getNodeIndexByname(source_name);
        if (source_id == -1) {
            cout << "Source location '" << source_name << "' not found. Aborting." << endl;
            return;
        }

        cout << "Enter the name of the destination location: ";
        getline(cin, dest_name);
        int dest_id = graph.getNodeIndexByname(dest_name);
        if (dest_id == -1) {
            cout << "Destination location '" << dest_name << "' not found. Aborting." << endl;
            return;
        }

        // Prevent adding an edge to itself
        if (source_id == dest_id) {
            cout << "Cannot add a route from a location to itself. Aborting." << endl;
            return;
        }

        cout << "Enter the weight (cost/time) of the route between " << source_name << " and " << dest_name << ": ";
        while (!(cin >> weight) || weight < 0) {
            cout << "Invalid weight. Please enter a non-negative number: ";
            clearInputBuffer();
        }
        clearInputBuffer();

        // Add the edge (assuming undirected, as in your graph setup)
        graph.addEdge(source_id, dest_id, weight);
        cout << "Successfully added route between " << source_name << " and " << dest_name << " with weight " << weight << endl;
        // Persist the new edge(s) to file
        appendEdgeToFile(source_id, dest_id, weight, edges_filename);

    }
    cout << "----------------------------\n" << endl;
}



void displayPathDetails(const PathDetails& path, Graph* graph) {
    cout << "\n--- Route Details ---" << endl;
    if (!path.path_exists) {
        cout << "No path found." << endl;
        return;
    }

    cout << "Path: ";
    bool is_first_node = true;
    for (int current_node_id : path.node_ids_in_path) { // Range-based for loop
        if (!is_first_node) {
            cout << " -> "; // Print separator before the element (but not for the first one)
        }
        cout << graph->getNode(current_node_id).name;
        is_first_node = false; // After processing the first node, set flag to false
    }
    cout << endl;

    if (path.num_stops != -1) {
        cout << "Number of stops (segments): " << path.num_stops << endl;
    }
    if (path.total_weight != DOUBLE_INF) {
        cout << "Total cost (time/distance): " << path.total_weight << endl;
    }
    cout << "---------------------\n" << endl;
}

int main() {
    Graph bus_network;


    cout << "Enter the filename for nodes (e.g., nodes.txt): ";
    string nodes_filename;
    cin >> nodes_filename;

    cout << "Enter the filename for edges (e.g., edges.txt): ";
    string edges_filename;
    cin >> edges_filename;

    // Assign files to map using the new constructor
    Map map1(nodes_filename, edges_filename);

    // Load map to graph
    if (!map1.map_to_graph(bus_network)) {
        cerr << "Failed to load map. Exiting." << endl;
        return 1; // Indicate an error
    }

    while (true) {
        cout << "\nUniversity Commute Optimizer Menu:" << endl;
        // University is now always ID 0, and its name is obtained from the Map object
        cout << "Destination is fixed to: " << map1.getUniversityName() << " (ID: 0)" << endl;

        cout << "1. Find Fastest Route (Dijkstra)" << endl;
        cout << "2. Find Route with Minimum Stops (BFS)" << endl;
        cout << "3. Add new location/route to map" << endl; // New Option
        cout << "4. Print Current Graph Map" << endl;       // New utility option
        cout << "5. Exit" << endl;
        cout << "Enter your choice: ";

        int main_choice;
        while (!(cin >> main_choice) || main_choice < 1 || main_choice > 5  ) {
            cout << "Invalid input. Please enter a positive number: ";
            clearInputBuffer();
        }

        string start_stop;
        // The university's ID is now always 0
        const int UNIVERSITY_NODE_ID = 0;

        switch (main_choice) {
            case 1: // Find Fastest Route
                cout << "Enter your starting location name (e.g., Home, CentralStation): ";
                cin >> start_stop;
                if (bus_network.getNodeIndexByname(start_stop) != -1) {
                    displayPathDetails(bus_network.Dijkstra(bus_network.getNodeIndexByname(start_stop), UNIVERSITY_NODE_ID), &bus_network);
                } else {
                    cout << "Starting location '" << start_stop << "' not found in the map." << endl;
                }
                break;
            case 2: // Find Route with Minimum Stops
                cout << "Enter your starting location name (e.g., Home, CentralStation): ";
                cin >> start_stop;
                cout << "\nFinding route with minimum stops from " << start_stop << " to " << map1.getUniversityName() << "..." << endl;
                if (bus_network.getNodeIndexByname(start_stop) != -1) {
                    displayPathDetails(bus_network.BFS(bus_network.getNodeIndexByname(start_stop), UNIVERSITY_NODE_ID), &bus_network);
                } else {
                    cout << "Starting location '" << start_stop << "' not found in the map." << endl;
                }
                break;
            case 3:
                handleAddData(bus_network, nodes_filename, edges_filename);
                break;
            case 4: // Print Graph
                bus_network.printAdjacencyMatrix();
                break;
            case 5: // Exit
                cout << "Exiting program. Safe travels!" << endl;
                return 0;
            default:
                cout << "Invalid choice. Please try again." << endl;
                break;
        }
    }
}
