#define THREADS_X 7
#define THREADS_Y 13

StructuredBuffer<uint> InputA : register(t0);
RWStructuredBuffer<uint4> OutputB : register(u1);

bool testBit(uint4 mask, uint bit) { return ((mask[bit / 32] >> (bit % 32)) & 1) != 0; }
static int outLoc = 0;
static int invocationStride = 91;

            void func0(uint gIndex, int loopIdx0) {
                if (WaveIsFirstLane()) {
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1012e;
                }
            }

[numthreads(THREADS_X, THREADS_Y, 1)]
void main(uint gIndex : SV_GroupIndex)
{
    for (int loopIdx0 = 0;
             loopIdx0 < InputA[5];
             loopIdx0++) {
        for (int loopIdx1 = 0;
                 loopIdx1 < WaveGetLaneIndex() + 1;
                 loopIdx1++) {
            if (WaveIsFirstLane()) {
                {
                    int loopIdx2 = 0;
                    do {
                        for (int loopIdx3 = 0;;loopIdx3++,                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true)) {
                            switch (WaveGetLaneIndex() & 3) {
                                case 0:
                                {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                    break;
                                }
                                case 1:
                                {
                                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1000a;
                                    break;
                                }
                                case 2:
                                {
                                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1000d;
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
                            if (WaveGetLaneIndex() == loopIdx3) {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            }
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10018;
                            if (WaveIsFirstLane()) {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                break;
                            }
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1001d;
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        if (WaveGetLaneIndex() == loopIdx2) {
                            for (int loopIdx3 = 0;
                                     loopIdx3 < WaveGetLaneIndex() + 1;
                                     loopIdx3++) {
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10023;
                            }
                            break;
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        } else {
                            if (InputA[3] == 3) {
                            }
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1002b;
                        if (WaveIsFirstLane()) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            break;
                            if (testBit(uint4(0xe459f25a, 0xdca35218, 0x278524d4, 0x6a18b2f7), WaveGetLaneIndex())) {
                                break;
                            } else {
                                break;
                            }
                        }
                        if (gIndex >= InputA[0x54]) {
                            if (testBit(uint4(0xbe1d7ec5, 0xeebe994d, 0x7b9a81b7, 0xf95758), WaveGetLaneIndex())) {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            } else {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            }
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            if (InputA[1] == 1) {
                            }
                        } else {
                            if (testBit(uint4(0xe459f25a, 0xdca35218, 0x278524d4, 0x6a18b2f7), WaveGetLaneIndex())) {
                            }
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10041;
                            switch (WaveGetLaneIndex() & 3) {
                                case 0:
                                {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
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
                                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1004d;
                                    break;
                                }
                            }
                        }
                        if (gIndex >= InputA[0x3]) {
                            switch (loopIdx1) {
                                case 1: {
                                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10054;
                                    break;
                                }
                                case 2: {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10058;
                                    break;
                                }
                                default: {
                                    break;
                                }
                            }
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1005d;
                            if (WaveGetLaneIndex() == loopIdx2) {
                            } else {
                            }
                        }
                        loopIdx2++;
                    } while (true);
                }
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                switch (WaveGetLaneIndex() & 3) {
                    case 0:
                    {
                        break;
                    }
                    case 1:
                    {
                        if (WaveGetLaneIndex() == loopIdx1) {
                            for (int loopIdx2 = 0;;loopIdx2++,                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true)) {
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1006a;
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                if (WaveIsFirstLane()) {
                                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1006d;
                                    break;
                                }
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            }
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            if (gIndex >= InputA[0x36]) {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            }
                        } else {
                            for (int loopIdx2 = 0;;loopIdx2++,                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true)) {
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1006a;
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                if (WaveIsFirstLane()) {
                                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1006d;
                                    break;
                                }
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            }
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            if (gIndex >= InputA[0x36]) {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            }
                        }
                        break;
                    }
                    case 2:
                    {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        for (int loopIdx2 = 0;
                                 loopIdx2 < InputA[3];
                                 loopIdx2++) {
                            if (gIndex >= InputA[0x32]) {
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1008a;
                            }
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            switch (WaveGetLaneIndex() & 3) {
                                case 0:
                                {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
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
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                        break;
                    }
                    case 3:
                    {
                        if (testBit(uint4(0xbe1d7ec5, 0xeebe994d, 0x7b9a81b7, 0xf95758), WaveGetLaneIndex())) {
                            if (testBit(uint4(0x477bc001, 0xe05428d, 0x20ee35be, 0xa56b6ae9), WaveGetLaneIndex())) {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            }
                            if (WaveIsFirstLane()) {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            }
                        } else {
                            break;
                            switch (loopIdx0) {
                                case 1: {
                                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100a9;
                                    break;
                                }
                                case 2: {
                                    break;
                                }
                                default: {
                                    break;
                                }
                            }
                        }
                        break;
                    }
                }
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            }
            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            for (int loopIdx2 = 0;
                     loopIdx2 < InputA[2];
                     loopIdx2++) {
                {
                    int loopIdx3 = 0;
                    do {
                        if (testBit(uint4(0xca9074f8, 0xe17081ae, 0x6b1dc3b1, 0x65b0e857), WaveGetLaneIndex())) {
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100ba;
                        if (InputA[0] == 0) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100bd;
                            if (testBit(uint4(0x9d39b956, 0x4db0a04, 0x32e93e3e, 0x63fbb43a), WaveGetLaneIndex())) {
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100bf;
                            }
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        } else {
                            if (WaveIsFirstLane()) {
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100c4;
                            }
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100c6;
                            if (InputA[2] == 2) {
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100c8;
                            } else {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            }
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        if (WaveIsFirstLane()) {
                            break;
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100d1;
                        break;
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        if (gIndex >= InputA[0x22]) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            if (testBit(uint4(0x9d39b956, 0x4db0a04, 0x32e93e3e, 0x63fbb43a), WaveGetLaneIndex())) {
                            } else {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            }
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        } else {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            if (testBit(uint4(0x9d39b956, 0x4db0a04, 0x32e93e3e, 0x63fbb43a), WaveGetLaneIndex())) {
                            } else {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            }
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        loopIdx3++;
                    } while (true);
                }
                if (gIndex >= InputA[0x1a]) {
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    switch (WaveGetLaneIndex() & 3) {
                        case 0:
                        {
                            switch (WaveGetLaneIndex() & 3) {
                                case 0:
                                {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100ec;
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
                                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100f4;
                                    break;
                                }
                            }
                            break;
                        }
                        case 1:
                        {
                            if (gIndex >= InputA[0xc]) {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            }
                            break;
                        }
                        case 2:
                        {
                            continue;
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100ff;
                            break;
                        }
                        case 3:
                        {
                            continue;
                            break;
                        }
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10105;
                    if (WaveGetLaneIndex() == loopIdx2) {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        if (testBit(uint4(0x9d39b956, 0x4db0a04, 0x32e93e3e, 0x63fbb43a), WaveGetLaneIndex())) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1010b;
                        continue;
                    }
                } else {
                    for (int loopIdx3 = 0;
                             loopIdx3 < InputA[3];
                             loopIdx3++) {
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
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1011c;
                    if (WaveIsFirstLane()) {
                        {
                            int loopIdx3 = 0;
                            do {
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1011f;
                                if (WaveIsFirstLane()) {
                                    break;
                                }
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10123;
                                loopIdx3++;
                            } while (true);
                        }
                        break;
                    }
                }
            }
        }
        if (WaveIsFirstLane()) {
            func0(gIndex, loopIdx0);
            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10132;
            if (gIndex >= InputA[0x57]) {
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                if (WaveIsFirstLane()) {
                    if (InputA[0] == 0) {
                        for (int loopIdx1 = 0;
                                 loopIdx1 < WaveGetLaneIndex() + 1;
                                 loopIdx1++) {
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        if (testBit(uint4(0xe459f25a, 0xdca35218, 0x278524d4, 0x6a18b2f7), WaveGetLaneIndex())) {
                            continue;
                        } else {
                            continue;
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1013f;
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10141;
                    {
                        int loopIdx1 = 0;
                        do {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            if (WaveGetLaneIndex() == loopIdx1) {
                            } else {
                            }
                            if (WaveIsFirstLane()) {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                if (testBit(uint4(0xe459f25a, 0xdca35218, 0x278524d4, 0x6a18b2f7), WaveGetLaneIndex())) {
                                    break;
                                } else {
                                    break;
                                }
                            }
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            for (int loopIdx2 = 0;;loopIdx2++,                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true)) {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10152;
                                if (WaveIsFirstLane()) {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10155;
                                    break;
                                }
                            }
                            if (InputA[0] == 0) {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            } else {
                            }
                            loopIdx1++;
                        } while (true);
                    }
                }
                for (int loopIdx1 = 0;
                         loopIdx1 < WaveGetLaneIndex() + 1;
                         loopIdx1++) {
                    break;
                    if (WaveGetLaneIndex() == loopIdx1) {
                        if (InputA[0] == 0) {
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    } else {
                        if (InputA[2] == 2) {
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    }
                }
            } else {
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                if (InputA[0] == 0) {
                    for (int loopIdx1 = 0;;loopIdx1++,                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true)) {
                        switch (loopIdx0) {
                            case 1: {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                break;
                            }
                            case 2: {
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10174;
                                break;
                            }
                            default: {
                                break;
                            }
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
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
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10182;
                                break;
                            }
                            case 3:
                            {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                break;
                            }
                        }
                        if (WaveIsFirstLane()) {
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10189;
                            break;
                        }
                        if (gIndex >= InputA[0x11]) {
                            if (InputA[0] == 12345) {
                                while (true) {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                }
                            }
                        }
                        {
                            int loopIdx2 = 0;
                            do {
                                loopIdx2++;
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            } while (loopIdx2 < InputA[2]);
                        }
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10193;
                }
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                if (testBit(uint4(0xe459f25a, 0xdca35218, 0x278524d4, 0x6a18b2f7), WaveGetLaneIndex())) {
                    continue;
                } else {
                    continue;
                }
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            }
        }
    }
}

