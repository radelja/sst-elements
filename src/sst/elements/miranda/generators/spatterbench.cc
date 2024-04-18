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
#include <sst/elements/miranda/generators/spatterbench.h>

using namespace SST::Miranda;


SpatterBenchGenerator::SpatterBenchGenerator( ComponentId_t id, Params& params ) : RequestGenerator(id, params) {
	build(params);
}

void SpatterBenchGenerator::build(Params& params) {
	const uint32_t verbose = params.find<uint32_t>("verbose", 0);
	const std::string args = "./Spatter " + params.find<std::string>("args", "");

	char **argv = nullptr;
	int argc    = 0;
	int res     = 0;

	out = new Output("SpatterBenchGenerator[@p:@l]: ", verbose, 0, Output::STDOUT);

	warmupRuns = params.find<uint32_t>("warmup_runs", 10);
	reqLength  = sizeof(size_t);
	configIdx  = 0;
	countIdx   = 0;
	patternIdx = 0;
	warmupIdx  = 0;
	configFin  = false;
	warmupFin  = (warmupRuns == 0);
	warmupAll  = !params.find<bool>("only_warmup_first", false);

	if (warmupRuns < 0) {
		out->fatal(CALL_INFO, -1, "Value for warmup_runs must be greater than or equal to zero\n");
	}

	statReadBytes  = registerStatistic<uint64_t>( "total_bytes_read" );
	statWriteBytes = registerStatistic<uint64_t>( "total_bytes_write" );
	statReqLatency = registerStatistic<uint64_t>( "req_latency" );
	statCycles     = registerStatistic<uint64_t>( "cycles" );

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
	} else {
		// Output specified arguments for each run config
		std::ostringstream oss;
		oss << cl;
		out->output("\n%s", oss.str().c_str());
	}

	out->output("\n%-15s", "config");
	out->output("%-15s",   "bytes");
	out->output("%-15s",   "time(s)");
	out->output("%-15s",   "bw(MB/s)");
	out->output("%-15s",   "cycles");
	out->output("%-15s\n", "time(s)/cycles");
}

SpatterBenchGenerator::~SpatterBenchGenerator() {
	delete out;
}

void SpatterBenchGenerator::generate(MirandaRequestQueue<GeneratorRequest*>* q) {
	if (!configFin) {
		const Spatter::ConfigurationBase *config = cl.configs[configIdx].get();
		queue = q;

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
}

bool SpatterBenchGenerator::isFinished() {
	if (configFin && warmupFin) {
		const Spatter::ConfigurationBase *prev_config = cl.configs[configIdx-1].get();
		uint64_t expected_bytes = get_pattern_size(prev_config) * prev_config->count * reqLength;
		uint64_t recorded_bytes = calc_bytes(prev_config);

		if (prev_config->kernel.compare("sg") == 0) {
			// GS patterns expect twice the number of bytes
			expected_bytes <<= 1;
		}

		if (warmupIdx != 0 && warmupRuns != 0) {
			// Completed warmp-up runs for previous run config
			recorded_bytes /= warmupRuns;
		}

		// Check if the last config has finished executing requests
		if (recorded_bytes == expected_bytes) {
			if (warmupIdx != 0) {
				// Completed warm-up runs for previous run config
				warmupIdx = 0;
				--configIdx;
			} else {
				// Completed previous run config
				print_stats();

				// Reset warm-up flag after each run config
				if (warmupAll) {
					warmupFin = (warmupRuns == 0);
				}
			}
			configFin = false;

			// Reset statistics for next run config
			statReadBytes->setCollectionCount(0);
			statWriteBytes->setCollectionCount(0);
			statReqLatency->setCollectionCount(0);
			statCycles->setCollectionCount(0);
		}
	}

	return (configIdx == cl.configs.size());
}

void SpatterBenchGenerator::completed() {
	out->output("\n");
}

/**
   * @brief Counts the number of arguments in a string
   *
   * @param args String of arguments to be counted
   * @param argc Number of arguments found in the string
   */
void SpatterBenchGenerator::count_args(const std::string &args, int32_t &argc) {
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
void SpatterBenchGenerator::tokenize_args(const std::string &args, const int32_t &argc, char ***argv) {
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
   * @brief Calculate the number of bytes read or written by memory requests
   *
   * @param config Run config used to determine the kernel type
   * @return Number of bytes read or written by memory requests
   */
uint64_t SpatterBenchGenerator::calc_bytes(const Spatter::ConfigurationBase *config) {
	uint64_t num_bytes = 0;

	if (config->kernel.compare("gather") == 0 || config->kernel.compare("multigather") == 0) {
		num_bytes = statReadBytes->getCollectionCount() * reqLength;
	} else if (config->kernel.compare("scatter") == 0 || config->kernel.compare("multiscatter") == 0) {
		num_bytes = statWriteBytes->getCollectionCount() * reqLength;
	} else if (config->kernel.compare("sg") == 0) {
		num_bytes = (statWriteBytes->getCollectionCount() + statReadBytes->getCollectionCount()) * reqLength;
	}

	return num_bytes;
}

/**
   * @brief Return the number of elements in the pattern(s)
   *
   * @param config Run config used to determine the kernel type
   * @return Number of elements in the pattern(s)
   */
size_t SpatterBenchGenerator::get_pattern_size(const Spatter::ConfigurationBase *config) {
	size_t pattern_size = 0;

	if (config->kernel.compare("gather") == 0 || config->kernel.compare("scatter") == 0) {
		pattern_size = config->pattern.size();
	} else if (config->kernel.compare("sg") == 0) {
		assert(config->pattern_scatter.size() == config->pattern_gather.size());
		pattern_size = config->pattern_scatter.size();
	} else if (config->kernel.compare("multigather") == 0) {
		pattern_size = config->pattern_gather.size();
	} else if (config->kernel.compare("multiscatter") == 0) {
		pattern_size = config->pattern_scatter.size();
	}

	return pattern_size;
}

/**
   * @brief Update the pattern, count, and config indices
   *
   */
void SpatterBenchGenerator::update_indices() {
	const Spatter::ConfigurationBase *config = cl.configs[configIdx].get();
	size_t pattern_size = get_pattern_size(config);

	if (patternIdx == pattern_size - 1) {
		patternIdx = 0;
		if (countIdx == config->count - 1) {
			countIdx = 0;
			if (!warmupFin) {
				// Check if the warm-up runs are now finished
				if (warmupIdx == warmupRuns - 1) {
					warmupFin = true;
				} else {
					++warmupIdx;
				}
			}
			if (warmupFin) {
				// Finished executing executing requests for current config
				configFin = true;
				++configIdx;
			}
		} else {
			++countIdx;
		}
	} else {
		++patternIdx;
	}
}

/**
   * @brief Output the statistics for the previous Spatter pattern
   *
   */
void SpatterBenchGenerator::print_stats() {
	const Spatter::ConfigurationBase *config = cl.configs[configIdx-1].get();
	uint64_t stat_bytes = calc_bytes(config);

	double latency_seconds;
	double bandwidth;
	double time_per_cycle;

	// Convert request latency from nanoseconds to seconds
	latency_seconds = statReqLatency->getCollectionCount() / 1'000'000'000.0;

	// Convert bytes to megabytes for calculation
	bandwidth = (stat_bytes / 1'000'000.0) / latency_seconds;

	time_per_cycle = latency_seconds / statCycles->getCollectionCount();

	out->output("%-15lu",  config->id);
	out->output("%-15lu",  stat_bytes);
	out->output("%-15g",   latency_seconds);
	out->output("%-15.2f", bandwidth);
	out->output("%-15lu",  statCycles->getCollectionCount());
	out->output("%-15g\n", time_per_cycle);
}

/**
   * @brief Generate a memory request for a Gather pattern
   *
   */
void SpatterBenchGenerator::gather() {
	const Spatter::ConfigurationBase *config = cl.configs[configIdx].get();

	// Source (sparse) buffer from 0 to (sparse.size()-1)
	uint64_t start_src  = 0;
	uint64_t src_offset = config->pattern[patternIdx] + config->delta * countIdx;

	queue->push_back(new MemoryOpRequest((start_src + src_offset), reqLength, READ));
}

/**
   * @brief Generate a memory request for a Scatter pattern
   *
   */
void SpatterBenchGenerator::scatter() {
	const Spatter::ConfigurationBase *config = cl.configs[configIdx].get();

	// Destination (sparse) buffer from sparse.size() to (dense.size()-1)
	uint64_t start_dst  = config->sparse.size();
	uint64_t dst_offset = config->pattern[patternIdx] + config->delta * countIdx;

	queue->push_back(new MemoryOpRequest(start_dst + dst_offset, reqLength, WRITE));
}

/**
   * @brief Generate memory requests for a GS pattern
   *
   */
void SpatterBenchGenerator::scatter_gather() {
	const Spatter::ConfigurationBase *config = cl.configs[configIdx].get();

	// Source (sparse_gather) buffer from 0 to (sparse_gather.size()-1)
	uint64_t start_src  = 0;
	uint64_t src_offset = config->pattern_gather[patternIdx] + config->delta_gather * countIdx;

	// Destination (sparse_scatter) buffer from sparse_gather.size() to (sparse_scatter.size()-1)
	uint64_t start_dst  = config->sparse_gather.size();
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
void SpatterBenchGenerator::multi_gather() {
	const Spatter::ConfigurationBase *config = cl.configs[configIdx].get();

	// Source (sparse) buffer from 0 to (sparse.size()-1)
	uint64_t start_src  = 0;
	uint64_t src_offset = config->pattern[config->pattern_gather[patternIdx]] + config->delta * countIdx;

	queue->push_back(new MemoryOpRequest(start_src + src_offset, reqLength, READ));
}

/**
   * @brief Generate a memory request for a MultiScatter pattern
   *
   */
void SpatterBenchGenerator::multi_scatter() {
	const Spatter::ConfigurationBase *config = cl.configs[configIdx].get();

	// Destination (sparse) buffer from dense.size() to (sparse.size()-1)
	uint64_t start_dst  = config->dense.size();
	uint64_t dst_offset = config->pattern[config->pattern_scatter[patternIdx]] + config->delta * countIdx;

	queue->push_back(new MemoryOpRequest(start_dst + dst_offset, reqLength, WRITE));
}
