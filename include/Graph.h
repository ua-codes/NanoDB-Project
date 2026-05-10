#pragma once
#include "DynamicArray.h"

struct Edge {
    int u, v;
    double weight;
    bool operator<(const Edge& o) const { return weight < o.weight; }
    bool operator>(const Edge& o) const { return weight > o.weight; }
};

class Graph {
private:
    int V;
    DynamicArray<Edge> edges;

    // Union-Find for Kruskal
    int* parent;
    int* rank_;

    int find(int x) {
        if (parent[x] != x) parent[x] = find(parent[x]);
        return parent[x];
    }

    void unite(int x, int y) {
        int px = find(x), py = find(y);
        if (rank_[px] < rank_[py]) { int t = px; px = py; py = t; }
        parent[py] = px;
        if (rank_[px] == rank_[py]) rank_[px]++;
    }

    void sortEdges() {
        // Simple insertion sort on edges by weight
        for (int i = 1; i < edges.size(); i++) {
            Edge key = edges[i];
            int j = i - 1;
            while (j >= 0 && edges[j].weight > key.weight) {
                edges[j+1] = edges[j]; j--;
            }
            edges[j+1] = key;
        }
    }

public:
    Graph(int vertices) : V(vertices) {
        parent = new int[V];
        rank_ = new int[V];
    }

    ~Graph() { delete[] parent; delete[] rank_; }

    void addEdge(int u, int v, double w) {
        edges.push_back({u, v, w});
    }

    DynamicArray<Edge> kruskalMST() {
        sortEdges();
        for (int i = 0; i < V; i++) { parent[i] = i; rank_[i] = 0; }
        DynamicArray<Edge> mst;
        for (int i = 0; i < edges.size() && mst.size() < V-1; i++) {
            int pu = find(edges[i].u), pv = find(edges[i].v);
            if (pu != pv) { unite(pu, pv); mst.push_back(edges[i]); }
        }
        return mst;
    }

    int vertexCount() const { return V; }
    int edgeCount() const { return edges.size(); }
    const DynamicArray<Edge>& getEdges() const { return edges; }
};
