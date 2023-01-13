#include "gtest/gtest.h"
#include "alignment/alignment.h"

/*
 Test readSequences(), readFasta(), readPhylip(), and generateRef()
 */
TEST(Alignment, readSequences)
{
    Alignment aln;
    StrVector sequences, seq_names;
    
    // Test readFasta()
    std::string file_path = "../../example/input.fa";
    char* file_path_ptr = new char[file_path.length() + 1];
    strcpy(file_path_ptr, file_path.c_str());
    aln.readSequences(file_path_ptr, sequences, seq_names);
    EXPECT_EQ(sequences.size(), 10);
    EXPECT_EQ(sequences.size(), seq_names.size());
    EXPECT_EQ(sequences[1], "ATTAAAGGTTTATACCTVCA");
    EXPECT_EQ(seq_names[9], "T9");
    
    // Test generateRef()
    std::string ref_str = std::move(aln.generateRef(sequences));
    EXPECT_EQ(aln.ref_seq.size(), 20);
    EXPECT_EQ(ref_str.length(), 20);
    EXPECT_EQ(ref_str[0], 'A');
    EXPECT_EQ(ref_str[7], 'G');
    EXPECT_EQ(ref_str[17], 'A');
    
    // Test readPhylip()
    sequences.clear();
    seq_names.clear();
    file_path = "../../example/input.phy";
    delete[] file_path_ptr;
    file_path_ptr = new char[file_path.length() + 1];
    strcpy(file_path_ptr, file_path.c_str());
    aln.readSequences(file_path_ptr, sequences, seq_names);
    EXPECT_EQ(sequences.size(), 10);
    EXPECT_EQ(sequences.size(), seq_names.size());
    EXPECT_EQ(sequences[1], "ATTAAAGGTTTATACCTVCA");
    EXPECT_EQ(seq_names[9], "T9");
    
    // Test generateRef()
    ref_str = std::move(aln.generateRef(sequences));
    EXPECT_EQ(aln.ref_seq.size(), 20);
    EXPECT_EQ(aln.ref_seq[0], 0);
    EXPECT_EQ(aln.ref_seq[10], 3);
    EXPECT_EQ(aln.ref_seq[18], 1);
}

/*
 Test readRef(char* ref_path)
 also test parseRefSeq(std::string& ref_sequence)
 */
TEST(Alignment, readRef)
{
    Alignment aln;
    std::string file_path("../../example/ref.fa");
    char* file_path_ptr = new char[file_path.length() + 1];
    strcpy(file_path_ptr, file_path.c_str());
    aln.readRef(file_path_ptr);
    
    EXPECT_EQ(aln.ref_seq.size(), 20);
    EXPECT_EQ(aln.ref_seq[0], 0);
    EXPECT_EQ(aln.ref_seq[2], 3);
    EXPECT_EQ(aln.ref_seq[7], 2);
    EXPECT_EQ(aln.ref_seq[11], 0);
}

/*
 Test extractMutations(StrVector &sequences, StrVector &seq_names, std::string ref_sequence, std::ofstream &out, bool only_extract_diff)
 Also test outputMutation()
 */
TEST(Alignment, extractMutations)
{
    // Don't need to test empty inputs (sequences, seq_names, ref_sequence); we have ASSERT to check them in extractMutations()
    // ----- test on input.fa; generate ref_sequence -----
    Alignment aln;
    StrVector sequences, seq_names;
    std::string ref_sequence;
    
    // output file
    std::string diff_file_path("../../example/output.diff");
    char* diff_file_path_ptr = new char[diff_file_path.length() + 1];
    strcpy(diff_file_path_ptr, diff_file_path.c_str());
    
    // read sequences
    std::string file_path = "../../example/input.fa";
    char* file_path_ptr = new char[file_path.length() + 1];
    strcpy(file_path_ptr, file_path.c_str());
    aln.readSequences(file_path_ptr, sequences, seq_names);
    
    // generate ref_sequence
    ref_sequence = aln.generateRef(sequences);

    // open the output file
    std::ofstream out = std::ofstream(diff_file_path_ptr);
    
    // write reference sequence to the output file
    out << ">" << REF_NAME << std::endl;
    out << ref_sequence << std::endl;
    
    // extract and write mutations of each sequence to file
    aln.extractMutations(sequences, seq_names, ref_sequence, out, true);
    out.close();
    
    // test the output data
    EXPECT_EQ(aln.data.size(), 0); // because only_extract_diff = true;
    EXPECT_EQ(aln.ref_seq.size(), 20);
    // ----- test on input.fa; generate ref_sequence-----
    
    // ----- test on input.fa; read ref_sequence -----
    std::string ref_file_path = "../../example/ref.fa";
    char* ref_file_path_ptr = new char[ref_file_path.length() + 1];
    strcpy(ref_file_path_ptr, ref_file_path.c_str());
    ref_sequence = aln.readRef(ref_file_path_ptr);
    
    // open the output file
    out = std::ofstream(diff_file_path_ptr);
    
    // write reference sequence to the output file
    out << ">" << REF_NAME << std::endl;
    out << ref_sequence << std::endl;
    
    // extract and write mutations of each sequence to file
    aln.extractMutations(sequences, seq_names, ref_sequence, out, false);
    out.close();
    
    // test the output data
    EXPECT_EQ(aln.data.size(), 10);
    EXPECT_EQ(aln.data[1].seq_name, "T2");
    EXPECT_EQ(aln.data[6].size(), 2);
    EXPECT_EQ(aln.data[7][0].type, TYPE_DEL);
    EXPECT_EQ(aln.data[7][1].getLength(), 2);
    EXPECT_EQ(aln.data[9][0].position, 7);
    EXPECT_EQ(aln.ref_seq.size(), 20);
    // ----- test on input.fa; read ref_sequence -----
    
    /*// ----- test on input_full.phy; generate ref_sequence -----
    aln.data.clear();
    sequences.clear();
    seq_names.clear();
    ref_sequence = "";
    
    // read sequences
    file_path = "../../example/input_full.phy";
    file_path_ptr = new char[file_path.length() + 1];
    strcpy(file_path_ptr, file_path.c_str());
    aln.readSequences(file_path_ptr, sequences, seq_names);
    
    // generate ref_sequence
    ref_sequence = aln.generateRef(sequences);
    
    // open the output file
    out = std::ofstream(diff_file_path_ptr);
    
    // write reference sequence to the output file
    out << ">" << REF_NAME << std::endl;
    out << ref_sequence << std::endl;
    
    // extract and write mutations of each sequence to file
    aln.extractMutations(sequences, seq_names, ref_sequence, out, false);
    out.close();
    
    // test the output data
    EXPECT_EQ(aln.data.size(), 1000);
    EXPECT_EQ(aln.data[3].seq_name, "T881");
    EXPECT_EQ(aln.data[220].size(), 284);
    EXPECT_EQ(aln.data[390][7].type, 2);
    EXPECT_EQ(aln.data[477][186].getLength(), 1);
    EXPECT_EQ(aln.data[796][356].position, 28897);
    EXPECT_EQ(aln.ref_seq.size(), 29903);
    // ----- test on input_full.phy; generate ref_sequence -----*/
}

/*
 Test readDiff(char* diff_path, char* ref_path)
 */
TEST(Alignment, readDiff)
{
    Alignment aln;
    
    // ----- test on test_100.diff -----
    std::string diff_file_path("../../example/test_100.diff");
    char* diff_file_path_ptr = new char[diff_file_path.length() + 1];
    strcpy(diff_file_path_ptr, diff_file_path.c_str());
    aln.readDiff(diff_file_path_ptr, NULL); // read ref_seq from the diff file
    
    // test the output data
    EXPECT_EQ(aln.data.size(), 100);
    EXPECT_EQ(aln.data[4].seq_name, "4");
    EXPECT_EQ(aln.data[18].size(), 33);
    EXPECT_EQ(aln.data[90][34].type, 1);
    EXPECT_EQ(aln.data[53][7].getLength(), 1);
    EXPECT_EQ(aln.data[46][11].position, 10516);
    EXPECT_EQ(aln.data[38].size(), 32);
    EXPECT_EQ(aln.data[49][5].type, 3);
    EXPECT_EQ(aln.data[66][30].getLength(), 1);
    EXPECT_EQ(aln.data[75][3].position, 11272);
    EXPECT_EQ(aln.data[30].size(), 35);
    EXPECT_EQ(aln.data[43][25].type, 3);
    EXPECT_EQ(aln.data[51][27].getLength(), 1);
    EXPECT_EQ(aln.data[66][4].position, 1426);

    EXPECT_EQ(aln.ref_seq.size(), 29891);
    EXPECT_EQ(aln.ref_seq[8], 3);
    EXPECT_EQ(aln.ref_seq[467], 0);
    EXPECT_EQ(aln.ref_seq[1593], 1);
    // ----- test on test_100.diff -----
    
    // ----- test on test_5K.diff, load ref_seq from test_100.diff -----
    aln.data.clear();
    diff_file_path = "../../example/test_5K.diff";
    delete[] diff_file_path_ptr;
    diff_file_path_ptr = new char[diff_file_path.length() + 1];
    strcpy(diff_file_path_ptr, diff_file_path.c_str());
    std::string ref_file_path("../../example/test_100.diff");
    char* ref_file_path_ptr = new char[ref_file_path.length() + 1];
    strcpy(ref_file_path_ptr, ref_file_path.c_str());
    aln.readDiff(diff_file_path_ptr, ref_file_path_ptr);
    
    // test the output data
    EXPECT_EQ(aln.data.size(), 5000);
    EXPECT_EQ(aln.data[454].seq_name, "454");
    EXPECT_EQ(aln.data[1328].size(), 15);
    EXPECT_EQ(aln.data[943][22].type, 1);
    EXPECT_EQ(aln.data[953][9].getLength(), 1);
    EXPECT_EQ(aln.data[76][25].position, 240);
    EXPECT_EQ(aln.data[1543].size(), 12);
    EXPECT_EQ(aln.data[2435][8].type, 0);
    EXPECT_EQ(aln.data[4864][17].getLength(), 1);
    EXPECT_EQ(aln.data[3854][23].position, 14407);
    EXPECT_EQ(aln.data[2454].size(), 12);
    EXPECT_EQ(aln.data[4423][14].type, 0);
    EXPECT_EQ(aln.data[643][5].getLength(), 1);
    EXPECT_EQ(aln.data[59][12].position, 25540);

    EXPECT_EQ(aln.ref_seq.size(), 29891);
    EXPECT_EQ(aln.ref_seq[8], 3);
    EXPECT_EQ(aln.ref_seq[467], 0);
    EXPECT_EQ(aln.ref_seq[1593], 1);
    // ----- test on test_5K.diff, load ref_seq from test_100.diff -----
}

/*
 Test extractDiffFile(Params& params)
 */
TEST(Alignment, extractDiffFile)
{
    Alignment aln;
    
    // ----- test on input.phy with ref file from ref.fa -----
    Params params = Params::getInstance();
    params.redo_inference = true;
    std::string aln_file_path("../../example/input.phy");
    char* aln_file_path_ptr = new char[aln_file_path.length() + 1];
    strcpy(aln_file_path_ptr, aln_file_path.c_str());
    params.aln_path = aln_file_path_ptr;
    std::string ref_file_path("../../example/ref.fa");
    char* ref_file_path_ptr = new char[ref_file_path.length() + 1];
    strcpy(ref_file_path_ptr, ref_file_path.c_str());
    params.ref_path = ref_file_path_ptr;
    
    // extract diff file
    aln.extractDiffFile(params);
    
    // reset data
    aln.data.clear();
    aln.ref_seq.clear();
    
    // read diff file (for testing)
    std::string diff_file_path(aln_file_path + ".diff");
    char* diff_file_path_ptr = new char[diff_file_path.length() + 1];
    strcpy(diff_file_path_ptr, diff_file_path.c_str());
    aln.readDiff(diff_file_path_ptr, NULL); // read ref_seq from the diff file
    
    // test the output data
    EXPECT_EQ(aln.data.size(), 10);
    EXPECT_EQ(aln.data[0].seq_name, "T1");
    EXPECT_EQ(aln.data[1].size(), 2);
    EXPECT_EQ(aln.data[2][0].type, TYPE_N);
    EXPECT_EQ(aln.data[3][0].getLength(), 1);
    EXPECT_EQ(aln.data[4][1].position, 17);
    EXPECT_EQ(aln.data[5].size(), 5);
    EXPECT_EQ(aln.data[6][1].type, 3);
    EXPECT_EQ(aln.data[7][2].getLength(), 1);
    EXPECT_EQ(aln.data[8][1].position, 12);
    EXPECT_EQ(aln.data[9].size(), 2);
    EXPECT_EQ(aln.data[0][0].type, 1);
    EXPECT_EQ(aln.data[1][1].getLength(), 1);
    EXPECT_EQ(aln.data[2][2].position, 17);

    EXPECT_EQ(aln.ref_seq.size(), 20);
    EXPECT_EQ(aln.ref_seq[8], 3);
    EXPECT_EQ(aln.ref_seq[2], 3);
    EXPECT_EQ(aln.ref_seq[15], 1);
    // ----- test on input.phy with ref file from ref.fa -----
    
    /*// ----- test on input.fa without ref file, specifying diff file path -----
    aln.data.clear();
    aln.ref_seq.clear();
    aln_file_path = "../../example/input_full.fa";
    delete[] aln_file_path_ptr;
    aln_file_path_ptr = new char[aln_file_path.length() + 1];
    strcpy(aln_file_path_ptr, aln_file_path.c_str());
    params.aln_path = aln_file_path_ptr;
    delete[] ref_file_path_ptr;
    params.ref_path = NULL;
    
    diff_file_path = aln_file_path + ".output.diff";
    delete[] diff_file_path_ptr;
    diff_file_path_ptr = new char[diff_file_path.length() + 1];
    strcpy(diff_file_path_ptr, diff_file_path.c_str());
    params.diff_path = diff_file_path_ptr;
    
    // extract diff file
    aln.extractDiffFile(params);
    
    // reset data
    aln.data.clear();
    aln.ref_seq.clear();
    
    // read diff file (for testing)
    aln.readDiff(diff_file_path_ptr, NULL); // read ref_seq from the diff file
    
    // test the output data
    EXPECT_EQ(aln.data.size(), 1000);
    EXPECT_EQ(aln.data[432].seq_name, "T400");
    EXPECT_EQ(aln.data[745].size(), 127);
    EXPECT_EQ(aln.data[23][12].type, 1);
    EXPECT_EQ(aln.data[8][6].getLength(), 1);
    EXPECT_EQ(aln.data[475][18].position, 4082);
    EXPECT_EQ(aln.data[51].size(), 355);
    EXPECT_EQ(aln.data[16][5].type, 1);
    EXPECT_EQ(aln.data[678][22].getLength(), 1);
    EXPECT_EQ(aln.data[87][14].position, 3216);
    EXPECT_EQ(aln.data[954].size(), 232);
    EXPECT_EQ(aln.data[245][7].type, 2);
    EXPECT_EQ(aln.data[58][4].getLength(), 1);
    EXPECT_EQ(aln.data[47][20].position, 2716);

    EXPECT_EQ(aln.ref_seq.size(), 29903);
    EXPECT_EQ(aln.ref_seq[8424], 0);
    EXPECT_EQ(aln.ref_seq[223], 3);
    EXPECT_EQ(aln.ref_seq[175], 1);
    // ----- with/without diff file path -----*/
}

/*
 Test convertState2Char(StateType state)
 */
TEST(Alignment, convertState2Char)
{
    Alignment aln;
    EXPECT_EQ(aln.convertState2Char(-1), '?');
    EXPECT_EQ(aln.convertState2Char(0.1), 'A');
    EXPECT_EQ(aln.convertState2Char(0.99), 'A');
    EXPECT_EQ(aln.convertState2Char(1000), '?');
    
    EXPECT_EQ(aln.convertState2Char(TYPE_N), '-');
    EXPECT_EQ(aln.convertState2Char(TYPE_DEL), '-');
    EXPECT_EQ(aln.convertState2Char(TYPE_INVALID), '?');
    
    EXPECT_EQ(aln.convertState2Char(0), 'A');
    EXPECT_EQ(aln.convertState2Char(3), 'T');
    
    EXPECT_EQ(aln.convertState2Char(1+4+3), 'R');
    EXPECT_EQ(aln.convertState2Char(2+8+3), 'Y');
    EXPECT_EQ(aln.convertState2Char(2+4+8+3), 'B');
    EXPECT_EQ(aln.convertState2Char(1+2+4+3), 'V');
    EXPECT_EQ(aln.convertState2Char(1+2+3), 'M');
}

/*
 Test convertChar2State(char state)
 */
TEST(Alignment, convertChar2State)
{
    Alignment aln;
    
    // Note: We don't test invalid characters since CMaple will output an error msg and terminate;
    // convertChar2State requires input is a capital character
    
    EXPECT_EQ(aln.convertChar2State('-'), TYPE_DEL);
    EXPECT_EQ(aln.convertChar2State('?'), TYPE_N);
    EXPECT_EQ(aln.convertChar2State('.'), TYPE_N);
    EXPECT_EQ(aln.convertChar2State('~'), TYPE_N);
    
    EXPECT_EQ(aln.convertChar2State('C'), 1);
    EXPECT_EQ(aln.convertChar2State('G'), 2);
    EXPECT_EQ(aln.convertChar2State('X'), TYPE_N);
    EXPECT_EQ(aln.convertChar2State('Y'), 2+8+3);
    EXPECT_EQ(aln.convertChar2State('K'), 4+8+3);
    EXPECT_EQ(aln.convertChar2State('B'), 2+4+8+3);
    EXPECT_EQ(aln.convertChar2State('D'), 1+4+8+3);
    EXPECT_EQ(aln.convertChar2State('V'), 1+2+4+3);
}

/*
 Test computeSeqDistance(Sequence& sequence, RealNumType hamming_weight)
 */
TEST(Alignment, computeSeqDistance)
{
    Alignment aln;
    
    // test empty sequence
    Sequence sequence1;
    EXPECT_EQ(aln.computeSeqDistance(sequence1, 1), 0);
    
    // test other sequences with different hamming_weight
    Sequence sequence2;
    sequence2.emplace_back(1, 313, 1);
    EXPECT_EQ(aln.computeSeqDistance(sequence2, 0), 0);
    EXPECT_EQ(aln.computeSeqDistance(sequence2, 1), 1);
    EXPECT_EQ(aln.computeSeqDistance(sequence2, 1000), 1000);
    
    Sequence sequence3;
    sequence3.emplace_back(TYPE_N, 65, 142);
    EXPECT_EQ(aln.computeSeqDistance(sequence3, 0), 142);
    EXPECT_EQ(aln.computeSeqDistance(sequence3, 1), 143);
    EXPECT_EQ(aln.computeSeqDistance(sequence3, 1000), 1142);
    
    Sequence sequence4;
    sequence4.emplace_back(TYPE_DEL, 65, 142);
    EXPECT_EQ(aln.computeSeqDistance(sequence4, 0), aln.computeSeqDistance(sequence3, 0));
    EXPECT_EQ(aln.computeSeqDistance(sequence4, 1), aln.computeSeqDistance(sequence3, 1));
    EXPECT_EQ(aln.computeSeqDistance(sequence4, 1000), aln.computeSeqDistance(sequence3, 1000));
    
    Sequence sequence5;
    sequence5.emplace_back(TYPE_O, 65, 1);
    EXPECT_EQ(aln.computeSeqDistance(sequence5, 0), 1);
    EXPECT_EQ(aln.computeSeqDistance(sequence5, 1), 2);
    EXPECT_EQ(aln.computeSeqDistance(sequence5, 1000), 1001);
    
    Sequence sequence6;
    sequence6.emplace_back(1, 132, 1);
    sequence6.emplace_back(TYPE_DEL, 153, 3);
    sequence6.emplace_back(1+4+3, 154, 1);
    sequence6.emplace_back(TYPE_N, 5434, 943);
    sequence6.emplace_back(1+2+8+3, 6390, 1);
    sequence6.emplace_back(0, 9563, 1);
    sequence6.emplace_back(TYPE_N, 15209, 8);
    sequence6.emplace_back(3, 28967, 1);
    sequence6.emplace_back(1+2+4+3, 28968, 1);
    
    EXPECT_EQ(aln.computeSeqDistance(sequence6, 0), 957);
    EXPECT_EQ(aln.computeSeqDistance(sequence6, 1), 966);
    EXPECT_EQ(aln.computeSeqDistance(sequence6, 1000), 9957);
}

/*
 Test sortSeqsByDistances(RealNumType hamming_weight)
 */
TEST(Alignment, sortSeqsByDistances)
{
    Alignment aln;
    
    // ----- test empty aln -----
    aln.sortSeqsByDistances(1000);
    
    // ----- test aln with one empty sequence -----
    Sequence sequence1(std::move("sequence 1")); // distance = 0
    aln.data.push_back(std::move(sequence1));
    aln.sortSeqsByDistances(1000);
    EXPECT_EQ(aln.data.size(), 1);
    EXPECT_EQ(aln.data[0].seq_name, "sequence 1");
    
    // ----- test aln with two empty sequences -----
    Sequence sequence2(std::move("sequence 2")); // distance = 0
    aln.data.push_back(std::move(sequence2));
    aln.sortSeqsByDistances(1000);
    EXPECT_EQ(aln.data.size(), 2);
    EXPECT_EQ(aln.data[0].seq_name, "sequence 2"); // sequence 1 and sequence 2 have the same zero-distance => quicksort arbitrarily put sequence 2 before sequence 1
    
    // ----- test with more sequences -----
    Sequence sequence3(std::move("sequence 3")); // distance = 1000
    sequence3.emplace_back(1, 313, 1);
    aln.data.push_back(std::move(sequence3));
    aln.sortSeqsByDistances(1000);
    EXPECT_EQ(aln.data.size(), 3);
    EXPECT_EQ(aln.data[0].seq_name, "sequence 1");
    EXPECT_EQ(aln.data[2].seq_name, "sequence 3");
    
    Sequence sequence4(std::move("sequence 4")); // distance = 1142
    sequence4.emplace_back(TYPE_DEL, 65, 142);
    aln.data.push_back(std::move(sequence4));
    aln.sortSeqsByDistances(1000);
    EXPECT_EQ(aln.data.size(), 4);
    EXPECT_EQ(aln.data[2].seq_name, "sequence 3");
    EXPECT_EQ(aln.data[3].seq_name, "sequence 4");
    
    Sequence sequence5(std::move("sequence 5")); // distance = 9957
    sequence5.emplace_back(1, 132, 1);
    sequence5.emplace_back(TYPE_DEL, 153, 3);
    sequence5.emplace_back(1+4+3, 154, 1);
    sequence5.emplace_back(TYPE_N, 5434, 943);
    sequence5.emplace_back(1+2+8+3, 6390, 1);
    sequence5.emplace_back(0, 9563, 1);
    sequence5.emplace_back(TYPE_N, 15209, 8);
    sequence5.emplace_back(3, 28967, 1);
    sequence5.emplace_back(1+2+4+3, 28968, 1);
    aln.data.push_back(std::move(sequence5));
    aln.sortSeqsByDistances(1000);
    EXPECT_EQ(aln.data.size(), 5);
    EXPECT_EQ(aln.data[2].seq_name, "sequence 3");
    EXPECT_EQ(aln.data[4].seq_name, "sequence 5");
    
    Sequence sequence6(std::move("sequence 6")); // distance = 1001
    sequence6.emplace_back(TYPE_O, 65, 1);
    aln.data.push_back(std::move(sequence6));
    aln.sortSeqsByDistances(1000);
    EXPECT_EQ(aln.data.size(), 6);
    EXPECT_EQ(aln.data[2].seq_name, "sequence 3");
    EXPECT_EQ(aln.data[3].seq_name, "sequence 6");
    EXPECT_EQ(aln.data[4].seq_name, "sequence 4");
    EXPECT_EQ(aln.data[5].seq_name, "sequence 5");
    
    // ----- test on data from diff file -----
    aln.data.clear();
    std::string diff_file_path("../../example/test_100.diff");
    char* diff_file_path_ptr = new char[diff_file_path.length() + 1];
    strcpy(diff_file_path_ptr, diff_file_path.c_str());
    aln.readDiff(diff_file_path_ptr, NULL); // read ref_seq from the diff file
    
    aln.sortSeqsByDistances(0);
    EXPECT_EQ(aln.data.size(), 100);
    EXPECT_EQ(aln.data[45].seq_name, "85");
    EXPECT_EQ(aln.data[32].seq_name, "98");
    EXPECT_EQ(aln.data[84].seq_name, "44");
    EXPECT_EQ(aln.data[67].seq_name, "1");
    EXPECT_EQ(aln.data[92].seq_name, "26");
    
    aln.sortSeqsByDistances(1);
    EXPECT_EQ(aln.data[26].seq_name, "62");
    EXPECT_EQ(aln.data[74].seq_name, "4");
    EXPECT_EQ(aln.data[33].seq_name, "52");
    EXPECT_EQ(aln.data[28].seq_name, "98");
    EXPECT_EQ(aln.data[19].seq_name, "53");
    
    aln.sortSeqsByDistances(1000);
    EXPECT_EQ(aln.data[83].seq_name, "27");
    EXPECT_EQ(aln.data[47].seq_name, "96");
    EXPECT_EQ(aln.data[39].seq_name, "65");
    EXPECT_EQ(aln.data[62].seq_name, "3");
    EXPECT_EQ(aln.data[74].seq_name, "64");
}