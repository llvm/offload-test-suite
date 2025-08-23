[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  uint counter0 = 0;
  while ((counter0 < 3)) {
    counter0 = (counter0 + 1);
    for (uint i1 = 0; (i1 < 3); i1 = (i1 + 1)) {
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
      default: {
          result = (result + WaveActiveSum(99));
          break;
        }
      }
      if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 12))) {
        result = (result + WaveActiveMin(WaveGetLaneIndex()));
      }
    }
  }
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      if (((WaveGetLaneIndex() & 1) == 0)) {
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveMin(result));
        }
        for (uint i2 = 0; (i2 < 3); i2 = (i2 + 1)) {
          if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 14))) {
            if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 10))) {
              result = (result + WaveActiveMax(WaveGetLaneIndex()));
            }
            if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 10))) {
              result = (result + WaveActiveMin(result));
            }
          }
          if (((WaveGetLaneIndex() & 1) == 1)) {
            result = (result + WaveActiveMin(WaveGetLaneIndex()));
          }
          if ((i2 == 1)) {
            continue;
          }
        }
        if (((WaveGetLaneIndex() & 1) == 1)) {
          result = (result + WaveActiveMin((WaveGetLaneIndex() + 1)));
        }
      } else {
      if ((WaveGetLaneIndex() >= 10)) {
        result = (result + WaveActiveSum(result));
      }
      switch ((WaveGetLaneIndex() % 4)) {
      case 0: {
          if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 13))) {
            if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 12))) {
              result = (result + WaveActiveMax((WaveGetLaneIndex() + 3)));
            }
            if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 15))) {
              result = (result + WaveActiveMax(WaveGetLaneIndex()));
            }
          }
          break;
        }
      case 1: {
          if (((WaveGetLaneIndex() & 1) == 0)) {
            if (((WaveGetLaneIndex() & 1) == 0)) {
              result = (result + WaveActiveMin(result));
            }
            if (((WaveGetLaneIndex() & 1) == 0)) {
              result = (result + WaveActiveSum(result));
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
          for (uint i3 = 0; (i3 < 3); i3 = (i3 + 1)) {
            if (((WaveGetLaneIndex() & 1) == 1)) {
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
  if ((WaveGetLaneIndex() < 3)) {
    if ((WaveGetLaneIndex() >= 9)) {
      result = (result + WaveActiveSum(WaveGetLaneIndex()));
    }
    for (uint i4 = 0; (i4 < 2); i4 = (i4 + 1)) {
      if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 11))) {
        result = (result + WaveActiveMax(1));
      }
      if ((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 7))) {
        if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 1))) {
          result = (result + WaveActiveMax(3));
        }
      }
    }
    if ((WaveGetLaneIndex() < 7)) {
      result = (result + WaveActiveMax((WaveGetLaneIndex() + 3)));
    }
  }
}
