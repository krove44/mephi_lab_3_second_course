#include <gtest/gtest.h>
#include <string>
#include <concepts>
#include <stdexcept>
#include <utility>
#include "../Libs/Queue.h"
#include "../Libs/Stack.h"
#include "Graph.h"

template <typename T, typename E, bool Directed = false>
using AvlGraph = Graph<
    avl_tree<std::pair<GraphNode<T>, GraphNode<T>>, GraphEdge<E>>,
    GraphNode<T>,
    GraphEdge<E>,
    Directed
>;

template <typename T>
void expect_sequence_eq(const ArraySequence<T>& actual, const ArraySequence<T>& expected) {
    ASSERT_EQ(actual.GetLenght(), expected.GetLenght());
    for (size_t i = 0; i < expected.GetLenght(); ++i) {
        EXPECT_EQ(actual[i], expected[i]) << "Index: " << i;
    }
}

template <typename T>
size_t find_index(const ArraySequence<T>& seq, const T& value) {
    for (size_t i = 0; i < seq.GetLenght(); ++i) {
        if (seq[i] == value) return i;
    }
    return seq.GetLenght();
}

template <typename G>
concept HasTopologicalSort = requires(G& g) {
    g.topological_sort();
};

static_assert(HasTopologicalSort<AvlGraph<int, int, true>>);
static_assert(!HasTopologicalSort<AvlGraph<int, int, false>>);

template <typename G>
concept HasSpanningTrees = requires(G& g) {
    { g.minimum_spanning_tree() } -> std::same_as<G>;
    { g.maximum_spanning_tree() } -> std::same_as<G>;
};

static_assert(HasSpanningTrees<AvlGraph<int, int>>);
static_assert(!HasSpanningTrees<AvlGraph<int, int, true>>);

TEST(GraphTest, VertexOperations) {
    AvlGraph<int, int, true> g;
    EXPECT_TRUE(g.is_directed());
    EXPECT_FALSE(g.has_vertex(1));

    g.add_vertex(1);
    g.add_vertex(2);
    g.add_vertex(1);

    EXPECT_TRUE(g.has_vertex(1));
    EXPECT_TRUE(g.has_vertex(2));
    EXPECT_EQ(g.get_vertices().GetLenght(), 2);

    g.remove_vertex(1);
    EXPECT_FALSE(g.has_vertex(1));
    EXPECT_EQ(g.get_vertices().GetLenght(), 1);
}

TEST(GraphTest, DirectedEdgeOperations) {
    AvlGraph<int, int, true> g;

    g.add_edge(1, 2, 10);
    g.add_edge(2, 3, 20);

    EXPECT_TRUE(g.has_vertex(1));
    EXPECT_TRUE(g.has_vertex(2));
    EXPECT_TRUE(g.has_vertex(3));
    EXPECT_TRUE(g.has_edge(1, 2));
    EXPECT_FALSE(g.has_edge(2, 1));
    ASSERT_TRUE(g.get_edge(1, 2).has_value());
    EXPECT_EQ(g.get_edge(1, 2).value(), 10);
    EXPECT_EQ(g.get_edge(2, 3).value(), 20);
    EXPECT_FALSE(g.get_edge(1, 3).has_value());

    g.add_edge(1, 2, 50);
    EXPECT_EQ(g.get_edge(1, 2).value(), 50);

    g.remove_edge(1, 2);
    EXPECT_FALSE(g.has_edge(1, 2));
    EXPECT_FALSE(g.get_edge(1, 2).has_value());

    g.add_edge(1, 2, 99);
    EXPECT_TRUE(g.has_edge(1, 2));
    EXPECT_EQ(g.get_edge(1, 2).value(), 99);
}

TEST(GraphTest, UndirectedEdgeOperations) {
    AvlGraph<std::string, double, false> g;
    EXPECT_FALSE(g.is_directed());

    g.add_edge("Moscow", "SPb", 700.5);
    EXPECT_TRUE(g.has_edge("Moscow", "SPb"));
    EXPECT_TRUE(g.has_edge("SPb", "Moscow"));
    ASSERT_TRUE(g.get_edge("SPb", "Moscow").has_value());
    EXPECT_DOUBLE_EQ(g.get_edge("SPb", "Moscow").value(), 700.5);

    g.remove_edge("SPb", "Moscow");
    EXPECT_FALSE(g.has_edge("Moscow", "SPb"));
    EXPECT_FALSE(g.has_edge("SPb", "Moscow"));
}

TEST(GraphTest, RemoveVertexCleansEdges) {
    AvlGraph<int, int, true> g;
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
    AvlGraph<int, int, true> g;
    g.add_edge(1, 2, 1);
    g.add_edge(1, 3, 1);
    g.add_edge(2, 4, 1);
    g.add_edge(3, 4, 1);
    g.add_vertex(99);

    expect_sequence_eq(g.bfs(1), ArraySequence<int>{1, 2, 3, 4});
    expect_sequence_eq(g.dfs(1), ArraySequence<int>{1, 2, 4, 3});
    expect_sequence_eq(g.bfs(99), ArraySequence<int>{99});
    expect_sequence_eq(g.dfs(99), ArraySequence<int>{99});
    EXPECT_EQ(g.bfs(500).GetLenght(), 0u);
    EXPECT_EQ(g.dfs(500).GetLenght(), 0u);
}

TEST(GraphTest, TopologicalSortValidDAG) {
    AvlGraph<int, int, true> dag;
    const ArraySequence<std::pair<int, int>> dependencies = {
        {5, 2}, {5, 0}, {4, 0}, {4, 1}, {2, 3}, {3, 1}
    };
    for (size_t i = 0; i < dependencies.GetLenght(); ++i) {
        dag.add_edge(dependencies[i].first, dependencies[i].second, 1);
    }

    const ArraySequence<int> sorted = dag.topological_sort();
    ASSERT_EQ(sorted.GetLenght(), 6);
    for (size_t i = 0; i < dependencies.GetLenght(); ++i) {
        const size_t u_pos = find_index(sorted, dependencies[i].first);
        const size_t v_pos = find_index(sorted, dependencies[i].second);
        ASSERT_LT(u_pos, sorted.GetLenght());
        ASSERT_LT(v_pos, sorted.GetLenght());
        EXPECT_LT(u_pos, v_pos);
    }
}

TEST(GraphTest, TopologicalSortWithSeveralValidOrders) {
    AvlGraph<int, int, true> dag;
    dag.add_edge(2, 1);
    dag.add_edge(2, 3);

    const ArraySequence<int> sorted = dag.topological_sort();
    ASSERT_EQ(sorted.GetLenght(), 3);
    EXPECT_EQ(sorted[0], 2);
    EXPECT_TRUE((sorted[1] == 1 && sorted[2] == 3) ||
                (sorted[1] == 3 && sorted[2] == 1));
}

TEST(GraphTest, TopologicalSortCycleThrows) {
    AvlGraph<int, int, true> cyclic;
    cyclic.add_edge(1, 2);
    cyclic.add_edge(2, 3);
    cyclic.add_edge(3, 1);
    EXPECT_THROW(cyclic.topological_sort(), std::runtime_error);
}

TEST(GraphTest, TopologicalSortSelfLoopThrows) {
    AvlGraph<int, int, true> cyclic;
    cyclic.add_edge(1, 1);
    EXPECT_THROW(cyclic.topological_sort(), std::runtime_error);
}

TEST(GraphTest, TopologicalSortEmptyGraph) {
    AvlGraph<int, int, true> dag;
    EXPECT_EQ(dag.topological_sort().GetLenght(), 0);
}

TEST(GraphTest, TopologicalSortIncludesIsolatedVertex) {
    AvlGraph<int, int, true> dag;
    dag.add_edge(1, 2);
    dag.add_vertex(99);
    const ArraySequence<int> sorted = dag.topological_sort();
    ASSERT_EQ(sorted.GetLenght(), 3);
    EXPECT_LT(find_index(sorted, 99), sorted.GetLenght());
    EXPECT_LT(find_index(sorted, 1), find_index(sorted, 2));
}

TEST(GraphTest, MinimumAndMaximumSpanningTrees) {
    AvlGraph<int, int> g;
    g.add_edge(1, 2, -3);
    g.add_edge(1, 3, 6);
    g.add_edge(2, 3, 0);
    g.add_edge(2, 4, 4);
    g.add_edge(3, 4, 2);
    g.add_edge(1, 1, -100);
    g.add_edge(4, 4, 100);

    AvlGraph<int, int>  minimum = g.minimum_spanning_tree();
    AvlGraph<int, int>  maximum = g.maximum_spanning_tree();
    const ArraySequence<int> vertices{1, 2, 3, 4};
    expect_sequence_eq(minimum.get_vertices(), vertices);
    expect_sequence_eq(maximum.get_vertices(), vertices);

    EXPECT_EQ(minimum.get_edge(2, 1), -3);
    EXPECT_EQ(minimum.get_edge(2, 3), 0);
    EXPECT_EQ(minimum.get_edge(3, 4), 2);
    EXPECT_FALSE(minimum.has_edge(1, 3));
    EXPECT_FALSE(minimum.has_edge(2, 4));

    EXPECT_EQ(maximum.get_edge(1, 3), 6);
    EXPECT_EQ(maximum.get_edge(2, 4), 4);
    EXPECT_EQ(maximum.get_edge(3, 4), 2);
    EXPECT_FALSE(maximum.has_edge(1, 2));
    EXPECT_FALSE(maximum.has_edge(2, 3));

    EXPECT_FALSE(minimum.has_edge(1, 4));
    EXPECT_FALSE(maximum.has_edge(1, 4));
    EXPECT_FALSE(minimum.has_edge(1, 1));
    EXPECT_FALSE(minimum.has_edge(4, 4));
    EXPECT_FALSE(maximum.has_edge(1, 1));
    EXPECT_FALSE(maximum.has_edge(4, 4));
    EXPECT_EQ(g.get_edge(1, 3), 6);
    EXPECT_EQ(g.get_edge(1, 2), -3);
    EXPECT_EQ(minimum.total_edge_weight(), -1);
    EXPECT_EQ(maximum.total_edge_weight(), 12);
}

TEST(GraphTest, SpanningTreesOfEmptyAndSingleVertexGraphs) {
    AvlGraph<int, int> g;
    EXPECT_EQ(g.minimum_spanning_tree().get_vertices().GetLenght(), 0);
    EXPECT_EQ(g.maximum_spanning_tree().get_vertices().GetLenght(), 0);

    g.add_edge(42, 42, 5);
    auto minimum = g.minimum_spanning_tree();
    auto maximum = g.maximum_spanning_tree();
    expect_sequence_eq(minimum.get_vertices(), ArraySequence<int>{42});
    expect_sequence_eq(maximum.get_vertices(), ArraySequence<int>{42});
    EXPECT_FALSE(minimum.has_edge(42, 42));
    EXPECT_FALSE(maximum.has_edge(42, 42));
}

TEST(GraphTest, SpanningTreesOfDisconnectedGraphThrow) {
    AvlGraph<int, int> g;
    g.add_edge(1, 2, 5);
    g.add_vertex(3);

    EXPECT_THROW(g.minimum_spanning_tree(), std::runtime_error);
    EXPECT_THROW(g.maximum_spanning_tree(), std::runtime_error);
}


TEST(QueueTest, FifoAndReuse) {
    Queue<int> queue;
    EXPECT_TRUE(queue.empty());
    EXPECT_THROW(queue.front(), std::out_of_range);
    EXPECT_THROW(queue.pop(), std::out_of_range);
    for (int value : {1, 2, 3}) queue.push(value);
    for (int value : {1, 2, 3}) {
        EXPECT_EQ(queue.front(), value);
        EXPECT_EQ(queue.front(), value);
        queue.pop();
    }
    EXPECT_TRUE(queue.empty());
    for (int value : {1, 2, 1}) queue.push(value);
    for (int value : {1, 2, 1}) {
        EXPECT_EQ(queue.front(), value);
        queue.pop();
    }
    EXPECT_TRUE(queue.empty());
}

TEST(StackTest, LifoAndReuse) {
    Stack<int> stack;
    EXPECT_TRUE(stack.empty());
    EXPECT_THROW(stack.top(), std::out_of_range);
    EXPECT_THROW(stack.pop(), std::out_of_range);
    for (int value : {1, 2, 3}) stack.push(value);
    for (int value : {3, 2, 1}) {
        EXPECT_EQ(stack.top(), value);
        EXPECT_EQ(stack.top(), value);
        stack.pop();
    }
    EXPECT_TRUE(stack.empty());
    for (int value : {1, 2, 1}) stack.push(value);
    for (int value : {1, 2, 1}) {
        EXPECT_EQ(stack.top(), value);
        stack.pop();
    }
    EXPECT_TRUE(stack.empty());
}

TEST(ContainerTest, StringValues) {
    Queue<std::string> queue;
    Stack<std::string> stack;
    for (const auto* word : {"один", "два"}) {
        queue.push(word);
        stack.push(word);
    }
    EXPECT_EQ(queue.front(), "один");
    EXPECT_EQ(stack.top(), "два");
}
