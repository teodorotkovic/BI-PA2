//
// Created by HP on 5/8/2024.
//

#ifndef VELKA_ULOHA_TESTS_H
#define VELKA_ULOHA_TESTS_H

#endif //VELKA_ULOHA_TESTS_H
void setCellRange(const std::vector<std::string>& cells, const std::vector<std::string>& values, CSpreadsheet& spreadsheet){
    for(int j = 0; j < cells.size(); j++){
        spreadsheet.setCell(CPos(cells[j]), values[j]);
    }
}

void copyRectRange(const std::vector<std::pair<std::string, std::string >>& fromTo, int h, int w, CSpreadsheet& spreadsheet){
    for(auto p: fromTo){
        spreadsheet.copyRect( CPos(p.first), CPos(p.second), w, h);
    }
}

void saveLoad(CSpreadsheet& spreadsheet){
    std::ostringstream oss;
    std::istringstream iss;
    std::string data;

    oss.clear();
    oss.str("");
    assert(spreadsheet.save(oss));
    data = oss.str();
    iss.clear();
    iss.str(data);
    assert(spreadsheet.load(iss));
}

// Function to add an edge to the graph
void addEdge(std::vector<std::vector<int>>& adj, int u, int v) {
    adj[u].push_back(v);
}


bool dfs(std::vector<std::vector<int>>& adj, std::vector<int>& color, int v, int parent) {
    if(color[v] == 1){
        color[v] = 2;
        return true;
    }

    if(color[v]== 2) return false;

    color[v] = 1;
    for (int u : adj[v]) {
        if (dfs(adj, color, u, v)) return true;
        color[u]=2;
    }
    return false;
}

bool hasCycle(std::vector<std::vector<int>>& adj, int from=0, int to=-1) {
    int V= adj.size();
    if(to==-1)to=adj.size();

    std::vector<int>color(V, 0);
    for (int v = from; v < to; ++v) {
        if (color[v] == 0) {
            if (dfs(adj, color, v,-1))
                return true;
        }
    }
    return false;
}

bool generateRandomGraph(std::vector<std::vector<int>>& adj, int V, int E) {
    std::unordered_set<int> edgeSet;
    while (E > 0) {
        int u = rand() % V;
        int v = rand() % V;
        if (u != v && edgeSet.find(u * V + v) == edgeSet.end()) {
            addEdge(adj, u, v);
            edgeSet.insert(u * V + v);
            if(rand()%10==0){
                addEdge(adj, v, u);
            }
            --E;
        }
    }
    return hasCycle(adj);
}

void generateTableWithCycles() {
    CSpreadsheet spreadsheet;
    int V = rand() %100+10;
    int E = V+rand()%10;
    std::vector<std::vector<int>> adj(V);
    bool hc = generateRandomGraph(adj, V,E);

    // make a labels to all nodes in a graph.
    std::unordered_map<int, std::string> nodeLabels;

    for(int j =0; j< V; j++){
        nodeLabels.insert({j, "A"+std::to_string(j)});
    }

    // Build a table.
    for(int j =0; j < V; j++){
        // generate expression.
        std::string expr= "=";
        for(auto node: adj[j]){
            expr+= "-"+nodeLabels[node];
        }
        expr+="-"+std::to_string(420);
        // Add to a table.
        spreadsheet.setCell(CPos(nodeLabels[j]),expr);
    }

    for(int j =0; j< V; j++){
        if(spreadsheet.getValue(CPos(nodeLabels[j])) == CValue()){
            assert(hasCycle(adj, j,j+1) && adj[j].size()!=0);
        }
        else {
            auto value = spreadsheet.getValue(CPos(nodeLabels[j]));
            if(std::holds_alternative<double>(value)){
                int v = std::stoi(std::to_string(std::abs(std::get<double>(value)))) %420;
                assert(v==0);
            }
        }
    }
}

#define SIMPLE_TESTS // Simple tests - getVal, save & load - no file corruption.
#define CYCLIC_DEPS_TESTS // Cycle generation, if time > 2s -> exception
//#define FILE_IO_TESTS // file corruption tests.
#include <future>
#include <chrono>

void runTests(){
    srand(time(nullptr));

#ifdef SIMPLE_TESTS
    CSpreadsheet preTests;
    preTests.setCell(CPos("d1"), "=12+10 + $E$1");
    assert(valueMatch(preTests.getValue(CPos("d1")), CValue()));
    preTests.setCell(CPos("e1"), "=-12-10");
    preTests.setCell(CPos("f1"), "=$d1 + E$1");
    assert(valueMatch(preTests.getValue(CPos("d1")), CValue(0.)));
    assert(valueMatch(preTests.getValue(CPos("f1")), CValue(-22.)));
    setCellRange({"d2", "d3", "d4", "d5", "g1", "h1", "h1"}, {"1","2", "3", "4", "1", "=2", "=3", "=4"}, preTests);
    copyRectRange({{"g2", "f1"}, {"g3", "g2"}, {"g4", "f1"}, {"g5", "f1"}}, 1, 1, preTests);

    assert(valueMatch(preTests.getValue(CPos("g2")), CValue(-21.)));
    assert(valueMatch(preTests.getValue(CPos("g3")), CValue(-20.)));
    assert(valueMatch(preTests.getValue(CPos("g4")), CValue(-19.)));
    assert(valueMatch(preTests.getValue(CPos("g5")), CValue(-18.)));

    copyRectRange({{"h2", "g2"}, {"i2", "h2"}, {"h2", "h2"}}, 4,1, preTests);

    assert(valueMatch(preTests.getValue(CPos("h2")), CValue(2.)));
    assert(valueMatch(preTests.getValue(CPos("h3")), CValue(3.)));
    assert(valueMatch(preTests.getValue(CPos("h4")), CValue(4.)));
    assert(valueMatch(preTests.getValue(CPos("h5")), CValue(5.)));

    assert(valueMatch(preTests.getValue(CPos("i2")), CValue(4.)));
    assert(valueMatch(preTests.getValue(CPos("i3")), CValue(5.)));
    assert(valueMatch(preTests.getValue(CPos("i4")), CValue(6.)));
    assert(valueMatch(preTests.getValue(CPos("i5")), CValue(7.)));

    saveLoad(preTests);

    assert(valueMatch(preTests.getValue(CPos("h2")), CValue(2.)));
    assert(valueMatch(preTests.getValue(CPos("h3")), CValue(3.)));
    assert(valueMatch(preTests.getValue(CPos("h4")), CValue(4.)));
    assert(valueMatch(preTests.getValue(CPos("h5")), CValue(5.)));

    assert(valueMatch(preTests.getValue(CPos("i2")), CValue(4.)));
    assert(valueMatch(preTests.getValue(CPos("i3")), CValue(5.)));
    assert(valueMatch(preTests.getValue(CPos("i4")), CValue(6.)));
    assert(valueMatch(preTests.getValue(CPos("i5")), CValue(7.)));

    assert(valueMatch(preTests.getValue(CPos("f1")), CValue(-22.)));

    preTests.copyRect(CPos("h3"), CPos("g2"), 4, 4);

    assert(valueMatch(preTests.getValue(CPos("h6")), CValue()));
    assert(valueMatch(preTests.getValue(CPos("i6")), CValue()));
    assert(valueMatch(preTests.getValue(CPos("h6")), CValue()));
    assert(valueMatch(preTests.getValue(CPos("k6")), CValue()));
    assert(valueMatch(preTests.getValue(CPos("k3")), CValue()));
    assert(valueMatch(preTests.getValue(CPos("k4")), CValue()));
    assert(valueMatch(preTests.getValue(CPos("k5")), CValue()));

    saveLoad(preTests);

    assert(valueMatch(preTests.getValue(CPos("h2")), CValue(2.)));
    assert(valueMatch(preTests.getValue(CPos("h3")), CValue(3.)));
    assert(valueMatch(preTests.getValue(CPos("h4")), CValue(4.)));
    assert(valueMatch(preTests.getValue(CPos("h5")), CValue(5.)));

    assert(valueMatch(preTests.getValue(CPos("i2")), CValue(4.)));
    assert(valueMatch(preTests.getValue(CPos("i3")), CValue(5.)));
    assert(valueMatch(preTests.getValue(CPos("i4")), CValue(6.)));
    assert(valueMatch(preTests.getValue(CPos("i5")), CValue(7.)));

    assert(valueMatch(preTests.getValue(CPos("f1")), CValue(-22.)));

    assert(valueMatch(preTests.getValue(CPos("h6")), CValue()));
    assert(valueMatch(preTests.getValue(CPos("i6")), CValue()));
    assert(valueMatch(preTests.getValue(CPos("h6")), CValue()));
    assert(valueMatch(preTests.getValue(CPos("k6")), CValue()));
    assert(valueMatch(preTests.getValue(CPos("k3")), CValue()));
    assert(valueMatch(preTests.getValue(CPos("k4")), CValue()));
    assert(valueMatch(preTests.getValue(CPos("k5")), CValue()));

    preTests.setCell(CPos("d6"), "0");

    assert(valueMatch(preTests.getValue(CPos("h6")), CValue(1.)));
    assert(valueMatch(preTests.getValue(CPos("i6")), CValue(3.)));
    assert(valueMatch(preTests.getValue(CPos("h6")), CValue(1.)));
    assert(valueMatch(preTests.getValue(CPos("k6")), CValue()));

    preTests.setCell(CPos("d10"), "=$d11+$E$10");
    assert(valueMatch(preTests.getValue(CPos("d10")), CValue()));

    preTests.setCell(CPos("d11"), "=1");
    preTests.setCell(CPos("e10"), "=10");
    assert(valueMatch(preTests.getValue(CPos("d10")), CValue(11.)));

    copyRectRange({{"i2", "d10"}, {"i3", "i2"}, {"i4", "d10"}, {"i5", "d10"}}, 1,1, preTests);

    assert(valueMatch(preTests.getValue(CPos("i2")), CValue(12.)));
    assert(valueMatch(preTests.getValue(CPos("i3")), CValue(13.)));
    assert(valueMatch(preTests.getValue(CPos("i4")), CValue(14.)));
    assert(valueMatch(preTests.getValue(CPos("i5")), CValue(10.)));

    saveLoad(preTests);

    assert(valueMatch(preTests.getValue(CPos("d10")), CValue(11.)));

    assert(valueMatch(preTests.getValue(CPos("i2")), CValue(12.)));
    assert(valueMatch(preTests.getValue(CPos("i3")), CValue(13.)));
    assert(valueMatch(preTests.getValue(CPos("i4")), CValue(14.)));
    assert(valueMatch(preTests.getValue(CPos("i5")), CValue(10.)));

    assert(valueMatch(preTests.getValue(CPos("h2")), CValue(2.)));
    assert(valueMatch(preTests.getValue(CPos("h3")), CValue(3.)));
    assert(valueMatch(preTests.getValue(CPos("h4")), CValue(4.)));
    assert(valueMatch(preTests.getValue(CPos("h5")), CValue(5.)));

    assert(valueMatch(preTests.getValue(CPos("f1")), CValue(-22.)));

    assert(valueMatch(preTests.getValue(CPos("h6")), CValue(1.)));
    assert(valueMatch(preTests.getValue(CPos("i6")), CValue(3.)));
    assert(valueMatch(preTests.getValue(CPos("h6")), CValue(1.)));
    assert(valueMatch(preTests.getValue(CPos("k6")), CValue()));
    assert(valueMatch(preTests.getValue(CPos("k3")), CValue()));
    assert(valueMatch(preTests.getValue(CPos("k4")), CValue()));
    assert(valueMatch(preTests.getValue(CPos("k5")), CValue()));
    std::cout<<"SIMPLE_TESTS PASSED\n";
#endif

#ifdef CYCLIC_DEPS_TESTS
    int TC = 150;
    while(TC--){
        std::future<void> future = std::async(std::launch::async, generateTableWithCycles);
        if (future.wait_for(std::chrono::seconds(2)) == std::future_status::timeout) {
            throw std::runtime_error("Cycle detection went wrong!");
        }
    }

    std::cout<<"CYCLIC_DEPS_TESTS PASSED\n";
#endif

#ifdef FILE_IO_TESTS

    CSpreadsheet fileIo;
    setCellRange({"A1", "B1","B3", "A8", "ZZZ8", "AAA69", "ZZZ1", "AHOJ1", "Never1", "Gonna1", "Give1", "You1", "Up1"}, {"10","=10+50", "A1+B1", "B3+A8", "5", "10", "Lez", "1000", "Never", "Gonna", "Let", "U", "Down"}, fileIo);

    std::ostringstream oss;
    std::istringstream iss;
    std::string data;

    saveLoad(fileIo);

    oss.clear();
    oss.str("");
    assert(fileIo.save(oss));
    data = oss.str();
    // swap 2 characters.
    for(int j=0; j< data.length(); j++){
        for(int i=j+1; i< data.length(); i++){
            std::string copy= data;
            std::swap(copy[j], copy[i]);
            iss.clear();
            iss.str(copy);
            assert(fileIo.load(iss) == (copy==data));
        }
    }

    // erase 1 char randomly
    for(int j=0; j< 50; j++){
        oss.clear();
        oss.str("");
        assert(fileIo.save(oss));
        data = oss.str();
        data.erase(rand()%data.length(),1);
        iss.clear();
        iss.str(data);
        assert(!fileIo.load(iss));
    }


    CSpreadsheet copied = fileIo;
    setCellRange({"A1", "B1","B3", "A8", "ZZZ8", "AAA69", "ZZZ1", "AHOJ1", "Never1", "Gonna1", "Give1", "You1", "Up1"}, {"0","0", "0", "0", "0", "0", "0", "0", "0", "0", "0", "0", "0"}, fileIo);

    CSpreadsheet toLoad;
    oss.clear();
    oss.str("");
    assert(copied.save(oss));
    data = oss.str();
    iss.clear();
    iss.str(data);
    assert(toLoad.load(iss));

    assert(valueMatch(toLoad.getValue(CPos("Never1")), CValue("Never")));
    assert(valueMatch(fileIo.getValue(CPos("A1")), CValue(0.)));

    std::cout << "FILE_IO_TESTS PASSED\n";
#endif
}