[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 4)) {
  case 0: {
      if (((WaveGetLaneIndex() & 1) == 1)) {
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveSum(2));
        }
        for (uint i0 = 0; (i0 < 2); i0 = (i0 + 1)) {
          if ((WaveGetLaneIndex() == 5)) {
            result = (result + WaveActiveMin(result));
          }
          if ((WaveGetLaneIndex() < 6)) {
            if ((WaveGetLaneIndex() < 3)) {
              result = (result + WaveActiveSum(1));
            }
          } else {
          if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 12))) {
            result = (result + WaveActiveSum(result));
          }
          if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 14))) {
            result = (result + WaveActiveSum(result));
          }
        }
        if ((i0 == 1)) {
          continue;
        }
      }
      if (((WaveGetLaneIndex() & 1) == 1)) {
        result = (result + WaveActiveMin(WaveGetLaneIndex()));
      }
    }
    break;
  }
  case 1: {
    if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 3))) {
      if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 15))) {
        if (((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 10))) {
          result = (result + WaveActiveMin(10));
        }
        for (uint i1 = 0; (i1 < 3); i1 = (i1 + 1)) {
          if (((WaveGetLaneIndex() & 1) == 1)) {
            result = (result + WaveActiveSum(result));
          }
        }
        if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 12))) {
          result = (result + WaveActiveSum(result));
        }
      }
      if ((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 1))) {
        result = (result + WaveActiveMin(WaveGetLaneIndex()));
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
  default: {
    result = (result + WaveActiveSum(99));
    break;
  }
  }
  switch ((WaveGetLaneIndex() % 3)) {
  case 0: {
      uint counter2 = 0;
      while ((counter2 < 2)) {
        counter2 = (counter2 + 1);
        if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 11))) {
          result = (result + WaveActiveMin(9));
        }
      }
    }
  case 1: {
      if (((WaveGetLaneIndex() & 1) == 1)) {
        if (((WaveGetLaneIndex() & 1) == 1)) {
          result = (result + WaveActiveMin(WaveGetLaneIndex()));
        }
        for (uint i3 = 0; (i3 < 2); i3 = (i3 + 1)) {
          if (((WaveGetLaneIndex() & 1) == 1)) {
            result = (result + WaveActiveMax(result));
          }
          if (((WaveGetLaneIndex() & 1) == 1)) {
            result = (result + WaveActiveSum((WaveGetLaneIndex() + 4)));
          }
        }
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveMax(WaveGetLaneIndex()));
        }
      }
    }
  case 2: {
      if (true) {
        result = (result + WaveActiveSum(3));
      }
      break;
    }
  }
}
