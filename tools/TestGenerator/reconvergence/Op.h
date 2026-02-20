#ifndef OP_H_
#define OP_H_

#include "Ballot.h"
#include "VectorUtils.h"

namespace reconvergence {
enum OPType {
  // store subgroupBallot().
  // For OP_BALLOT, OP::caseValue is initialized to zero, and then
  // set to 1 by simulate if the ballot is not threadgroup- (or wave-_uniform.
  // Only threadgroup-uniform ballots are validated for correctness in
  // WUCF modes.
  OP_BALLOT,

  // store literal constant
  OP_STORE,

  // if ((1ULL << gl_SubgroupInvocationID) & mask).
  // Special case if mask = ~0ULL, converted into "if (inputA[idx] == idx)"
  OP_IF_MASK,
  OP_ELSE_MASK,
  OP_ENDIF,

  // if (gl_SubgroupInvocationID == loopIdxN) (where N is most nested loop
  // counter)
  OP_IF_LOOPCOUNT,
  OP_ELSE_LOOPCOUNT,

  // if (gl_LocalInvocationIndex >= inputA[N]) (where N is most nested loop
  // counter)
  OP_IF_LOCAL_INVOCATION_INDEX,
  OP_ELSE_LOCAL_INVOCATION_INDEX,

  // break/continue
  OP_BREAK,
  OP_CONTINUE,

  // if (subgroupElect())
  OP_ELECT,

  // Loop with uniform number of iterations (read from a buffer)
  OP_BEGIN_FOR_UNIF,
  OP_END_FOR_UNIF,

  // for (int loopIdxN = 0; loopIdxN < gl_SubgroupInvocationID + 1; ++loopIdxN)
  OP_BEGIN_FOR_VAR,
  OP_END_FOR_VAR,

  // for (int loopIdxN = 0;; ++loopIdxN, OP_BALLOT)
  // Always has an "if (subgroupElect()) break;" inside.
  // Does the equivalent of OP_BALLOT in the continue construct
  OP_BEGIN_FOR_INF,
  OP_END_FOR_INF,

  // do { loopIdxN++; ... } while (loopIdxN < uniformValue);
  OP_BEGIN_DO_WHILE_UNIF,
  OP_END_DO_WHILE_UNIF,

  // do { ... } while (true);
  // Always has an "if (subgroupElect()) break;" inside
  OP_BEGIN_DO_WHILE_INF,
  OP_END_DO_WHILE_INF,

  // return;
  OP_RETURN,

  // function call (code bracketed by these is extracted into a separate
  // function)
  OP_CALL_BEGIN,
  OP_CALL_END,

  // switch statement on uniform value
  OP_SWITCH_UNIF_BEGIN,
  // switch statement on gl_SubgroupInvocationID & 3 value
  OP_SWITCH_VAR_BEGIN,
  // switch statement on loopIdx value
  OP_SWITCH_LOOP_COUNT_BEGIN,

  // case statement with a (invocation mask, case mask) pair
  OP_CASE_MASK_BEGIN,
  // case statement used for loop counter switches, with a value and a mask of
  // loop iterations
  OP_CASE_LOOP_COUNT_BEGIN,

  // end of switch/case statement
  OP_SWITCH_END,
  OP_CASE_END,

  // Extra code with no functional effect. Currently inculdes:
  // - value 0: while (!subgroupElect()) {}
  // - value 1: if (condition_that_is_false) { infinite loop }
  OP_NOISE,

  // do nothing, only markup
  OP_NOP
};

const char *OPtypeToStr(const OPType op) {
#define MAKETEXT(s__) #s__
#define CASETEXT(e__)                                                          \
  case e__:                                                                    \
    return MAKETEXT(e__)
  switch (op) {
    CASETEXT(OP_BALLOT);
    CASETEXT(OP_STORE);
    CASETEXT(OP_IF_MASK);
    CASETEXT(OP_ELSE_MASK);
    CASETEXT(OP_ENDIF);
    CASETEXT(OP_IF_LOOPCOUNT);
    CASETEXT(OP_ELSE_LOOPCOUNT);
    CASETEXT(OP_IF_LOCAL_INVOCATION_INDEX);
    CASETEXT(OP_ELSE_LOCAL_INVOCATION_INDEX);
    CASETEXT(OP_BREAK);
    CASETEXT(OP_CONTINUE);
    CASETEXT(OP_ELECT);
    CASETEXT(OP_BEGIN_FOR_UNIF);
    CASETEXT(OP_END_FOR_UNIF);
    CASETEXT(OP_BEGIN_FOR_VAR);
    CASETEXT(OP_END_FOR_VAR);
    CASETEXT(OP_BEGIN_FOR_INF);
    CASETEXT(OP_END_FOR_INF);
    CASETEXT(OP_BEGIN_DO_WHILE_UNIF);
    CASETEXT(OP_END_DO_WHILE_UNIF);
    CASETEXT(OP_BEGIN_DO_WHILE_INF);
    CASETEXT(OP_END_DO_WHILE_INF);
    CASETEXT(OP_RETURN);
    CASETEXT(OP_CALL_BEGIN);
    CASETEXT(OP_CALL_END);
    CASETEXT(OP_SWITCH_UNIF_BEGIN);
    CASETEXT(OP_SWITCH_VAR_BEGIN);
    CASETEXT(OP_SWITCH_LOOP_COUNT_BEGIN);
    CASETEXT(OP_CASE_MASK_BEGIN);
    CASETEXT(OP_CASE_LOOP_COUNT_BEGIN);
    CASETEXT(OP_SWITCH_END);
    CASETEXT(OP_CASE_END);
    CASETEXT(OP_NOISE);
    CASETEXT(OP_NOP);
  }
  return "<Unknown>";
}

typedef enum {
  // Different if test conditions
  IF_MASK,
  IF_UNIFORM,
  IF_LOOPCOUNT,
  IF_LOCAL_INVOCATION_INDEX,
} IFType;

class OP {
public:
  OP(OPType _type, uint64_t _value, uint32_t _caseValue = 0)
      : type(_type), value(_value)
        // by default, initialized only lower part with a repetition of _value
        ,
        bvalue(UVec4(static_cast<uint32_t>(_value),
                     static_cast<uint32_t>(_value >> 32),
                     static_cast<uint32_t>(_value),
                     static_cast<uint32_t>(_value >> 32))),
        caseValue(_caseValue) {}

  // The type of operation and an optional value.
  // The value could be a mask for an if test, the index of the loop
  // header for an end of loop, or the constant value for a store instruction
  OPType type;
  uint64_t value;
  Ballot bvalue;
  uint32_t caseValue;
};
} // namespace reconvergence

#endif // OP_H_
