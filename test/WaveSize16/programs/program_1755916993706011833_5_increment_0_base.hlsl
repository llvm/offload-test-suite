[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 3)) {
  case 0: {
      if ((WaveGetLaneIndex() >= 11)) {
        if ((WaveGetLaneIndex() >= 9)) {
          result = (result + WaveActiveMin(result));
        }
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
              if ((WaveGetLaneIndex() == 2)) {
                result = (result + WaveActiveMax(4));
              }
            }
            break;
          }
        default: {
            result = (result + WaveActiveSum(99));
            break;
          }
        }
      } else {
      uint counter1 = 0;
      while ((counter1 < 2)) {
        counter1 = (counter1 + 1);
        if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 12))) {
          result = (result + WaveActiveSum((WaveGetLaneIndex() + 1)));
        }
        if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 15))) {
          if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 10))) {
            result = (result + WaveActiveSum(WaveGetLaneIndex()));
          }
        }
        if ((((WaveGetLaneIndex() == 7) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 12))) {
          result = (result + WaveActiveSum((WaveGetLaneIndex() + 3)));
        }
        if ((counter1 == 1)) {
          break;
        }
      }
    }
    break;
  }
  case 1: {
    for (uint i2 = 0; (i2 < 2); i2 = (i2 + 1)) {
      if ((WaveGetLaneIndex() >= 9)) {
        result = (result + WaveActiveSum((WaveGetLaneIndex() + 1)));
      }
      if (((WaveGetLaneIndex() & 1) == 1)) {
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveSum((WaveGetLaneIndex() + 3)));
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
        default: {
            result = (result + WaveActiveSum(99));
            break;
          }
        }
      }
      if ((i2 == 1)) {
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
