[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 4)) {
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
          for (uint i0 = 0; (i0 < 2); i0 = (i0 + 1)) {
            if (((WaveGetLaneIndex() & 1) == 0)) {
              result = (result + WaveActiveMin(result));
            }
            if ((i0 == 1)) {
              break;
            }
          }
          break;
        }
      }
      break;
    }
  case 1: {
      if (((WaveGetLaneIndex() & 1) == 0)) {
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveSum(result));
        }
        uint counter1 = 0;
        while ((counter1 < 2)) {
          counter1 = (counter1 + 1);
          if ((WaveGetLaneIndex() < 3)) {
            result = (result + WaveActiveSum(10));
          }
          if ((WaveGetLaneIndex() >= 13)) {
            result = (result + WaveActiveMax((WaveGetLaneIndex() + 2)));
          }
          if ((counter1 == 1)) {
            break;
          }
        }
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveMin(result));
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
      if ((WaveGetLaneIndex() == 10)) {
        uint counter2 = 0;
        while ((counter2 < 3)) {
          counter2 = (counter2 + 1);
          if ((WaveGetLaneIndex() >= 11)) {
            result = (result + WaveActiveSum(WaveGetLaneIndex()));
          }
          if ((WaveGetLaneIndex() < 5)) {
            result = (result + WaveActiveSum(result));
          }
        }
      } else {
      if (((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 10))) {
        result = (result + WaveActiveMin(result));
      }
      for (uint i3 = 0; (i3 < 3); i3 = (i3 + 1)) {
        if ((WaveGetLaneIndex() >= 14)) {
          result = (result + WaveActiveSum(2));
        }
      }
      if (((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 13))) {
        result = (result + WaveActiveMin(result));
      }
    }
    break;
  }
  default: {
    result = (result + WaveActiveSum(99));
    break;
  }
  }
  switch ((WaveGetLaneIndex() % 3)) {
  case 0: {
      if ((WaveGetLaneIndex() < 8)) {
        if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 1))) {
          if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 14))) {
            result = (result + WaveActiveMin(result));
          }
        }
        if ((WaveGetLaneIndex() >= 9)) {
          result = (result + WaveActiveMin(8));
        }
      }
      break;
    }
  case 1: {
      if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 14))) {
        if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 13))) {
          result = (result + WaveActiveSum(WaveGetLaneIndex()));
        }
        for (uint i4 = 0; (i4 < 3); i4 = (i4 + 1)) {
          if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 10))) {
            result = (result + WaveActiveSum(result));
          }
          if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 13))) {
            result = (result + WaveActiveMax(result));
          }
          if ((i4 == 2)) {
            break;
          }
        }
        if (((WaveGetLaneIndex() == 7) || (WaveGetLaneIndex() == 13))) {
          result = (result + WaveActiveMax(9));
        }
      } else {
      if ((WaveGetLaneIndex() == 9)) {
        result = (result + WaveActiveSum(10));
      }
      if (((WaveGetLaneIndex() & 1) == 1)) {
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveMax(result));
        }
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveSum(result));
        }
      }
      if ((WaveGetLaneIndex() == 9)) {
        result = (result + WaveActiveMin(result));
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
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      for (uint i5 = 0; (i5 < 3); i5 = (i5 + 1)) {
        if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 11))) {
          result = (result + WaveActiveMin(result));
        }
        uint counter6 = 0;
        while ((counter6 < 3)) {
          counter6 = (counter6 + 1);
          if ((WaveGetLaneIndex() == 6)) {
            result = (result + WaveActiveMin(result));
          }
          if ((WaveGetLaneIndex() == 3)) {
            result = (result + WaveActiveMin(WaveGetLaneIndex()));
          }
          if ((counter6 == 2)) {
            break;
          }
        }
        if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 12))) {
          result = (result + WaveActiveSum(result));
        }
      }
      break;
    }
  case 1: {
      if (((WaveGetLaneIndex() & 1) == 1)) {
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveSum(WaveGetLaneIndex()));
        }
        for (uint i7 = 0; (i7 < 2); i7 = (i7 + 1)) {
          if ((WaveGetLaneIndex() >= 10)) {
            result = (result + WaveActiveMax(result));
          }
          if ((WaveGetLaneIndex() >= 11)) {
            result = (result + WaveActiveMax(9));
          }
          if ((i7 == 1)) {
            continue;
          }
        }
      } else {
      for (uint i8 = 0; (i8 < 3); i8 = (i8 + 1)) {
        if ((WaveGetLaneIndex() == 3)) {
          result = (result + WaveActiveSum((WaveGetLaneIndex() + 1)));
        }
      }
    }
    break;
  }
  default: {
    result = (result + WaveActiveSum(99));
    break;
  }
  }
}
