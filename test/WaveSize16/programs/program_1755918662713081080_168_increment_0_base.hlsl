[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if ((WaveGetLaneIndex() >= 8)) {
    if ((WaveGetLaneIndex() < 4)) {
      result = (result + WaveActiveMax(result));
    }
    if ((WaveGetLaneIndex() < 6)) {
      result = (result + WaveActiveSum(result));
    }
  } else {
  if ((WaveGetLaneIndex() == 14)) {
    result = (result + WaveActiveMax(1));
  }
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      if ((WaveGetLaneIndex() < 8)) {
        result = (result + WaveActiveSum(1));
      }
      break;
    }
  case 1: {
      if ((WaveGetLaneIndex() >= 14)) {
        if ((WaveGetLaneIndex() < 8)) {
          result = (result + WaveActiveSum((WaveGetLaneIndex() + 5)));
        }
        for (uint i0 = 0; (i0 < 3); i0 = (i0 + 1)) {
          if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 3))) {
            result = (result + WaveActiveSum(8));
          }
          if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 0))) {
            result = (result + WaveActiveMin(result));
          }
        }
      }
      break;
    }
  default: {
      result = (result + WaveActiveSum(99));
      break;
    }
  }
  }
}
