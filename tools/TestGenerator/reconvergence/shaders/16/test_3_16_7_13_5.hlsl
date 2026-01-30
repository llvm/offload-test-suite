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
    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10000;
    switch (WaveGetLaneIndex() & 3) {
        case 0:
        {
            if (testBit(uint4(0x6dfa865e, 0x5a043f88, 0x2d9b688a, 0x3e9ec403), WaveGetLaneIndex())) {
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10005;
            } else {
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
                        break;
                    }
                    case 3:
                    {
                        break;
                    }
                }
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10012;
            }
            break;
        }
        case 1:
        {
            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            switch (WaveGetLaneIndex() & 3) {
                case 0:
                {
                    if (testBit(uint4(0x6dfa865e, 0x5a043f88, 0x2d9b688a, 0x3e9ec403), WaveGetLaneIndex())) {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1001b;
                    } else {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1001b;
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    break;
                }
                case 1:
                {
                    if (gIndex >= InputA[0x46]) {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        while (!WaveIsFirstLane()) {}
                    } else {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        while (!WaveIsFirstLane()) {}
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1002b;
                    break;
                }
                case 2:
                {
                    if (gIndex >= InputA[0x27]) {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    }
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
            if (gIndex >= InputA[0x3e]) {
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                if (InputA[1] == 1) {
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                } else {
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                }
            }
            break;
        }
        case 3:
        {
            if (InputA[3] == 3) {
                if (WaveIsFirstLane()) {
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                }
            } else {
                if (gIndex >= InputA[0x47]) {
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10048;
                }
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            }
            break;
        }
    }
}

