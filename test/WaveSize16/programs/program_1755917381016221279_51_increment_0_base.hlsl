[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if ((WaveGetLaneIndex() < 6)) {
    if ((WaveGetLaneIndex() < 1)) {
      result = (result + WaveActiveMax(result));
    }
    if ((WaveGetLaneIndex() >= 10)) {
      result = (result + WaveActiveMax((WaveGetLaneIndex() + 1)));
    }
  }
  switch ((WaveGetLaneIndex() % 3)) {
  case 0: {
      for (uint i0 = 0; (i0 < 3); i0 = (i0 + 1)) {
        if ((WaveGetLaneIndex() >= 15)) {
          result = (result + WaveActiveSum((WaveGetLaneIndex() + 4)));
        }
        uint counter1 = 0;
        while ((counter1 < 3)) {
          counter1 = (counter1 + 1);
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
          }
          if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 14))) {
            result = (result + WaveActiveMin(WaveGetLaneIndex()));
          }
        }
        if ((WaveGetLaneIndex() < 2)) {
          result = (result + WaveActiveSum(WaveGetLaneIndex()));
        }
      }
    }
  case 1: {
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
      }
    }
  case 2: {
      if ((WaveGetLaneIndex() < 8)) {
        for (uint i2 = 0; (i2 < 3); i2 = (i2 + 1)) {
          if ((((WaveGetLaneIndex() == 7) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 2))) {
            result = (result + WaveActiveSum(WaveGetLaneIndex()));
          }
          for (uint i3 = 0; (i3 < 2); i3 = (i3 + 1)) {
            if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 15))) {
              result = (result + WaveActiveMax(result));
            }
            if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 15))) {
              result = (result + WaveActiveMin(result));
            }
            if ((i3 == 1)) {
              break;
            }
          }
          if ((((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 1))) {
            result = (result + WaveActiveSum(WaveGetLaneIndex()));
          }
        }
        if ((WaveGetLaneIndex() < 1)) {
          result = (result + WaveActiveSum(result));
        }
      }
      break;
    }
  }
  if ((((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 6))) {
    result = (result + WaveActiveSum((WaveGetLaneIndex() + 1)));
  } else {
  if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 3))) {
    result = (result + WaveActiveMin(2));
  } else {
  if ((WaveGetLaneIndex() < 2)) {
    result = (result + WaveActiveMax((WaveGetLaneIndex() + 3)));
  }
  }
  }
}
