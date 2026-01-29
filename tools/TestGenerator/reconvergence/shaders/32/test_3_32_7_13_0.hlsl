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
    if (WaveIsFirstLane()) {
        for (int loopIdx0 = 0;
                 loopIdx0 < InputA[4];
                 loopIdx0++) {
            {
                int loopIdx1 = 0;
                do {
                    loopIdx1++;
                } while (loopIdx1 < InputA[1]);
            }
            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
        }
        if (gIndex >= InputA[0x31]) {
            if (gIndex >= InputA[0x6]) {
            }
            if (WaveIsFirstLane()) {
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            }
        } else {
            if (gIndex >= InputA[0x2d]) {
                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1000e;
            }
            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10010;
        }
    }
    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10013;
    {
        int loopIdx0 = 0;
        do {
            loopIdx0++;
            if (gIndex >= InputA[0x31]) {
                switch (WaveGetLaneIndex() & 3) {
                    case 0:
                    {
                        break;
                    }
                    case 1:
                    {
                        break;
                    }
                    case 2:
                    {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        break;
                    }
                    case 3:
                    {
                        break;
                    }
                }
                {
                    int loopIdx1 = 0;
                    do {
                        loopIdx1++;
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    } while (loopIdx1 < InputA[4]);
                }
                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10024;
            } else {
                if (WaveGetLaneIndex() == loopIdx0) {
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10027;
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                } else {
                }
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1002c;
            }
            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            if (testBit(uint4(0xdca35e7e, 0xdca345ea, 0x1f7f4828, 0x52e219cc), WaveGetLaneIndex())) {
                break;
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                if (gIndex >= InputA[0x41]) {
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10033;
                } else {
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                }
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            }
        } while (loopIdx0 < InputA[2]);
    }
}

