#pragma once
#include <utility>
#include <optional>
#include <algorithm>
#include <stdexcept>
#include <concepts>
#include "../Libs/Queue.h"
#include "../Libs/Stack.h"
#include "../Libs/avl_tree.h"
#include "../mephi_lab2/Sequence/ArraySequence.h"

template <typename T>
struct GraphNode {
    using value_type = T;
    T data{};

    bool operator==(const GraphNode& other) const { return data == other.data; }
    bool operator!=(const GraphNode& other) const { return data != other.data; }
    bool operator<(const GraphNode& other) const { return data < other.data; }
    bool operator>(const GraphNode& other) const { return data > other.data; }
    bool operator<=(const GraphNode& other) const { return data <= other.data; }
    bool operator>=(const GraphNode& other) const { return data >= other.data; }
};

template <typename E>
struct GraphEdge {
    using weight_type = E;
    E weight{};
};

template<typename K, typename V, typename D>
concept SpecDictionary = requires (D dict, K key, V val) {
    {dict.has_key(key)} -> std::same_as<bool>;
    {dict.add(key, val)};
    {dict.erase(key)};
    {dict.get(key)} -> std::convertible_to<V&>;
};

template <typename Dict, typename NodeType, typename EdgeType, bool directed_ = false>
requires SpecDictionary<std::pair<NodeType, NodeType>, EdgeType, Dict>
class Graph {
private:
    using T = typename NodeType::value_type;
    using E = typename EdgeType::weight_type;

    using Key = std::pair<NodeType, NodeType>;
    Dict matrix_;
    ArraySequence<NodeType> vertices_;

    Key make_key(const NodeType& u, const NodeType& v) const {
        if (!directed_ && v < u) {
            return {v, u};
        }
        return {u, v};
    }

    static bool better(const E& lhs, const E& rhs, bool maximum) {
        if (maximum) {
            return rhs < lhs;
        }
        return lhs < rhs;
    }

    Graph spanning_tree(bool maximum) requires (!directed_) {
        struct Candidate {
            bool in_tree = false;
            size_t parent = 0;
            std::optional<typename EdgeType::weight_type> weight;
        };

        Graph tree;
        const size_t count = vertices_.GetLenght();
        if (count == 0) {
            return tree;
        }
        tree.vertices_ = vertices_;

        ArraySequence<Candidate> candidates(count);
        for (size_t i = 0; i < count; ++i) {
            candidates.Append(Candidate{});
        }

        size_t current = 0;
        for (size_t added = 1; added < count; ++added) {
            candidates[current].in_tree = true;
            const auto& from = vertices_[current].data;
            size_t next = count;

            for (size_t i = 0; i < count; ++i) {
                auto& candidate = candidates[i];
                if (candidate.in_tree) {
                    continue;
                }
                const auto weight = get_edge(from, vertices_[i].data);
                if (weight && (!candidate.weight || better(*weight, *candidate.weight, maximum))) {
                    candidate.parent = current;
                    candidate.weight = weight;
                }
                if (candidate.weight && (next == count || better(*candidate.weight, *candidates[next].weight, maximum))) {
                    next = i;
                }
            }

            if (next == count) {
                throw std::runtime_error("граф несвязный");
            }
            const auto& chosen = candidates[next];
            tree.add_edge(vertices_[chosen.parent].data, vertices_[next].data, *chosen.weight);
            current = next;
        }
        return tree;
    }

public:
    Graph() = default;

    bool is_directed() const {
        return directed_;
    }

    bool has_vertex(const T& val) const {
        NodeType target{val};
        for (size_t i = 0; i < vertices_.GetLenght(); ++i) {
            if (vertices_[i] == target) {
                return true;
            }
        }
        return false;
    }

    void add_vertex(const T& val) {
        if (!has_vertex(val)) {
            vertices_.Append(NodeType{val});
        }
    }

    void remove_vertex(const T& val) {
        NodeType target{val};
        if (has_vertex(val)) {
            for (size_t i = 0; i < vertices_.GetLenght(); i++) {
                const NodeType& v = vertices_[i];
                matrix_.erase(make_key(target, v));
                if (directed_) {
                    matrix_.erase(make_key(v, target));
                }
            }
            vertices_.Remove(target);
        }
    }

    void add_edge(const T& from, const T& to, const E& weight = E{}) {
        add_vertex(from);
        add_vertex(to);
        Key k = make_key(NodeType{from}, NodeType{to});
        if (matrix_.has_key(k)) {
            EdgeType& e = matrix_.get(k);
            e.weight = weight;
        } else {
            matrix_.add(k, EdgeType{weight});
        }
    }

    void remove_edge(const T& from, const T& to) {
        matrix_.erase(make_key(NodeType{from}, NodeType{to}));
    }

    bool has_edge(const T& from, const T& to) {
        return matrix_.has_key(make_key(NodeType{from}, NodeType{to}));
    }

    std::optional<E> get_edge(const T& from, const T& to) {
        if (has_edge(from, to)) {
            return matrix_.get(make_key(NodeType{from}, NodeType{to})).weight;
        }
        return std::nullopt;
    }

    ArraySequence<T> get_vertices() const {
        ArraySequence<T> res;
        for (size_t i = 0; i < vertices_.GetLenght(); ++i) {
            res.Append(vertices_[i].data);
        }
        return res;
    }

    ArraySequence<T> get_neighbors(const T& val) {
        ArraySequence<T> neighbors;
        if (!has_vertex(val)) return neighbors;

        for (size_t i = 0; i < vertices_.GetLenght(); ++i) {
            const T& neighbor = vertices_[i].data;
            if (has_edge(val, neighbor)) {
                neighbors.Append(neighbor);
            }
        }
        return neighbors;
    }

    Graph minimum_spanning_tree() requires (!directed_) {
        return spanning_tree(false);
    }

    Graph maximum_spanning_tree() requires (!directed_) {
        return spanning_tree(true);
    }

    E total_edge_weight() requires requires(E& total, const E& weight) {total += weight;} {
        E total{};
        const size_t count = vertices_.GetLenght();

        for (size_t i = 0; i < count; ++i) {
            const size_t first_neighbor = directed_ ? 0 : i;

            for (size_t j = first_neighbor; j < count; ++j) {
                const auto weight = get_edge(vertices_[i].data, vertices_[j].data);
                if (weight.has_value()) {
                    total += *weight;
                }
            }
        }

        return total;
    }

    ArraySequence<T> bfs(const T& start) {
        ArraySequence<T> visited_order;
        if (!has_vertex(start)) return visited_order;

        avl_tree<NodeType, bool> visited;
        Queue<T> q;
        visited.add(NodeType{start}, true);
        q.push(start);

        while (!q.empty()) {
            T curr = q.front();
            q.pop();
            visited_order.Append(curr);

            auto neighbors = get_neighbors(curr);
            for (size_t i = 0; i < neighbors.GetLenght(); ++i) {
                const T& neighbor = neighbors[i];
                NodeType node{neighbor};
                if (!visited.has_key(node)) {
                    visited.add(node, true);
                    q.push(neighbor);
                }
            }
        }
        return visited_order;
    }

    ArraySequence<T> dfs(const T& start) {
        ArraySequence<T> visited_order;
        if (!has_vertex(start)) {
            return visited_order;
        }

        avl_tree<NodeType, bool> visited;
        Stack<T> s;
        s.push(start);

        while (!s.empty()) {
            T curr = s.top();
            s.pop();
            NodeType node{curr};

            if (!visited.has_key(node)) {
                visited.add(node, true);
                visited_order.Append(curr);

                auto neighbors = get_neighbors(curr);
                for (size_t i = neighbors.GetLenght(); i > 0; --i) {
                    const T& neighbor = neighbors[i - 1];
                    if (!visited.has_key(NodeType{neighbor})) {
                        s.push(neighbor);
                    }
                }
            }
        }
        return visited_order;
    }

    ArraySequence<T> topological_sort() requires directed_ {

        avl_tree<NodeType, int> in_degree;
        for (size_t i = 0; i < vertices_.GetLenght(); ++i) {
            in_degree.add(vertices_[i], 0);
        }

        for (size_t i = 0; i < vertices_.GetLenght(); ++i) {
            for (size_t j = 0; j < vertices_.GetLenght(); ++j) {
                if (has_edge(vertices_[i].data, vertices_[j].data)) {
                    ++in_degree.get(vertices_[j]);
                }
            }
        }

        Queue<T> q;
        for (size_t i = 0; i < vertices_.GetLenght(); ++i) {
            const NodeType& node = vertices_[i];
            if (in_degree.get(node) == 0) {
                q.push(node.data);
            }
        }

        ArraySequence<T> order;
        while (!q.empty()) {
            T curr = q.front();
            q.pop();
            order.Append(curr);

            ArraySequence<T> neighbors = get_neighbors(curr);
            for (size_t i = 0; i < neighbors.GetLenght(); ++i) {
                NodeType node{neighbors[i]};
                int& deg = in_degree.get(node);
                if (--deg == 0) {
                    q.push(neighbors[i]);
                }
            }
        }

        if (order.GetLenght() != vertices_.GetLenght()) {
            throw std::runtime_error("граф содержит цикл");
        }
        return order;
    }
};
