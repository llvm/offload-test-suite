[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  for (uint i0 = 0; (i0 < 3); i0 = (i0 + 1)) {
    if ((WaveGetLaneIndex() == 5)) {
      result = (result + WaveActiveSum(result));
    }
    for (uint i1 = 0; (i1 < 2); i1 = (i1 + 1)) {
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
    if ((WaveGetLaneIndex() == 7)) {
      result = (result + WaveActiveSum((WaveGetLaneIndex() + 3)));
    }
    if ((i0 == 1)) {
      continue;
    }
    if ((i0 == 2)) {
      break;
    }
  }
  if ((((WaveGetLaneIndex() == 7) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 1))) {
    result = (result + WaveActiveSum((WaveGetLaneIndex() + 1)));
  } else {
  if ((WaveGetLaneIndex() >= 11)) {
    result = (result + WaveActiveMin(2));
  } else {
  if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 4))) {
    result = (result + WaveActiveMax(3));
  } else {
  if ((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 11))) {
    result = (result + WaveActiveSum(4));
  }
  }
  }
  }
}
