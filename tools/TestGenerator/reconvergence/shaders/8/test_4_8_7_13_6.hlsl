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
                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10005;
            }
            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            for (int loopIdx0 = 0;;loopIdx0++,            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true)) {
                if (InputA[1] == 1) {
                } else {
                }
                switch (WaveGetLaneIndex() & 3) {
                    case 0:
                    {
                        break;
                    }
                    case 1:
                    {
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10010;
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
                        break;
                    }
                }
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                if (WaveIsFirstLane()) {
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1001c;
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    break;
                }
                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10020;
                if (testBit(uint4(0xd49562fa, 0x4d048210, 0x5ea3e45b, 0x4bec7494), WaveGetLaneIndex())) {
                } else {
                }
                if (WaveGetLaneIndex() == loopIdx0) {
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10026;
                }
            }
            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
        }
        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
        for (int loopIdx0 = 0;
                 loopIdx0 < WaveGetLaneIndex() + 1;
                 loopIdx0++) {
            break;
            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            if (WaveGetLaneIndex() == loopIdx0) {
                for (int loopIdx1 = 0;
                         loopIdx1 < InputA[4];
                         loopIdx1++) {
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                }
                continue;
            }
        }
    }
}

