#define THREADS_X 7
#define THREADS_Y 13

StructuredBuffer<uint> InputA : register(t0);
RWStructuredBuffer<uint4> OutputB : register(u1);

bool testBit(uint4 mask, uint bit) { return ((mask[bit / 32] >> (bit % 32)) & 1) != 0; }
static int outLoc = 0;
static int invocationStride = 91;


[numthreads(THREADS_X, THREADS_Y, 1)]
void main(uint gIndex : SV_GroupIndex)
{
    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
    if (WaveIsFirstLane()) {
        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10002;
        if (WaveIsFirstLane()) {
            if (testBit(uint4(0xd49562fa, 0x4d048210, 0x5ea3e45b, 0x4bec7494), WaveGetLaneIndex())) {
            }
            if (gIndex >= InputA[0xb]) {
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            } else {
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            }
        }
        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
    }
    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
    for (int loopIdx0 = 0;;loopIdx0++,    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true)) {
        if (testBit(uint4(0x4823cf2e, 0x37def99d, 0x2183eb05, 0x60435a6a), WaveGetLaneIndex())) {
            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            if (testBit(uint4(0xd49562fa, 0x4d048210, 0x5ea3e45b, 0x4bec7494), WaveGetLaneIndex())) {
                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10013;
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            }
            switch (WaveGetLaneIndex() & 3) {
                case 0:
                {
                    break;
                }
                case 1:
                {
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    break;
                }
                case 2:
                {
                    break;
                }
                case 3:
                {
                    break;
                }
            }
            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10021;
        } else {
            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10024;
            if (WaveIsFirstLane()) {
                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10026;
            }
        }
        if (WaveIsFirstLane()) {
            if (testBit(uint4(0xad560636, 0xdca36a0f, 0x6e8b91d0, 0x23174fba), WaveGetLaneIndex())) {
                break;
            } else {
                break;
            }
        }
        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
        if (WaveIsFirstLane()) {
            switch (WaveGetLaneIndex() & 3) {
                case 0:
                {
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10035;
                    break;
                }
                case 1:
                {
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    break;
                }
                case 2:
                {
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    break;
                }
                case 3:
                {
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1003f;
                    break;
                }
            }
            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            if (InputA[1] == 1) {
                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10044;
            }
            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10047;
        }
        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1004a;
    }
    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
}

