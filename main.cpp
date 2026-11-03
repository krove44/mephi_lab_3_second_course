#include <iostream>
#include "Graph.h"
#include "mephi_lab_2_second_course/mephi_lab_1_second_course/avl_tree.h"
#include "mephi_lab2/Sequence/ArraySequence.h"




int main() {
    Graph<unsigned int, ArraySequence<unsigned int>, avl_tree<unsigned int, ArraySequence<unsigned int>>> graph;
    graph.add(1, {2, 3});
    return 0;
}