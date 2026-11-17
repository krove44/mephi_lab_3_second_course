#pragma once
#include <utility>
#include <optional>
#include <queue>
#include <stack>
#include <algorithm>
#include <stdexcept>
#include <concepts>
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
    {dict.find(key)} -> std::same_as<bool>;
    {dict.add(key, val)};
    {dict.erase(key)};
    {dict.get(key)} -> std::convertible_to<V&>;
};

template <typename Dict, typename NodeType, typename EdgeType>
requires SpecDictionary<std::pair<NodeType, NodeType>, EdgeType, Dict>
class Graph {
private:
    using Key = std::pair<NodeType, NodeType>;
    Dict matrix_;
    ArraySequence<NodeType> vertices_;
    bool directed_;

    Key make_key(const NodeType& u, const NodeType& v) const {
        if (!directed_ && v < u) {
            return {v, u};
        }
        return {u, v};
    }

public:
    using T = typename NodeType::value_type;
    using E = typename EdgeType::weight_type;

    Graph(bool directed = true) : directed_(directed) {}

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
        for (size_t i = 0; i < vertices_.GetLenght(); ++i) {
            if (vertices_[i] != target) continue;

            for (size_t j = 0; j < vertices_.GetLenght(); ++j) {
                const NodeType& v = vertices_[j];
                matrix_.erase(make_key(target, v));
                if (directed_) {
                    matrix_.erase(make_key(v, target));
                }
            }
            vertices_.EraseAt(i);
            return;
        }
    }

    void add_edge(const T& from, const T& to, const E& weight = E{}) {
        add_vertex(from);
        add_vertex(to);
        Key k = make_key(NodeType{from}, NodeType{to});
        if (matrix_.find(k)) {
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
        return matrix_.find(make_key(NodeType{from}, NodeType{to}));
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

    ArraySequence<T> bfs(const T& start) {
        ArraySequence<T> visited_order;
        if (!has_vertex(start)) return visited_order;

        avl_tree<NodeType, bool> visited;
        std::queue<T> q;
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
                if (!visited.find(node)) {
                    visited.add(node, true);
                    q.push(neighbor);
                }
            }
        }
        return visited_order;
    }

    ArraySequence<T> dfs(const T& start) {
        ArraySequence<T> visited_order;
        if (!has_vertex(start)) return visited_order;

        avl_tree<NodeType, bool> visited;
        std::stack<T> s;
        s.push(start);

        while (!s.empty()) {
            T curr = s.top();
            s.pop();
            NodeType node{curr};

            if (!visited.find(node)) {
                visited.add(node, true);
                visited_order.Append(curr);

                auto neighbors = get_neighbors(curr);
                for (size_t i = neighbors.GetLenght(); i > 0; --i) {
                    const T& neighbor = neighbors[i - 1];
                    if (!visited.find(NodeType{neighbor})) {
                        s.push(neighbor);
                    }
                }
            }
        }
        return visited_order;
    }

    ArraySequence<T> topological_sort() {
        if (!directed_) {
            throw std::logic_error("не ориентированный граф!");
        }

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

        std::queue<T> q;
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

            auto neighbors = get_neighbors(curr);
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
