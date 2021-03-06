//===---------------------------- Context.cpp -------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
/// \file
///
/// This file defines a class for holding ownership of various simulated
/// hardware units.  A Context also provides a utility routine for constructing
/// a default out-of-order pipeline with fetch, dispatch, execute, and retire
/// stages).
///
//===----------------------------------------------------------------------===//

#include "Context.h"
#include "DispatchStage.h"
#include "ExecuteStage.h"
#include "FetchStage.h"
#include "RegisterFile.h"
#include "RetireControlUnit.h"
#include "RetireStage.h"
#include "Scheduler.h"

namespace mca {

using namespace llvm;

std::unique_ptr<Pipeline>
Context::createDefaultPipeline(const PipelineOptions &Opts, InstrBuilder &IB,
                               SourceMgr &SrcMgr) {
  const MCSchedModel &SM = STI.getSchedModel();

  // Create the hardware units defining the backend.
  auto RCU = llvm::make_unique<RetireControlUnit>(SM);
  auto PRF = llvm::make_unique<RegisterFile>(SM, MRI, Opts.RegisterFileSize);
  auto HWS = llvm::make_unique<Scheduler>(
      SM, Opts.LoadQueueSize, Opts.StoreQueueSize, Opts.AssumeNoAlias);

  // Create the pipeline and its stages.
  auto StagePipeline = llvm::make_unique<Pipeline>();
  auto Fetch = llvm::make_unique<FetchStage>(IB, SrcMgr);
  auto Dispatch = llvm::make_unique<DispatchStage>(
      STI, MRI, Opts.RegisterFileSize, Opts.DispatchWidth, *RCU, *PRF);
  auto Execute = llvm::make_unique<ExecuteStage>(*HWS);
  auto Retire = llvm::make_unique<RetireStage>(*RCU, *PRF);

  // Add the hardware to the context.
  addHardwareUnit(std::move(RCU));
  addHardwareUnit(std::move(PRF));
  addHardwareUnit(std::move(HWS));

  // Build the pipeline.
  StagePipeline->appendStage(std::move(Fetch));
  StagePipeline->appendStage(std::move(Dispatch));
  StagePipeline->appendStage(std::move(Execute));
  StagePipeline->appendStage(std::move(Retire));
  return StagePipeline;
}

} // namespace mca
