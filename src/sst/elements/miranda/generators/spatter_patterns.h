// Copyright 2009-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2023, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_SST_MIRANDA_SPATTER_PATTERNS_GEN
#define _H_SST_MIRANDA_SPATTER_PATTERNS_GEN

#include <sst/elements/miranda/mirandaGenerator.h>
#include <sst/core/output.h>

#include <Spatter/Input.hh>

#include <queue>

namespace SST {
namespace Miranda {

class SpatterPatternsGenerator : public RequestGenerator {

public:
    SpatterPatternsGenerator( ComponentId_t id, Params& params );
    void build(Params& params);
    ~SpatterPatternsGenerator();
    void generate(MirandaRequestQueue<GeneratorRequest*>* q);
    bool isFinished();
    void completed();

    SST_ELI_REGISTER_SUBCOMPONENT(
        SpatterPatternsGenerator,
        "miranda",
        "SpatterPatternsGenerator",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Creates a stream of gather/scatter operations based on a Spatter pattern",
        SST::Miranda::RequestGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
        { "verbose",      "Sets the verbosity of the output", "0" },
        { "length",       "Sets the length of the request", "8" },
        { "start_src",    "Sets the start address of the source array", "0" },
        { "start_dst",    "Sets the start address of the destination array", "0" },
        { "args",         "Sets the arguments to describe Spatter pattern(s)", "" },
    )

private:
    void count_args(const std::string &args, int32_t &argc);
    void tokenize_args(const std::string &args, const int32_t &argc, char ***argv);
    void update_indices();

    void gather();
    void scatter();
    void scatter_gather();
    void multi_gather();
    void multi_scatter();
    
    uint64_t reqLength;
    size_t configIdx;
    size_t countIdx;
    size_t patternIdx;

    MirandaRequestQueue<GeneratorRequest*>* queue;
    Output* out;
    
    Spatter::ClArgs cl;
};

}
}

#endif
