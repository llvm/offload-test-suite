[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 14))) {
    result = (result + WaveActiveSum((WaveGetLaneIndex() + 1)));
  } else {
  if ((WaveGetLaneIndex() < 2)) {
    result = (result + WaveActiveMin((WaveGetLaneIndex() + 2)));
  } else {
  if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 5))) {
    result = (result + WaveActiveMax((WaveGetLaneIndex() + 3)));
  }
  }
  }
  switch ((WaveGetLaneIndex() % 3)) {
  case 0: {
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
      }
      break;
    }
  case 1: {
      switch ((WaveGetLaneIndex() % 3)) {
      case 0: {
          if ((WaveGetLaneIndex() == 10)) {
            if ((WaveGetLaneIndex() == 9)) {
              result = (result + WaveActiveMax(result));
            }
            if ((WaveGetLaneIndex() == 2)) {
              result = (result + WaveActiveMin(result));
            }
          } else {
          if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 11))) {
            result = (result + WaveActiveMin(result));
          }
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
        if ((((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 2))) {
          if (((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 6))) {
            result = (result + WaveActiveSum(9));
          }
        }
        break;
      }
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
}
