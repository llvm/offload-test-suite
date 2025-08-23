[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  for (uint i0 = 0; (i0 < 2); i0 = (i0 + 1)) {
    if ((WaveGetLaneIndex() == 8)) {
      result = (result + WaveActiveSum(5));
    }
    uint counter1 = 0;
    while ((counter1 < 3)) {
      counter1 = (counter1 + 1);
      if ((WaveGetLaneIndex() >= 11)) {
        if ((WaveGetLaneIndex() < 6)) {
          result = (result + WaveActiveMin(result));
        }
        if ((WaveGetLaneIndex() >= 13)) {
          result = (result + WaveActiveMax(2));
        }
      }
      if ((WaveGetLaneIndex() == 9)) {
        result = (result + WaveActiveSum(WaveGetLaneIndex()));
      }
    }
    if ((WaveGetLaneIndex() == 10)) {
      result = (result + WaveActiveMin(WaveGetLaneIndex()));
    }
    if ((i0 == 1)) {
      continue;
    }
  }
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 15))) {
        if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 14))) {
          result = (result + WaveActiveSum(result));
        }
        if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 12))) {
          if (((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 14))) {
            result = (result + WaveActiveMin(result));
          }
        }
      }
      break;
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
          if (true) {
            result = (result + WaveActiveSum(3));
          }
          break;
        }
      }
      break;
    }
  }
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      switch ((WaveGetLaneIndex() % 3)) {
      case 0: {
          if ((WaveGetLaneIndex() < 8)) {
            result = (result + WaveActiveSum(1));
          }
          break;
        }
      case 1: {
          if (((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 6))) {
            if (((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 8))) {
              result = (result + WaveActiveMin(result));
            }
            if (((WaveGetLaneIndex() & 1) == 0)) {
              if (((WaveGetLaneIndex() & 1) == 0)) {
                result = (result + WaveActiveMin(4));
              }
              if (((WaveGetLaneIndex() & 1) == 1)) {
                result = (result + WaveActiveMin(WaveGetLaneIndex()));
              }
            }
            if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 4))) {
              result = (result + WaveActiveMax(5));
            }
          }
          break;
        }
      case 2: {
          uint counter2 = 0;
          while ((counter2 < 3)) {
            counter2 = (counter2 + 1);
            if (((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 9))) {
              result = (result + WaveActiveMax(WaveGetLaneIndex()));
            }
            if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 13))) {
              if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 10))) {
                result = (result + WaveActiveSum(result));
              }
            }
            if ((counter2 == 2)) {
              break;
            }
          }
          break;
        }
      }
      break;
    }
  case 1: {
      uint counter3 = 0;
      while ((counter3 < 3)) {
        counter3 = (counter3 + 1);
        if ((WaveGetLaneIndex() == 15)) {
          result = (result + WaveActiveMax(WaveGetLaneIndex()));
        }
        for (uint i4 = 0; (i4 < 2); i4 = (i4 + 1)) {
          if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 12))) {
            result = (result + WaveActiveMax(result));
          }
          if ((WaveGetLaneIndex() >= 9)) {
            if ((WaveGetLaneIndex() < 2)) {
              result = (result + WaveActiveMax((WaveGetLaneIndex() + 2)));
            }
          }
          if ((i4 == 1)) {
            break;
          }
        }
        if ((WaveGetLaneIndex() == 4)) {
          result = (result + WaveActiveMax(result));
        }
      }
      break;
    }
  }
}
