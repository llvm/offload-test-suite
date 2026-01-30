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
    {
        int loopIdx0 = 0;
        do {
            loopIdx0++;
            if (InputA[2] == 2) {
                for (int loopIdx1 = 0;;loopIdx1++,                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true)) {
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    if (WaveIsFirstLane()) {
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10005;
                        break;
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10008;
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                }
                {
                    int loopIdx1 = 0;
                    do {
                        loopIdx1++;
                    } while (loopIdx1 < InputA[5]);
                }
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            }
            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1000f;
            switch (WaveGetLaneIndex() & 3) {
                case 0:
                {
                    if (WaveIsFirstLane()) {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    }
                    break;
                }
                case 1:
                {
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10017;
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
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            break;
                        }
                    }
                    break;
                }
                case 2:
                {
                    {
                        int loopIdx1 = 0;
                        do {
                            loopIdx1++;
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        } while (loopIdx1 < InputA[1]);
                    }
                    break;
                }
                case 3:
                {
                    break;
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1002c;
                    break;
                }
            }
        } while (loopIdx0 < InputA[2]);
    }
}

