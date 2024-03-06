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
        { "verbose",             "Sets the verbosity of the output", "0" },
        { "args",                "Sets the arguments to describe Spatter pattern(s)", "" },
    )

    SST_ELI_DOCUMENT_STATISTICS(
        { "total_bytes_read",    "Count the total bytes requested by read operations", "bytes", 1 },
        { "total_bytes_write",   "Count the total bytes requested by write operations", "bytes", 1 },
        { "req_latency",         "Running total of all latency for all requests", "ns", 2 },
    )

private:
    void count_args(const std::string &args, int32_t &argc);
    void tokenize_args(const std::string &args, const int32_t &argc, char ***argv);
    void update_indices();
    void print_stats();
    
    uint64_t calc_bytes(const Spatter::ConfigurationBase *config);
    size_t get_pattern_size(const Spatter::ConfigurationBase *config);

    void gather();
    void scatter();
    void scatter_gather();
    void multi_gather();
    void multi_scatter();
    
    uint64_t reqLength;
    size_t configIdx;
    size_t countIdx;
    size_t patternIdx;
    bool configFin;
    
    Statistic<uint64_t>* statReadBytes;
    Statistic<uint64_t>* statWriteBytes;
    Statistic<uint64_t>* statReqLatency;
    Statistic<uint64_t>* statTime;

    MirandaRequestQueue<GeneratorRequest*>* queue;
    Output* out;
    
    Spatter::ClArgs cl;
};

}
}

#endif
