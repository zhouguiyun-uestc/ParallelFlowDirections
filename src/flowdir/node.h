
#ifndef NODE_HEAD_H
#define NODE_HEAD_H

#include <functional>

class Node {
public:
    int   row;
    int   col;
    float spill;

    Node() {
        row   = 0;
        col   = 0;
        spill = -9999.0;
    }
    Node( int row, int col, float spill ) {
        this->row   = row;
        this->col   = col;
        this->spill = spill;
    }

    struct Greater : public std::binary_function< Node, Node, bool > {
        bool operator()( const Node n1, const Node n2 ) const {
            return n1.spill > n2.spill;
        }
    };
};

#endif