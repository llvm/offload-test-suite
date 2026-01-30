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
    if (InputA[1] == 1) {
        switch (WaveGetLaneIndex() & 3) {
            case 0:
            {
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                break;
            }
            case 1:
            {
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                if (testBit(uint4(0x89eabcfb, 0xd9b69862, 0xe646b896, 0xe35f35d1), WaveGetLaneIndex())) {
                    if (gIndex >= InputA[0x37]) {
                    } else {
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1000a;
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                }
                break;
            }
            case 2:
            {
                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10010;
                break;
            }
            case 3:
            {
                if (testBit(uint4(0xd690d39c, 0x218e2b3e, 0x893185a1, 0x44e5c750), WaveGetLaneIndex())) {
                    if (gIndex >= InputA[0x50]) {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10016;
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    if (gIndex >= InputA[0x31]) {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                } else {
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    if (WaveIsFirstLane()) {
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                }
                break;
            }
        }
    }
}

