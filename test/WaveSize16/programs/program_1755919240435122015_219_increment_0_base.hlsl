[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 4)) {
  case 0: {
      if ((WaveGetLaneIndex() < 8)) {
        result = (result + WaveActiveSum(1));
      }
      break;
    }
  case 1: {
      for (uint i0 = 0; (i0 < 2); i0 = (i0 + 1)) {
        if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 12))) {
          result = (result + WaveActiveSum(9));
        }
        if (((WaveGetLaneIndex() & 1) == 1)) {
          if (((WaveGetLaneIndex() & 1) == 0)) {
            result = (result + WaveActiveMin(result));
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
          if (((WaveGetLaneIndex() & 1) == 1)) {
            result = (result + WaveActiveMax(result));
          }
        }
        if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 12))) {
          result = (result + WaveActiveMin(result));
        }
        if ((i0 == 1)) {
          continue;
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
  case 3: {
      if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 0))) {
        if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 7))) {
          result = (result + WaveActiveMin(WaveGetLaneIndex()));
        }
        uint counter1 = 0;
        while ((counter1 < 3)) {
          counter1 = (counter1 + 1);
          if (((WaveGetLaneIndex() & 1) == 0)) {
            result = (result + WaveActiveSum(result));
          }
          for (uint i2 = 0; (i2 < 2); i2 = (i2 + 1)) {
            if ((WaveGetLaneIndex() >= 12)) {
              result = (result + WaveActiveSum((WaveGetLaneIndex() + 5)));
            }
          }
          if ((counter1 == 2)) {
            break;
          }
        }
        if ((((WaveGetLaneIndex() == 5) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 2))) {
          result = (result + WaveActiveMax(result));
        }
      } else {
      if (((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 13))) {
        result = (result + WaveActiveMax(WaveGetLaneIndex()));
      }
      if (((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 13))) {
        result = (result + WaveActiveMin(result));
      }
    }
    break;
  }
  }
}
