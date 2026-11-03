#pragma once
#include <concepts>

template<typename T, typename V, typename D>
concept SpecDictionary = requires (D dict, T key, V val) {
    {dict.find(key)} -> std::same_as<bool>;
    {dict.add(key, val)};
    {dict.get(key)} -> std::convertible_to<V&>;
    {val.find(key)} -> std::same_as<bool>;
    {val.Get(key)};
    {val.Append(key)};
};

template<typename T, typename V, typename Dict>
requires SpecDictionary<T, V, Dict>
class Graph {
    Dict dict_;

public:
    Graph() : dict_() {};

    bool find(T node) {
        return dict_.find(node);
    }

    V& get(T node) {
        return dict_.get(node);
    }

    void add(T node, const V& sp) {
        if (!find(node)) {
            dict_.add(node, sp);
            for (const T& i : sp) {
                if (find(i)) {
                    V& el = get(i);
                    if (!el.find(node)) {
                        el.Append(node);
                    }
                }
                else {
                    dict_.add(i, V({node}));
                }
            }
        }
    }

};
