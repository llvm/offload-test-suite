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
    if (testBit(uint4(0x4be9eec2, 0x80c7a6f2, 0xe252fe7f, 0xf1ddec4d), WaveGetLaneIndex())) {
        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
        if (WaveIsFirstLane()) {
            if (gIndex >= InputA[0x23]) {
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10005;
            }
            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10008;
        }
        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
        if (InputA[0] == 0) {
            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
        } else {
            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1000f;
            if (InputA[3] == 3) {
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            }
            if (InputA[0] == 0) {
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            }
            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10016;
        }
    }
    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
    {
        int loopIdx0 = 0;
        do {
            for (int loopIdx1 = 0;;loopIdx1++,            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true)) {
                for (int loopIdx2 = 0;
                         loopIdx2 < InputA[5];
                         loopIdx2++) {
                }
                if (WaveIsFirstLane()) {
                    break;
                }
                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10021;
                for (int loopIdx2 = 0;
                         loopIdx2 < InputA[3];
                         loopIdx2++) {
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10023;
                }
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                for (int loopIdx2 = 0;
                         loopIdx2 < InputA[1];
                         loopIdx2++) {
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                }
            }
            if (WaveIsFirstLane()) {
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                break;
            }
            if (gIndex >= InputA[0x4b]) {
                if (InputA[3] == 3) {
                } else {
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                }
            }
            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            if (WaveIsFirstLane()) {
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
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        break;
                    }
                    case 3:
                    {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        break;
                    }
                }
            }
            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            loopIdx0++;
        } while (true);
    }
}

