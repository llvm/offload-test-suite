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
    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
    if (testBit(uint4(0x7c271cdc, 0xcb18f481, 0x6ec4196a, 0x550db835), WaveGetLaneIndex())) {
        if (InputA[0] == 0) {
            switch (WaveGetLaneIndex() & 3) {
                case 0:
                {
                    break;
                }
                case 1:
                {
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10008;
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
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1000f;
                    break;
                }
            }
            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            if (InputA[0] == 0) {
                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10014;
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            }
        } else {
            if (WaveIsFirstLane()) {
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            }
            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
        }
        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1001d;
    }
    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
}

