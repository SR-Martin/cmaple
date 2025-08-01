#include "model_dna_rate_variation.h"
#include "../tree/tree.h"
#include "../tree/phylonode.h"

using namespace cmaple;

ModelDNARateVariation::ModelDNARateVariation( const cmaple::ModelBase::SubModel sub_model, PositionType _genomeSize, 
                                                bool _useSiteRates, cmaple::RealNumType _wtPseudocount, std::string _ratesFilename)
    : ModelDNA(sub_model) {
    
    genomeSize = _genomeSize;
    useSiteRates = _useSiteRates;
    matSize = row_index[num_states_];
    waitingTimePseudoCount = _wtPseudocount;
    ratesFilename = _ratesFilename;

    mutationMatrices = new RealNumType[matSize * genomeSize]();
    transposedMutationMatrices = new RealNumType[matSize * genomeSize]();
    diagonalMutationMatrices = new RealNumType[num_states_ * genomeSize]();
    freqiFreqjQijs = new RealNumType[matSize * genomeSize]();
    freqjTransposedijs = new RealNumType[matSize * genomeSize]();

    if(useSiteRates) {
        rates = new cmaple::RealNumType[genomeSize]();
    }
    if(ratesFilename.length() > 0) {
        this->readRatesFile();
    }
}

ModelDNARateVariation::~ModelDNARateVariation() { 
    delete[] mutationMatrices;
    delete[] transposedMutationMatrices;
    delete[] diagonalMutationMatrices;
    delete[] freqiFreqjQijs;
    delete[] freqjTransposedijs;
    if(useSiteRates) {
        delete[] rates;
    }
}

void ModelDNARateVariation::printMatrix(const RealNumType* matrix, std::ostream* outStream) {
    for(int j = 0; j < num_states_; j++) {
        std::string line = "|";
        for(int k = 0; k < num_states_; k++) {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(5) << matrix[row_index[j] + k];
            line += "\t" + oss.str();
            //line += "\t" + convertDoubleToString(matrix[row_index[j] + k], 5);
        }
        line += "\t|";
        (*outStream) << line << std::endl;
    }
}

void ModelDNARateVariation::printCountsAndWaitingTimes(const RealNumType* counts, const RealNumType* waitingTImes, std::ostream* outStream) {
    for(int j = 0; j < num_states_; j++) {
        std::string line = "|";
        for(int k = 0; k < num_states_; k++) {
            line += "\t" + convertDoubleToString(counts[row_index[j] + k]);
        }
        line += "\t|\t" + convertDoubleToString(waitingTImes[j]) + "\t|";
        (*outStream) << line << std::endl;
    }
}

bool ModelDNARateVariation::updateMutationMatEmpirical() {
    if(ratesFilename.length() > 0) {
        return true;
    }
    if(ratesEstimated) {
        std::cout << "[ModelDNARateVariation] Warning: Overwriting estimated rate matrices with single empirical mutation matrix." << std::endl;
    }
    bool val = ModelDNA::updateMutationMatEmpirical();
    for(int i = 0; i < genomeSize; i++) {
        for(int j = 0; j < matSize; j++) {
            mutationMatrices[i * matSize + j] = mutation_mat[j];
            transposedMutationMatrices[i * matSize + j] = transposed_mut_mat[j];
            freqiFreqjQijs[i * matSize + j] = freqi_freqj_qij[j];
            freqjTransposedijs[i * matSize + j] = freq_j_transposed_ij[j];
        }
        for(int j = 0; j < num_states_; j++) {
            diagonalMutationMatrices[i * num_states_ + j] = diagonal_mut_mat[j];
        }
    }
    return val;
}

void ModelDNARateVariation::estimateRates(cmaple::Tree* tree) {
    ratesEstimated = true;
    if(useSiteRates) {
        estimateRatePerSite(tree);

    } else {
        if(ratesFilename.size() == 0) {
            RealNumType oldLK = -std::numeric_limits<double>::infinity();
            RealNumType newLK = tree->computeLh();
            int numSteps = 0;
            while(newLK - oldLK > 1 && numSteps < 20) {
                estimateRatesPerSitePerEntry(tree);
                oldLK = newLK;
                newLK = tree->computeLh();
            }  
        }
    }

    // Write out rate matrices to file
    if(cmaple::verbose_mode > VB_MIN) 
    {
        const std::string prefix = tree->params->output_prefix.length() ? 
            tree->params->output_prefix : tree->params->aln_path;
        //std::cout << "Writing rate matrices to file " << prefix << ".rateMatrices.txt" << std::endl;
        std::ofstream outFile(prefix + ".rateMatrices.txt");
        outFile << "Rate matrix for all sites: " << std::endl;
        printMatrix(getOriginalRateMatrix(), &outFile);
        for(int i = 0; i < genomeSize; i++) {
            outFile << "Position: " << i << std::endl;
            if(useSiteRates) {
                outFile << "Rate: " << rates[i] << std::endl;
            }
            outFile << "Rate Matrix: " << std::endl;
            printMatrix(getMutationMatrix(i), &outFile);
            outFile << std::endl;

        }
        outFile.close();
    } 
}

void ModelDNARateVariation::estimateRatePerSite(cmaple::Tree* tree){
    std::cout << "Estimating mutation rate per site..." << std::endl;
    RealNumType* waitingTimes = new RealNumType[num_states_ * genomeSize];
    RealNumType* numSubstitutions = new RealNumType[genomeSize];
    for(int i = 0; i < genomeSize; i++) {
        for(int j = 0; j < num_states_; j++) {
            waitingTimes[i * num_states_ + j] = 0;
        }
        numSubstitutions[i] = 0;
    }

    std::stack<Index> nodeStack;
    const PhyloNode& root = tree->nodes[tree->root_vector_index];
    nodeStack.push(root.getNeighborIndex(RIGHT));
    nodeStack.push(root.getNeighborIndex(LEFT));
    while(!nodeStack.empty()) {

        Index index = nodeStack.top();
        nodeStack.pop();
        PhyloNode& node = tree->nodes[index.getVectorIndex()];
        RealNumType blength = node.getUpperLength();
        //std::cout << "blength: " << blength  << std::endl;

        if (node.isInternal()) {
            nodeStack.push(node.getNeighborIndex(RIGHT));
            nodeStack.push(node.getNeighborIndex(LEFT));
        }

        if(blength <= 0.) {
            continue;
        }

        Index parent_index = node.getNeighborIndex(TOP);
        PhyloNode& parent_node = tree->nodes[parent_index.getVectorIndex()];
        const std::unique_ptr<SeqRegions>& parentRegions = parent_node.getPartialLh(parent_index.getMiniIndex());
        const std::unique_ptr<SeqRegions>& childRegions = node.getPartialLh(TOP);

        PositionType pos = 0;
        const SeqRegions& seq1_regions = *parentRegions;
        const SeqRegions& seq2_regions = *childRegions;
        size_t iseq1 = 0;
        size_t iseq2 = 0;

        while(pos < genomeSize) {
            PositionType end_pos;
            SeqRegions::getNextSharedSegment(pos, seq1_regions, seq2_regions, iseq1, iseq2, end_pos);
            const auto* seq1_region = &seq1_regions[iseq1];
            const auto* seq2_region = &seq2_regions[iseq2];

            if(seq1_region->type == TYPE_R && seq2_region->type == TYPE_R) {
                // both states are type REF
                for(int i = pos; i <= end_pos; i++) {
                    int state = tree->aln->ref_seq[static_cast<std::vector<cmaple::StateType>::size_type>(i)];
                    waitingTimes[i * num_states_ + state] += blength;
                }
            }  else if(seq1_region->type == seq2_region->type && seq1_region->type < TYPE_R) {
                // both states are equal but not of type REF
                 for(int i = pos; i <= end_pos; i++) {
                    waitingTimes[i * num_states_ + seq1_region->type] += blength;
                }               
            } else if(seq1_region->type <= TYPE_R && seq2_region->type <= TYPE_R) {
                // both states are not equal
                  for(int i = pos; i <= end_pos; i++) {
                    numSubstitutions[i] += 1;
                }                 
            }
            pos = end_pos + 1;
        }
    }

    RealNumType rateCount = 0;
    for(int i = 0; i < genomeSize; i++) {
        if(numSubstitutions[i] == 0) {
            rates[i] = 0.001;
        } else {
            RealNumType expectedRateNoSubstitution = 0;
            for(int j = 0; j < num_states_; j++) {
                RealNumType summand = waitingTimes[i * num_states_ + j] * abs(diagonal_mut_mat[j]);
                expectedRateNoSubstitution += summand;
            }
            if(expectedRateNoSubstitution <= 0.01) {
                rates[i] = 1.;
            } else {
                rates[i] = numSubstitutions[i] / expectedRateNoSubstitution; 
            }
        }
        rateCount += rates[i];
    }

    RealNumType averageRate = rateCount / genomeSize;
    for(int i = 0; i < genomeSize; i++) {
        rates[i] /= averageRate;
        rates[i] = MIN(100.0, MAX(0.0001, rates[i])); 
        for(int stateA = 0; stateA < num_states_; stateA++) {
            RealNumType rowSum = 0;
            for(int stateB = 0; stateB < num_states_; stateB++) {
                if(stateA != stateB) {
                    RealNumType val = mutationMatrices[i * matSize + (stateB + row_index[stateA])] * rates[i];
                    mutationMatrices[i * matSize + (stateB + row_index[stateA])] = val;
                    transposedMutationMatrices[i * matSize + (stateA + row_index[stateB])] = val;
                    freqiFreqjQijs[i * matSize + (stateB + row_index[stateA])] = root_freqs[stateA] * inverse_root_freqs[stateB] * val;
                    rowSum += val;
                }
            }
            mutationMatrices[i * matSize + (stateA + row_index[stateA])] = -rowSum;
            transposedMutationMatrices[i * matSize + (stateA + row_index[stateA])] = -rowSum;
            diagonalMutationMatrices[i * num_states_ + stateA] = -rowSum;
            freqiFreqjQijs[i * matSize + (stateA + row_index[stateA])] = -rowSum;

            // pre-compute matrix to speedup
            const RealNumType* transposed_mut_mat_row = getTransposedMutationMatrixRow(stateA, i);
            RealNumType* freqjTransposedijsRow = freqjTransposedijs + (i * matSize) + row_index[stateA];
            setVecByProduct<4>(freqjTransposedijsRow, root_freqs, transposed_mut_mat_row);
        
        }
    }

    delete[] waitingTimes;
    delete[] numSubstitutions;
}

void ModelDNARateVariation::estimateRatesPerSitePerEntry(cmaple::Tree* tree) {

    RealNumType* C = new RealNumType[genomeSize * matSize];
    RealNumType* W = new RealNumType[genomeSize * num_states_];
    for(int i = 0; i < genomeSize; i++) {
        for(int j = 0; j < num_states_; j++) {
            W[i * num_states_ + j] = 0;
            for(int k = 0; k < num_states_; k++) {
                C[i * num_states_ + row_index[j] + k] = 0;
            }
        }
    }
    std::stack<Index> nodeStack;
    const PhyloNode& root = tree->nodes[tree->root_vector_index];
    nodeStack.push(root.getNeighborIndex(RIGHT));
    nodeStack.push(root.getNeighborIndex(LEFT));
    while(!nodeStack.empty()) {

        Index index = nodeStack.top();
        nodeStack.pop();
        PhyloNode& node = tree->nodes[index.getVectorIndex()];
        RealNumType blength = node.getUpperLength();

        if (node.isInternal()) {
            nodeStack.push(node.getNeighborIndex(RIGHT));
            nodeStack.push(node.getNeighborIndex(LEFT));
        }

        if(blength <= 0.) {
            continue;
        }

        Index parent_index = node.getNeighborIndex(TOP);
        PhyloNode& parent_node = tree->nodes[parent_index.getVectorIndex()];
        const std::unique_ptr<SeqRegions>& parentRegions = parent_node.getPartialLh(parent_index.getMiniIndex());
        const std::unique_ptr<SeqRegions>& childRegions = node.getPartialLh(TOP);

        PositionType pos = 0;
        const SeqRegions& seqP_regions = *parentRegions;
        const SeqRegions& seqC_regions = *childRegions;
        size_t iseq1 = 0;
        size_t iseq2 = 0;

        while(pos < genomeSize) {
            PositionType end_pos;
            SeqRegions::getNextSharedSegment(pos, seqP_regions, seqC_regions, iseq1, iseq2, end_pos);
            const auto* seqP_region = &seqP_regions[iseq1];
            const auto* seqC_region = &seqC_regions[iseq2];

            // if the child of this branch does not observe its state directly then 
            // skip this branch.
            if(seqC_region->plength_observation2node > 0) {
                pos = end_pos + 1;
                continue;
            }

            // distance to last observation or root if last observation was across the root.
            RealNumType branchLengthToObservation = blength;
            if(seqP_region->plength_observation2node > 0 && seqP_region->plength_observation2root <= 0) {
                branchLengthToObservation = blength + seqP_region->plength_observation2node;
            }
            else if(seqP_region->plength_observation2root > 0) {
                branchLengthToObservation = blength + seqP_region->plength_observation2root;
            }

            if(seqP_region->type == TYPE_R && seqC_region->type == TYPE_R) {
                // both states are type REF
                for(int i = pos; i <= end_pos; i++) {
                    StateType state = tree->aln->ref_seq[static_cast<std::vector<cmaple::StateType>::size_type>(i)];
                    W[i * num_states_ + state] += branchLengthToObservation;
                }
            }  else if(seqP_region->type == seqC_region->type && seqP_region->type < TYPE_R) {
                // both states are equal but not of type REF
                 for(int i = pos; i <= end_pos; i++) {
                    W[i * num_states_ + seqP_region->type] += branchLengthToObservation;
                }               
            } else if(seqP_region->type <= TYPE_R && seqC_region->type <= TYPE_R) {
                //states are not equal
                StateType stateA = seqP_region->type;
                StateType stateB = seqC_region->type;
                if(seqP_region->type == TYPE_R) {
                    stateA = tree->aln->ref_seq[static_cast<std::vector<cmaple::StateType>::size_type>(end_pos)];
                }
                if(seqC_region->type == TYPE_R) {
                    stateB = tree->aln->ref_seq[static_cast<std::vector<cmaple::StateType>::size_type>(end_pos)];
                }
                // Case 1: Last observation was this side of the root node
                if(seqP_region->plength_observation2root <= 0) {
                    for(int i = pos; i <= end_pos; i++) {
                        W[i * num_states_ + stateA] += branchLengthToObservation/2;
                        W[i * num_states_ + stateB] += branchLengthToObservation/2;
                        C[i * matSize + stateB + row_index[stateA]] += 1;
                    }
                } else {
                    // Case 2: Last observation was the other side of the root.
                    // In this case there are two further cases - the mutation happened either side of the root.
                    // We calculate the relative likelihood of each case and use this to weight waiting times etc.
                    RealNumType distToRoot = seqP_region->plength_observation2root + blength;
                    RealNumType distToObserved = seqP_region->plength_observation2node;
                    updateCountsAndWaitingTimesAcrossRoot(pos, end_pos, stateA, stateB, distToRoot, distToObserved, W, C);
                }              
            } else if(seqP_region->type <= TYPE_R && seqC_region->type == TYPE_O) {
                StateType stateA = seqP_region->type;
                if(seqP_region->type == TYPE_R) {
                    stateA = tree->aln->ref_seq[static_cast<std::vector<cmaple::StateType>::size_type>(end_pos)];
                }

                // Calculate a weight vector giving the relative probabilities of observing
                // each state at the O node.
                std::vector<RealNumType> weightVector(num_states_);
                RealNumType sum = 0.0;
                for(StateType stateB = 0; stateB < num_states_; stateB++) {
                    RealNumType likelihoodB = std::round(seqC_region->getLH(stateB) * 1000) / 1000.0;
                    if(stateB != stateA) {
                        RealNumType prob = likelihoodB * branchLengthToObservation * getMutationMatrixEntry(stateA, stateB, end_pos);
                        weightVector[stateB] += prob;
                        sum += prob;
                    } else {
                        RealNumType prob = likelihoodB * (1 - branchLengthToObservation * getMutationMatrixEntry(stateB, stateB, end_pos));
                        weightVector[stateB] += prob;
                        sum += prob; 
                    }
                }
                // Normalise weight vector 
                normalize_arr(weightVector.data(), num_states_, sum);

                // Case 1: Last observation was this side of the root node
                if(seqP_region->plength_observation2root < 0) {
                    for(StateType stateB = 0; stateB < num_states_; stateB++) {
                        RealNumType prob = weightVector[stateB];
                        if(stateB != stateA) {
                            C[end_pos * matSize + stateB + row_index[stateA]] += prob;

                            W[end_pos * num_states_ + stateA] += prob * branchLengthToObservation/2;
                            W[end_pos * num_states_ + stateB] += prob * branchLengthToObservation/2;
                        } else {
                            W[end_pos * num_states_ + stateA] += prob * branchLengthToObservation;
                        }
                    }
                } else {
                    // Case 2: Last observation was the other side of the root.
                    RealNumType distToRoot = seqP_region->plength_observation2root + blength;
                    RealNumType distToObserved = seqP_region->plength_observation2node;
                     for(StateType stateB = 0; stateB < num_states_; stateB++) {
                        RealNumType prob = weightVector[stateB];
                        updateCountsAndWaitingTimesAcrossRoot(pos, end_pos, stateA, stateB, distToRoot, distToObserved, W, C, prob);
                     }
                }
            } else if(seqP_region->type == TYPE_O && seqC_region->type <= TYPE_R) {
                StateType stateB = seqC_region->type;
                if(seqC_region->type == TYPE_R) {
                    stateB = tree->aln->ref_seq[static_cast<std::vector<cmaple::StateType>::size_type>(end_pos)];
                }

                // Calculate a weight vector giving the relative probabilities of observing
                // each state at the O node.
                std::vector<RealNumType> weightVector(num_states_);
                RealNumType sum = 0.0;
                for(StateType stateA = 0; stateA < num_states_; stateA++) {
                    RealNumType likelihoodA = std::round(seqP_region->getLH(stateA) * 1000)/1000.0;
                    if(stateA != stateB) {
                        RealNumType prob = likelihoodA * branchLengthToObservation * getMutationMatrixEntry(stateA, stateB, end_pos);
                        weightVector[stateA] += prob;
                        sum += prob;
                    } else {
                        RealNumType prob = likelihoodA * (1 - branchLengthToObservation * getMutationMatrixEntry(stateA, stateA, end_pos));
                        weightVector[stateA] += prob;
                        sum += prob;
                    }
                }
                // Normalise weight vector 
                normalize_arr(weightVector.data(), num_states_, sum);

                // Case 1: Last observation was this side of the root node 
                if(seqP_region->plength_observation2root < 0) {
                    for(StateType stateA = 0; stateA < num_states_; stateA++) {
                        RealNumType prob = weightVector[stateA];
                        if(stateB != stateA) {
                            C[end_pos * matSize + stateB + row_index[stateA]] += prob;

                            W[end_pos * num_states_ + stateA] += prob * branchLengthToObservation/2;
                            W[end_pos * num_states_ + stateB] += prob * branchLengthToObservation/2;
                        } else {
                            W[end_pos * num_states_ + stateA] += prob * branchLengthToObservation;
                        }
                    }
                } else {
                    // Case 2: Last observation was the other side of the root.
                    RealNumType distToRoot = seqP_region->plength_observation2root + blength;
                    RealNumType distToObserved = seqP_region->plength_observation2node;
                    for(StateType stateA = 0; stateA < num_states_; stateA++) {
                        RealNumType prob = weightVector[stateA];
                        updateCountsAndWaitingTimesAcrossRoot(pos, end_pos, stateA, stateB, distToRoot, distToObserved, W, C, prob);
                    }                    
                }
            } else if(seqP_region->type == TYPE_O && seqC_region->type == TYPE_O) {
                // Calculate a weight vector giving the relative probabilities of observing
                // each state at each of the O nodes.
                std::vector<RealNumType> weightVector(num_states_ * num_states_);
                RealNumType sum = 0.0;
                for(StateType stateA = 0; stateA < num_states_; stateA++) {
                    RealNumType likelihoodA = std::round(seqP_region->getLH(stateA) * 1000)/1000.0;
                    for(StateType stateB = 0; stateB < num_states_; stateB++) {
                        RealNumType likelihoodB = std::round(seqC_region->getLH(stateB)*1000)/1000.0;
                        if(stateA != stateB) {
                            RealNumType prob = likelihoodA * likelihoodB * branchLengthToObservation * getMutationMatrixEntry(stateA, stateB, end_pos);
                            weightVector[row_index[stateA] + stateB] += prob;
                            sum += prob;
                        } else {
                            RealNumType prob = likelihoodA * likelihoodB * (1 - branchLengthToObservation * getMutationMatrixEntry(stateA, stateA, end_pos));
                            weightVector[row_index[stateA] + stateB] += prob;
                            sum += prob;
                        }
                    }
                }
                // Normalise weight vector 
                normalize_arr(weightVector.data(), num_states_*num_states_, sum);

                // Case 1: Last observation was this side of the root node
                if(seqP_region->plength_observation2root < 0) {
                    for(StateType stateA = 0; stateA < num_states_; stateA++) {
                        for(StateType stateB = 0; stateB < num_states_; stateB++) {
                            RealNumType prob = weightVector[row_index[stateA] + stateB];
                            if(stateB != stateA) {
                                C[end_pos * matSize + stateB + row_index[stateA]] += prob;

                                W[end_pos * num_states_ + stateA] +=  prob * branchLengthToObservation/2;
                                W[end_pos * num_states_ + stateB] +=  prob * branchLengthToObservation/2;
                            } else {
                                W[end_pos * num_states_ + stateA] +=  prob * branchLengthToObservation;
                            }
                        }
                    }
                } else {
                     // Case 2: Last observation was the other side of the root.
                    RealNumType distToRoot = seqP_region->plength_observation2root + blength;
                    RealNumType distToObserved = seqP_region->plength_observation2node;
                    for(StateType stateA = 0; stateA < num_states_; stateA++) {
                        for(StateType stateB = 0; stateB < num_states_; stateB++) {
                            RealNumType prob = weightVector[row_index[stateA] + stateB];
                            updateCountsAndWaitingTimesAcrossRoot(pos, end_pos, stateA, stateB, distToRoot, distToObserved, W, C, prob);
                        }
                    }                
                }
            } 
            pos = end_pos + 1;
        }
    }

    if(cmaple::verbose_mode > VB_MIN) 
    {
        const std::string prefix = tree->params->output_prefix.length() ? tree->params->output_prefix : tree->params->aln_path;
        //std::cout << "Writing rate matrices to file " << prefix << ".rateMatrices.txt" << std::endl;
        std::ofstream outFile(prefix + ".countMatrices.txt");
        for(int i = 0; i < genomeSize; i++) {
            outFile << "Position: " << i << std::endl;
            outFile << "Count Matrix: " << std::endl;
            printCountsAndWaitingTimes(C + (i * matSize), W + (i * num_states_), &outFile);
            outFile << std::endl;
        }
        outFile.close();
    }

    // Get genome-wide average mutation counts and waiting times
    RealNumType* globalCounts = new RealNumType[matSize];
    RealNumType* globalWaitingTimes = new RealNumType[num_states_];
    for(int i = 0; i < num_states_; i++) {
        globalWaitingTimes[i] = 0;
        for(int j = 0; j < num_states_; j++) {
            globalCounts[row_index[i] + j] = 0;
        }
    }

    for(int i = 0; i < genomeSize; i++) {
        for(int j = 0; j < num_states_; j++) {
            globalWaitingTimes[j] += W[i * num_states_ + j];
            for(int k = 0; k < num_states_; k++) {
                globalCounts[row_index[j] + k] += C[i * matSize + row_index[j] + k];
            }
        }        
    }

    for(int j = 0; j < num_states_; j++) {
        globalWaitingTimes[j] /= genomeSize;
        for(int k = 0; k < num_states_; k++) {
            globalCounts[row_index[j] + k] /= genomeSize;
        }
    }

    // Add pseudocounts
    for(int i = 0; i < genomeSize; i++) {
        for(int j = 0; j < num_states_; j++) {
            // Add pseudocounts to waitingTimes
            W[i * num_states_ + j] += waitingTimePseudoCount;
            for(int k = 0; k < num_states_; k++) {
                // Add pseudocount of average rate across genome * waitingTime pseudocount for counts
                C[i * matSize + row_index[j] + k] += globalCounts[row_index[j] + k] * waitingTimePseudoCount / globalWaitingTimes[j];
            }
        }
    }

    /*if(cmaple::verbose_mode > VB_MIN) 
    {
        RealNumType* referenceFreqs = new RealNumType[num_states_];
        for(int j = 0; j < num_states_; j++) {
            referenceFreqs[j] = 0;
        }
        for(int i = 0; i < genomeSize; i++) {
            referenceFreqs[tree->aln->ref_seq[i]]++;
        }
        for(int j = 0; j < num_states_; j++) {
            referenceFreqs[j] /= genomeSize;
        }

        std::cout << "Genome-wide average waiting times:\t\t";
        for(int j = 0; j < num_states_; j++) {
            std::cout << globalWaitingTimes[j] << "\t" ;
        }
        std::cout << std::endl;
        std::cout << "Reference nucleotide frequencies:\t\t";
        for(int j = 0; j < num_states_; j++) {
            std::cout << referenceFreqs[j] << "\t" ;
        }
        std::cout << std::endl;
        delete[] referenceFreqs;
    }*/

    delete[] globalCounts;
    delete[] globalWaitingTimes;

    RealNumType totalRate = 0;
    // Update mutation matrices with new rate estimation
    for(int i = 0; i < genomeSize; i++) {
        RealNumType* Ci = C + (i * matSize);
        RealNumType* Wi = W + (i * num_states_);
        StateType refState = tree->aln->ref_seq[static_cast<std::vector<cmaple::StateType>::size_type>(i)];

        for(int stateA = 0; stateA < num_states_; stateA++) {
            for(int stateB = 0; stateB < num_states_; stateB++) {
                if(stateA != stateB) { 
                    RealNumType newRate = Ci[stateB + row_index[stateA]] / Wi[stateA];                
                    mutationMatrices[i * matSize + (stateB + row_index[stateA])] = newRate;

                    // Approximate total rate by considering rates from reference nucleotide
                    if(refState == stateA) {
                        totalRate += newRate;
                    }
                }
            }
        }
    } 

    // Normalise entries of mutation matrices
    totalRate /= genomeSize;
    //RealNumType averageRate = totalRate / genomeSize;
    for(int i = 0; i < genomeSize; i++) {
        for(int stateA = 0; stateA < num_states_; stateA++) {
            RealNumType rowSum = 0;
            for(int stateB = 0; stateB < num_states_; stateB++) {
                if(stateA != stateB) {
                    RealNumType val = mutationMatrices[i * matSize + (stateB + row_index[stateA])];
                    //val /= averageRate;
                    val /= totalRate;
                    val = MIN(250.0, MAX(0.001, val)); 

                    mutationMatrices[i * matSize + (stateB + row_index[stateA])] = val;
                    transposedMutationMatrices[i * matSize + (stateA + row_index[stateB])] = val;
                    freqiFreqjQijs[i * matSize + (stateB + row_index[stateA])] = root_freqs[stateA] * inverse_root_freqs[stateB] * val;

                    rowSum += val;
                } 
            }
            mutationMatrices[i * matSize + (stateA + row_index[stateA])] = -rowSum;
            transposedMutationMatrices[i * matSize + (stateA + row_index[stateA])] = -rowSum;
            diagonalMutationMatrices[i * num_states_ + stateA] = -rowSum;
            freqiFreqjQijs[i * matSize + (stateA + row_index[stateA])] = -rowSum;

            // pre-compute matrix to speedup
            const RealNumType* transposed_mut_mat_row = getTransposedMutationMatrixRow(stateA, i);
            RealNumType* freqjTransposedijsRow = freqjTransposedijs + (i * matSize) + row_index[stateA];
            setVecByProduct<4>(freqjTransposedijsRow, root_freqs, transposed_mut_mat_row);
        
        }
    } 

    // Clean-up
    delete[] C;
    delete[] W;
}

void ModelDNARateVariation::updateCountsAndWaitingTimesAcrossRoot( PositionType start, PositionType end, 
                                                StateType parentState, StateType childState,
                                                RealNumType distToRoot, RealNumType distToObserved,
                                                RealNumType* waitingTimes, RealNumType* counts,
                                                RealNumType weight)
{
    if(parentState != childState) {
        for(int i = start; i <= end; i++) {
            RealNumType pRootIsStateParent = root_freqs[parentState] * getMutationMatrixEntry(parentState, childState, i) * distToRoot;
            RealNumType pRootIsStateChild = root_freqs[childState] * getMutationMatrixEntry(childState, parentState, i) * distToObserved;
            RealNumType relativeRootIsStateParent = pRootIsStateParent / (pRootIsStateParent + pRootIsStateChild);
            waitingTimes[i * num_states_ +  parentState] += weight * relativeRootIsStateParent * distToRoot/2;
            waitingTimes[i * num_states_ + childState] += weight * relativeRootIsStateParent * distToRoot/2;
            counts[i * matSize + childState + row_index[parentState]] += weight * relativeRootIsStateParent;

            RealNumType relativeRootIsStateChild = 1 - relativeRootIsStateParent;
            waitingTimes[i * num_states_ + childState] += weight * relativeRootIsStateChild * distToRoot;
        }
    } else {
        for(int i = start; i <= end; i++) {
            waitingTimes[i * num_states_ + childState] += weight * distToRoot;
        }      
    }
}

void ModelDNARateVariation::setAllMatricesToDefault() {
    for(int i = 0; i < genomeSize; i++) {
        for(int stateA = 0; stateA < num_states_; stateA++) {
            RealNumType rowSum = 0;
            for(int stateB = 0; stateB < num_states_; stateB++) {
                mutationMatrices[i * matSize + (stateB + row_index[stateA])] = mutation_mat[stateB + row_index[stateA]];
                transposedMutationMatrices[i * matSize + (stateB + row_index[stateA])] = transposed_mut_mat[stateB + row_index[stateA]];
                freqiFreqjQijs[i * matSize + (stateB + row_index[stateA])] = freqi_freqj_qij[stateB + row_index[stateA]];
            }

            diagonalMutationMatrices[i * num_states_ + stateA] = diagonal_mut_mat[stateA];

            // pre-compute matrix to speedup
            const RealNumType* transposed_mut_mat_row = getTransposedMutationMatrixRow(stateA, i);
            RealNumType* freqjTransposedijsRow = freqjTransposedijs + (i * matSize) + row_index[stateA];
            setVecByProduct<4>(freqjTransposedijsRow, root_freqs, transposed_mut_mat_row);
        
        }
    }  
}

void ModelDNARateVariation::setMatrixAtPosition(RealNumType* matrix, PositionType i) {
    for(int stateA = 0; stateA < num_states_; stateA++) {
        diagonalMutationMatrices[i * num_states_ + stateA] = matrix[stateA + row_index[stateA]];
        for(int stateB = 0; stateB < num_states_; stateB++) {
            mutationMatrices[i * matSize + (stateB + row_index[stateA])] = matrix[stateB + row_index[stateA]];
            transposedMutationMatrices[i * matSize + (stateB + row_index[stateA])] = matrix[stateA + row_index[stateB]];
        }
    }
}

void ModelDNARateVariation::readRatesFile() {
    std::ifstream infile(ratesFilename);
    std::string line;
    if (infile.is_open()) {
        PositionType genomePosition = 0;
        while (std::getline(infile, line)) {
            std::stringstream ss(line);
            std::string field;
            std::vector<std::string> fields;
            while (std::getline(ss, field, ' ')) {
                // Add the word to the vector
                fields.push_back(field);
            }

            if(fields.size() != 12) {
                std::cerr << "Error: Rate matrix file not in expected format." << std::endl;
                std::cerr << "Expected exactly 12 entries." << std::endl;
                continue;
            }
            RealNumType* rateMatrix = mutationMatrices + (genomePosition * matSize);
            
            // A row
            rateMatrix[1] = std::stof(fields[0]);
            rateMatrix[2] = std::stof(fields[1]);
            rateMatrix[3] = std::stof(fields[2]);

            // C row
            rateMatrix[4] = std::stof(fields[3]);
            rateMatrix[6] = std::stof(fields[4]);
            rateMatrix[7] = std::stof(fields[5]);

            // G row
            rateMatrix[8] = std::stof(fields[6]);
            rateMatrix[9] = std::stof(fields[7]);
            rateMatrix[11] = std::stof(fields[8]);

             // T row
            rateMatrix[12] = std::stof(fields[9]);
            rateMatrix[13] = std::stof(fields[10]);
            rateMatrix[14] = std::stof(fields[11]);

            genomePosition++;
        }
        infile.close();

        if(genomePosition != genomeSize) {
            std::cerr << "Error: Rate matrix file contained unexpected number of lines." << std::endl;
            return;
        }

        for(int i = 0; i < genomeSize; i++) {
            for(int stateA = 0; stateA < num_states_; stateA++) {
                RealNumType rowSum = 0;
                for(int stateB = 0; stateB < num_states_; stateB++) {
                    if(stateA != stateB) {
                        RealNumType val = mutationMatrices[i * matSize + (stateB + row_index[stateA])];
                        mutationMatrices[i * matSize + (stateB + row_index[stateA])] = val;
                        transposedMutationMatrices[i * matSize + (stateA + row_index[stateB])] = val;
                        freqiFreqjQijs[i * matSize + (stateB + row_index[stateA])] = root_freqs[stateA] * inverse_root_freqs[stateB] * val;

                        rowSum += val;
                    } 
                }
                mutationMatrices[i * matSize + (stateA + row_index[stateA])] = -rowSum;
                transposedMutationMatrices[i * matSize + (stateA + row_index[stateA])] = -rowSum;
                diagonalMutationMatrices[i * num_states_ + stateA] = -rowSum;
                freqiFreqjQijs[i * matSize + (stateA + row_index[stateA])] = -rowSum;

                // pre-compute matrix to speedup
                const RealNumType* transposed_mut_mat_row = getTransposedMutationMatrixRow(stateA, i);
                RealNumType* freqjTransposedijsRow = freqjTransposedijs + (i * matSize) + row_index[stateA];
                setVecByProduct<4>(freqjTransposedijsRow, root_freqs, transposed_mut_mat_row);      
            }
        } 
    }
    else {
        std::cerr << "Unable to open rate matrix file " << ratesFilename << std::endl;
    }
}