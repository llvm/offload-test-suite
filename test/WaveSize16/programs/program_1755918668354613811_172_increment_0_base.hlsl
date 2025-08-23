[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 14))) {
    if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 15))) {
      result = (result + WaveActiveMin(result));
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
    if ((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 1))) {
      result = (result + WaveActiveMax((WaveGetLaneIndex() + 4)));
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
          if ((((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 0))) {
            if ((((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 14))) {
              result = (result + WaveActiveSum(WaveGetLaneIndex()));
            }
            if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 2))) {
              result = (result + WaveActiveSum(result));
            }
          }
          break;
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
      if ((((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 5))) {
        if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 14))) {
          result = (result + WaveActiveSum(6));
        }
        for (uint i0 = 0; (i0 < 3); i0 = (i0 + 1)) {
          if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 10))) {
            result = (result + WaveActiveSum(9));
          }
          if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 15))) {
            result = (result + WaveActiveMax(result));
          }
        }
      } else {
      for (uint i1 = 0; (i1 < 2); i1 = (i1 + 1)) {
        if ((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 3))) {
          result = (result + WaveActiveMax((WaveGetLaneIndex() + 3)));
        }
        if ((((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 8))) {
          result = (result + WaveActiveSum((WaveGetLaneIndex() + 5)));
        }
        if ((i1 == 1)) {
          continue;
        }
      }
      if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 14))) {
        result = (result + WaveActiveMin(result));
      }
    }
    break;
  }
  }
  switch ((WaveGetLaneIndex() % 3)) {
  case 0: {
      if ((WaveGetLaneIndex() < 8)) {
        result = (result + WaveActiveSum(1));
      }
      break;
    }
  case 1: {
      if ((WaveGetLaneIndex() < 6)) {
        if ((WaveGetLaneIndex() >= 11)) {
          result = (result + WaveActiveMax(result));
        }
        if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 12))) {
          if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 13))) {
            result = (result + WaveActiveMin(result));
          }
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
