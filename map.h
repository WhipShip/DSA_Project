#ifndef MAP_H
#define MAP_H
#include <fstream>   // For file input
#include <sstream>   // For robust input for numbers
#include <string>
#include <limits>
#include "graphV1.h"

using namespace std;


class Map {
private:
    string nodes_filename;
    string edges_filename;
    string university_name;

public:
    Map(string const& nodes_name, string const& edges_name) {
        nodes_filename = nodes_name;
        edges_filename = edges_name;
    }

    bool map_to_graph(Graph& graph) {
        // --- 1. Load Nodes ---
        ifstream nodesFile(nodes_filename);
        if (!nodesFile.is_open()) {
            cerr << "Error: Could not open nodes file '" << nodes_filename << "'" << endl;
            return false;
        }

        int id;
        string name;
        int max_node_id = -1; // To determine the total number of nodes for graph resizing

        // Read nodes in a simpler way: assuming ID and Name are space-separated, single words
        while (nodesFile >> id >> name) { // Simpler read using operator>>
            if (id < 0 || name.empty()) {
                cerr << "Error: Invalid node definition (ID or name empty) in nodes file. Line: '" << id << " " << name << "'" << endl;
                nodesFile.close();
                return false;
            }

            // If this is the university (ID 0), store its name
            if (id == 0) {
                university_name = name;
            }

            graph.addNode(id, name); // Add node to graph
            if (id > max_node_id) {
                max_node_id = id;
            }
        }
        nodesFile.close();

        // After reading all nodes, ensure graph has enough space.
        // The graph's numNodes will be max_node_id + 1
        graph.numNodes = max_node_id + 1; // Explicitly set numNodes
        graph.nodes_list.resize(graph.numNodes); // Resize nodes_list if necessary

        // This loop ensures any default-constructed nodes due to resize are given correct IDs
        // in case addNode didn't handle it precisely or for sparse IDs (not the case here)
        for (int i = 0; i <= max_node_id; ++i) {
            if (graph.nodes_list[i].id == -1) { // If it's a default-constructed node without ID set
                graph.nodes_list[i].id = i;
            }
        }

        // --- 2. Load Edges ---
        ifstream edgesFile(edges_filename);
        if (!edgesFile.is_open()) {
            cerr << "Error: Could not open edges file '" << edges_filename << "'" << endl;
            return false;
        }

        int source_id, dest_id;
        double weight;
        while (edgesFile >> source_id >> dest_id >> weight) {
            if (source_id < 0 || source_id >= graph.getNumNodes() ||
                dest_id < 0 || dest_id >= graph.getNumNodes() || weight < 0) {
                cerr << "Error: Invalid edge definition in edges file (ID out of range or invalid weight): "
                     << source_id << " " << dest_id << " " << weight << endl;
                edgesFile.close();
                return false;
            }
            graph.addEdge(source_id, dest_id, weight);
        }
        edgesFile.close();

        cout << "Successfully loaded map from '" << nodes_filename << "' and '" << edges_filename << "'." << endl;
        cout << "University stop set to: " << university_name << " (ID: 0)" << endl;
        return true;
    }

    void setNodesFilename(string const &name) { nodes_filename = name; }
    void setEdgesFilename(string const &name) { edges_filename = name; }
    string getNodesFilename() { return nodes_filename; }
    string getEdgesFilename() { return edges_filename; }
    string getUniversityName() { return university_name; }

    ~Map() {};
};

#endif // MAP_H
