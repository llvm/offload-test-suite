[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
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
      for (uint i0 = 0; (i0 < 3); i0 = (i0 + 1)) {
        for (uint i1 = 0; (i1 < 3); i1 = (i1 + 1)) {
          if ((WaveGetLaneIndex() == 3)) {
            result = (result + WaveActiveMax(result));
          }
          if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 12))) {
            if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 15))) {
              result = (result + WaveActiveSum((WaveGetLaneIndex() + 2)));
            }
          } else {
          if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 13))) {
            result = (result + WaveActiveMin(result));
          }
        }
        if ((i1 == 1)) {
          continue;
        }
        if ((i1 == 2)) {
          break;
        }
      }
    }
    break;
  }
  }
  switch ((WaveGetLaneIndex() % 2)) {
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
  default: {
      result = (result + WaveActiveSum(99));
      break;
    }
  }
  if ((WaveGetLaneIndex() == 6)) {
    result = (result + WaveActiveSum(1));
  } else {
  if ((WaveGetLaneIndex() == 5)) {
    result = (result + WaveActiveMin(2));
  } else {
  if ((((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 9))) {
    result = (result + WaveActiveMax((WaveGetLaneIndex() + 3)));
  } else {
  if ((WaveGetLaneIndex() < 5)) {
    result = (result + WaveActiveSum(4));
  }
  }
  }
  }
}
