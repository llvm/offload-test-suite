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
        }
        for (int loopIdx0 = 0;
                 loopIdx0 < WaveGetLaneIndex() + 1;
                 loopIdx0++) {
        }
        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10007;
    }
    {
        int loopIdx0 = 0;
        do {
            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            if (WaveIsFirstLane()) {
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            }
            if (WaveIsFirstLane()) {
                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1000f;
                break;
            }
            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            return;
            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            {
                int loopIdx1 = 0;
                do {
                    loopIdx1++;
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                } while (loopIdx1 < InputA[3]);
            }
            loopIdx0++;
        } while (true);
    }
    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
    if (testBit(uint4(0xb4c5b9ab, 0xa9bf7d1c, 0x1f014b71, 0x2d365a16), WaveGetLaneIndex())) {
        if (WaveIsFirstLane()) {
            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1001c;
        }
        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
    }
}

