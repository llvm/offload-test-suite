[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if ((WaveGetLaneIndex() == 6)) {
    if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 13))) {
      if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 11))) {
        result = (result + WaveActiveMin(4));
      }
      for (uint i0 = 0; (i0 < 3); i0 = (i0 + 1)) {
        for (uint i1 = 0; (i1 < 2); i1 = (i1 + 1)) {
          if ((WaveGetLaneIndex() < 2)) {
            result = (result + WaveActiveMin(result));
          }
        }
        if ((i0 == 1)) {
          continue;
        }
      }
    }
    if ((WaveGetLaneIndex() == 11)) {
      result = (result + WaveActiveMax(result));
    }
  } else {
  if ((WaveGetLaneIndex() == 11)) {
    result = (result + WaveActiveMax(result));
  }
  uint counter2 = 0;
  while ((counter2 < 2)) {
    counter2 = (counter2 + 1);
    if ((WaveGetLaneIndex() >= 14)) {
      result = (result + WaveActiveMin(WaveGetLaneIndex()));
    }
    if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 14))) {
      if ((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 1))) {
        result = (result + WaveActiveMax(WaveGetLaneIndex()));
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
      if ((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 12))) {
        result = (result + WaveActiveMin(result));
      }
    }
    if ((WaveGetLaneIndex() < 7)) {
      result = (result + WaveActiveMax(8));
    }
    if ((counter2 == 1)) {
      break;
    }
  }
  }
  switch ((WaveGetLaneIndex() % 3)) {
  case 0: {
      uint counter3 = 0;
      while ((counter3 < 3)) {
        counter3 = (counter3 + 1);
        uint counter4 = 0;
        while ((counter4 < 2)) {
          counter4 = (counter4 + 1);
          if ((WaveGetLaneIndex() == 10)) {
            result = (result + WaveActiveMax(result));
          }
          if ((WaveGetLaneIndex() == 9)) {
            if ((WaveGetLaneIndex() == 3)) {
              result = (result + WaveActiveSum(result));
            }
          }
          if ((WaveGetLaneIndex() == 3)) {
            result = (result + WaveActiveSum(result));
          }
        }
      }
      break;
    }
  case 1: {
      if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 13))) {
        if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 15))) {
          result = (result + WaveActiveMax(8));
        }
        for (uint i5 = 0; (i5 < 3); i5 = (i5 + 1)) {
          if (((WaveGetLaneIndex() & 1) == 0)) {
            result = (result + WaveActiveSum(WaveGetLaneIndex()));
          }
          uint counter6 = 0;
          while ((counter6 < 2)) {
            counter6 = (counter6 + 1);
            if (((WaveGetLaneIndex() & 1) == 0)) {
              result = (result + WaveActiveMin(result));
            }
            if (((WaveGetLaneIndex() & 1) == 1)) {
              result = (result + WaveActiveMax(result));
            }
          }
          if (((WaveGetLaneIndex() & 1) == 0)) {
            result = (result + WaveActiveSum((WaveGetLaneIndex() + 1)));
          }
        }
      }
      break;
    }
  case 2: {
      switch ((WaveGetLaneIndex() % 3)) {
      case 0: {
          for (uint i7 = 0; (i7 < 2); i7 = (i7 + 1)) {
            if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 14))) {
              result = (result + WaveActiveSum(10));
            }
            uint counter8 = 0;
            while ((counter8 < 2)) {
              counter8 = (counter8 + 1);
              if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 4))) {
                result = (result + WaveActiveSum(result));
              }
              if ((counter8 == 1)) {
                break;
              }
            }
          }
          break;
        }
      case 1: {
          uint counter9 = 0;
          while ((counter9 < 2)) {
            counter9 = (counter9 + 1);
            if ((WaveGetLaneIndex() < 6)) {
              result = (result + WaveActiveMin(5));
            }
            if (((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 8))) {
              if ((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 10))) {
                result = (result + WaveActiveSum(result));
              }
            }
            if ((counter9 == 1)) {
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
      }
      break;
    }
  }
}
