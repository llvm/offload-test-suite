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
      if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 2))) {
        if ((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 13))) {
          result = (result + WaveActiveSum(result));
        }
        uint counter0 = 0;
        while ((counter0 < 3)) {
          counter0 = (counter0 + 1);
          if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 14))) {
            result = (result + WaveActiveSum(result));
          }
          if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 12))) {
            result = (result + WaveActiveSum(result));
          }
        }
      }
      break;
    }
  case 2: {
      uint counter1 = 0;
      while ((counter1 < 2)) {
        counter1 = (counter1 + 1);
        if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 11))) {
          result = (result + WaveActiveSum(5));
        }
        for (uint i2 = 0; (i2 < 2); i2 = (i2 + 1)) {
          if ((WaveGetLaneIndex() == 4)) {
            result = (result + WaveActiveSum(WaveGetLaneIndex()));
          }
          if ((WaveGetLaneIndex() == 9)) {
            result = (result + WaveActiveMax(5));
          }
          if ((i2 == 1)) {
            continue;
          }
        }
        if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 0))) {
          result = (result + WaveActiveMin(result));
        }
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
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      if ((WaveGetLaneIndex() < 8)) {
        result = (result + WaveActiveSum(1));
      }
      break;
    }
  case 1: {
      if ((WaveGetLaneIndex() >= 14)) {
        if ((WaveGetLaneIndex() < 5)) {
          result = (result + WaveActiveMin(6));
        }
        uint counter3 = 0;
        while ((counter3 < 2)) {
          counter3 = (counter3 + 1);
          for (uint i4 = 0; (i4 < 3); i4 = (i4 + 1)) {
            if (((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 14))) {
              result = (result + WaveActiveSum((WaveGetLaneIndex() + 1)));
            }
            if (((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 14))) {
              result = (result + WaveActiveMax(result));
            }
          }
          if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 8))) {
            result = (result + WaveActiveMin(result));
          }
        }
        if ((WaveGetLaneIndex() >= 10)) {
          result = (result + WaveActiveMin(WaveGetLaneIndex()));
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
        result = (result + WaveActiveSum(1));
      }
    }
  case 1: {
      if (((WaveGetLaneIndex() % 2) == 0)) {
        result = (result + WaveActiveSum(2));
      }
      break;
    }
  case 2: {
      uint counter5 = 0;
      while ((counter5 < 2)) {
        counter5 = (counter5 + 1);
        if ((WaveGetLaneIndex() == 1)) {
          if ((WaveGetLaneIndex() == 4)) {
            result = (result + WaveActiveSum(WaveGetLaneIndex()));
          }
          uint counter6 = 0;
          while ((counter6 < 3)) {
            counter6 = (counter6 + 1);
            if ((WaveGetLaneIndex() == 15)) {
              result = (result + WaveActiveMax((WaveGetLaneIndex() + 5)));
            }
            if ((WaveGetLaneIndex() == 1)) {
              result = (result + WaveActiveMax(10));
            }
          }
        } else {
        if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 1))) {
          result = (result + WaveActiveMax(WaveGetLaneIndex()));
        }
        for (uint i7 = 0; (i7 < 2); i7 = (i7 + 1)) {
          if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 14))) {
            result = (result + WaveActiveMin(result));
          }
        }
        if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 0))) {
          result = (result + WaveActiveMin((WaveGetLaneIndex() + 5)));
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
