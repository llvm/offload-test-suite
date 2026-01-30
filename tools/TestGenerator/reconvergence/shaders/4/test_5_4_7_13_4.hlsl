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
        if (InputA[1] == 1) {
            switch (WaveGetLaneIndex() & 3) {
                case 0:
                {
                    if (WaveIsFirstLane()) {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10008;
                        for (int loopIdx0 = 0;
                                 loopIdx0 < InputA[5];
                                 loopIdx0++) {
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1000a;
                        }
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    break;
                }
                case 1:
                {
                    if (gIndex >= InputA[0x44]) {
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10011;
                        if (WaveIsFirstLane()) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                    } else {
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10016;
                    }
                    break;
                }
                case 2:
                {
                    break;
                }
                case 3:
                {
                    if (InputA[1] == 1) {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        for (int loopIdx0 = 0;
                                 loopIdx0 < WaveGetLaneIndex() + 1;
                                 loopIdx0++) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                        if (testBit(uint4(0x55f9c0db, 0x2434d3fe, 0xbccf3e1c, 0x2fb1f2c0), WaveGetLaneIndex())) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        } else {
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10024;
                        }
                    }
                    break;
                }
            }
            if (gIndex >= InputA[0x48]) {
                if (InputA[1] == 1) {
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    {
                        int loopIdx0 = 0;
                        do {
                            loopIdx0++;
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1002d;
                        } while (loopIdx0 < InputA[1]);
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    if (testBit(uint4(0x56e4eb08, 0xc50314e5, 0xf94d540e, 0x41e77aae), WaveGetLaneIndex())) {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    } else {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    }
                } else {
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    {
                        int loopIdx0 = 0;
                        do {
                            loopIdx0++;
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1002d;
                        } while (loopIdx0 < InputA[1]);
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    if (testBit(uint4(0x56e4eb08, 0xc50314e5, 0xf94d540e, 0x41e77aae), WaveGetLaneIndex())) {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    } else {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    }
                }
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                switch (InputA[1]) {
                    case 2:
                    {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        if (WaveIsFirstLane()) {
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10046;
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                        break;
                    }
                    case 1:
                    {
                        if (gIndex >= InputA[0xf]) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1004d;
                        } else {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                        {
                            int loopIdx0 = 0;
                            do {
                                loopIdx0++;
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            } while (loopIdx0 < InputA[5]);
                        }
                        break;
                    }
                    case 3:
                    {
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10056;
                        {
                            int loopIdx0 = 0;
                            do {
                                loopIdx0++;
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10059;
                            } while (loopIdx0 < InputA[4]);
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        break;
                    }
                }
            }
        }
        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
        if (gIndex >= InputA[0x29]) {
            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            if (testBit(uint4(0x55f9c0db, 0x2434d3fe, 0xbccf3e1c, 0x2fb1f2c0), WaveGetLaneIndex())) {
                if (gIndex >= InputA[0x1e]) {
                    if (testBit(uint4(0x56e4eb08, 0xc50314e5, 0xf94d540e, 0x41e77aae), WaveGetLaneIndex())) {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    } else {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1006b;
                    {
                        int loopIdx0 = 0;
                        do {
                            if (WaveIsFirstLane()) {
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1006e;
                                break;
                            }
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            loopIdx0++;
                        } while (true);
                    }
                } else {
                    if (gIndex >= InputA[0x5]) {
                    } else {
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
                }
                switch (WaveGetLaneIndex() & 3) {
                    case 0:
                    {
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10087;
                        for (int loopIdx0 = 0;
                                 loopIdx0 < InputA[3];
                                 loopIdx0++) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                        break;
                    }
                    case 1:
                    {
                        if (InputA[2] == 2) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                        break;
                    }
                    case 2:
                    {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        break;
                    }
                    case 3:
                    {
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10095;
                        switch (InputA[3]) {
                            case 4:
                            {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                break;
                            }
                            case 3:
                            {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                break;
                            }
                            case 5:
                            {
                                break;
                            }
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100a0;
                        break;
                    }
                }
            }
        }
    } else {
        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100a7;
        if (gIndex >= InputA[0x27]) {
            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100a9;
            {
                int loopIdx0 = 0;
                do {
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100ab;
                    switch (WaveGetLaneIndex() & 3) {
                        case 0:
                        {
                            for (int loopIdx1 = 0;
                                     loopIdx1 < InputA[5];
                                     loopIdx1++) {
                            }
                            break;
                        }
                        case 1:
                        {
                            for (int loopIdx1 = 0;
                                     loopIdx1 < InputA[1];
                                     loopIdx1++) {
                            }
                            break;
                        }
                        case 2:
                        {
                            break;
                        }
                        case 3:
                        {
                            break;
                            break;
                        }
                    }
                    if (testBit(uint4(0xc06a63de, 0xdca35a29, 0x3b774a8, 0x4e2b27f5), WaveGetLaneIndex())) {
                        if (WaveIsFirstLane()) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                        break;
                    } else {
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100c1;
                        {
                            int loopIdx1 = 0;
                            do {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                if (WaveIsFirstLane()) {
                                    break;
                                }
                                loopIdx1++;
                            } while (true);
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100c8;
                        {
                            int loopIdx1 = 0;
                            do {
                                loopIdx1++;
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            } while (loopIdx1 < InputA[3]);
                        }
                    }
                    if (WaveIsFirstLane()) {
                        {
                            int loopIdx1 = 0;
                            do {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                if (WaveIsFirstLane()) {
                                    break;
                                }
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                loopIdx1++;
                            } while (true);
                        }
                        break;
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100d7;
                    switch (WaveGetLaneIndex() & 3) {
                        case 0:
                        {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            if (gIndex >= InputA[0x3d]) {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            }
                            break;
                        }
                        case 1:
                        {
                            if (WaveIsFirstLane()) {
                            }
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            break;
                        }
                        case 2:
                        {
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100e5;
                            switch (WaveGetLaneIndex() & 3) {
                                case 0:
                                {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100e9;
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
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100f4;
                            break;
                        }
                        case 3:
                        {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            break;
                        }
                    }
                    break;
                    loopIdx0++;
                } while (true);
            }
        } else {
            if (gIndex >= InputA[0x1c]) {
                switch (InputA[3]) {
                    case 4:
                    {
                        if (WaveIsFirstLane()) {
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10102;
                        break;
                    }
                    case 3:
                    {
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
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1010c;
                                break;
                            }
                            case 3:
                            {
                                break;
                            }
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10112;
                        break;
                    }
                    case 5:
                    {
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10115;
                        break;
                    }
                }
                if (gIndex >= InputA[0x50]) {
                    switch (InputA[1]) {
                        case 2:
                        {
                            break;
                        }
                        case 1:
                        {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            break;
                        }
                        case 3:
                        {
                            break;
                        }
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10122;
                    if (gIndex >= InputA[0x25]) {
                    }
                } else {
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    if (InputA[3] == 3) {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10129;
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    for (int loopIdx0 = 0;
                             loopIdx0 < InputA[5];
                             loopIdx0++) {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    }
                }
            } else {
                switch (InputA[0]) {
                    case 1:
                    {
                        for (int loopIdx0 = 0;;loopIdx0++,                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true)) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            if (WaveIsFirstLane()) {
                                break;
                            }
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                        break;
                    }
                    case 0:
                    {
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1013c;
                        switch (InputA[1]) {
                            case 2:
                            {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                break;
                            }
                            case 1:
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
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10148;
                        for (int loopIdx0 = 0;
                                 loopIdx0 < WaveGetLaneIndex() + 1;
                                 loopIdx0++) {
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1014b;
                        break;
                    }
                    case 2:
                    {
                        if (WaveIsFirstLane()) {
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1014f;
                        }
                        break;
                    }
                }
                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10153;
                if (gIndex >= InputA[0x48]) {
                    {
                        int loopIdx0 = 0;
                        do {
                            loopIdx0++;
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10157;
                        } while (loopIdx0 < InputA[1]);
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10159;
                }
            }
        }
        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1015d;
        if (WaveIsFirstLane()) {
        }
    }
    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
}

