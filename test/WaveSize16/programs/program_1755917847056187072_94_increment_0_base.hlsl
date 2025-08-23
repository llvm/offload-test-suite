[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 4)) {
  case 0: {
      uint counter0 = 0;
      while ((counter0 < 2)) {
        counter0 = (counter0 + 1);
        if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 10))) {
          result = (result + WaveActiveSum(WaveGetLaneIndex()));
        }
        uint counter1 = 0;
        while ((counter1 < 3)) {
          counter1 = (counter1 + 1);
          if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 14))) {
            result = (result + WaveActiveMax(result));
          }
          if (((WaveGetLaneIndex() & 1) == 1)) {
            if (((WaveGetLaneIndex() & 1) == 1)) {
              result = (result + WaveActiveSum(WaveGetLaneIndex()));
            }
          }
        }
        if ((((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 4))) {
          result = (result + WaveActiveSum(5));
        }
        if ((counter0 == 1)) {
          break;
        }
      }
      break;
    }
  case 1: {
      if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 15))) {
        if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 13))) {
          result = (result + WaveActiveMax(result));
        }
        uint counter2 = 0;
        while ((counter2 < 3)) {
          counter2 = (counter2 + 1);
          if ((WaveGetLaneIndex() == 1)) {
            result = (result + WaveActiveMax(result));
          }
          for (uint i3 = 0; (i3 < 2); i3 = (i3 + 1)) {
            if ((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 11))) {
              result = (result + WaveActiveMin(result));
            }
            if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 11))) {
              result = (result + WaveActiveMax((WaveGetLaneIndex() + 5)));
            }
          }
          if ((WaveGetLaneIndex() == 0)) {
            result = (result + WaveActiveMin(7));
          }
          if ((counter2 == 2)) {
            break;
          }
        }
      } else {
      if (((WaveGetLaneIndex() & 1) == 1)) {
        result = (result + WaveActiveSum(result));
      }
      if ((WaveGetLaneIndex() >= 9)) {
        if ((WaveGetLaneIndex() < 7)) {
          result = (result + WaveActiveMin(result));
        }
      }
      if (((WaveGetLaneIndex() & 1) == 1)) {
        result = (result + WaveActiveMin(6));
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
    if ((WaveGetLaneIndex() < 20)) {
      result = (result + WaveActiveSum(4));
    }
    break;
  }
  }
  switch ((WaveGetLaneIndex() % 4)) {
  case 0: {
      for (uint i4 = 0; (i4 < 3); i4 = (i4 + 1)) {
        uint counter5 = 0;
        while ((counter5 < 2)) {
          counter5 = (counter5 + 1);
          if ((WaveGetLaneIndex() >= 11)) {
            result = (result + WaveActiveSum(9));
          }
        }
        if ((WaveGetLaneIndex() < 3)) {
          result = (result + WaveActiveMin(WaveGetLaneIndex()));
        }
      }
      break;
    }
  case 1: {
      for (uint i6 = 0; (i6 < 3); i6 = (i6 + 1)) {
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveMax(result));
        }
        if ((WaveGetLaneIndex() >= 14)) {
          if ((WaveGetLaneIndex() < 3)) {
            result = (result + WaveActiveSum(result));
          }
        }
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveMax(5));
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
      if ((WaveGetLaneIndex() < 20)) {
        result = (result + WaveActiveSum(4));
      }
      break;
    }
  }
}
