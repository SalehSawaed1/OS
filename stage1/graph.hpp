#pragma once
#include <vector>
#include <string>
#include <cstddef>

// Adjacency-list graph with optional directed edges.
// Guarantees: ignores self-loops; avoids duplicates.
struct Graph {
    std::size_t n{};
    bool directed{false};
    std::vector<std::vector<int>> adj; // 0..n-1
    std::size_t m{};                   // logical edge/arc count

    explicit Graph(std::size_t n_, bool directed_ = false);

    // Add u->v (and v->u if !directed). Ignores invalid/self-loop/duplicate.
    void add_edge(int u, int v);

    // Remove u->v (and v->u if !directed). Returns true if removed something.
    bool remove_edge(int u, int v);

    // Degrees
    std::vector<std::size_t> out_degrees() const; // for undirected: degree
    std::vector<std::size_t> in_degrees()  const; // for undirected: equals out_degrees

    // Utilities
    std::size_t edges() const { return m; }
    bool valid_vertex(int v) const { return v >= 0 && static_cast<std::size_t>(v) < n; }

    // Optional consistency check (undirected symmetry)
    bool validate() const;

    // Debug dump
    std::string to_string() const;
};
