[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 12))) {
    if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 12))) {
      result = (result + WaveActiveMin((WaveGetLaneIndex() + 3)));
    }
    for (uint i0 = 0; (i0 < 2); i0 = (i0 + 1)) {
      if ((WaveGetLaneIndex() == 14)) {
        result = (result + WaveActiveSum(5));
      }
      if (((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 13))) {
        switch ((WaveGetLaneIndex() % 3)) {
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
        default: {
            result = (result + WaveActiveSum(99));
            break;
          }
        }
        if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 12))) {
          result = (result + WaveActiveMax(result));
        }
      } else {
      if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 12))) {
        result = (result + WaveActiveMin(result));
      }
      if ((WaveGetLaneIndex() < 4)) {
        if ((WaveGetLaneIndex() < 4)) {
          result = (result + WaveActiveMax(WaveGetLaneIndex()));
        }
        if ((WaveGetLaneIndex() >= 11)) {
          result = (result + WaveActiveSum(WaveGetLaneIndex()));
        }
      }
      if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 14))) {
        result = (result + WaveActiveSum(3));
      }
    }
    if ((WaveGetLaneIndex() == 4)) {
      result = (result + WaveActiveMin(4));
    }
    if ((i0 == 1)) {
      continue;
    }
    if ((i0 == 1)) {
      break;
    }
  }
  if (((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 13))) {
    result = (result + WaveActiveSum(result));
  }
  }
}
