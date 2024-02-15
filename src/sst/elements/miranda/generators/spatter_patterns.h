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
        { "startat",      "Sets the start address of the array", "0" },
        { "max_address",  "Maximum address allowed for generation", "524288" },
    )

private:
    void count_args(const std::string &args, int32_t &argc);
    void tokenize_args(const std::string &args, const int32_t &argc, char ***argv);
    void update_indices();
    
    uint32_t reqLength;
    uint64_t maxAddr;
    uint64_t startAddr;

    uint64_t configIdx;
    uint64_t countIdx;
    uint64_t patternIdx;

    Output*  out;
    
    Spatter::ClArgs cl;
};

}
}

#endif
