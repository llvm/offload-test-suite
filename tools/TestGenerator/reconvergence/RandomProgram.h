#ifndef RANDOMPROGRAM_H_
#define RANDOMPROGRAM_H_

#include <cstdint>
#include <memory>
#include <vector>

#include "Ballot.h"
#include "Ballots.h"
#include "MaskUtils.h"
#include "MathUtils.h"
#include "Op.h"
#include "RandomUtils.h"
#include "TestCase.h"
#include "VectorUtils.h"

namespace reconvergence {

class RandomProgram {
public:
  RandomProgram(const TestCase &testCase, uint32_t invocationCount = 0u)
      : testCase(testCase),
        invocationStride(invocationCount ? invocationCount
                                         : (testCase.getThreadgroupSizeX() *
                                            testCase.getThreadgroupSizeY())),
        rnd(), ops(), masks(), ballotMasks(), numMasks(5), nesting(0),
        maxNesting(testCase.getMaxNestingLevel()), loopNesting(0),
        loopNestingThisFunction(0), callNesting(0), minCount(30), indent(0),
        isLoopInf(100, false), doneInfLoopBreak(100, false),
        storeBase(0x10000) {
    Random_init(&rnd, testCase.getSeed());
    for (int i = 0; i < numMasks; ++i) {
      const uint64_t lo = Random_getUint64(&rnd);
      const uint64_t hi = Random_getUint64(&rnd);
      const UVec4 v4(static_cast<uint32_t>(lo), static_cast<uint32_t>(lo >> 32),
                     static_cast<uint32_t>(hi),
                     static_cast<uint32_t>(hi >> 32));
      ballotMasks.emplace_back(v4);
      masks.push_back(lo);
    }
  }
  virtual ~RandomProgram() = default;

  const TestCase &testCase;
  const uint32_t invocationStride;
  Random rnd;
  std::vector<OP> ops;
  std::vector<uint64_t> masks;
  std::vector<Ballot> ballotMasks;
  int32_t numMasks;
  int32_t nesting;
  int32_t maxNesting;
  int32_t loopNesting;
  int32_t loopNestingThisFunction;
  int32_t callNesting;
  int32_t minCount;
  int32_t indent;
  std::vector<bool> isLoopInf;
  std::vector<bool> doneInfLoopBreak;
  // Offset the value we use for OP_STORE, to avoid colliding with fully
  // converged active masks with small subgroup sizes (e.g. with subgroupSize ==
  // 4, the SUCF tests need to know that 0xF is really an active mask).
  int32_t storeBase;

  virtual void genIf(IFType ifType, uint32_t maxLocalIndexCmp = 0u) {
    uint32_t maskIdx = Random_getUint32(&rnd) % numMasks;
    uint64_t mask = masks[maskIdx];
    Ballot bmask = ballotMasks[maskIdx];
    if (ifType == IF_UNIFORM) {
      mask = ~0ULL;
      bmask.set();
    }

    uint32_t localIndexCmp =
        Random_getUint32(&rnd) %
        (maxLocalIndexCmp ? maxLocalIndexCmp : invocationStride);
    if (ifType == IF_LOCAL_INVOCATION_INDEX) {
      ops.push_back({OP_IF_LOCAL_INVOCATION_INDEX, localIndexCmp});
    } else if (ifType == IF_LOOPCOUNT) {
      ops.push_back({OP_IF_LOOPCOUNT, 0});
    } else {
      ops.push_back({OP_IF_MASK, mask});
      ops.back().bvalue = bmask;
    }

    nesting++;

    size_t thenBegin = ops.size();
    pickOP(2);
    size_t thenEnd = ops.size();

    uint32_t randElse = (Random_getUint32(&rnd) % 100);
    if (randElse < 50) {
      if (ifType == IF_LOCAL_INVOCATION_INDEX)
        ops.push_back({OP_ELSE_LOCAL_INVOCATION_INDEX, localIndexCmp});
      else if (ifType == IF_LOOPCOUNT)
        ops.push_back({OP_ELSE_LOOPCOUNT, 0});
      else
        ops.push_back({OP_ELSE_MASK, 0});

      if (randElse < 10) {
        // Sometimes make the else block identical to the then block
        for (size_t i = thenBegin; i < thenEnd; ++i)
          ops.push_back(ops[i]);
      } else {
        pickOP(2);
      }
    }
    ops.push_back({OP_ENDIF, 0});
    nesting--;
  }

  void genForUnif() {
    uint32_t iterCount = (Random_getUint32(&rnd) % 5) + 1;
    ops.push_back({OP_BEGIN_FOR_UNIF, iterCount});
    uint32_t loopheader = (uint32_t)ops.size() - 1;
    nesting++;
    loopNesting++;
    loopNestingThisFunction++;
    pickOP(2);
    ops.push_back({OP_END_FOR_UNIF, loopheader});
    loopNestingThisFunction--;
    loopNesting--;
    nesting--;
  }

  void genDoWhileUnif() {
    uint32_t iterCount = (Random_getUint32(&rnd) % 5) + 1;
    ops.push_back({OP_BEGIN_DO_WHILE_UNIF, iterCount});
    uint32_t loopheader = (uint32_t)ops.size() - 1;
    nesting++;
    loopNesting++;
    loopNestingThisFunction++;
    pickOP(2);
    ops.push_back({OP_END_DO_WHILE_UNIF, loopheader});
    loopNestingThisFunction--;
    loopNesting--;
    nesting--;
  }

  void genForVar() {
    ops.push_back({OP_BEGIN_FOR_VAR, 0});
    uint32_t loopheader = (uint32_t)ops.size() - 1;
    nesting++;
    loopNesting++;
    loopNestingThisFunction++;
    pickOP(2);
    ops.push_back({OP_END_FOR_VAR, loopheader});
    loopNestingThisFunction--;
    loopNesting--;
    nesting--;
  }

  void genForInf() {
    ops.push_back({OP_BEGIN_FOR_INF, 0});
    uint32_t loopheader = (uint32_t)ops.size() - 1;

    nesting++;
    loopNesting++;
    loopNestingThisFunction++;
    isLoopInf[loopNesting] = true;
    doneInfLoopBreak[loopNesting] = false;

    pickOP(2);

    genElect(true);
    doneInfLoopBreak[loopNesting] = true;

    pickOP(2);

    ops.push_back({OP_END_FOR_INF, loopheader});

    isLoopInf[loopNesting] = false;
    doneInfLoopBreak[loopNesting] = false;
    loopNestingThisFunction--;
    loopNesting--;
    nesting--;
  }

  void genDoWhileInf() {
    ops.push_back({OP_BEGIN_DO_WHILE_INF, 0});
    uint32_t loopheader = (uint32_t)ops.size() - 1;

    nesting++;
    loopNesting++;
    loopNestingThisFunction++;
    isLoopInf[loopNesting] = true;
    doneInfLoopBreak[loopNesting] = false;

    pickOP(2);

    genElect(true);
    doneInfLoopBreak[loopNesting] = true;

    pickOP(2);

    ops.push_back({OP_END_DO_WHILE_INF, loopheader});

    isLoopInf[loopNesting] = false;
    doneInfLoopBreak[loopNesting] = false;
    loopNestingThisFunction--;
    loopNesting--;
    nesting--;
  }

  void genBreak() {
    if (loopNestingThisFunction > 0) {
      // Sometimes put the break in a divergent if
      if ((Random_getUint32(&rnd) % 100) < 10) {
        ops.push_back({OP_IF_MASK, masks[0]});
        ops.back().bvalue = ballotMasks[0];
        ops.push_back({OP_BREAK, 0});
        ops.push_back({OP_ELSE_MASK, 0});
        ops.push_back({OP_BREAK, 0});
        ops.push_back({OP_ENDIF, 0});
      } else {
        ops.push_back({OP_BREAK, 0});
      }
    }
  }

  void genContinue() {
    // continues are allowed if we're in a loop and the loop is not infinite,
    // or if it is infinite and we've already done a subgroupElect+break.
    // However, adding more continues seems to reduce the failure rate, so
    // disable it for now
    if (loopNestingThisFunction > 0 &&
        !(isLoopInf[loopNesting] /*&& !doneInfLoopBreak[loopNesting]*/)) {
      // Sometimes put the continue in a divergent if
      if ((Random_getUint32(&rnd) % 100) < 10) {
        ops.push_back({OP_IF_MASK, masks[0]});
        ops.back().bvalue = ballotMasks[0];
        ops.push_back({OP_CONTINUE, 0});
        ops.push_back({OP_ELSE_MASK, 0});
        ops.push_back({OP_CONTINUE, 0});
        ops.push_back({OP_ENDIF, 0});
      } else {
        ops.push_back({OP_CONTINUE, 0});
      }
    }
  }

  // doBreak is used to generate "if (subgroupElect()) { ... break; }" inside
  // infinite loops
  void genElect(bool doBreak) {
    ops.push_back({OP_ELECT, 0});
    nesting++;
    if (doBreak) {
      // Put something interesting before the break
      genBallot();
      genBallot();
      if ((Random_getUint32(&rnd) % 100) < 10)
        pickOP(1);

      // if we're in a function, sometimes  use return instead
      if (callNesting > 0 && (Random_getUint32(&rnd) % 100) < 30)
        ops.push_back({OP_RETURN, 0});
      else
        genBreak();
    } else {
      pickOP(2);
    }

    ops.push_back({OP_ENDIF, 0});
    nesting--;
  }

  void genReturn() {
    uint32_t r = Random_getUint32(&rnd) % 100;
    if (nesting > 0 &&
        // Use return rarely in main, 20% of the time in a singly nested loop in
        // a function and 50% of the time in a multiply nested loop in a
        // function
        (r < 5 || (callNesting > 0 && loopNestingThisFunction > 0 && r < 20) ||
         (callNesting > 0 && loopNestingThisFunction > 1 && r < 50))) {
      genBallot();
      if ((Random_getUint32(&rnd) % 100) < 10) {
        ops.push_back({OP_IF_MASK, masks[0]});
        ops.back().bvalue = ballotMasks[0];
        ops.push_back({OP_RETURN, 0});
        ops.push_back({OP_ELSE_MASK, 0});
        ops.push_back({OP_RETURN, 0});
        ops.push_back({OP_ENDIF, 0});
      } else {
        ops.push_back({OP_RETURN, 0});
      }
    }
  }

  // Generate a function call. Save and restore some loop information, which is
  // used to determine when it's safe to use break/continue
  void genCall() {
    ops.push_back({OP_CALL_BEGIN, 0});
    callNesting++;
    nesting++;
    int32_t saveLoopNestingThisFunction = loopNestingThisFunction;
    loopNestingThisFunction = 0;

    pickOP(2);

    loopNestingThisFunction = saveLoopNestingThisFunction;
    nesting--;
    callNesting--;
    ops.push_back({OP_CALL_END, 0});
  }

  // Generate switch on a uniform value:
  // switch (InputA[r]) {
  // case r+1: ... break; // should not execute
  // case r:   ... break; // should branch uniformly
  // case r+2: ... break; // should not execute
  // }
  void genSwitchUnif() {
    uint32_t r = Random_getUint32(&rnd) % 5;
    ops.push_back({OP_SWITCH_UNIF_BEGIN, r});
    nesting++;

    ops.push_back({OP_CASE_MASK_BEGIN, 0, 1u << (r + 1)});
    pickOP(1);
    ops.push_back({OP_CASE_END, 0});

    ops.push_back({OP_CASE_MASK_BEGIN, ~0ULL, 1u << r});
    ops.back().bvalue.set();
    pickOP(2);
    ops.push_back({OP_CASE_END, 0});

    ops.push_back({OP_CASE_MASK_BEGIN, 0, 1u << (r + 2)});
    pickOP(1);
    ops.push_back({OP_CASE_END, 0});

    ops.push_back({OP_SWITCH_END, 0});
    nesting--;
  }

  // switch (gl_SubgroupInvocationID & 3) with four unique targets
  void genSwitchVar() {
    ops.push_back({OP_SWITCH_VAR_BEGIN, 0});
    nesting++;

    ops.push_back({OP_CASE_MASK_BEGIN, 0x1111111111111111ULL, 1 << 0});
    ops.back().bvalue = UVec4(0x11111111);
    pickOP(1);
    ops.push_back({OP_CASE_END, 0});

    ops.push_back({OP_CASE_MASK_BEGIN, 0x2222222222222222ULL, 1 << 1});
    ops.back().bvalue = UVec4(0x22222222);
    pickOP(1);
    ops.push_back({OP_CASE_END, 0});

    ops.push_back({OP_CASE_MASK_BEGIN, 0x4444444444444444ULL, 1 << 2});
    ops.back().bvalue = UVec4(0x44444444);
    pickOP(1);
    ops.push_back({OP_CASE_END, 0});

    ops.push_back({OP_CASE_MASK_BEGIN, 0x8888888888888888ULL, 1 << 3});
    ops.back().bvalue = UVec4(0x88888888);
    pickOP(1);
    ops.push_back({OP_CASE_END, 0});

    ops.push_back({OP_SWITCH_END, 0});
    nesting--;
  }

  // switch (gl_SubgroupInvocationID & 3) with two shared targets.
  // XXX TODO: The test considers these two targets to remain converged,
  // though we haven't agreed to that behavior yet.
  void genSwitchMulticase() {
    ops.push_back({OP_SWITCH_VAR_BEGIN, 0});
    nesting++;

    ops.push_back(
        {OP_CASE_MASK_BEGIN, 0x3333333333333333ULL, (1 << 0) | (1 << 1)});
    ops.back().bvalue = UVec4(0x33333333);
    pickOP(2);
    ops.push_back({OP_CASE_END, 0});

    ops.push_back(
        {OP_CASE_MASK_BEGIN, 0xCCCCCCCCCCCCCCCCULL, (1 << 2) | (1 << 3)});
    ops.back().bvalue = UVec4(0xCCCCCCCC);
    pickOP(2);
    ops.push_back({OP_CASE_END, 0});

    ops.push_back({OP_SWITCH_END, 0});
    nesting--;
  }

  // switch (loopIdxN) {
  // case 1:  ... break;
  // case 2:  ... break;
  // default: ... break;
  // }
  void genSwitchLoopCount() {
    uint32_t r = Random_getUint32(&rnd) % loopNesting;
    ops.push_back({OP_SWITCH_LOOP_COUNT_BEGIN, r});
    nesting++;

    ops.push_back({OP_CASE_LOOP_COUNT_BEGIN, 1ULL << 1, 1});
    ops.back().bvalue = UVec4(1 << 1, 0, 0, 0);
    pickOP(1);
    ops.push_back({OP_CASE_END, 0});

    ops.push_back({OP_CASE_LOOP_COUNT_BEGIN, 1ULL << 2, 2});
    ops.back().bvalue = UVec4(1 << 2, 0, 0, 0);
    pickOP(1);
    ops.push_back({OP_CASE_END, 0});

    // default:
    ops.push_back({OP_CASE_LOOP_COUNT_BEGIN, ~6ULL, 0xFFFFFFFF});
    ops.back().bvalue = UVec4(~6u, ~0u, ~0u, ~0u);
    pickOP(1);
    ops.push_back({OP_CASE_END, 0});

    ops.push_back({OP_SWITCH_END, 0});
    nesting--;
  }

  void pickOP(uint32_t count) {
    // Pick "count" instructions. These can recursively insert more
    // instructions, so "count" is just a seed
    for (uint32_t i = 0; i < count; ++i) {
      genBallot();
      if (nesting < maxNesting) {
        uint32_t r = Random_getUint32(&rnd) % 11;
        switch (r) {
        default:
          [[fallthrough]];
          // fallthrough
        case 2:
          if (loopNesting) {
            genIf(IF_LOOPCOUNT);
            break;
          }
          [[fallthrough]];
          // fallthrough
        case 10:
          genIf(IF_LOCAL_INVOCATION_INDEX);
          break;
        case 0:
          genIf(IF_MASK);
          break;
        case 1:
          genIf(IF_UNIFORM);
          break;
        case 3: {
          // don't nest loops too deeply, to avoid extreme memory usage or
          // timeouts
          if (loopNesting <= 3) {
            uint32_t r2 = Random_getUint32(&rnd) % 3;
            switch (r2) {
            default:
            case 0:
              genForUnif();
              break;
            case 1:
              genForInf();
              break;
            case 2:
              genForVar();
              break;
            }
          }
        } break;
        case 4:
          genBreak();
          break;
        case 5:
          genContinue();
          break;
        case 6:
          genElect(false);
          break;
        case 7: {
          uint32_t r2 = Random_getUint32(&rnd) % 5;
          if (r2 == 0 && callNesting == 0 && nesting < maxNesting - 2)
            genCall();
          else
            genReturn();
          break;
        }
        case 8: {
          // don't nest loops too deeply, to avoid extreme memory usage or
          // timeouts
          if (loopNesting <= 3) {
            uint32_t r2 = Random_getUint32(&rnd) % 2;
            switch (r2) {
            default:
            case 0:
              genDoWhileUnif();
              break;
            case 1:
              genDoWhileInf();
              break;
            }
          }
        } break;
        case 9: {
          uint32_t r2 = Random_getUint32(&rnd) % 4;
          switch (r2) {
          default:
          case 0:
            genSwitchUnif();
            break;
          case 1:
            if (loopNesting > 0) {
              genSwitchLoopCount();
              break;
            }
            [[fallthrough]];
            // fallthrough
          case 2:
            [[fallthrough]];
            // fallthrough
          case 3:
            genSwitchVar();
            break;
          }
        } break;
        }
      }
      genBallot();
    }
  }

  void genBallot() {
    // optionally insert ballots, stores, and noise. Ballots and stores are used
    // to determine correctness.
    if ((Random_getUint32(&rnd) % 100) < 20) {
      if (ops.size() < 2 || !(ops[ops.size() - 1].type == OP_BALLOT ||
                              (ops[ops.size() - 1].type == OP_STORE &&
                               ops[ops.size() - 2].type == OP_BALLOT))) {
        ops.push_back({OP_BALLOT, 0});
      }
    }

    if ((Random_getUint32(&rnd) % 100) < 10) {
      if (ops.size() < 2 || !(ops[ops.size() - 1].type == OP_STORE ||
                              (ops[ops.size() - 1].type == OP_BALLOT &&
                               ops[ops.size() - 2].type == OP_STORE))) {
        ops.push_back({OP_STORE, (uint32_t)ops.size() + storeBase});
      }
    }

    uint32_t r = Random_getUint32(&rnd) % 10000;
    if (r < 3)
      ops.push_back({OP_NOISE, 0});
    else if (r < 10)
      ops.push_back({OP_NOISE, 1});
  }

  void generateRandomProgram() {
    std::vector<UVec4> ref;

    ops.clear();
    while ((int32_t)ops.size() < minCount)
      pickOP(1);
  }

  void printIndent(std::stringstream &css) {
    for (int32_t i = 0; i < indent; ++i)
      css << " ";
  }

  struct FlowState {
    add_cref<std::vector<OP>> ops;
    const int32_t opsIndex;
    const int32_t loopNesting;
    const int funcNum;
  };

  // State of the subgroup at each level of nesting
  struct SubgroupState {
    // Currently executing
    bitset_inv_t activeMask;
    // Have executed a continue instruction in this loop
    bitset_inv_t continueMask;
    // index of the current if test or loop header
    uint32_t header;
    // number of loop iterations performed
    uint32_t tripCount;
    // is this nesting a loop?
    uint32_t isLoop;
    // is this nesting a function call?
    uint32_t isCall;
    // is this nesting a switch?
    uint32_t isSwitch;
  };

  struct WaveState {
    // Currently executing
    Ballots activeMask;
    // Have executed a continue instruction in this loop
    Ballots continueMask;
    // index of the current if test or loop header
    uint32_t header;
    // number of loop iterations performed
    uint32_t tripCount;
    // is this nesting a loop?
    uint32_t isLoop;
    // is this nesting a function call?
    uint32_t isCall;
    // is this nesting a switch?
    uint32_t isSwitch;
    virtual ~WaveState() = default;
    WaveState() : WaveState(0) {}
    WaveState(uint32_t waveCount)
        : activeMask(waveCount), continueMask(waveCount), header(), tripCount(),
          isLoop(), isCall(), isSwitch() {}
  };

  struct Prerequisites {};

  virtual std::string getPartitionBallotTextHlsl() {
    return "WaveActiveBallot(true)";
  }

  virtual void printIfLocalInvocationIndexHlsl(std::stringstream &css,
                                               add_cref<FlowState> flow) {
    printIndent(css);
    css << "if (gIndex >= InputA[0x" << std::hex
        << flow.ops[flow.opsIndex].value << "]) {\n";
  }

  virtual void printStoreHlsl(std::stringstream &css,
                              add_cref<FlowState> flow) {
    printIndent(css);
    css << "OutputB[(outLoc++)*invocationStride + gIndex].x = 0x" << std::hex
        << flow.ops[flow.opsIndex].value << ";\n";
  }

  virtual void printBallotHlsl(std::stringstream &css, add_cref<FlowState>,
                               bool endWithSemicolon = false) {
    printIndent(css);

    // When inside loop(s), use partitionBallot rather than WaveActiveBallot to
    // compute a ballot, to make sure the ballot is "diverged enough". Don't do
    // this for subgroup_uniform_control_flow, since we only validate results
    // that must be fully reconverged.
    if (loopNesting > 0) {
      css << "OutputB[(outLoc++)*invocationStride + gIndex] = "
          << getPartitionBallotTextHlsl() << ".xy";
    } else {
      css << "OutputB[(outLoc++)*invocationStride + gIndex] = "
             "WaveActiveBallot(true).xy";
    }
    if (endWithSemicolon) {
      css << ";\n";
    }
  }

  void printCodeHlsl(std::stringstream &functions, std::stringstream &main) {
    std::stringstream *css = &main;
    indent = 4;
    loopNesting = 0;
    int funcNum = 0;
    int32_t i = 0;

    auto makeFlowState = [&]() -> FlowState {
      return FlowState{ops, i, loopNesting, funcNum};
    };

    for (; i < (int32_t)ops.size(); ++i) {
      switch (ops[i].type) {
      case OP_IF_MASK:
        printIndent(*css);
        if (ops[i].value == ~0ULL) {
          // This equality test will always succeed, since InputA[i] == i
          int idx = Random_getUint32(&rnd) % 4;
          *css << "if (InputA[" << idx << "] == " << idx << ") {\n";
        } else {
          const UVec4 v(ops[i].bvalue);
          *css << std::hex << "if (testBit(uint4("
               << "0x" << v.x() << ", "
               << "0x" << v.y() << ", "
               << "0x" << v.z() << ", "
               << "0x" << v.w() << std::dec << "), WaveGetLaneIndex())) {\n";
        }
        indent += 4;
        break;
      case OP_IF_LOOPCOUNT:
        printIndent(*css);
        *css << "if (WaveGetLaneIndex() == loopIdx" << loopNesting - 1
             << ") {\n";
        indent += 4;
        break;
      case OP_IF_LOCAL_INVOCATION_INDEX:
        printIfLocalInvocationIndexHlsl(*css, makeFlowState());
        indent += 4;
        break;
      case OP_ELSE_MASK:
      case OP_ELSE_LOOPCOUNT:
      case OP_ELSE_LOCAL_INVOCATION_INDEX:
        indent -= 4;
        printIndent(*css);
        *css << "} else {\n";
        indent += 4;
        break;
      case OP_ENDIF:
        indent -= 4;
        printIndent(*css);
        *css << "}\n";
        break;
      case OP_BALLOT:
        printBallotHlsl(*css, makeFlowState(), true);
        break;
      case OP_STORE:
        printStoreHlsl(*css, makeFlowState());
        break;
      case OP_BEGIN_FOR_VAR:
        printIndent(*css);
        *css << "for (int loopIdx" << loopNesting << " = 0;\n";
        printIndent(*css);
        *css << "         loopIdx" << loopNesting
             << " < WaveGetLaneIndex() + 1;\n";
        printIndent(*css);
        *css << "         loopIdx" << loopNesting << "++) {\n";
        indent += 4;
        loopNesting++;
        break;
      case OP_END_FOR_VAR:
        loopNesting--;
        indent -= 4;
        printIndent(*css);
        *css << "}\n";
        break;
      case OP_BEGIN_FOR_UNIF:
        printIndent(*css);
        *css << "for (int loopIdx" << loopNesting << " = 0;\n";
        printIndent(*css);
        *css << "         loopIdx" << loopNesting << " < InputA["
             << ops[i].value << "];\n";
        printIndent(*css);
        *css << "         loopIdx" << loopNesting << "++) {\n";
        indent += 4;
        loopNesting++;
        break;
      case OP_END_FOR_UNIF:
        loopNesting--;
        indent -= 4;
        printIndent(*css);
        *css << "}\n";
        break;
      case OP_BEGIN_FOR_INF:
        printIndent(*css);
        *css << "for (int loopIdx" << loopNesting << " = 0;;loopIdx"
             << loopNesting << "++,";
        loopNesting++;
        printBallotHlsl(*css, makeFlowState());
        *css << ") {\n";
        indent += 4;
        break;
      case OP_END_FOR_INF:
        loopNesting--;
        indent -= 4;
        printIndent(*css);
        *css << "}\n";
        break;
      case OP_BEGIN_DO_WHILE_UNIF:
        printIndent(*css);
        *css << "{\n";
        indent += 4;
        printIndent(*css);
        *css << "int loopIdx" << loopNesting << " = 0;\n";
        printIndent(*css);
        *css << "do {\n";
        indent += 4;
        printIndent(*css);
        *css << "loopIdx" << loopNesting << "++;\n";
        loopNesting++;
        break;
      case OP_END_DO_WHILE_UNIF:
        loopNesting--;
        indent -= 4;
        printIndent(*css);
        *css << "} while (loopIdx" << loopNesting << " < InputA["
             << ops[(uint32_t)ops[i].value].value << "]);\n";
        indent -= 4;
        printIndent(*css);
        *css << "}\n";
        break;
      case OP_BEGIN_DO_WHILE_INF:
        printIndent(*css);
        *css << "{\n";
        indent += 4;
        printIndent(*css);
        *css << "int loopIdx" << loopNesting << " = 0;\n";
        printIndent(*css);
        *css << "do {\n";
        indent += 4;
        loopNesting++;
        break;
      case OP_END_DO_WHILE_INF:
        loopNesting--;
        printIndent(*css);
        *css << "loopIdx" << loopNesting << "++;\n";
        indent -= 4;
        printIndent(*css);
        *css << "} while (true);\n";
        indent -= 4;
        printIndent(*css);
        *css << "}\n";
        break;
      case OP_BREAK:
        printIndent(*css);
        *css << "break;\n";
        break;
      case OP_CONTINUE:
        printIndent(*css);
        *css << "continue;\n";
        break;
      case OP_ELECT:
        printIndent(*css);
        *css << "if (WaveIsFirstLane()) {\n";
        indent += 4;
        break;
      case OP_RETURN:
        printIndent(*css);
        *css << "return;\n";
        break;
      case OP_CALL_BEGIN:
        printIndent(*css);
        *css << "func" << funcNum << "(gIndex";
        if (loopNesting > 0) {
          *css << ", ";
        }
        for (int32_t n = 0; n < loopNesting; ++n) {
          *css << "loopIdx" << n;
          if (n != loopNesting - 1)
            *css << ", ";
        }
        *css << ");\n";
        css = &functions;
        printIndent(*css);
        *css << "void func" << funcNum << "(uint gIndex";
        if (loopNesting > 0) {
          *css << ", ";
        }
        for (int32_t n = 0; n < loopNesting; ++n) {
          *css << "int loopIdx" << n;
          if (n != loopNesting - 1)
            *css << ", ";
        }
        *css << ") {\n";
        indent += 4;
        funcNum++;
        break;
      case OP_CALL_END:
        indent -= 4;
        printIndent(*css);
        *css << "}\n";
        css = &main;
        break;
      case OP_NOISE:
        if (ops[i].value == 0) {
          printIndent(*css);
          *css << "while (!WaveIsFirstLane()) {}\n";
        } else {
          printIndent(*css);
          *css << "if (InputA[0] == 12345) {\n";
          indent += 4;
          printIndent(*css);
          *css << "while (true) {\n";
          indent += 4;
          printBallotHlsl(*css, makeFlowState(), true);
          indent -= 4;
          printIndent(*css);
          *css << "}\n";
          indent -= 4;
          printIndent(*css);
          *css << "}\n";
        }
        break;
      case OP_SWITCH_UNIF_BEGIN:
        printIndent(*css);
        *css << "switch (InputA[" << ops[i].value << "]) {\n";
        indent += 4;
        break;
      case OP_SWITCH_VAR_BEGIN:
        printIndent(*css);
        *css << "switch (WaveGetLaneIndex() & 3) {\n";
        indent += 4;
        break;
      case OP_SWITCH_LOOP_COUNT_BEGIN:
        printIndent(*css);
        *css << "switch (loopIdx" << ops[i].value << ") {\n";
        indent += 4;
        break;
      case OP_SWITCH_END:
        indent -= 4;
        printIndent(*css);
        *css << "}\n";
        break;
      case OP_CASE_MASK_BEGIN:
        for (int32_t b = 0; b < 32; ++b) {
          if ((1u << b) & ops[i].caseValue) {
            printIndent(*css);
            *css << "case " << b << ":\n";
          }
        }
        printIndent(*css);
        *css << "{\n";
        indent += 4;
        break;
      case OP_CASE_LOOP_COUNT_BEGIN:
        if (ops[i].caseValue == 0xFFFFFFFF) {
          printIndent(*css);
          *css << "default: {\n";
        } else {
          printIndent(*css);
          *css << "case " << ops[i].caseValue << ": {\n";
        }
        indent += 4;
        break;
      case OP_CASE_END:
        printIndent(*css);
        *css << "break;\n";
        indent -= 4;
        printIndent(*css);
        *css << "}\n";
        break;
      default:
        break;
      }
    }
  }

  // Simulate execution of the program. If countOnly is true, just return
  // the max number of outputs written. If it's false, store out the result
  // values to ref.
  virtual uint32_t simulate(bool countOnly, uint32_t waveSize,
                            add_ref<std::vector<uint64_t>> ref) = 0;

  virtual uint32_t execute(bool countOnly, const uint32_t waveSize,
                           const uint32_t primitiveStride,
                           add_ref<std::vector<UVec4>> ref,
                           add_cref<std::vector<uint32_t>> outputP = {},
                           const UVec4 *cmp = nullptr,
                           const uint32_t primitiveID = (~0u)) {
    // Per-invocation output location counters
    std::vector<uint32_t> outLoc;
    std::vector<WaveState> stateStack;
    uint32_t waveCount;
    uint32_t logFailureCount;
    auto prerequisites = makePrerequisites(outputP, waveSize, primitiveStride,
                                           stateStack, outLoc, waveCount);

    logFailureCount = 10u;
    nesting = 0;
    loopNesting = 0;

    int32_t i = 0;

    while (i < (int32_t)ops.size()) {
      add_cref<Ballots> activeMask = stateStack[nesting].activeMask;

      switch (ops[i].type) {
      case OP_BALLOT:
        simulateBallot(countOnly, activeMask, primitiveID, i, outLoc, ref,
                       prerequisites, logFailureCount,
                       (i > 0 ? ops[i - 1].type : OP_BALLOT), cmp);
        break;
      case OP_STORE:
        simulateStore(countOnly, stateStack[nesting].activeMask, primitiveID,
                      ops[i].value, outLoc, ref, prerequisites, logFailureCount,
                      (i > 0 ? ops[i - 1].type : OP_STORE), cmp);
        break;
      case OP_IF_MASK:
        nesting++;
        stateStack[nesting].activeMask =
            stateStack[nesting - 1].activeMask &
            ballotsFromBallot(ops[i].bvalue, waveSize, waveCount);
        stateStack[nesting].header = i;
        stateStack[nesting].isLoop = 0;
        stateStack[nesting].isSwitch = 0;
        break;
      case OP_ELSE_MASK:
        stateStack[nesting].activeMask =
            stateStack[nesting - 1].activeMask &
            ~ballotsFromBallot(ops[stateStack[nesting].header].bvalue, waveSize,
                               waveCount);
        break;
      case OP_IF_LOOPCOUNT: {
        uint32_t n = nesting;
        while (!stateStack[n].isLoop)
          n--;
        const Ballot tripBallot = Ballot::withSetBit(stateStack[n].tripCount);

        nesting++;
        stateStack[nesting].activeMask =
            stateStack[nesting - 1].activeMask &
            ballotsFromBallot(tripBallot, waveSize, waveCount);
        stateStack[nesting].header = i;
        stateStack[nesting].isLoop = 0;
        stateStack[nesting].isSwitch = 0;
        break;
      }
      case OP_ELSE_LOOPCOUNT: {
        uint32_t n = nesting;
        while (!stateStack[n].isLoop)
          n--;
        const Ballot tripBallot = Ballot::withSetBit(stateStack[n].tripCount);

        stateStack[nesting].activeMask =
            stateStack[nesting - 1].activeMask &
            ~ballotsFromBallot(tripBallot, waveSize, waveCount);
        break;
      }
      case OP_IF_LOCAL_INVOCATION_INDEX: {
        // all bits >= N
        Ballots mask(waveCount);
        const uint32_t maxID = waveCount * waveSize;
        for (uint32_t id = static_cast<uint32_t>(ops[i].value); id < maxID;
             ++id) {
          mask.set(Ballots::findBit(id, waveSize));
        }

        nesting++;
        stateStack[nesting].activeMask =
            stateStack[nesting - 1].activeMask & mask;
        stateStack[nesting].header = i;
        stateStack[nesting].isLoop = 0;
        stateStack[nesting].isSwitch = 0;
        break;
      }
      case OP_ELSE_LOCAL_INVOCATION_INDEX: {
        // all bits < N
        Ballots mask(waveCount);
        const uint32_t maxID = waveCount * waveSize;
        for (uint32_t id = 0u;
             id < static_cast<uint32_t>(ops[i].value) && id < maxID; ++id) {
          mask.set(Ballots::findBit(id, waveSize));
        }

        stateStack[nesting].activeMask =
            stateStack[nesting - 1].activeMask & mask;
        break;
      }
      case OP_ENDIF:
        nesting--;
        break;
      case OP_BEGIN_FOR_UNIF:
        // XXX TODO: We don't handle a for loop with zero iterations
        nesting++;
        loopNesting++;
        stateStack[nesting].activeMask = stateStack[nesting - 1].activeMask;
        stateStack[nesting].header = i;
        stateStack[nesting].tripCount = 0;
        stateStack[nesting].isLoop = 1;
        stateStack[nesting].isSwitch = 0;
        stateStack[nesting].continueMask = 0;
        break;
      case OP_END_FOR_UNIF:
        stateStack[nesting].tripCount++;
        stateStack[nesting].activeMask |= stateStack[nesting].continueMask;
        stateStack[nesting].continueMask = 0;
        if (stateStack[nesting].tripCount <
                ops[stateStack[nesting].header].value &&
            stateStack[nesting].activeMask.any()) {
          i = stateStack[nesting].header + 1;
          continue;
        } else {
          loopNesting--;
          nesting--;
        }
        break;
      case OP_BEGIN_DO_WHILE_UNIF:
        // XXX TODO: We don't handle a for loop with zero iterations
        nesting++;
        loopNesting++;
        stateStack[nesting].activeMask = stateStack[nesting - 1].activeMask;
        stateStack[nesting].header = i;
        stateStack[nesting].tripCount = 1;
        stateStack[nesting].isLoop = 1;
        stateStack[nesting].isSwitch = 0;
        stateStack[nesting].continueMask = 0;
        break;
      case OP_END_DO_WHILE_UNIF:
        stateStack[nesting].activeMask |= stateStack[nesting].continueMask;
        stateStack[nesting].continueMask = 0;
        if (stateStack[nesting].tripCount <
                ops[stateStack[nesting].header].value &&
            stateStack[nesting].activeMask.any()) {
          i = stateStack[nesting].header + 1;
          stateStack[nesting].tripCount++;
          continue;
        } else {
          loopNesting--;
          nesting--;
        }
        break;
      case OP_BEGIN_FOR_VAR:
        // XXX TODO: We don't handle a for loop with zero iterations
        nesting++;
        loopNesting++;
        stateStack[nesting].activeMask = stateStack[nesting - 1].activeMask;
        stateStack[nesting].header = i;
        stateStack[nesting].tripCount = 0;
        stateStack[nesting].isLoop = 1;
        stateStack[nesting].isSwitch = 0;
        stateStack[nesting].continueMask = 0;
        break;
      case OP_END_FOR_VAR: {
        stateStack[nesting].tripCount++;
        stateStack[nesting].activeMask |= stateStack[nesting].continueMask;
        stateStack[nesting].continueMask = 0;
        Ballot tripBallot;
        if (waveSize != stateStack[nesting].tripCount) {
          for (uint32_t bit = stateStack[nesting].tripCount;
               bit < tripBallot.size(); ++bit)
            tripBallot.set(bit);
        }
        stateStack[nesting].activeMask &=
            ballotsFromBallot(tripBallot, waveSize, waveCount);

        if (stateStack[nesting].activeMask.any()) {
          i = stateStack[nesting].header + 1;
          continue;
        } else {
          loopNesting--;
          nesting--;
        }
        break;
      }
      case OP_BEGIN_FOR_INF:
      case OP_BEGIN_DO_WHILE_INF:
        nesting++;
        loopNesting++;
        stateStack[nesting].activeMask = stateStack[nesting - 1].activeMask;
        stateStack[nesting].header = i;
        stateStack[nesting].tripCount = 0;
        stateStack[nesting].isLoop = 1;
        stateStack[nesting].isSwitch = 0;
        stateStack[nesting].continueMask = 0;
        break;
      case OP_END_FOR_INF:
        stateStack[nesting].tripCount++;
        stateStack[nesting].activeMask |= stateStack[nesting].continueMask;
        stateStack[nesting].continueMask = 0;
        if (stateStack[nesting].activeMask.any()) {
          // output expected OP_BALLOT values
          simulateBallot(countOnly, stateStack[nesting].activeMask, primitiveID,
                         i, outLoc, ref, prerequisites, logFailureCount,
                         (i > 0 ? ops[i - 1].type : OP_BALLOT), cmp);

          i = stateStack[nesting].header + 1;
          continue;
        } else {
          loopNesting--;
          nesting--;
        }
        break;
      case OP_END_DO_WHILE_INF:
        stateStack[nesting].tripCount++;
        stateStack[nesting].activeMask |= stateStack[nesting].continueMask;
        stateStack[nesting].continueMask = 0;
        if (stateStack[nesting].activeMask.any()) {
          i = stateStack[nesting].header + 1;
          continue;
        } else {
          loopNesting--;
          nesting--;
        }
        break;
      case OP_BREAK: {
        uint32_t n = nesting;
        const Ballots mask = stateStack[nesting].activeMask;
        while (true) {
          stateStack[n].activeMask &= ~mask;
          if (stateStack[n].isLoop || stateStack[n].isSwitch)
            break;

          n--;
        }
      } break;
      case OP_CONTINUE: {
        uint32_t n = nesting;
        const Ballots mask = stateStack[nesting].activeMask;
        while (true) {
          stateStack[n].activeMask &= ~mask;
          if (stateStack[n].isLoop) {
            stateStack[n].continueMask |= mask;
            break;
          }
          n--;
        }
      } break;
      case OP_ELECT: {
        nesting++;
        stateStack[nesting].activeMask =
            bitsetElect(stateStack[nesting - 1].activeMask);
        stateStack[nesting].header = i;
        stateStack[nesting].isLoop = 0;
        stateStack[nesting].isSwitch = 0;
      } break;
      case OP_RETURN: {
        const Ballots mask = stateStack[nesting].activeMask;
        for (int32_t n = nesting; n >= 0; --n) {
          stateStack[n].activeMask &= ~mask;
          if (stateStack[n].isCall)
            break;
        }
      } break;

      case OP_CALL_BEGIN:
        nesting++;
        stateStack[nesting].activeMask = stateStack[nesting - 1].activeMask;
        stateStack[nesting].isLoop = 0;
        stateStack[nesting].isSwitch = 0;
        stateStack[nesting].isCall = 1;
        break;
      case OP_CALL_END:
        stateStack[nesting].isCall = 0;
        nesting--;
        break;
      case OP_NOISE:
        break;

      case OP_SWITCH_UNIF_BEGIN:
      case OP_SWITCH_VAR_BEGIN:
      case OP_SWITCH_LOOP_COUNT_BEGIN:
        nesting++;
        stateStack[nesting].activeMask = stateStack[nesting - 1].activeMask;
        stateStack[nesting].header = i;
        stateStack[nesting].isLoop = 0;
        stateStack[nesting].isSwitch = 1;
        break;
      case OP_SWITCH_END:
        nesting--;
        break;
      case OP_CASE_MASK_BEGIN:
        stateStack[nesting].activeMask =
            stateStack[nesting - 1].activeMask &
            ballotsFromBallot(ops[i].bvalue, waveSize, waveCount);
        break;
      case OP_CASE_LOOP_COUNT_BEGIN: {
        uint32_t n = nesting;
        uint32_t l = loopNesting;

        while (true) {
          if (stateStack[n].isLoop) {
            l--;
            if (l == ops[stateStack[nesting].header].value)
              break;
          }
          n--;
        }

        if ((Ballot::withSetBit(stateStack[n].tripCount) &
             Ballot(ops[i].bvalue))
                .any())
          stateStack[nesting].activeMask = stateStack[nesting - 1].activeMask;
        else
          stateStack[nesting].activeMask = 0;
        break;
      }
      case OP_CASE_END:
        break;

      default:
        break;
      }
      i++;
    }
    uint32_t maxLoc = 0;
    for (uint32_t id = 0; id < (uint32_t)outLoc.size(); ++id)
      maxLoc = max(maxLoc, outLoc[id]);

    return maxLoc;
  }

protected:
  virtual std::shared_ptr<Prerequisites>
  makePrerequisites(add_cref<std::vector<uint32_t>> /*outputP*/,
                    const uint32_t /*waveSize*/,
                    const uint32_t /*primitiveStride*/,
                    add_ref<std::vector<WaveState>> /*stateStack*/,
                    add_ref<std::vector<uint32_t>> /*outLoc*/,
                    add_ref<uint32_t> /*waveCount*/) {
    return std::make_shared<Prerequisites>();
  }

  virtual void simulateBallot(const bool /*countOnly*/,
                              add_cref<Ballots> /*activeMask*/,
                              const uint32_t /*primitiveID*/,
                              const int32_t /*opsIndex*/,
                              add_ref<std::vector<uint32_t>> /*outLoc*/,
                              add_ref<std::vector<UVec4>> /*ref*/,
                              std::shared_ptr<Prerequisites> /*prerequisites*/,
                              add_ref<uint32_t> /*logFailureCount*/,
                              const OPType /*reason*/, const UVec4 * /*cmp*/) {}

  virtual void simulateStore(const bool /*countOnly*/,
                             add_cref<Ballots> /*activeMask*/,
                             const uint32_t /*primitiveID*/,
                             const uint64_t /*storeValue*/,
                             add_ref<std::vector<uint32_t>> /*outLoc*/,
                             add_ref<std::vector<UVec4>> /*ref*/,
                             std::shared_ptr<Prerequisites> /*prerequisites*/,
                             add_ref<uint32_t> /*logFailureCount*/,
                             const OPType /*reason*/, const UVec4 * /*cmp*/) {}
};
} // namespace reconvergence

#endif // RANDOMPROGRAM_H_
