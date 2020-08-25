// Copyright 2009-2020 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2020, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _LLYR_MAPPER_H
#define _LLYR_MAPPER_H

#include <sst/core/module.h>

#include "../graph.h"
#include "../processingElement.h"

namespace SST {
namespace Llyr {

// template<class T>
class LlyrMapper : public SST::Module
{

public:
    SST_ELI_REGISTER_MODULE_API(SST::Llyr::LlyrMapper);

    LlyrMapper() : Module() {}
    virtual ~LlyrMapper() {}

//     virtual void mapGraph(LlyrGraph<T> hardwareGraph, LlyrGraph<T> appGraph, LlyrGraph<T> graphOut) = 0;
    virtual void mapGraph(LlyrGraph<std::string> hardwareGraph, LlyrGraph<std::string> appGraph, LlyrGraph<opType> graphOut) = 0;

};

}
} // namespace SST

#endif // _LLYR_MAPPER_H
