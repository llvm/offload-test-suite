[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 13))) {
    if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 15))) {
      result = (result + WaveActiveMin(6));
    }
    uint counter0 = 0;
    while ((counter0 < 3)) {
      counter0 = (counter0 + 1);
      if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 12))) {
        result = (result + WaveActiveMin(WaveGetLaneIndex()));
      }
      if ((WaveGetLaneIndex() >= 12)) {
        if ((WaveGetLaneIndex() >= 11)) {
          result = (result + WaveActiveMax(result));
        }
        switch ((WaveGetLaneIndex() % 4)) {
        case 0: {
            if ((WaveGetLaneIndex() < 8)) {
              result = (result + WaveActiveSum(1));
            }
            break;
          }
        case 1: {
            if (((WaveGetLaneIndex() % 2) == 0)) {
              result = (result + WaveActiveSum(2));
            }
            break;
          }
        case 2: {
            if (true) {
              result = (result + WaveActiveSum(3));
            }
            break;
          }
        case 3: {
            if ((WaveGetLaneIndex() < 20)) {
              result = (result + WaveActiveSum(4));
            }
            break;
          }
        }
      }
      if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 15))) {
        result = (result + WaveActiveSum(WaveGetLaneIndex()));
      }
    }
    if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 10))) {
      result = (result + WaveActiveSum(result));
    }
  } else {
  if (((WaveGetLaneIndex() & 1) == 0)) {
    result = (result + WaveActiveSum(2));
  }
  }
}
