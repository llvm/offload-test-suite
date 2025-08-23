[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 4)) {
  case 0: {
      for (uint i0 = 0; (i0 < 3); i0 = (i0 + 1)) {
        if (((WaveGetLaneIndex() & 1) == 1)) {
          if (((WaveGetLaneIndex() & 1) == 1)) {
            result = (result + WaveActiveMin(result));
          }
          if ((WaveGetLaneIndex() == 7)) {
            if ((WaveGetLaneIndex() == 12)) {
              result = (result + WaveActiveSum(WaveGetLaneIndex()));
            }
            if ((WaveGetLaneIndex() == 3)) {
              result = (result + WaveActiveMin(result));
            }
          }
        }
      }
      break;
    }
  case 1: {
      if ((WaveGetLaneIndex() == 10)) {
        if ((WaveGetLaneIndex() == 11)) {
          result = (result + WaveActiveSum(result));
        }
        for (uint i1 = 0; (i1 < 2); i1 = (i1 + 1)) {
          if (((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 13))) {
            result = (result + WaveActiveMin(WaveGetLaneIndex()));
          }
        }
      } else {
      if ((WaveGetLaneIndex() == 12)) {
        result = (result + WaveActiveMax(result));
      }
      if ((WaveGetLaneIndex() == 14)) {
        result = (result + WaveActiveMin(result));
      }
    }
    break;
  }
  case 2: {
    if ((WaveGetLaneIndex() < 4)) {
      if ((WaveGetLaneIndex() < 4)) {
        result = (result + WaveActiveSum(WaveGetLaneIndex()));
      }
      if (((WaveGetLaneIndex() & 1) == 0)) {
        if (((WaveGetLaneIndex() & 1) == 1)) {
          result = (result + WaveActiveMax(result));
        }
      }
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
