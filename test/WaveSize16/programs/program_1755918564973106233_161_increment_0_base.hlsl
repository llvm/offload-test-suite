[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 13))) {
    if ((((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 10))) {
      result = (result + WaveActiveSum(result));
    }
    uint counter0 = 0;
    while ((counter0 < 3)) {
      counter0 = (counter0 + 1);
      if (((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 8))) {
        result = (result + WaveActiveMax(result));
      }
      if ((WaveGetLaneIndex() == 11)) {
        for (uint i1 = 0; (i1 < 3); i1 = (i1 + 1)) {
          if (((WaveGetLaneIndex() & 1) == 0)) {
            result = (result + WaveActiveMin((WaveGetLaneIndex() + 3)));
          }
          if (((WaveGetLaneIndex() & 1) == 1)) {
            result = (result + WaveActiveMax((WaveGetLaneIndex() + 1)));
          }
          if ((i1 == 2)) {
            break;
          }
        }
        if ((WaveGetLaneIndex() == 10)) {
          result = (result + WaveActiveMax(WaveGetLaneIndex()));
        }
      } else {
      for (uint i2 = 0; (i2 < 3); i2 = (i2 + 1)) {
        if (((WaveGetLaneIndex() & 1) == 1)) {
          result = (result + WaveActiveMax((WaveGetLaneIndex() + 2)));
        }
        if ((i2 == 2)) {
          break;
        }
      }
      if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 15))) {
        result = (result + WaveActiveMax(result));
      }
    }
    if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 14))) {
      result = (result + WaveActiveMax(result));
    }
  }
  if ((((WaveGetLaneIndex() == 5) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 10))) {
    result = (result + WaveActiveMax(5));
  }
  }
  switch ((WaveGetLaneIndex() % 3)) {
  case 0: {
      switch ((WaveGetLaneIndex() % 3)) {
      case 0: {
          uint counter3 = 0;
          while ((counter3 < 2)) {
            counter3 = (counter3 + 1);
            if ((WaveGetLaneIndex() >= 14)) {
              result = (result + WaveActiveMin(result));
            }
            for (uint i4 = 0; (i4 < 2); i4 = (i4 + 1)) {
              if ((WaveGetLaneIndex() >= 8)) {
                result = (result + WaveActiveSum(8));
              }
              if ((i4 == 1)) {
                continue;
              }
              if ((i4 == 1)) {
                break;
              }
            }
            if ((WaveGetLaneIndex() < 3)) {
              result = (result + WaveActiveMax((WaveGetLaneIndex() + 4)));
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
          if (true) {
            result = (result + WaveActiveSum(3));
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
      if (true) {
        result = (result + WaveActiveSum(3));
      }
      break;
    }
  }
}
