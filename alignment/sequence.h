//
//  sequence.h
//  alignment
//
//  Created by Nhan Ly-Trong on 24/01/2022.
//

#include "seqregions.h"

#ifndef SEQUENCE_H
#define SEQUENCE_H

class Alignment;
class SeqRegions;

/** Class present a sequence */
class Sequence: public std::vector<Mutation> {
public:
    /**
        Name of the sequence
     */
     std::string seq_name;
    
    /**
    *  Sequence constructor
    */
    Sequence() = default;
    
    /**
    *  Sequence constructor
    */
    Sequence(std::string n_seq_name);
    
    /**
    *  Sequence constructor
    */
    Sequence(std::string n_seq_name, vector<Mutation> n_mutations);
    
    /// Move Ctor
    Sequence(Sequence&&) noexcept = default;
    /// Move assignment
    Sequence& operator=(Sequence&&) = default;

    /**
    *  Sequence destructor
    */
    ~Sequence() = default;
    
    /**
        Extract the lower likelihood vector (converting a vector of Mutations into a vector of Regions)
    */
    std::unique_ptr<SeqRegions> getLowerLhVector(const PositionType sequence_length, const StateType num_states, const SeqType seq_type);
};
#endif
