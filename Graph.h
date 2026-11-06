#pragma once
#include <utility>
#include <optional>
#include <vector>
#include <queue>
#include <stack>
#include <algorithm>
#include <stdexcept>
#include "../mephi_lab_2_second_course/mephi_lab_1_second_course/avl_tree.h"

template<typename K, typename V, typename D>
concept SpecDictionary = requires (D dict, K key, V val) {
    {dict.find(key)} -> std::same_as<bool>;
    {dict.add(key, val)};
    {dict.get(key)} -> std::convertible_to<V&>;
};

template <typename Dict, typename NodeType, typename EdgeType>
requires SpecDictionary<std::pair<NodeType, NodeType>, EdgeType, Dict>
class Graph {
private:
    using Key = std::pair<NodeType, NodeType>;
    Dict matrix_;
    std::vector<NodeType> vertices_;
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
        return std::find(vertices_.begin(), vertices_.end(), target) != vertices_.end();
    }

    void add_vertex(const T& val) {
        if (!has_vertex(val)) {
            vertices_.push_back(NodeType{val});
        }
    }

    void remove_vertex(const T& val) {
        NodeType target{val};
        auto it = std::find(vertices_.begin(), vertices_.end(), target);
        if (it == vertices_.end()) return;

        for (const auto& v : vertices_) {
            Key k1 = make_key(target, v);
            if (matrix_.find(k1)) {
                matrix_.get(k1).active = false;
            }
            Key k2 = make_key(v, target);
            if (matrix_.find(k2)) {
                matrix_.get(k2).active = false;
            }
        }
        vertices_.erase(it);
    }

    void add_edge(const T& from, const T& to, const E& weight = E{}) {
        add_vertex(from);
        add_vertex(to);
        Key k = make_key(NodeType{from}, NodeType{to});
        if (matrix_.find(k)) {
            EdgeType& e = matrix_.get(k);
            e.weight = weight;
            e.active = true;
        } else {
            matrix_.add(k, EdgeType{weight, true});
        }
    }

    void remove_edge(const T& from, const T& to) {
        Key k = make_key(NodeType{from}, NodeType{to});
        if (matrix_.find(k)) {
            matrix_.get(k).active = false;
        }
    }

    bool has_edge(const T& from, const T& to) {
        if (!has_vertex(from) || !has_vertex(to)) return false;
        Key k = make_key(NodeType{from}, NodeType{to});
        if (matrix_.find(k)) {
            return matrix_.get(k).active;
        }
        return false;
    }

    std::optional<E> get_edge(const T& from, const T& to) {
        if (has_edge(from, to)) {
            return matrix_.get(make_key(NodeType{from}, NodeType{to})).weight;
        }
        return std::nullopt;
    }

    std::vector<T> get_vertices() const {
        std::vector<T> res;
        res.reserve(vertices_.size());
        for (const auto& n : vertices_) {
            res.push_back(n.data);
        }
        return res;
    }

    std::vector<T> get_neighbors(const T& val) {
        std::vector<T> neighbors;
        if (!has_vertex(val)) {
            return neighbors;
        }
        for (const auto& v : vertices_) {
            if (has_edge(val, v.data)) {
                neighbors.push_back(v.data);
            }
        }
        return neighbors;
    }

    std::vector<T> bfs(const T& start) {
        std::vector<T> visited_order;
        if (!has_vertex(start)) {
            return visited_order;
        }

        avl_tree<NodeType, bool> visited;
        std::queue<T> q;

        visited.add(NodeType{start}, true);
        q.push(start);

        while (!q.empty()) {
            T curr = q.front();
            q.pop();
            visited_order.push_back(curr);

            for (const auto& neighbor : get_neighbors(curr)) {
                NodeType n_node{neighbor};
                if (!visited.find(n_node)) {
                    visited.add(n_node, true);
                    q.push(neighbor);
                }
            }
        }
        return visited_order;
    }

    std::vector<T> dfs(const T& start) {
        std::vector<T> visited_order;
        if (!has_vertex(start)) return visited_order;

        avl_tree<NodeType, bool> visited;
        std::stack<T> s;

        s.push(start);

        while (!s.empty()) {
            T curr = s.top();
            s.pop();
            NodeType c_node{curr};

            if (!visited.find(c_node)) {
                visited.add(c_node, true);
                visited_order.push_back(curr);

                auto neighbors = get_neighbors(curr);
                for (auto it = neighbors.rbegin(); it != neighbors.rend(); ++it) {
                    if (!visited.find(NodeType{*it})) {
                        s.push(*it);
                    }
                }
            }
        }
        return visited_order;
    }

    std::vector<T> topological_sort() {
        if (!directed_) {
            throw std::logic_error("не ориентированный граф!");
        }

        avl_tree<NodeType, int> in_degree;
        for (const auto& u : vertices_) {
            in_degree.add(u, 0);
        }

        for (const auto& u : vertices_) {
            for (const auto& v : vertices_) {
                if (has_edge(u.data, v.data)) {
                    in_degree.get(v) += 1;
                }
            }
        }

        std::queue<T> q;
        for (const auto& u : vertices_) {
            if (in_degree.get(u) == 0) {
                q.push(u.data);
            }
        }

        std::vector<T> order;
        while (!q.empty()) {
            T curr = q.front();
            q.pop();
            order.push_back(curr);

            for (const auto& neighbor : get_neighbors(curr)) {
                NodeType n_node{neighbor};
                int& deg = in_degree.get(n_node);
                deg -= 1;
                if (deg == 0) {
                    q.push(neighbor);
                }
            }
        }

        if (order.size() != vertices_.size()) {
            throw std::runtime_error("граф содержит цикл");
        }

        return order;
    }
};
