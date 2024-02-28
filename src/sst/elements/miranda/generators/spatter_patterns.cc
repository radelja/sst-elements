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

#include <sst_config.h>
#include <sst/core/params.h>
#include <sst/elements/miranda/generators/spatter_patterns.h>

using namespace SST::Miranda;


SpatterPatternsGenerator::SpatterPatternsGenerator( ComponentId_t id, Params& params ) : RequestGenerator(id, params) {
	build(params);
}

void SpatterPatternsGenerator::build(Params& params) {
	const uint32_t verbose  = params.find<uint32_t>("verbose", 0);
	const std::string args  = "./Spatter " + params.find<std::string>("args", "");
	
	char **argv = nullptr;
	int argc    = 0;
	int res     = 0;
	
	out = new Output("SpatterPatternsGenerator[@p:@l]: ", verbose, 0, Output::STDOUT);
	
	reqLength   = params.find<uint64_t>("length", sizeof(double));
	configIdx   = 0;
	countIdx    = 0;
	patternIdx  = 0;
	
	// Convert arguments to compatible format before parsing
	count_args(args, argc);
	tokenize_args(args, argc, &argv);
	
	res = Spatter::parse_input(argc, argv, cl);
	
	
	// Allocated memory no longer needed
	for (int i = 0; i < argc; ++i) {
		delete [] argv[i];
	}
	
	delete [] argv;
	
	
	if (res != 0) {
		out->fatal(CALL_INFO, -1, "Failed to parse provided arguments\n");
	}
}

SpatterPatternsGenerator::~SpatterPatternsGenerator() {
	delete out;
}

void SpatterPatternsGenerator::generate(MirandaRequestQueue<GeneratorRequest*>* q) {
	const Spatter::ConfigurationBase *config = cl.configs[configIdx].get();
	
	queue  = q;
	
	if (config->kernel.compare("gather") == 0) {
		gather();
	} else if (config->kernel.compare("scatter") == 0) {
		scatter();
	} else if (config->kernel.compare("sg") == 0) {
		scatter_gather();
	} else if (config->kernel.compare("multigather") == 0) {
		multi_gather();
	} else if (config->kernel.compare("multiscatter") == 0) {
		multi_scatter();
	} else {
		out->fatal(CALL_INFO, -1, "Invalid kernel: %s\n", config->kernel.c_str());
	}
	
	update_indices();
}

bool SpatterPatternsGenerator::isFinished() {
	return (configIdx == cl.configs.size());
}

void SpatterPatternsGenerator::completed() {

}

/**
   * @brief Counts the number of arguments in a string
   *
   * @param args String of arguments to be counted
   * @param argc Number of arguments found in the string
   */
void SpatterPatternsGenerator::count_args(const std::string &args, int32_t &argc) {
	std::istringstream iss(args);
	std::string tok;
	
	while (iss >> tok) {
		++argc;
	}
}

/**
   * @brief Tokenize a string of arguments into an array of arguments
   *        and allocates memory for the array of arguments
   *
   * @param args String of arguments to be tokenized
   * @param argc Number of arguments in the string
   * @param argv Destination array for the arguments
   */
void SpatterPatternsGenerator::tokenize_args(const std::string &args, const int32_t &argc, char ***argv) {
	std::istringstream iss(args);
	std::string tok;
	
	char **argv_ptr = new char *[argc + 1];
	
	int argv_i = 0;
	while (iss >> tok) {
		int arg_size = tok.size() + 1;
		
		argv_ptr[argv_i] = new char[arg_size];
		strncpy(argv_ptr[argv_i], tok.c_str(), arg_size);
		
		++argv_i;
	}
	argv_ptr[argv_i] = nullptr;
	
	*argv = argv_ptr;
}

/**
   * @brief Update the pattern, count, and config indices
   *
   */
void SpatterPatternsGenerator::update_indices() {
	const Spatter::ConfigurationBase *config = cl.configs[configIdx].get();
	size_t pattern_size = 0;
	
	if (config->kernel.compare("sg") == 0) {
		assert(config->pattern_scatter.size() == config->pattern_gather.size());
		pattern_size = config->pattern_scatter.size();
	} else {
		pattern_size = config->pattern.size();
	}
	
	if (patternIdx == pattern_size - 1) {
		patternIdx = 0;
		
		if (countIdx == config->count - 1) {
			countIdx = 0;
			
			++configIdx;
		} else {
			++countIdx;
		}
	} else {
		++patternIdx;
	}
}

/**
   * @brief Generate a memory request for a Gather pattern
   *
   */
void SpatterPatternsGenerator::gather() {
	const Spatter::ConfigurationBase *config = cl.configs[configIdx].get();
	
	// Source buffer => sparse buffer, destination buffer => dense buffer
	uint64_t start_src  = 0;
	uint64_t src_offset = config->pattern[patternIdx] + config->delta * countIdx;

	queue->push_back(new MemoryOpRequest((start_src + src_offset), reqLength, READ));
}

/**
   * @brief Generate a memory request for a Scatter pattern
   *
   */
void SpatterPatternsGenerator::scatter() {
	const Spatter::ConfigurationBase *config = cl.configs[configIdx].get();
	
	// Source buffer => dense buffer, destination buffer => sparse buffer
	uint64_t start_dst  = config->dense.size();
	uint64_t dst_offset = config->pattern[patternIdx] + config->delta * countIdx;

	queue->push_back(new MemoryOpRequest(start_dst + dst_offset, reqLength, WRITE));
}

/**
   * @brief Generate memory requests for a GS pattern
   *
   */
void SpatterPatternsGenerator::scatter_gather() {
	const Spatter::ConfigurationBase *config = cl.configs[configIdx].get();
	
	// Source buffer => sparse_gather buffer, destination buffer => sparse_scatter buffer
	uint64_t start_src  = 0;
	uint64_t start_dst  = config->sparse_gather.size();
	uint64_t src_offset = config->pattern_gather[patternIdx] + config->delta_gather * countIdx;
	uint64_t dst_offset = config->pattern_scatter[patternIdx] + config->delta_scatter * countIdx;
	
	MemoryOpRequest* read_req = new MemoryOpRequest(start_src + src_offset, reqLength, READ);
	MemoryOpRequest* write_req = new MemoryOpRequest(start_dst + dst_offset, reqLength, WRITE);
	
	write_req->addDependency(read_req->getRequestID());
	
	queue->push_back(read_req);
	queue->push_back(write_req);
}

/**
   * @brief Generate a memory request for a MultiGather pattern
   *
   */
void SpatterPatternsGenerator::multi_gather() {
	const Spatter::ConfigurationBase *config = cl.configs[configIdx].get();
	
	// Source buffer => sparse buffer, destination buffer => dense buffer
	uint64_t start_src  = 0;
	uint64_t src_offset = config->pattern[patternIdx] + config->delta * countIdx;

	queue->push_back(new MemoryOpRequest(start_src + src_offset, reqLength, READ));
}

/**
   * @brief Generate a memory request for a MultiScatter pattern
   *
   */
void SpatterPatternsGenerator::multi_scatter() {
	const Spatter::ConfigurationBase *config = cl.configs[configIdx].get();
	
	// Source buffer => dense buffer, destination buffer => sparse buffer
	uint64_t start_dst  = config->dense.size();
	uint64_t dst_offset = config->pattern[patternIdx] + config->delta * countIdx;

	queue->push_back(new MemoryOpRequest(start_dst + dst_offset, reqLength, WRITE));
}
