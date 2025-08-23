[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 4)) {
  case 0: {
      for (uint i0 = 0; (i0 < 3); i0 = (i0 + 1)) {
        if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 15))) {
          result = (result + WaveActiveSum((WaveGetLaneIndex() + 2)));
        }
        if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 1))) {
          if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 4))) {
            result = (result + WaveActiveMin(result));
          }
          for (uint i1 = 0; (i1 < 3); i1 = (i1 + 1)) {
            if (((WaveGetLaneIndex() & 1) == 0)) {
              result = (result + WaveActiveSum(result));
            }
          }
          if ((((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 2))) {
            result = (result + WaveActiveMax((WaveGetLaneIndex() + 3)));
          }
        } else {
        if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 15))) {
          result = (result + WaveActiveMax((WaveGetLaneIndex() + 1)));
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
        }
      }
      if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 15))) {
        result = (result + WaveActiveMax(WaveGetLaneIndex()));
      }
    }
    break;
  }
  case 1: {
    if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 4))) {
      if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 9))) {
        result = (result + WaveActiveSum(WaveGetLaneIndex()));
      }
      if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 10))) {
        if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 14))) {
          result = (result + WaveActiveMin(result));
        }
        if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 11))) {
          result = (result + WaveActiveMax((WaveGetLaneIndex() + 3)));
        }
      }
      if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 14))) {
        result = (result + WaveActiveSum(result));
      }
    } else {
    if (((WaveGetLaneIndex() & 1) == 0)) {
      result = (result + WaveActiveMax((WaveGetLaneIndex() + 1)));
    }
    uint counter2 = 0;
    while ((counter2 < 2)) {
      counter2 = (counter2 + 1);
      if ((WaveGetLaneIndex() == 6)) {
        result = (result + WaveActiveMax(result));
      }
      if (((WaveGetLaneIndex() & 1) == 0)) {
        if (((WaveGetLaneIndex() & 1) == 1)) {
          result = (result + WaveActiveMax(result));
        }
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveMax(WaveGetLaneIndex()));
        }
      }
      if ((WaveGetLaneIndex() == 5)) {
        result = (result + WaveActiveMax(WaveGetLaneIndex()));
      }
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
