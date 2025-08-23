[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if ((WaveGetLaneIndex() < 8)) {
    if ((WaveGetLaneIndex() == 9)) {
      if ((WaveGetLaneIndex() == 10)) {
        result = (result + WaveActiveMax(result));
      }
      if ((WaveGetLaneIndex() == 5)) {
        result = (result + WaveActiveMin(2));
      }
    }
  }
  for (uint i0 = 0; (i0 < 2); i0 = (i0 + 1)) {
    if ((WaveGetLaneIndex() == 13)) {
      if ((WaveGetLaneIndex() == 8)) {
        result = (result + WaveActiveMax(result));
      }
      if ((WaveGetLaneIndex() == 3)) {
        result = (result + WaveActiveMax(9));
      }
    }
  }
  switch ((WaveGetLaneIndex() % 4)) {
  case 0: {
      if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 1))) {
        if ((((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 13))) {
          result = (result + WaveActiveMax(9));
        }
        if ((WaveGetLaneIndex() >= 11)) {
          if ((WaveGetLaneIndex() < 5)) {
            result = (result + WaveActiveSum(result));
          }
          if ((WaveGetLaneIndex() == 14)) {
            if ((WaveGetLaneIndex() == 1)) {
              result = (result + WaveActiveMax(result));
            }
          }
          if ((WaveGetLaneIndex() < 2)) {
            result = (result + WaveActiveMin(result));
          }
        }
      }
    }
  case 1: {
      for (uint i1 = 0; (i1 < 3); i1 = (i1 + 1)) {
        if ((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 11))) {
          result = (result + WaveActiveMax(WaveGetLaneIndex()));
        }
        uint counter2 = 0;
        while ((counter2 < 3)) {
          counter2 = (counter2 + 1);
          if ((((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 5))) {
            result = (result + WaveActiveSum(result));
          }
          if ((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 4))) {
            result = (result + WaveActiveSum(result));
          }
        }
        if ((i1 == 1)) {
          continue;
        }
      }
    }
  case 2: {
      if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 11))) {
        if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 11))) {
          result = (result + WaveActiveMax(WaveGetLaneIndex()));
        }
        if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 10))) {
          if ((WaveGetLaneIndex() < 7)) {
            if ((WaveGetLaneIndex() >= 15)) {
              result = (result + WaveActiveSum(WaveGetLaneIndex()));
            }
          }
          if ((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 14))) {
            result = (result + WaveActiveMax(result));
          }
        }
      } else {
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
      }
      if ((WaveGetLaneIndex() >= 12)) {
        result = (result + WaveActiveMin(result));
      }
    }
  }
  case 3: {
    for (uint i3 = 0; (i3 < 2); i3 = (i3 + 1)) {
      if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 4))) {
        result = (result + WaveActiveSum(result));
      }
      if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 2))) {
        result = (result + WaveActiveMin((WaveGetLaneIndex() + 2)));
      }
      if ((i3 == 1)) {
        break;
      }
    }
    break;
  }
  }
}
