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


#ifndef _H_SST_MIRANDA_SPATTER_BENCH_GEN
#define _H_SST_MIRANDA_SPATTER_BENCH_GEN

#include <sst/elements/miranda/mirandaGenerator.h>
#include <sst/core/output.h>

#include <queue>

#include <Spatter/Input.hh>

namespace SST {
namespace Miranda {

class SpatterBenchGenerator : public RequestGenerator {

public:
    SpatterBenchGenerator( ComponentId_t id, Params& params );
    void build(Params& params);
    ~SpatterBenchGenerator();
    void generate(MirandaRequestQueue<GeneratorRequest*>* q);
    bool isFinished();
    void completed();

    SST_ELI_REGISTER_SUBCOMPONENT(
        SpatterBenchGenerator,
        "miranda",
        "SpatterBenchGenerator",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Creates a stream of gather/scatter operations based on a Spatter pattern",
        SST::Miranda::RequestGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
        { "verbose",             "Sets the verbosity of the output", "0" },
        { "args",                "Sets the arguments to describe Spatter pattern(s)", "" },
        { "datawidth",           "Sets the width of the memory operation", "8" },
        { "start_source",        "Sets the start address of the source array", "0" },
        { "start_target",        "Sets the start address of the target array", "0" }
    )

    SST_ELI_DOCUMENT_STATISTICS(
        { "total_bytes_read",    "Count the total bytes requested by read operations", "bytes", 1 },
        { "total_bytes_write",   "Count the total bytes requested by write operations", "bytes", 1 }
    )

private:
    void countArgs(const std::string &args, int32_t &argc);
    void tokenizeArgs(const std::string &args, const int32_t &argc, char ***argv);
    void updateIndices();

    uint64_t calcBytes(const Spatter::ConfigurationBase *config);
    size_t getPatternSize(const Spatter::ConfigurationBase *config);

    void gather();
    void scatter();
    void scatterGather();
    void multiGather();
    void multiScatter();

    uint32_t datawidth;
    uint32_t startSource;
    uint32_t startTarget;

    size_t patternIdx;
    size_t countIdx;
    size_t configIdx;

    bool configFin;

    Statistic<uint64_t>* statBytes[OPCOUNT];

    MirandaRequestQueue<GeneratorRequest*>* queue;

    Output* out;

    Spatter::ClArgs cl;
};

}
}

#endif
