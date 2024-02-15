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


SpatterPatternsGenerator::SpatterPatternsGenerator( ComponentId_t id, Params& params ) :
	RequestGenerator(id, params) {
            build(params);
        }

void SpatterPatternsGenerator::build(Params& params) {
	const uint32_t verbose  = params.find<uint32_t>("verbose", 0);
	const std::string args  = "./Spatter " + params.find<std::string>("args", "");
	
	char **argv  = nullptr;
	int32_t argc = 0;
	int32_t res  = 0;
	
	out = new Output("SpatterPatternsGenerator[@p:@l]: ", verbose, 0, Output::STDOUT);
	
	reqLength    = params.find<uint64_t>("length", sizeof(double));
	startAddr    = params.find<uint64_t>("startat", 0);
	maxAddr      = params.find<uint64_t>("max_address", 524288);
	
	configIdx    = 0;
	countIdx     = 0;
	patternIdx   = 0;
	
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
	/* TODO */
	
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
	if (patternIdx == cl.configs[configIdx]->pattern.size()) {
		patternIdx = 0;
		
		if (countIdx == cl.configs[configIdx]->count) {
			countIdx = 0;
			
			++configIdx;
		} else {
			++countIdx;
		}
	} else {
		++patternIdx;
	}
}
