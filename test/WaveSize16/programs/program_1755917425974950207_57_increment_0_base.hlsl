[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 4)) {
  case 0: {
      if ((WaveGetLaneIndex() < 3)) {
        if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 10))) {
          if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 11))) {
            result = (result + WaveActiveMax(result));
          }
        }
        if ((WaveGetLaneIndex() < 2)) {
          result = (result + WaveActiveMin(result));
        }
      }
      break;
    }
  case 1: {
      if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 10))) {
        uint counter0 = 0;
        while ((counter0 < 2)) {
          counter0 = (counter0 + 1);
          if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 14))) {
            result = (result + WaveActiveSum(2));
          }
          if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 13))) {
            result = (result + WaveActiveSum((WaveGetLaneIndex() + 5)));
          }
        }
      } else {
      if (((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 3))) {
        result = (result + WaveActiveMin(result));
      }
      if ((WaveGetLaneIndex() == 1)) {
        if ((WaveGetLaneIndex() == 6)) {
          result = (result + WaveActiveMax((WaveGetLaneIndex() + 3)));
        }
      }
      if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 12))) {
        result = (result + WaveActiveSum(WaveGetLaneIndex()));
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
    for (uint i1 = 0; (i1 < 3); i1 = (i1 + 1)) {
      if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 15))) {
        result = (result + WaveActiveMax(result));
      }
      if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 8))) {
        if ((((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 4))) {
          result = (result + WaveActiveMin(result));
        }
      } else {
      if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 12))) {
        result = (result + WaveActiveMin((WaveGetLaneIndex() + 3)));
      }
      if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 13))) {
        result = (result + WaveActiveSum((WaveGetLaneIndex() + 3)));
      }
    }
    if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 14))) {
      result = (result + WaveActiveMin((WaveGetLaneIndex() + 3)));
    }
  }
  break;
  }
  default: {
    result = (result + WaveActiveSum(99));
    break;
  }
  }
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      if ((WaveGetLaneIndex() < 7)) {
        if ((WaveGetLaneIndex() < 6)) {
          result = (result + WaveActiveSum(result));
        }
        uint counter2 = 0;
        while ((counter2 < 2)) {
          counter2 = (counter2 + 1);
          if (((WaveGetLaneIndex() == 7) || (WaveGetLaneIndex() == 14))) {
            result = (result + WaveActiveSum(result));
          }
        }
      }
    }
  case 1: {
      if ((WaveGetLaneIndex() == 4)) {
        if ((WaveGetLaneIndex() == 1)) {
          result = (result + WaveActiveMin(result));
        }
        uint counter3 = 0;
        while ((counter3 < 3)) {
          counter3 = (counter3 + 1);
          if ((WaveGetLaneIndex() >= 9)) {
            result = (result + WaveActiveMin(result));
          }
        }
      } else {
      if ((((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 9))) {
        result = (result + WaveActiveSum(result));
      }
      if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 1))) {
        result = (result + WaveActiveSum(result));
      }
    }
    break;
  }
  }
  if ((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 12))) {
    if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 14))) {
      if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 5))) {
        result = (result + WaveActiveMin(result));
      }
      switch ((WaveGetLaneIndex() % 2)) {
      case 0: {
          uint counter4 = 0;
          while ((counter4 < 3)) {
            counter4 = (counter4 + 1);
            if (((WaveGetLaneIndex() & 1) == 0)) {
              result = (result + WaveActiveMin((WaveGetLaneIndex() + 5)));
            }
            if (((WaveGetLaneIndex() & 1) == 1)) {
              result = (result + WaveActiveSum(5));
            }
            if ((counter4 == 2)) {
              break;
            }
          }
          break;
        }
      case 1: {
          if ((((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 12))) {
            if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 15))) {
              result = (result + WaveActiveMin(result));
            }
          } else {
          if ((WaveGetLaneIndex() == 2)) {
            result = (result + WaveActiveSum(result));
          }
          if ((WaveGetLaneIndex() == 5)) {
            result = (result + WaveActiveMin(WaveGetLaneIndex()));
          }
        }
        break;
      }
    }
    if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 15))) {
      result = (result + WaveActiveMin(result));
    }
  }
  if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 11))) {
    result = (result + WaveActiveMax(result));
  }
  }
}
