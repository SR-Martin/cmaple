#include "alignment/regions.h"
#include "alignment/sequence.h"
#include "alignment/alignment.h"
#include "model/model.h"

#ifndef NODE_H
#define NODE_H

class Node {
public:
    // node's id
    int id;
    
    // sequence name
    string seq_name;
    
    // likelihood of the data arrived from other next nodes
    Regions* partial_lh = NULL;
    
    // total likelihood at the current node
    Regions* total_lh = NULL;
    
    // length of branch connecting to parent
    double length;
    
    // next node in the circle of neighbors. For tips, circle = NULL
    Node* next;
    
    // next node in the phylo tree
    Node* neighbor;
    
    // vector of less informative sequences
    vector<string> less_info_seqs;
    
    // flag tp prevent traversing the same part of the tree multiple times.
    bool dirty;

    // flexible string attributes
    map<string,string> str_attributes;
    
    // flexible double attributes
    map<string,double> double_attributes;
    
    /**
        constructor
        @param n_seq_name the sequence name
     */
    Node(int n_id, string n_seq_name = "");
    
    /**
        constructor
        @param n_seq_name the sequence name
     */
    Node(string n_seq_name);
    
    /**
        constructor
        @param is_top_node TRUE if this is the first node in the next circle visited from root
     */
    Node(bool is_top_node = false);
    
    /**
        deconstructor
     */
    ~Node();
    
    /**
        TRUE if this node is a leaf
     */
    bool isLeave();
    
    /**
        get the top node
     */
    Node* getTopNode();
    
    /**
        export string: name + branch length
     */
    string exportString();
};

#endif

#define FOR_NEXT(node, next_node) \
for(next_node = node->next; next_node && next_node != node; next_node = next_node->next)

#define FOR_NEIGHBOR(node, neighbor_node) \
if (node->next) \
for(neighbor_node = node->next->neighbor; neighbor_node && neighbor_node != node->neighbor; neighbor_node = neighbor_node->neighbor->next->neighbor)
