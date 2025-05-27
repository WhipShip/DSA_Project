#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <limits>
#include <sstream>
#include <stdio.h>
#include <iomanip> // Needed for std::fixed and std::setprecision

// Your graph and map headers
#include "graphV1.h"
#include "map.h"

// ImGui and its backends
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_internal.h> // For ImGui::GetWindowDrawList() if not already included implicitly

// Define these flags at the top for ImGui features
#define ImGuiConfigFlags_DockingEnable          (1 << 20)
// Removed: #define ImGuiConfigFlags_ViewportsEnable        (1 << 10)

// Use namespace std to avoid std:: prefixes
using namespace std;


// Global or accessible objects for graph and map data
Graph bus_network;
Map* map_instance = nullptr;

// Buffers for ImGui text input
char start_location_input[256] = "";

// Buffers for displaying path details
string path_display_text = "No path calculated yet.";
string add_data_status_text = ""; // To display status of add operations


// displayPathDetails now updates a string for GUI display
void displayPathDetails(const PathDetails& path, Graph* graph) {
    ostringstream oss;

    oss << "--- Route Details ---" << endl;
    if (!path.path_exists) {
        oss << "No path found." << endl;
        path_display_text = oss.str();
        return;
    }

    oss << "Path: ";
    bool is_first_node = true;
    for (int current_node_id : path.node_ids_in_path) {
        if (!is_first_node) {
            oss << " -> ";
        }
        oss << graph->getNode(current_node_id).name;
        is_first_node = false;
    }
    oss << endl;

    if (path.num_stops != -1) {
        oss << "Number of stops (segments): " << path.num_stops << endl;
    }
    if (path.total_weight != DOUBLE_INF) {
        oss << "Total cost (time/distance): " << path.total_weight << endl;
    }
    oss << "---------------------\n" << endl;

    path_display_text = oss.str();
}

// Function to append a new node to the nodes file
void appendNodeToFile(const Node& node, const string& filename) {
    ofstream outFile(filename, ios::app);
    if (!outFile.is_open()) {
        // Corrected string concatenation using '+'
        add_data_status_text = "Error: Could not open nodes file for appending: '" + filename + "'";
        return;
    }
    outFile << endl << node.id << " " << node.name; // Output to file with endl
    outFile.close();
    // Corrected string concatenation using '+'
    add_data_status_text = "Node '" + node.name + "' appended to " + filename;
}

// Function to append a new edge to the edges file
void appendEdgeToFile(int source_id, int dest_id, double weight, const string& filename) {
    ofstream outFile(filename, ios::app);
    if (!outFile.is_open()) {
        // Corrected string concatenation using '+'
        add_data_status_text = "Error: Could not open edges file for appending: '" + filename + "'";
        return;
    }
    outFile << endl << source_id << " " << dest_id << " " << fixed << setprecision(1) << weight; // Output to file with endl
    outFile.close();
    // Corrected string concatenation using '+'
    add_data_status_text = "Edge " + to_string(source_id) + " " + to_string(dest_id) + " " + to_string(weight) + " appended to " + filename;
}

// Global char buffers for add data inputs
char new_location_name_input[256] = "";
char source_name_input[256] = "";
char dest_name_input[256] = "";
float new_weight_input = 0.0f;

void handleAddDataGUI(Graph& graph, const string& nodes_filename, const string& edges_filename) {
    // This function will be called within an ImGui::Begin/End block
    ImGui::Text("Add New Location or Route:");
    ImGui::Separator();

    // Add New Location (Node)
    ImGui::Text("Add New Location:");
    ImGui::InputText("##NewLocationName", new_location_name_input, IM_ARRAYSIZE(new_location_name_input));
    ImGui::SameLine();
    if (ImGui::Button("Add Location")) {
        string new_location_name_str(new_location_name_input);
        if (new_location_name_str.empty()) {
            add_data_status_text = "Error: Location name cannot be empty.";
        } else if (graph.getNodeIndexByname(new_location_name_str) != -1) {
            add_data_status_text = "Error: Location '" + new_location_name_str + "' already exists.";
        } else {
            int new_node_id = graph.getNumNodes();
            graph.addNode(new_node_id, new_location_name_str);
            appendNodeToFile(graph.getNode(new_node_id), nodes_filename); // Persist
            add_data_status_text = "Successfully added new location: " + new_location_name_str + " (ID: " + to_string(new_node_id) + ")";
            new_location_name_input[0] = '\0'; // Clear input field
        }
    }

    ImGui::Spacing();
    ImGui::Separator();

    // Add New Route (Edge)
    ImGui::Text("Add New Route:");
    ImGui::InputText("Source Name", source_name_input, IM_ARRAYSIZE(source_name_input));
    ImGui::InputText("Destination Name", dest_name_input, IM_ARRAYSIZE(dest_name_input));
    ImGui::InputFloat("Weight", &new_weight_input, 0.1f, 1.0f, "%.1f"); // For float input

    if (ImGui::Button("Add Route")) {
        string source_name_str(source_name_input);
        string dest_name_str(dest_name_input);

        int source_id = graph.getNodeIndexByname(source_name_str);
        if (source_id == -1) {
            add_data_status_text = "Error: Source location '" + source_name_str + "' not found.";
        } else {
            int dest_id = graph.getNodeIndexByname(dest_name_str);
            if (dest_id == -1) {
                add_data_status_text = "Error: Destination location '" + dest_name_str + "' not found.";
            } else if (source_id == dest_id) {
                add_data_status_text = "Error: Cannot add route to itself.";
            } else if (new_weight_input < 0) {
                 add_data_status_text = "Error: Weight cannot be negative.";
            }
            else {
                try {
                    graph.addEdge(source_id, dest_id, new_weight_input);
                    appendEdgeToFile(source_id, dest_id, new_weight_input, edges_filename); // Persist
                    add_data_status_text = "Successfully added route between " + source_name_str + " and " + dest_name_str + " with weight " + to_string(new_weight_input);
                    source_name_input[0] = '\0'; // Clear input fields
                    dest_name_input[0] = '\0';
                    new_weight_input = 0.0f;
                } catch (const out_of_range& e) {
                    add_data_status_text = "Error adding edge: " + string(e.what());
                }
            }
        }
    }
    ImGui::TextWrapped("Status: %s", add_data_status_text.c_str());
}


// Function to handle GLFW errors
void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

int main() {
    // 1. Initialize GLFW
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) {
        cerr << "Failed to initialize GLFW!" << endl;
        return 1;
    }

    // 2. Configure OpenGL Context (for ImGui OpenGL3 backend)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);

    // 3. Create a GLFW window
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Commute Optimizer GUI", NULL, NULL);
    if (window == NULL) {
        cerr << "Failed to create GLFW window!" << endl;
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // 4. Load OpenGL functions using GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        cerr << "Failed to initialize GLAD!" << endl;
        glfwTerminate();
        return 1;
    }

    // 5. Setup ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // 6. Setup ImGui style (more customization options)
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    // Example: Make window background slightly transparent for a cooler look
    style.Colors[ImGuiCol_WindowBg].w = 0.9f;
    // Example: Change button colors
    style.Colors[ImGuiCol_Button] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);

    // 7. Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");


    // --- File loading UI and logic ---
    bool map_loaded = false;
    char nodes_filename_buffer[256] = "nodes.txt";
    char edges_filename_buffer[256] = "edges.txt";

    while (!map_loaded && !glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::Begin("Load Map Files", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);
        ImGui::Text("Please enter map filenames:");
        ImGui::InputText("Nodes File", nodes_filename_buffer, IM_ARRAYSIZE(nodes_filename_buffer));
        ImGui::InputText("Edges File", edges_filename_buffer, IM_ARRAYSIZE(edges_filename_buffer));

        if (ImGui::Button("Load Map")) {
            if (map_instance) delete map_instance;
            map_instance = new Map(nodes_filename_buffer, edges_filename_buffer);

            if (map_instance->map_to_graph(bus_network)) {
                map_loaded = true;
                cout << "Map loaded successfully for GUI." << endl;
            } else {
                cerr << "Failed to load map. Check filenames." << endl;
                delete map_instance;
                map_instance = nullptr;
            }
        }
        ImGui::End();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    if (!map_loaded) {
        if (map_instance) {
            delete map_instance;
            map_instance = nullptr;
        }
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }


    // Main GUI loop
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    const int UNIVERSITY_NODE_ID = 0;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 8. Main Commute Optimizer UI
        ImGui::Begin("University Commute Optimizer", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        // --- Header Section with Colors/Shapes ---
        ImGui::TextColored(ImVec4(0.8f, 0.2f, 0.2f, 1.0f), "Welcome to Your Commute Optimizer!");
        ImGui::SameLine(0.0f, ImGui::GetContentRegionAvail().x - 120);
        ImGui::TextDisabled("v1.0.0");

        ImGui::Dummy(ImVec2(0.0f, 5.0f));
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 p = ImGui::GetCursorScreenPos();
        draw_list->AddRectFilled(p, ImVec2(p.x + ImGui::GetContentRegionAvail().x, p.y + 3.0f), ImU32(IM_COL32(0, 100, 150, 255)));
        ImGui::Dummy(ImVec2(0.0f, 5.0f));

        ImGui::Text("Destination: %s (ID: %d)", map_instance->getUniversityName().c_str(), UNIVERSITY_NODE_ID);
        ImGui::Separator();

        // --- Route Finding Section ---
        ImGui::Text("Find Your Route:");
        ImGui::InputText("##StartLoc", start_location_input, IM_ARRAYSIZE(start_location_input));
        ImGui::SetItemTooltip("Enter your starting location name here.");

        ImGui::Spacing();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.5f, 0.1f, 1.0f));
        if (ImGui::Button("Find Fastest Route (Dijkstra)", ImVec2(200, 30))) {
            string start_stop_name(start_location_input);
            int start_node_id = bus_network.getNodeIndexByname(start_stop_name);
            if (start_node_id != -1) {
                displayPathDetails(bus_network.Dijkstra(start_node_id, UNIVERSITY_NODE_ID), &bus_network);
            } else {
                path_display_text = "Error: Starting location '" + start_stop_name + "' not found in the map.";
            }
        }
        ImGui::PopStyleColor(3);

        ImGui::SameLine(0.0f, 10.0f);

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.4f, 0.1f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.5f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.3f, 0.0f, 1.0f));
        if (ImGui::Button("Find Minimum Stops (BFS)", ImVec2(200, 30))) {
            string start_stop_name(start_location_input);
            int start_node_id = bus_network.getNodeIndexByname(start_stop_name);
            if (start_node_id != -1) {
                displayPathDetails(bus_network.BFS(start_node_id, UNIVERSITY_NODE_ID), &bus_network);
            } else {
                path_display_text = "Error: Starting location '" + start_stop_name + "' not found in the map.";
            }
        }
        ImGui::PopStyleColor(3);

        ImGui::Separator();
        ImGui::Text("Route Details:");
        ImGui::TextWrapped("%s", path_display_text.c_str());

        ImGui::Spacing();
        ImGui::Separator();

        // --- Add New Data Section ---
        handleAddDataGUI(bus_network, nodes_filename_buffer, edges_filename_buffer);

        ImGui::Spacing();
        ImGui::Separator();

        // --- Exit Button ---
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.0f, 0.0f, 1.0f));
        if (ImGui::Button("Exit", ImVec2(ImGui::GetContentRegionAvail().x, 30))) {
            glfwSetWindowShouldClose(window, true);
        }
        ImGui::PopStyleColor(3);

        ImGui::End(); // End Main Commute Optimizer window

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Cleanup
    if (map_instance) {
        delete map_instance;
        map_instance = nullptr;
    }
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
