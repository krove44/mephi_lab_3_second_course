#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <algorithm>
#include "Graph.h"

template <typename T, typename E>
using AvlGraph = Graph<
    avl_tree<std::pair<GraphNode<T>, GraphNode<T>>, GraphEdge<E>>,
    GraphNode<T>,
    GraphEdge<E>
>;

TEST(GraphTest, VertexOperations) {
    AvlGraph<int, int> g(true);
    EXPECT_FALSE(g.has_vertex(1));

    g.add_vertex(1);
    g.add_vertex(2);
    g.add_vertex(1); // Дубликат

    EXPECT_TRUE(g.has_vertex(1));
    EXPECT_TRUE(g.has_vertex(2));
    EXPECT_EQ(g.get_vertices().size(), 2);

    g.remove_vertex(1);
    EXPECT_FALSE(g.has_vertex(1));
    EXPECT_EQ(g.get_vertices().size(), 1);
}

TEST(GraphTest, DirectedEdgeOperations) {
    AvlGraph<int, int> g(true);

    g.add_edge(1, 2, 10);
    g.add_edge(2, 3, 20);

    EXPECT_TRUE(g.has_vertex(1));
    EXPECT_TRUE(g.has_vertex(2));
    EXPECT_TRUE(g.has_vertex(3));

    EXPECT_TRUE(g.has_edge(1, 2));
    EXPECT_FALSE(g.has_edge(2, 1));
    EXPECT_EQ(g.get_edge(1, 2).value(), 10);
    EXPECT_EQ(g.get_edge(2, 3).value(), 20);
    EXPECT_FALSE(g.get_edge(1, 3).has_value());

    // Перезапись веса ребра
    g.add_edge(1, 2, 50);
    EXPECT_EQ(g.get_edge(1, 2).value(), 50);

    // Удаление ребра и повторная вставка
    g.remove_edge(1, 2);
    EXPECT_FALSE(g.has_edge(1, 2));
    EXPECT_FALSE(g.get_edge(1, 2).has_value());

    g.add_edge(1, 2, 99);
    EXPECT_TRUE(g.has_edge(1, 2));
    EXPECT_EQ(g.get_edge(1, 2).value(), 99);
}

TEST(GraphTest, UndirectedEdgeOperations) {
    AvlGraph<std::string, double> ug(false);
    ug.add_edge("Moscow", "SPb", 700.5);

    EXPECT_TRUE(ug.has_edge("Moscow", "SPb"));
    EXPECT_TRUE(ug.has_edge("SPb", "Moscow"));
    EXPECT_DOUBLE_EQ(ug.get_edge("SPb", "Moscow").value(), 700.5);

    ug.remove_edge("SPb", "Moscow");
    EXPECT_FALSE(ug.has_edge("Moscow", "SPb"));
}

TEST(GraphTest, RemoveVertexCleansEdges) {
    AvlGraph<int, int> g(true);
    g.add_edge(1, 2, 5);
    g.add_edge(2, 3, 15);
    g.add_edge(3, 1, 25);

    g.remove_vertex(2);
    EXPECT_FALSE(g.has_vertex(2));
    EXPECT_FALSE(g.has_edge(1, 2));
    EXPECT_FALSE(g.has_edge(2, 3));
    EXPECT_TRUE(g.has_edge(3, 1));
}

TEST(GraphTest, BFSAndDFSTraversals) {
    AvlGraph<int, int> g(true);
    g.add_edge(1, 2, 1);
    g.add_edge(1, 3, 1);
    g.add_edge(2, 4, 1);
    g.add_edge(3, 4, 1);
    g.add_vertex(99); // Изолированная вершина

    auto bfs_res = g.bfs(1);
    ASSERT_EQ(bfs_res.size(), 4);
    EXPECT_EQ(bfs_res[0], 1);
    EXPECT_EQ(bfs_res[3], 4);

    auto dfs_res = g.dfs(1);
    ASSERT_EQ(dfs_res.size(), 4);
    EXPECT_EQ(dfs_res[0], 1);

    EXPECT_TRUE(g.bfs(99).size() == 1);
    EXPECT_TRUE(g.bfs(500).empty());
}

TEST(GraphTest, TopologicalSortValidDAG) {
    AvlGraph<int, int> dag(true);
    std::vector<std::pair<int, int>> dependencies = {
        {5, 2}, {5, 0}, {4, 0}, {4, 1}, {2, 3}, {3, 1}
    };

    for (const auto& [u, v] : dependencies) {
        dag.add_edge(u, v, 1);
    }

    std::vector<int> sorted = dag.topological_sort();
    ASSERT_EQ(sorted.size(), 6);

    auto pos = [&](int val) {
        auto it = std::find(sorted.begin(), sorted.end(), val);
        return std::distance(sorted.begin(), it);
    };

    for (const auto& [u, v] : dependencies) {
        EXPECT_LT(pos(u), pos(v));
    }
}

TEST(GraphTest, TopologicalSortExceptions) {
    AvlGraph<int, int> cyclic(true);
    cyclic.add_edge(1, 2, 1);
    cyclic.add_edge(2, 3, 1);
    cyclic.add_edge(3, 1, 1);
    EXPECT_THROW(cyclic.topological_sort(), std::runtime_error);

    AvlGraph<int, int> undirected(false);
    undirected.add_edge(1, 2, 1);
    EXPECT_THROW(undirected.topological_sort(), std::logic_error);
}
