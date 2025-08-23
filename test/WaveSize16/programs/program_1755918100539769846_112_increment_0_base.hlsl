[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  for (uint i0 = 0; (i0 < 2); i0 = (i0 + 1)) {
    if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 11))) {
      result = (result + WaveActiveMin(result));
    }
    if ((i0 == 1)) {
      break;
    }
  }
  if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 13))) {
    result = (result + WaveActiveSum((WaveGetLaneIndex() + 1)));
  } else {
  if (((WaveGetLaneIndex() == 7) || (WaveGetLaneIndex() == 8))) {
    result = (result + WaveActiveMin((WaveGetLaneIndex() + 2)));
  } else {
  if ((WaveGetLaneIndex() == 14)) {
    result = (result + WaveActiveMax(3));
  }
  }
  }
  if ((((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 6))) {
    if ((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 4))) {
      result = (result + WaveActiveMax((WaveGetLaneIndex() + 3)));
    }
    switch ((WaveGetLaneIndex() % 2)) {
    case 0: {
        if ((WaveGetLaneIndex() < 8)) {
          result = (result + WaveActiveSum(1));
        }
        break;
      }
    case 1: {
        for (uint i1 = 0; (i1 < 3); i1 = (i1 + 1)) {
          if ((WaveGetLaneIndex() == 3)) {
            result = (result + WaveActiveMin(result));
          }
          if ((i1 == 2)) {
            break;
          }
        }
        break;
      }
    default: {
        result = (result + WaveActiveSum(99));
        break;
      }
    }
    if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 6))) {
      result = (result + WaveActiveMin(WaveGetLaneIndex()));
    }
  }
}
