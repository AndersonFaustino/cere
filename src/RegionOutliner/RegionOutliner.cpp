/*****************************************************************************
 * This file is part of CERE.                                                *
 *                                                                           *
 * Copyright (c) 2013-2015, Universite de Versailles St-Quentin-en-Yvelines  *
 *                                                                           *
 * CERE is free software: you can redistribute it and/or modify it under     *
 * the terms of the GNU Lesser General Public License as published by        *
 * the Free Software Foundation, either version 3 of the License,            *
 * or (at your option) any later version.                                    *
 *                                                                           *
 * Foobar is distributed in the hope that it will be useful,                 *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU General Public License for more details.                              *
 *                                                                           *
 * You should have received a copy of the GNU General Public License         *
 * along with Foobar.  If not, see <http://www.gnu.org/licenses/>.           *
 *****************************************************************************/

//===----------------------------------------------------------------------===//
//
// A pass wrapper around the ExtractLoop() scalar transformation to extract each
// top-level loop into its own new function.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "region-outliner"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Transforms/Scalar.h"
#include "RegionExtractor.h"

using namespace llvm;

STATISTIC(NumExtracted, "Number of regions extracted");

static cl::opt<std::string>
IsolateRegion("isolate-region", cl::init("all"), cl::value_desc("String"),
              cl::desc("RegionOutliner will only isolate this region"));
static cl::opt<bool>
AppMeasure("instrument-app", cl::init(false), cl::value_desc("Boolean"),
        cl::desc("If you want to isolate regions to profile the application"));

namespace {
struct RegionOutliner : public LoopPass {
  static char ID; // Pass identification, replacement for typeid
  unsigned NumLoops;
  bool ProfileApp;
  std::string RegionName;

  explicit RegionOutliner(unsigned numLoops = ~0,
                          const std::string &regionName = "")
      : LoopPass(ID), NumLoops(numLoops), ProfileApp(AppMeasure),
        RegionName(regionName) {
    if (regionName.empty())
      RegionName = IsolateRegion;
  }

  virtual bool runOnLoop(Loop *L, LPPassManager &LPM);

  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
    AU.addRequiredID(BreakCriticalEdgesID);
    AU.addRequiredID(LoopSimplifyID);
    AU.addRequired<DominatorTree>();
  }
};
}

char RegionOutliner::ID = 0;
static RegisterPass<RegionOutliner> X("region-outliner", "Outline all loops",
                                      false, false);

bool RegionOutliner::runOnLoop(Loop *L, LPPassManager &LPM) {
  // Only visit top-level loops.
  if (L->getParentLoop())
    return false;

  // If LoopSimplify form is not available, stay out of trouble.
  if (!L->isLoopSimplifyForm())
    return false;

  DominatorTree &DT = getAnalysis<DominatorTree>();
  bool Changed = false;

  // Extract the loop if it was not previously extracted:
  // If this loop is inside a function prefixed with __cere__
  // it means we are looking at an already outlined loop
  bool ShouldExtractLoop = true;
  Function *function = L->getHeader()->getParent();
  std::string name = function->getName();
  std::size_t found = name.find("__cere__");
  if (found != std::string::npos) {
    ShouldExtractLoop = false;
  }

  if (ShouldExtractLoop) {
    if (NumLoops == 0)
      return Changed;
    --NumLoops;
    RegionExtractor Extractor(DT, *L, RegionName, ProfileApp);
    if (Extractor.extractCodeRegion() != 0) {
      Changed = true;
      // After extraction, the loop is replaced by a function call, so
      // we shouldn't try to run any more loop passes on it.
      LPM.deleteLoopFromQueue(L);
    }
    ++NumExtracted;
  }

  return Changed;
}
