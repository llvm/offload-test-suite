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
    if (InputA[0] == 0) {
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
        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
    } else {
        if (gIndex >= InputA[0xb]) {
            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
        } else {
            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
        }
        if (InputA[0] == 0) {
            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10015;
        }
        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10017;
    }
    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10019;
    {
        int loopIdx0 = 0;
        do {
            loopIdx0++;
            for (int loopIdx1 = 0;;loopIdx1++,            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true)) {
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                if (WaveIsFirstLane()) {
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1001e;
                    break;
                }
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            }
            if (WaveGetLaneIndex() == loopIdx0) {
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            } else {
                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10026;
            }
        } while (loopIdx0 < InputA[3]);
    }
}

